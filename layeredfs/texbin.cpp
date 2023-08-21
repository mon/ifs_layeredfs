#define LOG_MODULE "layeredfs-texbin"

#include <cinttypes>
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

using namespace std;
using std::nullopt;

#ifdef TEXBIN_VERBOSE
#define VLOG(...) log_misc(__VA_ARGS__)
#else
#define VLOG(...)
#endif

// raw data from bin
#pragma pack(push,1)

class TexbinHdr {
    public:
    char magic[4] = {'P','X','E','T'};
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

        log_misc("  archive size:      %" PRId32, archive_size);
        log_misc("  file count:        %" PRId32, file_count);
        log_misc("  data offset:       %" PRId32, data_offset);
        log_misc("  rect offset:       %" PRId32, rect_offset);
        log_misc("  name offset:       %" PRId32, name_offset);
        log_misc("  data entry offset: %" PRId32, data_entry_offset);
    }
};

class TexbinNamesHdr {
    public:
    char magic[4] = {'P','M','A','N'};
    uint32_t sect_size; // includes header
    uint8_t unk1[8] = { 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00 };
    uint32_t names_count;
    // probably some sort of hash bucket sizes/masks
    uint16_t unkA; // 1 << bit_length(names_count / 4)
    uint16_t unkB; // (1 << bit_length(names_count / 2)) - 1
    uint32_t header_len = sizeof(TexbinNamesHdr);

    void debug() {
        log_misc("texbin names hdr");

        log_misc("  section size:      %" PRId32, sect_size);
        log_misc("  names count:       %" PRId32, names_count);
    }
};

class TexbinRectHdr {
    public:
    char magic[4] = {'T','C','E','R'};
    uint8_t unk1[8] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00};
    uint32_t sect_size; // includes header
    uint32_t image_count;
    uint32_t name_offset; // from the start of the header
    uint32_t rect_entry_offset; // from the start of the header

    void debug() {
        log_misc("texbin rect hdr");

        log_misc("  section size:      %" PRId32, sect_size);
        log_misc("  image count:       %" PRId32, image_count);
        log_misc("  name offset:       %" PRId32, name_offset);
        log_misc("  entries offset:    %" PRId32, rect_entry_offset);
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

#pragma pack(pop)

// texbintool always sets little_endian=true, unsure where it's seen most often
static optional<vector<uint8_t>> image_data_from_png(const char *path, bool little_endian = true) {
    unsigned error;
    unsigned char *image;
    unsigned width, height;
    error = lodepng_decode32_file(&image, &width, &height, path);
    if (error) {
        log_warning("can't load png %u: %s\n", error, lodepng_error_text(error));
        return nullopt;
    }

    // TODO: width/height check

    size_t image_size = 4 * width * height;

    vector<uint8_t> data;
    data.reserve(0x40 + image_size);
    auto write = [&data, little_endian] (const vector<uint8_t> &bytes) {
        if(little_endian) {
            data.insert(data.end(),bytes.rbegin(),bytes.rend());
        } else {
            data.insert(data.end(),bytes.begin(),bytes.end());
        }
    };
    auto write_int = [&data, little_endian] (auto val) {
        uint8_t b[sizeof(val)];
        memcpy(b, &val, sizeof(val));
        vector v(b, b + sizeof(val));

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

static vector<string> load_names(ifstream &f, uint32_t name_offset) {
    vector<string> ret;

    f.seekg(name_offset);

    TexbinNamesHdr name_hdr;
    if(!f.read((char*)&name_hdr, sizeof(name_hdr))) {
        log_warning("bad names");
        return ret;
    }

    if(memcmp(name_hdr.magic, "PMAN", sizeof(name_hdr.magic))) {
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
            log_warning("bad name entry at %" PRId32, i);
            return ret;
        }
        auto pos = f.tellg();

        f.seekg(name_offset + entry.str_offset);
        string name = "";
        char ch;
        while(f.get(ch) && ch > '\0') {
            name += ch;
        }
        ret[entry.id] = name;

        f.seekg(pos);
    }

    return ret;
}

static vector<vector<uint8_t>> load_data(ifstream &f, const TexbinHdr& hdr) {
    vector<vector<uint8_t>> ret;
    ret.reserve(hdr.file_count);

    f.seekg(hdr.data_entry_offset);

    TexbinDataEntry entry;
    for(uint32_t i = 0; i < hdr.file_count; i++) {
        if(!f.read((char*)&entry, sizeof(entry))) {
            log_warning("bad data entry at %" PRId32, i);
            return ret;
        }
        auto pos = f.tellg();

        f.seekg(entry.offset);
        vector<uint8_t> data;
        data.resize(entry.size);
        if(!f.read((char*)&data[0], entry.size)) {
            log_warning("can't read data at %i" PRId32, i);
            return ret;
        }
        ret.push_back(data);

        f.seekg(pos);
    }

    return ret;
}

string basename_without_extension(string const & path) {
    auto basename = path.substr(path.find_last_of("/\\") + 1);
    string::size_type const p(basename.find_last_of('.'));
    return p > 0 && p != string::npos ? basename.substr(0, p) : basename;
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
            hash = (hash >> 31) & 0x4C11DB7 ^ ((hash << 1) | ((*name >> i) & 1));
        }
    }

    return hash;
}

void pad32(ofstream &f) {
    // pad to 4 bytes
    auto pad = 4 - (f.tellp() % 4);
    if(pad != 4) {
        char zeros[4] = {0};
        f.write(zeros, pad);
    }
}

template<typename T>
void write_names(ofstream &f, unordered_map<string, T> &names) {
    auto start = f.tellp();
    TexbinNamesHdr hdr;
    f.write((char*)&hdr, sizeof(hdr)); // update with real values later

    // Hashes are written in ascending order. Ensure this by using the (sorted)
    // std::map
    map<uint32_t, TexbinNameEntry> entries;
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
    f.seekp(0, ios::end);
}

bool Texbin::add_or_replace_image(const char *image_name, const char *png_path) {
    auto _data = image_data_from_png(png_path);
    if(!_data) {
        log_warning("Could not load %s", png_path);
        return false;
    }
    auto data = *_data;

    auto existing_image = images.find(image_name);
    auto existing_rect = rects.find(image_name);
    if(existing_image != images.end()) {
        log_info("Replacing %s", image_name);
        images[image_name] = data;
    } else if(existing_rect != rects.end()) {
        // TODO: this might not work! Needs testing!
        log_info("Replacing rect image %s with standalone", image_name);
        images[image_name] = data;
        rects.erase(existing_rect);
    } else{
        log_info("Adding new image %s", image_name);
        images[image_name] = data;
    }

    return true;
}

void Texbin::debug() {
    uint32_t total = 0;
    for(auto &[name, image] : images) {
        VLOG("file: %s len %d\n", name.c_str(), image.size());
        total += (uint32_t)image.size();
    }

    for(auto &[name, rect] : rects) {
        VLOG("rect: %s parent %s dims (%d,%d,%d,%d)\n",
            name.c_str(), rect.parent_name.c_str(),
            rect.x, rect.y, rect.w, rect.h
        );
    }

    VLOG("Total data: %d\n", total);
}

optional<Texbin> Texbin::from_path(const char *path) {
    // there are a handful of .bin files we might try to parse that *aren't*
    // texbins, so gate all logs before header magic check behind log_verbose
    log_verbose("Opening %s", path);
    auto basename = basename_without_extension(path);
    ifstream f (path, ios::binary | ios::ate);
    if(!f) {
        log_verbose("cannot open");
        return nullopt;
    }
    auto file_len = f.tellg();
    f.seekg(0);

    TexbinHdr hdr;
    if(!f.read((char*)&hdr, sizeof(hdr))) {
        log_verbose("cannot read header");
        return nullopt;
    }

    if(memcmp(hdr.magic, "PXET", sizeof(hdr.magic))) {
        log_verbose("bad magic");
        return nullopt;
    }

    if(hdr.archive_size != file_len) {
        log_warning("bad archive size");
        return nullopt;
    }

#ifdef TEXBIN_VERBOSE
    hdr.debug();
#endif

    auto names = load_names(f, 0 + hdr.name_offset);
    if(names.size() != hdr.file_count) {
        log_warning("Name section mismatch against files");
        return nullopt;
    }

    auto data = load_data(f, hdr);

    unordered_map<string, vector<uint8_t>> images;
    images.reserve(hdr.file_count);

    for(uint32_t i = 0; i < hdr.file_count; i++) {
        images[names[i]] = data[i];
    }

    unordered_map<string, RectEntryParsed> rects;
    if(hdr.rect_offset) {
        TexbinRectHdr rect_hdr;
        f.seekg(hdr.rect_offset);
        if(!f.read((char*)&rect_hdr, sizeof(rect_hdr))) {
            log_warning("cannot read rect header");
            return nullopt;
        }
        if(memcmp(rect_hdr.magic, "TCER", sizeof(hdr.magic))) {
            log_warning("bad rect magic");
            return nullopt;
        }

#ifdef TEXBIN_VERBOSE
        rect_hdr.debug();
#endif

        auto rect_names = load_names(f, hdr.rect_offset + rect_hdr.name_offset);

        if(rect_names.size() != rect_hdr.image_count) {
            log_warning("Rect name section mismatch against files");
            return nullopt;
        }

        f.seekg(hdr.rect_offset + rect_hdr.rect_entry_offset);
        rects.reserve(rect_hdr.image_count);
        for(uint32_t i = 0; i < rect_hdr.image_count; i++) {
            TexbinRectEntry entry;
            if(!f.read((char*)&entry, sizeof(entry))) {
                log_warning("cannot read rect entry");
                return nullopt;
            }

            if(entry.image_id >= names.size()) {
                log_warning("rect entry refers to invalid parent");
                return nullopt;
            }

            if(entry.x1 >= entry.x2 || entry.y1 >= entry.y2) {
                log_warning("rect entry has invalid dimensions (%d,%d,%d,%d)",
                    entry.x1, entry.x2, entry.y1, entry.y2
                );
                return nullopt;
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

    return Texbin(basename, images, rects);
}

bool Texbin::save(const char *dest) {
    ofstream f(dest, ios::binary);
    if(!f) {
        log_warning("Can't open output");
        return false;
    }

    TexbinHdr hdr;

    f.write((char*)&hdr, sizeof(hdr)); // updated later
    hdr.name_offset = (uint32_t)f.tellp();
    write_names(f, images);

    hdr.data_entry_offset = (uint32_t)f.tellp();
    uint32_t data_offset = hdr.data_entry_offset + (uint32_t)(images.size() * sizeof(TexbinDataEntry));
    for(auto &[_name, data] : images) {
        TexbinDataEntry entry;
        entry.size = (uint32_t)data.size();
        entry.offset = data_offset;
        f.write((char*)&entry, sizeof(entry));

        data_offset += (uint32_t)data.size();
        uint32_t pad = 4 - (data.size() % 4);
        if(pad != 4) {
            data_offset += pad;
        }
    }

    hdr.data_offset = (uint32_t)f.tellp();

    for(auto &[_name, data] : images) {
        f.write((char*)&data[0], data.size());
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
                log_warning("Rect entry \"%s\" has an invalid parent name \"%s\"",
                    name.c_str(), rect.parent_name.c_str()
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
        f.seekp(0, ios::end);
    }

    // update header with actual written values
    hdr.archive_size = (uint32_t)f.tellp();
    hdr.file_count = (uint32_t)images.size();

    f.seekp(0);
    f.write((char*)&hdr, sizeof(hdr));

    return true;
}

// Based on: https://github.com/littlecxm/gitadora-textool/blob/fb55c4b813994fb46edecef358319432c17fe072/gitadora-texbintool/Program.cs#L174
// Which itself is based on: https://github.com/gdkchan/LegaiaText/blob/bbec0465428a9ff1858e4177588599629ca43302/LegaiaText/Legaia/Compression/LZSS.cs
// Many thanks to windyfairy for this, without which this layeredfs feature would
// not exist
vector<uint8_t> texbin_lz77_compress(const vector<uint8_t> &data) {
    vector<uint8_t> output(8, 0); // fill 8 bytes for header
    output.reserve(data.size()); // should be performant enough

    uint64_t lookup[0x10000];

    vector<uint8_t> dict(0x1000, 0);

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

        int32_t Length = 2;
        int32_t DictPos = 0;

        if (data_i + 2 < data.size()) {
            int32_t Value;

            Value = data[data_i + 0] << 8;
            Value |= data[data_i + 1] << 0;

            for (int32_t i = 0; i < 5; i++) {
                int32_t Index = (int32_t)((lookup[Value] >> (i * 12)) & 0xfff);

                //First byte doesn't match, so the others won't match too
                if (data[data_i] != dict[Index]) break;

                //Temporary dictionary used on comparisons
                auto CmpDict = dict;
                size_t CmpAddr = dict_i;

                int32_t MatchLen = 0;

                for (int32_t j = 0; j < 18 && data_i + j < data.size(); j++)
                {
                    if (CmpDict[(Index + j) & 0xfff] == data[data_i + j])
                        MatchLen++;
                    else
                        break;

                    CmpDict[CmpAddr] = data[data_i + j];
                    CmpAddr = (CmpAddr + 1) & 0xfff;
                }

                if (MatchLen > Length && MatchLen < output.size())
                {
                    Length = MatchLen;
                    DictPos = Index;
                }
            }
        }

        if (Length > 2)
        {
            output.push_back(DictPos);

            int32_t NibLo = (Length - 3) & 0xf;
            int32_t NibHi = (DictPos >> 4) & 0xf0;

            output.push_back((NibLo | NibHi));
        }
        else
        {
            header |= (uint8_t)mask;

            output.push_back(data[data_i]);

            Length = 1;
        }

        for (int32_t i = 0; i < Length; i++)
        {
            if (data_i + 1 < data.size())
            {
                int32_t Value;

                Value = data[data_i + 0] << 8;
                Value |= data[data_i + 1] << 0;

                lookup[Value] <<= 12;
                lookup[Value] |= (uint32_t)dict_i;
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

vector<uint8_t> texbin_lz77_decompress(const vector<uint8_t> &comp_with_hdr) {
    size_t decomp_len = _byteswap_ulong(*(uint32_t*)&comp_with_hdr[0]);
    size_t comp_len = _byteswap_ulong(*(uint32_t*)&comp_with_hdr[4]);
    auto comp = &comp_with_hdr[8];
    log_info("%s: Comp sz %u decomp sz %u", __FUNCTION__, comp_len, decomp_len);

    vector<uint8_t> decomp;
    decomp.reserve(decomp_len);

    uint8_t window[4096];
    uint32_t comp_i = 0, window_i = 4078;

    uint8_t controlByte = 0;

    while (comp_i < comp_len) {
        controlByte = comp[comp_i++];

        for(auto i = 0; i < 8; i++) {
            if ((controlByte & 0x01) != 0) {
                if (comp_i >= comp_len) {
                    return decomp;
                }

                decomp.push_back(window[window_i++] = comp[comp_i++]);

                if (decomp.size() >= decomp_len) {
                    return decomp;
                }

                window_i &= 0xfff;
            } else {
                if (decomp.size() >= decomp_len - 1) {
                    return decomp;
                }

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

            controlByte >>= 1;
        }
    }

    return decomp;
}
