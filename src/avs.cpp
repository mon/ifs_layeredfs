#include <optional>
#define NOMINMAX
#include <windows.h>
#include <stdio.h>

#include "avs.h"
#include "hook.h"
#include "log.hpp"
#include "3rd_party/MinHook.h"
#include "utils.hpp"

static std::string_view get_prop_error_str(int32_t code);

#define AVS_STRUCT_DEF(ret_type, name, ...) const char* name;

std::string_view avs_loaded_dll_name;
uint16_t avs_loaded_version;

struct avs_exports_t {
    std::string_view version_name;
    uint16_t version;
    // for IIDX vs SDVX cloud, almost all funcs are identical
    const char* unique_check = NULL;

    FOREACH_AVS_FUNC(AVS_STRUCT_DEF)
    FOREACH_AVS_FUNC_OPTIONAL(AVS_STRUCT_DEF)
};

const LPCWSTR dll_names[] = {
    L"libavs-win32.dll",
    L"libavs-win64.dll",
    L"avs2-core.dll",
};

const avs_exports_t avs_exports[] = {
    {
    .version_name                        = "plain (2.12.x and older)",
    .version                             = 1200,
    .log_body_fatal                      = "log_body_fatal",
    .log_body_warning                    = "log_body_warning",
    .log_body_info                       = "log_body_info",
    .log_body_misc                       = "log_body_misc",
    .avs_fs_open                         = "avs_fs_open",
    .avs_fs_close                        = "avs_fs_close",
    .avs_fs_convert_path                 = "avs_fs_convert_path",
    .avs_fs_fstat                        = "avs_fs_fstat",
    .avs_fs_lstat                        = "avs_fs_lstat",
    .avs_fs_lseek                        = "avs_fs_lseek",
    .avs_fs_mount                        = "avs_fs_mount",
    .avs_fs_read                         = "avs_fs_read",
    .property_read_query_memsize         = "property_read_query_memsize",
    .property_create                     = "property_create",
    .property_insert_read                = "property_insert_read",
    .property_mem_write                  = "property_mem_write",
    .property_query_size                 = "property_query_size",
    .property_destroy                    = "property_destroy",
    .cstream_create                      = "cstream_create",
    .cstream_operate                     = "cstream_operate",
    .cstream_finish                      = "cstream_finish",
    .cstream_destroy                     = "cstream_destroy",
    .property_read_query_memsize_long    = "property_read_query_memsize_long",
    },
    {
    .version_name                        = "2.13.x (XC058ba5------)",
    .version                             = 1300,
    .log_body_fatal                      = "XC058ba5000084",
    .log_body_warning                    = "XC058ba50000e1",
    .log_body_info                       = "XC058ba500015a",
    .log_body_misc                       = "XC058ba500002d",
    .avs_fs_open                         = "XC058ba50000b6",
    .avs_fs_close                        = "XC058ba500011b",
    .avs_fs_convert_path                 = "XC058ba50000d5",
    .avs_fs_fstat                        = "XC058ba50000d0",
    .avs_fs_lstat                        = "XC058ba5000063",
    .avs_fs_lseek                        = "XC058ba500000f",
    .avs_fs_mount                        = "XC058ba500009c",
    .avs_fs_read                         = "XC058ba5000139",
    .property_read_query_memsize         = "XC058ba5000066",
    .property_create                     = "XC058ba5000107",
    .property_insert_read                = "XC058ba5000016",
    .property_mem_write                  = "XC058ba5000162",
    .property_query_size                 = "XC058ba5000101",
    .property_destroy                    = "XC058ba500010f",
    .cstream_create                      = "XC058ba5000118",
    .cstream_operate                     = "XC058ba5000078",
    .cstream_finish                      = "XC058ba5000130",
    .cstream_destroy                     = "XC058ba500012b",
    .property_read_query_memsize_long    = "XC058ba5000091",
    },
    {
    .version_name                        = "2.15.x (XCd229cc------)",
    .version                             = 1500,
    .log_body_fatal                      = "XCd229cc0000e6",
    .log_body_warning                    = "XCd229cc000018",
    .log_body_info                       = "XCd229cc0000dc",
    .log_body_misc                       = "XCd229cc000075",
    .avs_fs_open                         = "XCd229cc000090",
    .avs_fs_close                        = "XCd229cc00011f",
    .avs_fs_convert_path                 = "XCd229cc00001e",
    .avs_fs_fstat                        = "XCd229cc0000c3",
    .avs_fs_lstat                        = "XCd229cc0000c0",
    .avs_fs_lseek                        = "XCd229cc00004d",
    .avs_fs_mount                        = "XCd229cc0000ce",
    .avs_fs_read                         = "XCd229cc00010d",
    .property_read_query_memsize         = "XCd229cc0000ff",
    .property_create                     = "XCd229cc000126",
    .property_insert_read                = "XCd229cc00009a",
    .property_mem_write                  = "XCd229cc000033",
    .property_query_size                 = "XCd229cc000032",
    .property_destroy                    = "XCd229cc00013c",
    .cstream_create                      = "XCd229cc000141",
    .cstream_operate                     = "XCd229cc00008c",
    .cstream_finish                      = "XCd229cc000025",
    .cstream_destroy                     = "XCd229cc0000e3",
    .property_read_query_memsize_long    = "XCd229cc00002b",
    },
    {
    .version_name                        = "2.16.[3-7] (XCnbrep7------)",
    .version                             = 1630,
    .unique_check                        = "XCnbrep700013c",
    .log_body_fatal                      = "XCnbrep700017a",
    .log_body_warning                    = "XCnbrep700017b",
    .log_body_info                       = "XCnbrep700017c",
    .log_body_misc                       = "XCnbrep700017d",
    .avs_fs_open                         = "XCnbrep700004e",
    .avs_fs_close                        = "XCnbrep7000055",
    .avs_fs_convert_path                 = "XCnbrep7000046",
    .avs_fs_fstat                        = "XCnbrep7000062",
    .avs_fs_lstat                        = "XCnbrep7000063",
    .avs_fs_lseek                        = "XCnbrep700004f",
    .avs_fs_mount                        = "XCnbrep700004b",
    .avs_fs_read                         = "XCnbrep7000051",
    .property_read_query_memsize         = "XCnbrep70000b0",
    .property_create                     = "XCnbrep7000090",
    .property_insert_read                = "XCnbrep7000094",
    .property_mem_write                  = "XCnbrep70000b8",
    .property_query_size                 = "XCnbrep700009f",
    .property_destroy                    = "XCnbrep7000091",
    .cstream_create                      = "XCnbrep7000130",
    .cstream_operate                     = "XCnbrep7000132",
    .cstream_finish                      = "XCnbrep7000133",
    .cstream_destroy                     = "XCnbrep7000134",
    .property_read_query_memsize_long    = "XCnbrep70000b1",
    },
    { // IIDX
    .version_name                        = "2.16.1 (XCnbrep7 but different)",
    .version                             = 1610,
    .log_body_fatal                      = "XCnbrep7000168",
    .log_body_warning                    = "XCnbrep7000169",
    .log_body_info                       = "XCnbrep700016a",
    .log_body_misc                       = "XCnbrep700016b",
    .avs_fs_open                         = "XCnbrep7000039",
    .avs_fs_close                        = "XCnbrep7000040",
    .avs_fs_convert_path                 = "XCnbrep7000031",
    .avs_fs_fstat                        = "XCnbrep700004d",
    .avs_fs_lstat                        = "XCnbrep700004e",
    .avs_fs_lseek                        = "XCnbrep700003a",
    .avs_fs_mount                        = "XCnbrep7000036",
    .avs_fs_read                         = "XCnbrep700003c",
    .property_read_query_memsize         = "XCnbrep700009b",
    .property_create                     = "XCnbrep700007b",
    .property_insert_read                = "XCnbrep700007f",
    .property_mem_write                  = "XCnbrep70000a3",
    .property_query_size                 = "XCnbrep700008a",
    .property_destroy                    = "XCnbrep700007c",
    .cstream_create                      = "XCnbrep7000124",
    .cstream_operate                     = "XCnbrep7000126",
    .cstream_finish                      = "XCnbrep7000127",
    .cstream_destroy                     = "XCnbrep7000128",
    .property_read_query_memsize_long    = "XCnbrep700009c",
    },
    { // avs 64 bit, pretty much. 2.16.3 with different prefix
    .version_name                        = "2.17.x (XCgsqzn0------)",
    .version                             = 1700,
    .log_body_fatal                      = "XCgsqzn000017a",
    .log_body_warning                    = "XCgsqzn000017b",
    .log_body_info                       = "XCgsqzn000017c",
    .log_body_misc                       = "XCgsqzn000017d",
    .avs_fs_open                         = "XCgsqzn000004e",
    .avs_fs_close                        = "XCgsqzn0000055",
    .avs_fs_convert_path                 = "XCgsqzn0000046",
    .avs_fs_fstat                        = "XCgsqzn0000062",
    .avs_fs_lstat                        = "XCgsqzn0000063",
    .avs_fs_lseek                        = "XCgsqzn000004f",
    .avs_fs_mount                        = "XCgsqzn000004b",
    .avs_fs_read                         = "XCgsqzn0000051",
    .property_read_query_memsize         = "XCgsqzn00000b0",
    .property_create                     = "XCgsqzn0000090",
    .property_insert_read                = "XCgsqzn0000094",
    .property_mem_write                  = "XCgsqzn00000b8",
    .property_query_size                 = "XCgsqzn000009f",
    .property_destroy                    = "XCgsqzn0000091",
    .cstream_create                      = "XCgsqzn0000130",
    .cstream_operate                     = "XCgsqzn0000132",
    .cstream_finish                      = "XCgsqzn0000133",
    .cstream_destroy                     = "XCgsqzn0000134",
    .property_read_query_memsize_long    = "XCgsqzn00000b1",
    },
};

#define AVS_FUNC_PTR(ret_type, name, ...) ret_type (* name )( __VA_ARGS__ );
FOREACH_AVS_FUNC(AVS_FUNC_PTR)
FOREACH_AVS_FUNC_OPTIONAL(AVS_FUNC_PTR)

#define TEST_HOOK_AND_APPLY(func) if (MH_CreateHookApi(dll_name, exports.func, (LPVOID)hook_ ## func, (LPVOID*)&func) != MH_OK || func == NULL) continue
#define LOAD_FUNC(func) if( (func = (decltype(func))GetProcAddress(mod_handle, exports.func)) == NULL) continue
#define CHECK_UNIQUE(func) if( exports.func != NULL && GetProcAddress(mod_handle, exports.func) == NULL) continue

#define AVS_FUNC_LOAD(ret_type, name, ...) LOAD_FUNC(name);
#define AVS_FUNC_LOAD_OPTIONAL(ret_type, name, ...) name = (decltype(name))GetProcAddress(mod_handle, exports.name);

bool init_avs(void) {
    bool success = false;

#ifdef _DEBUG
    for (const auto &exports : avs_exports) {
#define VERBOSE_EXPORT_CHECK(ret_type, name, ...) if(exports.name == NULL) log_warning("MISSING EXPORT {}: {}", i, #name);
        FOREACH_AVS_FUNC(VERBOSE_EXPORT_CHECK)
        FOREACH_AVS_FUNC_OPTIONAL(VERBOSE_EXPORT_CHECK)
    }
#endif

    // presumably we don't load more than 1 unique avs dll.
    // Please don't be proven wrong
    LPCWSTR dll_name = NULL;
    HMODULE mod_handle = NULL;
    for (auto name : dll_names) {
        mod_handle = GetModuleHandleW(name);
        if (mod_handle != NULL) {
            dll_name = name;
            break;
        }
    }

    if (mod_handle == NULL) {
        log_warning("Couldn't find AVS dll in memory");
        return false;
    }

    for (const auto &exports : avs_exports) {
        // make sure this is the right DLL
        CHECK_UNIQUE(unique_check);

        // load all our imports, fail if any cannot be found
        FOREACH_AVS_FUNC(AVS_FUNC_LOAD)
        FOREACH_AVS_FUNC_OPTIONAL(AVS_FUNC_LOAD_OPTIONAL)

        // apply hooks
        TEST_HOOK_AND_APPLY(avs_fs_open);
        TEST_HOOK_AND_APPLY(avs_fs_lstat);
        TEST_HOOK_AND_APPLY(avs_fs_mount);
        TEST_HOOK_AND_APPLY(avs_fs_convert_path);
        TEST_HOOK_AND_APPLY(avs_fs_read);

        success = true;
        avs_loaded_dll_name = exports.version_name;
        avs_loaded_version = exports.version;

        break;
    }
    return success;
}

property_t prop_from_file_handle(AVS_FILE f) {
    void* prop_buffer = NULL;
    property_t prop = NULL;
    int ret;

    int flags = PROP_CREATE_FLAGS;
    int32_t memsize = -1;
    if(property_read_query_memsize_long) { // may not exist
        memsize = property_read_query_memsize_long(avs_fs_read, f, NULL, NULL, NULL);
    }
    if (memsize < 0) {
        // normal prop
        flags &= ~PROP_BIN_PLAIN_NODE_NAMES;

        avs_fs_lseek(f, 0, SEEK_SET);
        memsize = property_read_query_memsize(avs_fs_read, f, NULL, NULL);
        if (memsize < 0) {
            log_warning("Couldn't get memsize {:08X} ({})", memsize, get_prop_error_str(memsize));
            goto FAIL;
        }
    }

    // MUST be 8-byte aligned so prop_free doesn't crash
    prop_buffer = _aligned_malloc((memsize + 7) & ~7, 8);
    if(!prop_buffer) {
        log_warning("_aligned_malloc failed :(");
        goto FAIL;
    }
    prop = property_create(flags, prop_buffer, memsize);
    if (!prop) {
        // double cast to squash truncation warning
        log_warning("Couldn't create prop ({})", get_prop_error_str((int32_t)(size_t)prop));
        goto FAIL;
    }

    avs_fs_lseek(f, 0, SEEK_SET);
    ret = property_insert_read(prop, 0, avs_fs_read, f);
    avs_fs_close(f);

    if (ret < 0) {
        log_warning("Couldn't read prop ({})", get_prop_error_str(ret));
        goto FAIL;
    }

    return prop;

FAIL:
    avs_fs_close(f);
    if (prop)
        property_destroy(prop);
    if (prop_buffer)
        _aligned_free(prop_buffer);
    return NULL;
}

property_t prop_from_file_path(std::string const&path) {
    AVS_FILE f = avs_fs_open(path.c_str(), avs_open_mode_read(), 420);
    if (f < 0) {
        log_warning("Couldn't open prop");
        return NULL;
    }

    return prop_from_file_handle(f);
}

static char* prop_to_xml_string(property_t prop, rapidxml::xml_document<>& allocator) {
    auto prop_size = property_query_size(prop);
    char* xml = allocator.allocate_string(NULL, prop_size + 1);

    auto written = property_mem_write(prop, xml, prop_size);
    if (written > 0) {
        xml[written] = '\0';
    }
    else {
        xml[0] = '\0';
        log_warning("property_mem_write failed ({})", get_prop_error_str(written));
    }

    return xml;
}

char* avs_file_to_string(AVS_FILE f, rapidxml::xml_document<>& allocator) {
    avs_stat stat = {0};
    avs_fs_fstat(f, &stat);
    char* ret = allocator.allocate_string(NULL, stat.filesize + 1);
    avs_fs_read(f, ret, stat.filesize);
    ret[stat.filesize] = '\0';
    return ret;
}

std::vector<uint8_t> avs_file_to_vec(AVS_FILE f) {
    avs_stat stat = {0};
    avs_fs_fstat(f, &stat);
    std::vector<uint8_t> ret;
    ret.resize(stat.filesize);
    avs_fs_read(f, &ret[0], stat.filesize);
    return ret;
}

static bool is_binary_prop(AVS_FILE f) {
    avs_fs_lseek(f, 0, SEEK_SET);
    unsigned char head;
    auto read = avs_fs_read(f, &head, 1);
    bool ret = (read == 1) && head == 0xA0;
    avs_fs_lseek(f, 0, SEEK_SET);
    //logf("detected binary: %s (read %d byte %02x)", ret ? "true": "false", read, head&0xff);
    return ret;
}

bool rapidxml_from_avs_filepath(
    std::string const& path,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_document<>& doc_to_allocate_with
) {
    AVS_FILE f = avs_fs_open(path.c_str(), avs_open_mode_read(), 420);
    if (f < 0) {
        log_warning("Couldn't open prop");
        return false;
    }

    return rapidxml_from_avs_file(f, doc, doc_to_allocate_with);
}

bool rapidxml_from_avs_file(
    AVS_FILE f,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_document<>& doc_to_allocate_with
) {
    // if it's not binary, don't even bother parsing with avs
    char* xml = NULL;
    if (is_binary_prop(f)) {
        auto prop = prop_from_file_handle(f);
        if (!prop)
            return false;

        xml = prop_to_xml_string(prop, doc_to_allocate_with);
        prop_free(prop);
    }
    else {
        xml = avs_file_to_string(f, doc_to_allocate_with);
    }
    avs_fs_close(f);

    try {
        // parse_declaration_node: to get the header <?xml version="1.0" encoding="shift-jis"?>
        doc.parse<rapidxml::parse_declaration_node | rapidxml::parse_no_utf8>(xml);
    } catch (const rapidxml::parse_error& e) {
        log_warning("Couldn't parse xml ({} byte {})", e.what(), (int)(e.where<char>() - xml));
        auto f = fopen("debug.xml", "wb");
        fwrite(xml, strlen(xml), 1, f);
        return false;
    }

    return true;
}

// the given property MUST have been created with an 8-byte aligned memory
// address. We can't use property_desc_to_buffer to get the unaligned memory
// because super old AVS versions don't have it
void prop_free(property_t prop) {
    property_destroy(prop);
    _aligned_free(prop);
}

unsigned char* lz_compress(unsigned char* input, size_t length, size_t *compressed_length) {
    auto compressor = cstream_create(AVS_COMPRESS_AVSLZ);
    if (!compressor) {
        log_warning("Couldn't create");
        return NULL;
    }
    compressor->input_buffer = input;
    compressor->input_size = (uint32_t)length;
    // worst case, for every 8 bytes there will be an extra flag byte
    auto to_add = std::max((length + 7) / 8, (size_t)1);
    auto compress_size = length + to_add;
    auto compress_buffer = (unsigned char*)malloc(compress_size);
    compressor->output_buffer = compress_buffer;
    compressor->output_size = (uint32_t)compress_size;

    bool ret;
    ret = cstream_operate(compressor);
    if (!ret && !compressor->input_size) {
        compressor->input_buffer = NULL;
        compressor->input_size = -1;
        ret = cstream_operate(compressor);
    }
    if (!ret) {
        log_warning("Couldn't operate");
        return NULL;
    }
    if (cstream_finish(compressor)) {
        log_warning("Couldn't finish");
        return NULL;
    }
    *compressed_length = compress_size - compressor->output_size;
    cstream_destroy(compressor);
    return compress_buffer;
}

unsigned char* lz_decompress(unsigned char* input, size_t length, size_t *decompressed_length) {
    auto decompressor = cstream_create(AVS_DECOMPRESS_AVSLZ);
    if (!decompressor) {
        log_warning("Couldn't create decompressor");
        return NULL;
    }
    auto decompress_buffer = (unsigned char*)malloc(*decompressed_length);
    decompressor->input_buffer = input;
    decompressor->input_size = (uint32_t)length;
    decompressor->output_buffer = decompress_buffer;
    decompressor->output_size = (uint32_t)*decompressed_length;

    bool ret = cstream_operate(decompressor);
    if (!ret && !decompressor->input_size) {
        decompressor->input_buffer = NULL;
        decompressor->input_size = -1;
        ret = cstream_operate(decompressor);
    }
    if (!ret) {
        log_warning("Couldn't decompress");
        free(decompress_buffer);
        cstream_destroy(decompressor);
        return NULL;
    }
    if (cstream_finish(decompressor)) {
        log_warning("Couldn't finish decompression");
        free(decompress_buffer);
        cstream_destroy(decompressor);
        return NULL;
    }
    *decompressed_length = *decompressed_length - decompressor->output_size;
    cstream_destroy(decompressor);
    return decompress_buffer;
}

typedef struct {
    uint32_t code;
    std::string_view msg;
} prop_error_info_t;

const prop_error_info_t prop_error_list[73] = {
    { 0x80092000, "invalid type" },
    { 0x80092001, "type cannot use as array" },
    { 0x80092002, "invalid" },
    { 0x80092003, "too large data size" },
    { 0x80092004, "too small buffer size" },
    { 0x80092005, "passcode 0 is not allowed" },
    { 0x80092040, "invalid node name" },
    { 0x80092041, "invalid attribute name" },
    { 0x80092042, "reserved attribute name" },
    { 0x80092043, "cannot find node/attribute" },
    { 0x80092080, "cannot allocate node" },
    { 0x80092081, "cannot allocate node value" },
    { 0x80092082, "cannot allocate mdigest for finger-print" },
    { 0x80092083, "cannot allocate nodename" },
    { 0x800920C0, "node type differs" },
    { 0x800920C1, "node type is VOID" },
    { 0x800920C2, "node is array" },
    { 0x800920C3, "node is not array" },
    { 0x80092100, "node is create-disabled" },
    { 0x80092101, "node is read-disabled" },
    { 0x80092102, "node is write-disabled" },
    { 0x80092103, "flag is already locked" },
    { 0x80092104, "passcode differs" },
    { 0x80092105, "insert_read() is applied to attribute" },
    { 0x80092106, "part_write() is applied to attribute" },
    { 0x80092107, "MODE_EXTEND flag differs" },
    { 0x80092140, "root node already exists" },
    { 0x80092141, "attribute cannot have children" },
    { 0x80092142, "node/attribute already exists" },
    { 0x80092143, "number of nodes exceeds 65535" },
    { 0x80092144, "cannot interpret as number" },
    { 0x80092145, "property is empty" },
    { 0x80092180, "I/O error" },
    { 0x80092181, "unexpected EOF" },
    { 0x80092182, "unknown format" },
    { 0x800921C0, "broken magic" },
    { 0x800921C1, "broken metadata" },
    { 0x800921C2, "broken databody" },
    { 0x800921C3, "invalid type" },
    { 0x800921C4, "too large data size" },
    { 0x800921C5, "too long node/attribute name" },
    { 0x800921C6, "attribute name is too long" },
    { 0x800921C7, "node/attribute already exists" },
    { 0x80092200, "invalid encoding " },
    { 0x80092201, "invalid XML token" },
    { 0x80092202, "XML syntax error" },
    { 0x80092203, "STag/ETag mismatch" },
    { 0x80092204, "too large node data" },
    { 0x80092205, "too deep node tree" },
    { 0x80092206, "invalid type" },
    { 0x80092207, "invalid size" },
    { 0x80092208, "invalid count" },
    { 0x80092209, "invalid value" },
    { 0x8009220A, "invalid node name" },
    { 0x8009220B, "invalid attribute name" },
    { 0x8009220C, "reserved attribute name" },
    { 0x8009220D, "node/attribute already exists" },
    { 0x8009220E, "too many elements in node data" },
    { 0x80092240, "JSON syntax error" },
    { 0x80092241, "invalid JSON literal" },
    { 0x80092242, "invalid JSON number" },
    { 0x80092243, "invalid JSON string" },
    { 0x80092244, "invalid JSON object name" },
    { 0x80092245, "object name already exists" },
    { 0x80092246, "too long JSON object name" },
    { 0x80092247, "too deep JSON object/array nesting" },
    { 0x80092248, "cannot convert JSON array to property" },
    { 0x80092249, "cannot convert empty JSON object to property" },
    { 0x8009224A, "root node already exists" },
    { 0x8009224B, "cannot convert root node to TYPE_ARRAY" },
    { 0x8009224C, "name represents reserved attribute" },
    { 0x80092280, "finger-print differs" },
    { 0x800922C0, "operation is not supported" },
};

static std::string_view get_prop_error_str(int32_t code) {
    static char ret[64];
    for (const auto &error : prop_error_list) {
        if (error.code == (uint32_t)code)
            return error.msg;
    }
    snprintf(ret, sizeof(ret), "unknown (%X)", code);
    return ret;
}
