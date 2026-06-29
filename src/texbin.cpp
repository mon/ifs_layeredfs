#define LOG_MODULE "layeredfs-texbin"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#include <fstream>
#include <string>
#include <optional>
#include <vector>
#include <map>
#include <unordered_map>

#include "texbin.hpp"
#include "avs.h"
#include "log.hpp"
#include "3rd_party/lodepng.h"
#include "3rd_party/libsquish/squish.h"

#ifdef TEXBIN_VERBOSE
#define VLOG(...) log_misc(__VA_ARGS__)
#else
#define VLOG(...)
#endif

// raw data from bin
#pragma pack(push,1)

class Magic {
    public:
    Magic(const char literal[5]) { // 4 + NUL
        std::copy(literal, literal + sizeof(magic), magic.begin());
    }

    bool operator==(std::string_view s) {
        return std::string_view(magic) == s;
    }

    private:
    std::array<char, 4> magic;
};

class TexbinHdr {
    public:
    Magic magic = "PXET";
    uint8_t unk1[8] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00};
    uint32_t archive_size; // full, including header
    uint32_t unk2 = 1;
    uint32_t file_count;
    uint32_t unk3 = 0;
    uint32_t data_offset;
    uint32_t rect_offset;
    uint8_t unk4[16] = {0};
    uint32_t name_offset;
    uint32_t unk5 = 0;
    uint32_t data_entry_offset;

    void debug() {
        log_misc("texbin hdr");

        log_misc("  archive size:      {}", archive_size);
        log_misc("  file count:        {}", file_count);
        log_misc("  data offset:       {}", data_offset);
        log_misc("  rect offset:       {}", rect_offset);
        log_misc("  name offset:       {}", name_offset);
        log_misc("  data entry offset: {}", data_entry_offset);
    }
};

class TexbinNamesHdr {
    public:
    Magic magic = "PMAN";
    uint32_t sect_size; // includes header
    uint8_t unk1[8] = { 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00 };
    uint32_t names_count;
    // probably some sort of hash bucket sizes/masks
    uint16_t unkA; // 1 << bit_length(names_count / 4)
    uint16_t unkB; // (1 << bit_length(names_count / 2)) - 1
    uint32_t header_len = sizeof(TexbinNamesHdr);

    void debug() {
        log_misc("texbin names hdr");

        log_misc("  section size:      {}", sect_size);
        log_misc("  names count:       {}", names_count);
    }
};

class TexbinRectHdr {
    public:
    Magic magic = "TCER";
    uint8_t unk1[8] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00};
    uint32_t sect_size; // includes header
    uint32_t image_count;
    uint32_t name_offset; // from the start of the header
    uint32_t rect_entry_offset; // from the start of the header

    void debug() {
        log_misc("texbin rect hdr");

        log_misc("  section size:      {}", sect_size);
        log_misc("  image count:       {}", image_count);
        log_misc("  name offset:       {}", name_offset);
        log_misc("  entries offset:    {}", rect_entry_offset);
    }
};

class TexbinDataEntry {
    public:
    uint32_t unk1 = 0;
    uint32_t size; // raw data, compressed file has more metadata
    uint32_t offset; // from the start of the file
};

// validate my understanding of the C++ spec
static_assert(sizeof(TexbinHdr) == 64, "Texbin header size is wrong");
static_assert(sizeof(TexbinNamesHdr) == 28, "Texbin name header size is wrong");
static_assert(sizeof(TexbinRectHdr) == 28, "Texbin rect header size is wrong");
static_assert(sizeof(TexbinDataEntry) == 12, "Texbin data entry header size is wrong");

class TexbinNameEntry {
    public:
    uint32_t hash;
    uint32_t id;
    uint32_t str_offset; // from the start of the header
};

class TexbinRectEntry {
    public:
    uint32_t image_id;
    uint16_t x1, x2, y1, y2;
};

// just used for size peeking
struct TexHdr {
    Magic magic;
    uint32_t check1;
    uint32_t check2;
    uint32_t archive_size;
    uint16_t width, height;
    uint32_t format1;
    uint8_t unk1[0x14] = {0};
    uint32_t format2;
    uint8_t unk2[0x10] = {0};
};

static_assert(sizeof(TexHdr) == 64, "Texbin tex header size is wrong");

#pragma pack(pop)

// texbintool always sets little_endian=true, unsure where it's seen most often
static std::vector<uint8_t> argb8888_to_texture_data(
        const unsigned char *image, unsigned width, unsigned height,
        bool little_endian = true) {
    size_t image_size = 4 * width * height;

    std::vector<uint8_t> data;
    data.reserve(0x40 + image_size);
    auto write = [&data, little_endian] (const std::vector<uint8_t> &bytes) {
        if(little_endian) {
            data.insert(data.end(),bytes.rbegin(),bytes.rend());
        } else {
            data.insert(data.end(),bytes.begin(),bytes.end());
        }
    };
    auto write_int = [&data, little_endian] (auto val) {
        uint8_t b[sizeof(val)];
        memcpy(b, &val, sizeof(val));
        std::vector v(b, b + sizeof(val));

        // note this is reversed to `write`, since the value is LE by default
        if(little_endian) {
            data.insert(data.end(),v.begin(),v.end());
        } else {
            data.insert(data.end(),v.rbegin(),v.rend());
        }
    };

    write({'T','X','D','T'});
    if (little_endian) {
        write({ 0x00, 0x01, 0x00, 0x00 });
        write({ 0x00, 0x01, 0x01, 0x00 });
    } else {
        write({ 0x00, 0x01, 0x02, 0x00 });
        write({ 0x00, 0x01, 0x02, 0x00 });
    }

    write_int((uint32_t)(image_size + 0x40));
    write_int((uint16_t)(width));
    write_int((uint16_t)(height));

    // TODO: is this like IFS? _Can_ we get away with just ARGB8888?
    if (little_endian) {
        write({ 0x11, 0x22, 0x10, 0x10 });
    } else {
        write({ 0x11, 0x11, 0x10, 0x10 });
    }

    for (int i = 0; i < 0x14; i++) {
        data.push_back(0);
    }

    write_int((uint32_t)0x03);

    for (int i = 0; i < 0x10; i++) {
        data.push_back(0);
    }

    data.insert(data.end(), image, image + image_size);

    return texbin_lz77_compress(data);
}

static std::vector<istring> load_names(std::istream &f, uint32_t name_offset) {
    std::vector<istring> ret;

    f.seekg(name_offset);

    TexbinNamesHdr name_hdr;
    if(!f.read((char*)&name_hdr, sizeof(name_hdr))) {
        log_warning("bad names");
        return ret;
    }

    if(name_hdr.magic != "PMAN") {
        log_warning("bad name magic");
        return ret;
    }

#ifdef TEXBIN_VERBOSE
    name_hdr.debug();
#endif

    ret.resize(name_hdr.names_count);
    TexbinNameEntry entry;
    for(uint32_t i = 0; i < name_hdr.names_count; i++) {
        if(!f.read((char*)&entry, sizeof(entry))) {
            log_warning("bad name entry at {}", i);
            return ret;
        }
        auto pos = f.tellg();

        f.seekg(name_offset + entry.str_offset);
        istring name = "";
        char ch;
        while(f.get(ch) && ch > '\0') {
            name += ch;
        }
        if(!f) {
            log_warning("bad name entry at {}", i);
            return ret;
        }
        ret[entry.id] = name;

        f.seekg(pos);
    }

    return ret;
}

static std::vector<std::vector<uint8_t>> load_data(std::istream &f, const TexbinHdr& hdr) {
    bool warned_about_size_mismatch = false;

    std::vector<std::vector<uint8_t>> ret;
    ret.reserve(hdr.file_count);

    f.seekg(hdr.data_entry_offset);

    TexbinDataEntry entry;
    for(uint32_t i = 0; i < hdr.file_count; i++) {
        if(!f.read((char*)&entry, sizeof(entry))) {
            log_warning("bad data entry at {}", i);
            return ret;
        }
        auto pos = f.tellg();

        f.seekg(entry.offset);

        // the size *appears* to be the compressed size, but older (??) versions
        // of texbintool seem to emit the decompressed size, so do a quick read
        // and double check. The game seems to ignore the "bad" size and use the
        // actual data len, so they're not broken.
        uint32_t sizes[2];
        if(!f.read((char*)&sizes[0], sizeof(sizes))) {
            log_warning("can't read data at i {} offset {}", i, entry.offset);
            return ret;
        }

        auto comp_len = _byteswap_ulong(sizes[1]);
        if(entry.size != (comp_len + 8)) {
            if(!warned_about_size_mismatch) {
                warned_about_size_mismatch = true;
                log_warning("File has invalid entry size, using real size");
            }
            entry.size = comp_len + 8;
        }

        f.seekg(entry.offset);
        std::vector<uint8_t> data;
        data.resize(entry.size);
        if(!f.read((char*)&data[0], entry.size)) {
            log_warning("can't read data at i {} offset {} len {}", i, entry.offset, entry.size);
            return ret;
        }
        ret.push_back(data);

        f.seekg(pos);
    }

    return ret;
}

// only have C++17, so can't use std::countl_zero
uint8_t bit_length(uint32_t val) {
    uint8_t bits;
    for(bits = 0; val != 0; bits++) val >>= 1;

    return bits;
}

uint32_t hash_name(const char *name) {
    int hash = 0;

    for(; *name; name++) {
        for (int i = 0; i <= 5; i++) {
            hash = ((hash >> 31) & 0x4C11DB7) ^ ((hash << 1) | ((*name >> i) & 1));
        }
    }

    return hash;
}

void pad32(std::ofstream &f) {
    // pad to 4 bytes
    auto pad = 4 - (f.tellp() % 4);
    if(pad != 4) {
        char zeros[4] = {0};
        f.write(zeros, pad);
    }
}

template<typename T>
void write_names(std::ofstream &f, std::map<istring, T> &names) {
    auto start = f.tellp();
    TexbinNamesHdr hdr;
    f.write((char*)&hdr, sizeof(hdr)); // update with real values later

    // Hashes are written in ascending order. Ensure this by using the (sorted)
    // std::map
    std::map<uint32_t, TexbinNameEntry> entries;
    uint32_t str_offset = (uint32_t)(sizeof(hdr) + (names.size() * sizeof(TexbinNameEntry)));
    uint32_t id = 0;
    for(auto &[name, _val] : names) {
        TexbinNameEntry entry;
        entry.hash = hash_name(name.c_str());
        entry.id = id;
        entry.str_offset = str_offset;
        entries[entry.hash] = entry;

        id++;
        str_offset += (uint32_t)name.size() + 1; // include NUL
    }

    // now we've made and sorted the hashes, emit all the data
    for(auto &[_hash, entry] : entries) {
        f.write((char*)&entry, sizeof(TexbinNameEntry));
    }
    for(auto &[name, _val] : names) {
        f.write(name.c_str(), name.size() + 1);
    }

    pad32(f);

    auto end = f.tellp();

    // update header with known values
    hdr.sect_size = (uint32_t)(end - start);
    hdr.names_count = (uint32_t)names.size();
    hdr.unkA = 1 << bit_length(hdr.names_count / 4);
    hdr.unkB = (1 << bit_length(hdr.names_count / 2)) - 1;

    f.seekp(start);
    f.write((char*)&hdr, sizeof(hdr));

    // back to the end for our consumer
    f.seekp(0, std::ios::end);
}

bool Texbin::add_or_replace_image(const char *image_name, const char *png_path) {
    unsigned error;
    std::vector<uint8_t> image;
    unsigned width, height;
    error = lodepng::decode(image, width, height, png_path);
    if (error) {
        log_warning("Can't load png {}: {}", error, lodepng_error_text(error));
        return false;
    }

    auto existing_image = images.find(image_name);
    auto existing_rect = rects.find(image_name);
    // rect image names may shadow normal image names, so check them first
    if(existing_rect != rects.end()) {
        if(width != existing_rect->second.w || height != existing_rect->second.h) {
            log_info("Replacement rect image {} has dimensions {}x{} but original is {}x{}, ignoring",
                image_name, width, height, existing_rect->second.w, existing_rect->second.h
            );
            return false;
        }
        log_info("Replacing rect image {}", image_name);
        existing_rect->second.dirty_data = image;
    } else if(existing_image != images.end()) {
        auto [w, h] = existing_image->second.peek_dimensions();
        if(width != w || height != h) {
            log_info("Replacement image {} has dimensions {}x{} but original is {}x{}, repacking anyway",
                image_name, width, height, w, h
            );
        }

        log_info("Replacing {}", image_name);
        images[image_name] = ImageEntryParsed(argb8888_to_texture_data(&image[0], width, height));
    } else{
        log_info("Adding new image {}", image_name);
        images[image_name] = ImageEntryParsed(argb8888_to_texture_data(&image[0], width, height));
    }

    return true;
}

void Texbin::debug() {
    [[maybe_unused]] uint32_t total = 0;
    for(auto &[name, image] : images) {
        [[maybe_unused]] auto [w,h] = image.peek_dimensions();
        VLOG("file: %s len %d fmt %s dims(%d,%d)",
            name.c_str(), image.tex.size(), image.tex_type_str(false).c_str(),
            w, h);
        total += (uint32_t)image.tex.size();
    }

    for([[maybe_unused]] auto &[name, rect] : rects) {
        VLOG("rect: %s parent %s dims (%d,%d,%d,%d)",
            name.c_str(), rect.parent_name.c_str(),
            rect.x, rect.y, rect.w, rect.h
        );
    }

    VLOG("Total data: %d", total);
}

std::optional<Texbin> Texbin::from_stream(std::istream &f) {
    f.seekg(0, std::ios::end);
    auto file_len = f.tellg();
    f.seekg(0);

    TexbinHdr hdr;
    if(!f.read((char*)&hdr, sizeof(hdr))) {
        log_verbose("cannot read header");
        return std::nullopt;
    }

    if(hdr.magic != "PXET") {
        log_verbose("bad magic");
        return std::nullopt;
    }

    if(hdr.archive_size != file_len) {
        log_warning("bad archive size (file said {} stream said {})",
            hdr.archive_size,
            // TODO: hope that P3374R1 is implemented for C++26?
            static_cast<std::streamoff>(file_len)
        );
        return std::nullopt;
    }

#ifdef TEXBIN_VERBOSE
    hdr.debug();
#endif

    auto names = load_names(f, 0 + hdr.name_offset);
    if(names.size() != hdr.file_count) {
        log_warning("Name section mismatch against files");
        return std::nullopt;
    }

    auto data = load_data(f, hdr);

    std::map<istring, ImageEntryParsed> images;
    // images.reserve(hdr.file_count);

    for(uint32_t i = 0; i < hdr.file_count; i++) {
        images[names[i]] = ImageEntryParsed(data[i]);
    }

    std::map<istring, RectEntryParsed> rects;
    if(hdr.rect_offset) {
        TexbinRectHdr rect_hdr;
        f.seekg(hdr.rect_offset);
        if(!f.read((char*)&rect_hdr, sizeof(rect_hdr))) {
            log_warning("cannot read rect header");
            return std::nullopt;
        }
        if(rect_hdr.magic != "TCER") {
            log_warning("bad rect magic");
            return std::nullopt;
        }

#ifdef TEXBIN_VERBOSE
        rect_hdr.debug();
#endif

        auto rect_names = load_names(f, hdr.rect_offset + rect_hdr.name_offset);

        if(rect_names.size() != rect_hdr.image_count) {
            log_warning("Rect name section mismatch against files");
            return std::nullopt;
        }

        f.seekg(hdr.rect_offset + rect_hdr.rect_entry_offset);
        for(uint32_t i = 0; i < rect_hdr.image_count; i++) {
            TexbinRectEntry entry;
            if(!f.read((char*)&entry, sizeof(entry))) {
                log_warning("cannot read rect entry");
                return std::nullopt;
            }

            if(entry.image_id >= names.size()) {
                log_warning("rect entry refers to invalid parent");
                return std::nullopt;
            }

            if(entry.x1 >= entry.x2 || entry.y1 >= entry.y2) {
                log_warning("rect entry has invalid dimensions ({},{},{},{})",
                    entry.x1, entry.x2, entry.y1, entry.y2
                );
                return std::nullopt;
            }

            RectEntryParsed entry_parsed;
            entry_parsed.parent_name = names[entry.image_id];
            entry_parsed.x = entry.x1;
            entry_parsed.y = entry.y1;
            entry_parsed.w = entry.x2 - entry.x1;
            entry_parsed.h = entry.y2 - entry.y1;
            rects[rect_names[i]] = entry_parsed;
        }
    }

    auto ret = Texbin(images, rects);
#ifdef TEXBIN_VERBOSE
    ret.debug();
#endif
    return ret;
}

std::optional<Texbin> Texbin::from_path(const char *path) {
    // there are a handful of .bin files we might try to parse that *aren't*
    // texbins, so gate all logs before header magic check behind log_verbose
    log_verbose("Opening {}", path);
    std::ifstream f (path, std::ios::binary);
    if(!f) {
        log_verbose("cannot open");
        return std::nullopt;
    }

    return Texbin::from_stream(f);
}

void Texbin::process_dirty_rects() {
    std::unordered_map<istring, std::vector<RectEntryParsed*>> updates;
    for(auto &[key, rect] : rects) {
        if(rect.dirty_data) {
            auto [it, _] = updates.emplace(rect.parent_name, std::vector<RectEntryParsed*>());
            it->second.push_back(&rect);
        }
    }

    for(auto &[rect_name, rects] : updates) {
        auto _image = images.find(rect_name);
        if(_image == images.end()) {
            log_warning("Can't update rect {}: no tex???", rect_name);
            continue;
        }
        auto image = &_image->second;
        auto _tex = image->tex_to_argb8888();
        if(!_tex) {
            log_warning("Can't update rect {}: cannot load tex", rect_name);
            continue;
        }
        auto [tex, width, height] = *_tex;

        for(auto &rect : rects) {
            if(rect->x2() > width || rect->y2() > height) {
                log_warning("Can't update rect in {}: out of bounds (canvas is {}x{}, rect is x1,x2,y1,y2 {},{},{},{})",
                    rect_name,
                    width,
                    height,
                    rect->x, rect->x2(), rect->y, rect->y2()
                );
                continue;
            }

            auto dirty = *rect->dirty_data;

            for(size_t y = 0; y < rect->h; y++) {
                size_t src_start = y * rect->w * 4;
                size_t dst_start = ((y + rect->y) * width * 4) + (rect->x * 4);
                size_t len = rect->w * 4;
                memcpy(&tex[dst_start], &dirty[src_start], len);
            }

            rect->dirty_data = std::nullopt;
        }

        image->tex = argb8888_to_texture_data(&tex[0], width, height);
    }
}

bool Texbin::save(const char *dest) {
    std::ofstream f(dest, std::ios::binary);
    if(!f) {
        log_warning("Can't open output");
        return false;
    }

    process_dirty_rects(); // update any rect textures we modified

    TexbinHdr hdr;

    f.write((char*)&hdr, sizeof(hdr)); // updated later
    hdr.name_offset = (uint32_t)f.tellp();
    write_names(f, images);

    hdr.data_entry_offset = (uint32_t)f.tellp();
    uint32_t data_offset = hdr.data_entry_offset + (uint32_t)(images.size() * sizeof(TexbinDataEntry));
    for(auto &[_name, data] : images) {
        TexbinDataEntry entry;
        entry.size = (uint32_t)data.tex.size();
        entry.offset = data_offset;
        f.write((char*)&entry, sizeof(entry));

        data_offset += (uint32_t)data.tex.size();
        uint32_t pad = 4 - (data.tex.size() % 4);
        if(pad != 4) {
            data_offset += pad;
        }
    }

    hdr.data_offset = (uint32_t)f.tellp();

    for(auto &[_name, data] : images) {
        f.write((char*)&data.tex[0], data.tex.size());
        // the test files I have all seem to conform to this, but texbintool
        // only aligns the entire section. Better safe than sorry...
        pad32(f);
    }

    if(rects.size()) {
        hdr.rect_offset = (uint32_t)f.tellp();
        TexbinRectHdr rect_hdr;
        f.write((char*)&rect_hdr, sizeof(rect_hdr));
        rect_hdr.name_offset = (uint32_t)f.tellp() - hdr.rect_offset;

        write_names(f, rects);

        rect_hdr.rect_entry_offset = (uint32_t)f.tellp() - hdr.rect_offset;
        for(auto &[name, rect] : rects) {
            auto parent = images.find(rect.parent_name);
            if(parent == images.end()) {
                log_warning("Rect entry \"{}\" has an invalid parent name \"{}\"",
                    name, rect.parent_name
                );
                return false;
            }

            TexbinRectEntry entry;
            entry.image_id = (uint32_t)distance(images.begin(), parent);
            entry.x1 = rect.x;
            entry.y1 = rect.y;
            entry.x2 = rect.x + rect.w;
            entry.y2 = rect.y + rect.h;
            f.write((char*)&entry, sizeof(entry));
        }

        uint32_t rect_end = (uint32_t)f.tellp();
        rect_hdr.sect_size = rect_end - hdr.rect_offset;
        rect_hdr.image_count = (uint32_t)rects.size();
        f.seekp(hdr.rect_offset);
        f.write((char*)&rect_hdr, sizeof(rect_hdr));
        f.seekp(0, std::ios::end);
    } else {
        hdr.rect_offset = 0;
    }

    // update header with actual written values
    hdr.archive_size = (uint32_t)f.tellp();
    hdr.file_count = (uint32_t)images.size();

    f.seekp(0);
    f.write((char*)&hdr, sizeof(hdr));

    return true;
}

std::pair<uint16_t, uint16_t> ImageEntryParsed::peek_dimensions() {
    auto raw = texbin_lz77_decompress(tex, 0x40);
    if(raw.size() < 0x40) {
        return std::make_pair(0, 0);
    }

    auto hdr = reinterpret_cast<TexHdr*>(&raw[0]);
    // note: texbintool has a different check. I think this is better?
    if(hdr->magic == "TDXT") {
        // little endian
        return std::make_pair(hdr->width, hdr->height);
    } else if(hdr->magic == "TXDT") {
        return std::make_pair(_byteswap_ushort(hdr->width), _byteswap_ushort(hdr->height));
    } else {
        return std::make_pair(0, 0);
    }
}

enum TexFormat: uint8_t {
    GRAYSCALE_2 = 0x06,
    GRAYSCALE   = 0x01,
    BGR_16BIT   = 0x0C,
    BGRA_16BIT  = 0x0D,
    BGR         = 0x0E,
    BGRA        = 0x10,
    BGR_4BIT    = 0x11,
    BGR_8BIT    = 0x12,
    DXT1        = 0x16,
    DXT3        = 0x18,
    DXT5        = 0x1A,
};

std::string ImageEntryParsed::tex_type_str(bool debug_lz77) {
    auto raw = texbin_lz77_decompress(tex, 0x40, debug_lz77);
    if(raw.size() < 0x40) {
        return "SHORT TEX " + std::to_string(raw.size());
    }

    auto hdr = reinterpret_cast<TexHdr*>(&raw[0]);
    // note: texbintool has a different check. I think this is better?
    if(hdr->magic == "TDXT") {
        // little endian, nothing to do
    } else if(hdr->magic == "TXDT") {
        hdr->format1 = _byteswap_ulong(hdr->format1);
    } else {
        return "BAD TEX";
    }

    switch(hdr->format1 & 0xFF) {
        case TexFormat::GRAYSCALE_2: return "GRAYSCALE_2";
        case TexFormat::GRAYSCALE:   return "GRAYSCALE";
        case TexFormat::BGR_16BIT:   return "BGR_16BIT";
        case TexFormat::BGRA_16BIT:  return "BGRA_16BIT";
        case TexFormat::BGR:         return "BGR";
        case TexFormat::BGRA:        return "BGRA";
        case TexFormat::BGR_4BIT:    return "BGR_4BIT";
        case TexFormat::BGR_8BIT:    return "BGR_8BIT";
        case TexFormat::DXT1:        return "DXT1";
        case TexFormat::DXT3:        return "DXT3";
        case TexFormat::DXT5:        return "DXT5";

        default: return "UNK " + std::to_string(hdr->format1 & 0xFF);
    }
}

std::optional<std::tuple<std::vector<uint8_t>, uint16_t, uint16_t>> ImageEntryParsed::tex_to_argb8888() {
    auto raw = texbin_lz77_decompress(tex);
    if(raw.size() < 0x40) {
        return std::nullopt;
    }

    auto hdr = reinterpret_cast<TexHdr*>(&raw[0]);
    std::vector<uint8_t> data(&raw[0x40], &raw[raw.size()]);
    // note: texbintool has a different check. I think this is better?
    if(hdr->magic == "TDXT") {
        // little endian, nothing to do
    } else if(hdr->magic == "TXDT") {
        hdr->width = _byteswap_ushort(hdr->width);
        hdr->height = _byteswap_ushort(hdr->height);
        hdr->format1 = _byteswap_ulong(hdr->format1);
    } else {
        log_warning("Not a TXDT file");
        return std::nullopt;
    }

    // happy path, already the right format
    if((hdr->format1 & 0xFF) == TexFormat::BGRA) {
        return make_tuple(data, hdr->width, hdr->height);
    }

    // useful constants
    size_t pixel_count = hdr->width * hdr->height;
    size_t out_sz_bytes = pixel_count * 4;

    std::vector<uint8_t> out_data;
    out_data.resize(out_sz_bytes);

    LodePNGColorMode out_mode;
    out_mode.colortype = LCT_RGBA;
    out_mode.bitdepth = 8;

    LodePNGColorMode in_mode;
    in_mode.key_defined = 0; // we never use a transparency key
    // rest of values left to the individual formats

    // note: the supported types were run based on texture analysis of my rhythm
    // game folder. It should cover most Gitadora/Jubeat scenarios.

    // todo: maybe do some bounds checks here?
    switch(hdr->format1 & 0xFF) {
        case TexFormat::GRAYSCALE:
        case TexFormat::GRAYSCALE_2:
            in_mode.colortype = LCT_GREY;
            in_mode.bitdepth = 8;
            lodepng_convert(&out_data[0], &data[0], &out_mode, &in_mode, hdr->width, hdr->height);
            break;

        case TexFormat::BGR_16BIT: // rgb565?
            for(size_t i = 0; i < pixel_count; i++) {
                size_t oi = i*4;
                size_t ii = i*2;

                // really don't know how this bit munging works, but it does
                data[ii] = ((data[ii] & 0xc0) | (data[ii] & 0x3f) >> 1);
                uint16_t pix = (data[ii] << 0) | (data[ii+1] << 8);

                // https://stackoverflow.com/a/9069480/7972801
                out_data[oi+0] = (((pix & 0xF800) >> 11) * 527 + 23) >> 6 ;
                out_data[oi+1] = (((pix & 0x07E0) >> 5 ) * 259 + 33) >> 6 ;
                out_data[oi+2] = (((pix & 0x001F)      ) * 527 + 23) >> 6 ;
                out_data[oi+3] = 0xFF;
            }
            break;

        case TexFormat::BGRA_16BIT:
            for(size_t i = 0; i < pixel_count; i++) {
                size_t oi = i*4;
                size_t ii = i*2;
                uint16_t pix = data[ii] | (data[ii+1] << 8);
                uint16_t a = (pix & 0x000F) >> 0;
                uint16_t b = (pix & 0x00F0) >> 4;
                uint16_t g = (pix & 0x0F00) >> 8;
                uint16_t r = (pix & 0xF000) >> 12;
                out_data[oi+0] = r | (r << 4);
                out_data[oi+1] = g | (g << 4);
                out_data[oi+2] = b | (b << 4);
                out_data[oi+3] = a | (a << 4);
            }
            break;

        case TexFormat::BGR: // todo: might be nice to support no-alpha textures
            in_mode.colortype = LCT_RGB;
            in_mode.bitdepth = 8;
            lodepng_convert(&out_data[0], &data[0], &out_mode, &in_mode, hdr->width, hdr->height);
            break;

        // already handled above
        // case TexFormat::BGRA:
        //     return make_tuple(data, hdr->width, hdr->height);

        case TexFormat::BGR_4BIT:
            log_warning("Unsupported format BGR_4BIT, raise an issue about it!");
            return std::nullopt;

            // don't actually have a test file to validate against
            // in_mode.colortype = LCT_PALETTE;
            // in_mode.bitdepth = 4;
            // // 0x14 byte palette header we skip
            // in_mode.palette = &data[pixel_count + 0x14];
            // in_mode.palettesize = (data.size() - pixel_count - 0x14) / 4;
            // lodepng_convert(&out_data[0], &data[0], &out_mode, &in_mode, hdr->width, hdr->height);
            // break;

        case TexFormat::BGR_8BIT:
            log_warning("Unsupported format BGR_8BIT, raise an issue about it!");
            return std::nullopt;

            // don't actually have a test file to validate against
            // in_mode.colortype = LCT_PALETTE;
            // in_mode.bitdepth = 8;
            // // 0x14 byte palette header we skip
            // in_mode.palette = &data[pixel_count + 0x14];
            // in_mode.palettesize = (data.size() - pixel_count - 0x14) / 4;
            // lodepng_convert(&out_data[0], &data[0], &out_mode, &in_mode, hdr->width, hdr->height);
            // break;

        case TexFormat::DXT1:
            squish::DecompressImage(&out_data[0], hdr->width, hdr->height, &data[0], squish::kDxt1);
            break;

        case TexFormat::DXT3:
            squish::DecompressImage(&out_data[0], hdr->width, hdr->height, &data[0], squish::kDxt3);
            break;

        case TexFormat::DXT5:
            squish::DecompressImage(&out_data[0], hdr->width, hdr->height, &data[0], squish::kDxt5);
            break;

        default:
            log_warning("Unsupported tex format type {:#x}", hdr->format1 & 0xFF);
            return std::nullopt;
    }

    return make_tuple(out_data, hdr->width, hdr->height);
}

// Based on: https://github.com/littlecxm/gitadora-textool/blob/fb55c4b813994fb46edecef358319432c17fe072/gitadora-texbintool/Program.cs#L174
// Which itself is based on: https://github.com/gdkchan/LegaiaText/blob/bbec0465428a9ff1858e4177588599629ca43302/LegaiaText/Legaia/Compression/LZSS.cs
// Many thanks to windyfairy for this, without which this layeredfs feature would
// not exist
std::vector<uint8_t> texbin_lz77_compress(const std::vector<uint8_t> &data) {
    std::vector<uint8_t> output(8, 0); // fill 8 bytes for header
    output.reserve(data.size()); // should be performant enough

    uint64_t lookup[0x10000];

    std::vector<uint8_t> dict(0x1000, 0);

    size_t dict_i = 4078;
    size_t data_i = 0;
    size_t bits_i = 0;

    uint16_t mask = 0x80;

    uint8_t header = 0;

    while (data_i < data.size()) {
        if ((mask <<= 1) == 0x100) {
            output[bits_i] = header;

            bits_i = output.size();
            output.push_back(0);

            header = 0;
            mask = 1;
        }

        uint32_t length = 2;
        int32_t dict_pos = 0;

        if (data_i + 2 < data.size()) {
            uint32_t value;

            value = data[data_i + 0] << 8;
            value |= data[data_i + 1] << 0;

            for (int32_t i = 0; i < 5; i++) {
                uint32_t index = (uint32_t)((lookup[value] >> (i * 12)) & 0xfff);

                //First byte doesn't match, so the others won't match too
                if (data[data_i] != dict[index]) break;

                //Temporary dictionary used on comparisons
                auto cmp_dict = dict;
                size_t cmp_addr = dict_i;

                uint32_t match_len = 0;

                for (int32_t j = 0; j < 18 && data_i + j < data.size(); j++) {
                    if (cmp_dict[(index + j) & 0xfff] == data[data_i + j])
                        match_len++;
                    else
                        break;

                    cmp_dict[cmp_addr] = data[data_i + j];
                    cmp_addr = (cmp_addr + 1) & 0xfff;
                }

                if (match_len > length && match_len < output.size()) {
                    length = match_len;
                    dict_pos = index;
                }
            }
        }

        if (length > 2) {
            output.push_back(dict_pos);

            uint32_t niblo = (length - 3) & 0xf;
            uint32_t nibhi = (dict_pos >> 4) & 0xf0;

            output.push_back((niblo | nibhi));
        } else {
            header |= (uint8_t)mask;

            output.push_back(data[data_i]);

            length = 1;
        }

        for (uint32_t i = 0; i < length; i++) {
            if (data_i + 1 < data.size()) {
                uint32_t value;

                value = data[data_i + 0] << 8;
                value |= data[data_i + 1] << 0;

                lookup[value] <<= 12;
                lookup[value] |= (uint32_t)dict_i;
            }

            dict[dict_i] = data[data_i++];
            dict_i = (dict_i + 1) & 0xfff;
        }
    }

    output[bits_i] = header;
    *(uint32_t*)&output[0] = _byteswap_ulong((uint32_t)data.size());
    *(uint32_t*)&output[4] = _byteswap_ulong((uint32_t)(output.size() - 8));

    return output;
}

std::vector<uint8_t> texbin_lz77_decompress(const std::vector<uint8_t> &comp_with_hdr, size_t max_len, bool debug) {
    size_t decomp_len = _byteswap_ulong(*(uint32_t*)&comp_with_hdr[0]);
    size_t comp_len = _byteswap_ulong(*(uint32_t*)&comp_with_hdr[4]);
    auto comp = &comp_with_hdr[8];
    if(debug) {
        VLOG("%s: Comp sz %u decomp sz %u (clamp: %d)",
            __FUNCTION__, comp_len, decomp_len, max_len
        );
    }
    // optionally extract only the first n bytes
    if(max_len) {
        decomp_len = std::min(max_len, decomp_len);
    }

    // actually not compressed
    if(comp_len == 0) {
        size_t data_len = std::min(decomp_len, comp_with_hdr.size() - 8);
        return std::vector<uint8_t>(comp, comp + data_len);
    }

    std::vector<uint8_t> decomp;
    decomp.reserve(decomp_len);

    uint8_t window[4096] = {0};
    uint32_t comp_i = 0, window_i = 4078;

    uint8_t control = 0;

    while (comp_i < comp_len) {
        control = comp[comp_i++];

        for(auto i = 0; i < 8 && decomp.size() < decomp_len; i++) {
            if ((control & 0x01) != 0) {
                if (comp_i >= comp_len) {
                    return decomp;
                }

                decomp.push_back(window[window_i++] = comp[comp_i++]);

                window_i &= 0xfff;
            } else {
                size_t slide_off = (((comp[comp_i + 1] & 0xf0) << 4) | comp[comp_i]) & 0xfff;
                size_t slide_len = (comp[comp_i + 1] & 0x0f) + 3;
                comp_i += 2;

                if (decomp.size() + slide_len > decomp_len) {
                    slide_len = decomp_len - decomp.size();
                }

                while (slide_len > 0) {
                    decomp.push_back(window[window_i++] = window[slide_off++]);

                    window_i &= 0xfff;
                    slide_off &= 0xfff;
                    slide_len--;
                }
            }

            control >>= 1;
        }
    }

    return decomp;
}
