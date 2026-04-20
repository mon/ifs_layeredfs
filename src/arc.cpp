#include "arc.hpp"

#include <cstring>
#include <fstream>

#include "avs.h"
#include "log.hpp"

#pragma pack(push, 1)
struct ArcHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t filecount;
    uint32_t compression;
};

struct ArcEntry {
    uint32_t str_offset;
    uint32_t file_offset;
    uint32_t unpacked_size;
    uint32_t packed_size;
};
#pragma pack(pop)

static constexpr uint32_t ARC_MAGIC             = 0x19751120;
static constexpr uint32_t ARC_VERSION           = 1;
static constexpr uint32_t ARC_COMPRESSION_AVSLZ = 2;

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
        std::vector<uint8_t> data(e.packed_size);
        if (!stream.read(reinterpret_cast<char*>(data.data()), e.packed_size)) {
            log_warning("arc: couldn't read data for '%s'", name.c_str());
            return std::nullopt;
        }

        arc.files[name] = {std::move(data), e.unpacked_size};
    }

    return arc;
}

void ArcArchive::add_or_replace(std::string const& name, std::vector<uint8_t> data) {
    uint32_t unpacked_size = (uint32_t)data.size();
    size_t compressed_size;
    auto compressed = lz_compress(data.data(), data.size(), &compressed_size);
    if (compressed) {
        files[name] = {std::vector<uint8_t>(compressed, compressed + compressed_size), unpacked_size};
        free(compressed);
    } else {
        log_warning("arc: compression failed for '%s', storing uncompressed", name.c_str());
        files[name] = {std::move(data), unpacked_size};
    }
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

    // String table immediately after header + entries
    uint32_t str_table_offset = sizeof(ArcHeader) + filecount * sizeof(ArcEntry);
    std::vector<uint32_t> str_offsets;
    str_offsets.reserve(filecount);
    uint32_t str_cursor = str_table_offset;
    for (auto &[name, _] : files) {
        str_offsets.push_back(str_cursor);
        str_cursor += (uint32_t)name.size() + 1;
    }

    // File data starts at the next 64-byte boundary after the string table
    uint32_t data_start = align_up(str_cursor, ARC_ALIGN);
    std::vector<uint32_t> file_offsets;
    file_offsets.reserve(filecount);
    uint32_t data_cursor = data_start;
    for (auto &[_, file] : files) {
        file_offsets.push_back(data_cursor);
        // Each file is padded to 64-byte boundary
        data_cursor = align_up(data_cursor + (uint32_t)file.packed_data.size(), ARC_ALIGN);
    }

    ArcHeader hdr = {ARC_MAGIC, ARC_VERSION, filecount, ARC_COMPRESSION_AVSLZ};
    f.write(reinterpret_cast<char*>(&hdr), sizeof(hdr));

    size_t i = 0;
    for (auto &[_, file] : files) {
        ArcEntry e;
        e.str_offset    = str_offsets[i];
        e.file_offset   = file_offsets[i];
        e.unpacked_size = file.unpacked_size;
        e.packed_size   = (uint32_t)file.packed_data.size();
        f.write(reinterpret_cast<char*>(&e), sizeof(e));
        i++;
    }

    for (auto &[name, _] : files) {
        f.write(name.c_str(), name.size() + 1);
    }

    // Pad string table to 64-byte boundary
    static const char zeros[ARC_ALIGN] = {};
    uint32_t initial_pad = data_start - str_cursor;
    f.write(zeros, initial_pad);

    for (auto &[_, file] : files) {
        f.write(reinterpret_cast<const char*>(file.packed_data.data()), file.packed_data.size());
        uint32_t pad = align_up((uint32_t)file.packed_data.size(), ARC_ALIGN) - (uint32_t)file.packed_data.size();
        f.write(zeros, pad);
    }

    return f.good();
}
