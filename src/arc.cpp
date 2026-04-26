#include "arc.hpp"

#include <cstring>
#include <fstream>

#include "avs.h"
#include "log.hpp"

std::optional<ArcArchive> ArcArchive::from_stream(std::istream &stream) {
    ArcHeader hdr;
    if (!stream.read(reinterpret_cast<char*>(&hdr), sizeof(hdr))) {
        log_warning("arc: couldn't read header");
        return std::nullopt;
    }
    if (hdr.magic != ARC_MAGIC) {
        log_warning("arc: bad magic %08x", hdr.magic);
        return std::nullopt;
    }
    if (hdr.compression != ARC_COMPRESSION_NONE && hdr.compression != ARC_COMPRESSION_AVSLZ) {
        log_warning("arc: unknown compression %u", hdr.compression);
        return std::nullopt;
    }

    std::vector<ArcEntry> entries(hdr.filecount);
    if (!stream.read(reinterpret_cast<char*>(entries.data()), hdr.filecount * sizeof(ArcEntry))) {
        log_warning("arc: couldn't read entries");
        return std::nullopt;
    }

    ArcArchive arc;
    for (auto &e : entries) {
        stream.seekg(e.str_offset);
        std::string name;
        std::getline(stream, name, '\0');

        stream.seekg(e.file_offset);
        std::vector<uint8_t> packed(e.packed_size);
        if (!stream.read(reinterpret_cast<char*>(packed.data()), e.packed_size)) {
            log_warning("arc: couldn't read data for '%s'", name.c_str());
            return std::nullopt;
        }

        if (hdr.compression == ARC_COMPRESSION_AVSLZ && e.packed_size != e.unpacked_size) {
            size_t out_size = e.unpacked_size;
            auto raw = lz_decompress(packed.data(), packed.size(), &out_size);
            if (!raw || out_size != e.unpacked_size) {
                log_warning("arc: decompression failed for '%s'", name.c_str());
                if (raw) free(raw);
                return std::nullopt;
            }
            arc.files[name] = std::vector<uint8_t>(raw, raw + out_size);
            free(raw);
        } else {
            arc.files[name] = std::move(packed);
        }
    }

    return arc;
}

void ArcArchive::add_or_replace(std::string const& name, std::vector<uint8_t> data) {
    files[name] = std::move(data);
}

static constexpr uint32_t ARC_ALIGN = 64;

static uint32_t align_up(uint32_t v, uint32_t align) {
    return (v + align - 1) & ~(align - 1);
}

bool ArcArchive::save(const char* path) {
    std::ofstream f(path, std::ios::binary);
    if (!f) {
        log_warning("arc: couldn't open '%s' for writing", path);
        return false;
    }

    uint32_t filecount = (uint32_t)files.size();

    uint32_t str_table_offset = sizeof(ArcHeader) + filecount * sizeof(ArcEntry);
    std::vector<uint32_t> str_offsets;
    str_offsets.reserve(filecount);
    uint32_t str_cursor = str_table_offset;
    for (auto &[name, _] : files) {
        str_offsets.push_back(str_cursor);
        str_cursor += (uint32_t)name.size() + 1;
    }

    uint32_t data_start = align_up(str_cursor, ARC_ALIGN);
    std::vector<uint32_t> file_offsets;
    file_offsets.reserve(filecount);
    uint32_t data_cursor = data_start;
    for (auto &[_, data] : files) {
        file_offsets.push_back(data_cursor);
        data_cursor = align_up(data_cursor + (uint32_t)data.size(), ARC_ALIGN);
    }

    ArcHeader hdr = {ARC_MAGIC, ARC_VERSION, filecount, ARC_COMPRESSION_NONE};
    f.write(reinterpret_cast<char*>(&hdr), sizeof(hdr));

    size_t i = 0;
    for (auto &[_, data] : files) {
        ArcEntry e;
        e.str_offset    = str_offsets[i];
        e.file_offset   = file_offsets[i];
        e.unpacked_size = (uint32_t)data.size();
        e.packed_size   = (uint32_t)data.size();
        f.write(reinterpret_cast<char*>(&e), sizeof(e));
        i++;
    }

    for (auto &[name, _] : files) {
        f.write(name.c_str(), name.size() + 1);
    }

    static const char zeros[ARC_ALIGN] = {};
    uint32_t initial_pad = data_start - str_cursor;
    f.write(zeros, initial_pad);

    for (auto &[_, data] : files) {
        f.write(reinterpret_cast<const char*>(data.data()), data.size());
        uint32_t pad = align_up((uint32_t)data.size(), ARC_ALIGN) - (uint32_t)data.size();
        f.write(zeros, pad);
    }

    return f.good();
}
