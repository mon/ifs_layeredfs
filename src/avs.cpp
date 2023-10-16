#define NOMINMAX
#include <windows.h>
#include <stdio.h>

#include "avs.h"
#include "hook.h"
#include "log.hpp"
#include "3rd_party/MinHook.h"
#include "utils.hpp"

#define AVS_STRUCT_DEF(ret_type, name, ...) const char* name;

const char *avs_loaded_dll_name;
uint16_t avs_loaded_version;

typedef struct {
    const char *version_name;
    uint16_t version;
    const char *unique_check; // for IIDX vs SDVX cloud, almost all funcs are identical

    FOREACH_AVS_FUNC(AVS_STRUCT_DEF)
    FOREACH_AVS_FUNC_OPTIONAL(AVS_STRUCT_DEF)
} avs_exports_t;

const LPCWSTR dll_names[] = {
    L"libavs-win32.dll",
    L"libavs-win64.dll",
    L"avs2-core.dll",
};

const avs_exports_t avs_exports[] = {
    [] { avs_exports_t x = { 0 };
    x.version_name                        = "normal";
    x.version                             = 1200;
    x.unique_check                        = NULL;
    x.log_body_fatal                      = "log_body_fatal";
    x.log_body_warning                    = "log_body_warning";
    x.log_body_info                       = "log_body_info";
    x.log_body_misc                       = "log_body_misc";
    x.avs_fs_open                         = "avs_fs_open";
    x.avs_fs_close                        = "avs_fs_close";
    x.avs_fs_convert_path                 = "avs_fs_convert_path";
    x.avs_fs_read                         = "avs_fs_read";
    x.avs_fs_lseek                        = "avs_fs_lseek";
    x.avs_fs_fstat                        = "avs_fs_fstat";
    x.avs_fs_lstat                        = "avs_fs_lstat";
    x.avs_fs_mount                        = "avs_fs_mount";
    x.mdigest_create                      = "mdigest_create";
    x.mdigest_update                      = "mdigest_update";
    x.mdigest_finish                      = "mdigest_finish";
    x.mdigest_destroy                     = "mdigest_destroy";
    x.property_read_query_memsize         = "property_read_query_memsize";
    x.property_read_query_memsize_long    = "property_read_query_memsize_long";
    x.property_create                     = "property_create";
    x.property_insert_read                = "property_insert_read";
    x.property_mem_write                  = "property_mem_write";
    x.property_destroy                    = "property_destroy";
    x.property_query_size                 = "property_query_size";
    x.cstream_create                      = "cstream_create";
    x.cstream_operate                     = "cstream_operate";
    x.cstream_finish                      = "cstream_finish";
    x.cstream_destroy                     = "cstream_destroy";
    return x;
    }(),
    [] { avs_exports_t x = { 0 };
    x.version_name                        = "2.13.x (XC058ba5------)";
    x.version                             = 1300;
    x.unique_check                        = NULL;
    x.log_body_fatal                      = "XC058ba5000084";
    x.log_body_warning                    = "XC058ba50000e1";
    x.log_body_info                       = "XC058ba500015a";
    x.log_body_misc                       = "XC058ba500002d";
    x.avs_fs_open                         = "XC058ba50000b6";
    x.avs_fs_close                        = "XC058ba500011b";
    x.avs_fs_convert_path                 = "XC058ba50000d5";
    x.avs_fs_read                         = "XC058ba5000139";
    x.avs_fs_lseek                        = "XC058ba500000f";
    x.avs_fs_fstat                        = "XC058ba50000d0";
    x.avs_fs_lstat                        = "XC058ba5000063";
    x.avs_fs_mount                        = "XC058ba500009c";
    x.mdigest_create                      = "XC058ba50000db";
    x.mdigest_update                      = "XC058ba5000096";
    x.mdigest_finish                      = "XC058ba500002f";
    x.mdigest_destroy                     = "XC058ba5000004";
    x.property_read_query_memsize         = "XC058ba5000066";
    x.property_read_query_memsize_long    = "XC058ba5000091";
    x.property_create                     = "XC058ba5000107";
    x.property_insert_read                = "XC058ba5000016";
    x.property_mem_write                  = "XC058ba5000162";
    x.property_destroy                    = "XC058ba500010f";
    x.property_query_size                 = "XC058ba5000101";
    x.cstream_create                      = "XC058ba5000118";
    x.cstream_operate                     = "XC058ba5000078";
    x.cstream_finish                      = "XC058ba5000130";
    x.cstream_destroy                     = "XC058ba500012b";
    return x;
    }(),
    [] { avs_exports_t x = { 0 };
    x.version_name                        = "2.15.x (XCd229cc------)";
    x.version                             = 1500;
    x.unique_check                        = NULL;
    x.log_body_fatal                      = "XCd229cc0000e6";
    x.log_body_warning                    = "XCd229cc000018";
    x.log_body_info                       = "XCd229cc0000dc";
    x.log_body_misc                       = "XCd229cc000075";
    x.avs_fs_open                         = "XCd229cc000090";
    x.avs_fs_close                        = "XCd229cc00011f";
    x.avs_fs_convert_path                 = "XCd229cc00001e";
    x.avs_fs_read                         = "XCd229cc00010d";
    x.avs_fs_lseek                        = "XCd229cc00004d";
    x.avs_fs_fstat                        = "XCd229cc0000c3";
    x.avs_fs_lstat                        = "XCd229cc0000c0";
    x.avs_fs_mount                        = "XCd229cc0000ce";
    x.mdigest_create                      = "XCd229cc00003d";
    x.mdigest_update                      = "XCd229cc000157";
    x.mdigest_finish                      = "XCd229cc000015";
    x.mdigest_destroy                     = "XCd229cc000050";
    x.property_read_query_memsize         = "XCd229cc0000ff";
    x.property_read_query_memsize_long    = "XCd229cc00002b";
    x.property_create                     = "XCd229cc000126";
    x.property_insert_read                = "XCd229cc00009a";
    x.property_mem_write                  = "XCd229cc000033";
    x.property_destroy                    = "XCd229cc00013c";
    x.property_query_size                 = "XCd229cc000032";
    x.cstream_create                      = "XCd229cc000141";
    x.cstream_operate                     = "XCd229cc00008c";
    x.cstream_finish                      = "XCd229cc000025";
    x.cstream_destroy                     = "XCd229cc0000e3";
    return x;
    }(),
    [] { avs_exports_t x = { 0 }; // sdvx cloud
    x.version_name                        = "2.16.[3-7] (XCnbrep7------)";
    x.version                             = 1630;
    x.unique_check                        = "XCnbrep700013c";
    x.log_body_fatal                      = "XCnbrep700017a";
    x.log_body_warning                    = "XCnbrep700017b";
    x.log_body_info                       = "XCnbrep700017c";
    x.log_body_misc                       = "XCnbrep700017d";
    x.avs_fs_open                         = "XCnbrep700004e";
    x.avs_fs_close                        = "XCnbrep7000055";
    x.avs_fs_convert_path                 = "XCnbrep7000046";
    x.avs_fs_read                         = "XCnbrep7000051";
    x.avs_fs_lseek                        = "XCnbrep700004f";
    x.avs_fs_fstat                        = "XCnbrep7000062";
    x.avs_fs_lstat                        = "XCnbrep7000063";
    x.avs_fs_mount                        = "XCnbrep700004b";
    x.mdigest_create                      = "XCnbrep700013f";
    x.mdigest_update                      = "XCnbrep7000141";
    x.mdigest_finish                      = "XCnbrep7000142";
    x.mdigest_destroy                     = "XCnbrep7000143";
    x.property_read_query_memsize         = "XCnbrep70000b0";
    x.property_read_query_memsize_long    = "XCnbrep70000b1";
    x.property_create                     = "XCnbrep7000090";
    x.property_insert_read                = "XCnbrep7000094";
    x.property_mem_write                  = "XCnbrep70000b8";
    x.property_destroy                    = "XCnbrep7000091";
    x.property_query_size                 = "XCnbrep700009f";
    x.cstream_create                      = "XCnbrep7000130";
    x.cstream_operate                     = "XCnbrep7000132";
    x.cstream_finish                      = "XCnbrep7000133";
    x.cstream_destroy                     = "XCnbrep7000134";
    return x;
    }(),
    [] { avs_exports_t x = { 0 }; // IIDX, "nbrep but different"
    x.version_name                        = "2.16.1 (XCnbrep7 but different)",
    x.version                             = 1610;
    x.unique_check                        = NULL;
    x.log_body_fatal                      = "XCnbrep7000168";
    x.log_body_warning                    = "XCnbrep7000169";
    x.log_body_info                       = "XCnbrep700016a";
    x.log_body_misc                       = "XCnbrep700016b";
    x.avs_fs_open                         = "XCnbrep7000039";
    x.avs_fs_close                        = "XCnbrep7000040";
    x.avs_fs_convert_path                 = "XCnbrep7000031";
    x.avs_fs_read                         = "XCnbrep700003c";
    x.avs_fs_lseek                        = "XCnbrep700003a";
    x.avs_fs_fstat                        = "XCnbrep700004d";
    x.avs_fs_lstat                        = "XCnbrep700004e";
    x.avs_fs_mount                        = "XCnbrep7000036";
    x.mdigest_create                      = "XCnbrep7000133";
    x.mdigest_update                      = "XCnbrep7000135";
    x.mdigest_finish                      = "XCnbrep7000136";
    x.mdigest_destroy                     = "XCnbrep7000137";
    x.property_read_query_memsize         = "XCnbrep700009b";
    x.property_read_query_memsize_long    = "XCnbrep700009c";
    x.property_create                     = "XCnbrep700007b";
    x.property_insert_read                = "XCnbrep700007f";
    x.property_mem_write                  = "XCnbrep70000a3";
    x.property_destroy                    = "XCnbrep700007c";
    x.property_query_size                 = "XCnbrep700008a";
    x.cstream_create                      = "XCnbrep7000124";
    x.cstream_operate                     = "XCnbrep7000126";
    x.cstream_finish                      = "XCnbrep7000127";
    x.cstream_destroy                     = "XCnbrep7000128";
    return x;
    }(),
    [] { avs_exports_t x = { 0 }; // avs 64 bit, pretty much. 2.16.3 with different prefix
    x.version_name                        = "2.17.x (XCgsqzn0------)";
    x.version                             = 1700;
    x.unique_check                        = NULL;
    x.log_body_fatal                      = "XCgsqzn000017a";
    x.log_body_warning                    = "XCgsqzn000017b";
    x.log_body_info                       = "XCgsqzn000017c";
    x.log_body_misc                       = "XCgsqzn000017d";
    x.avs_fs_open                         = "XCgsqzn000004e";
    x.avs_fs_close                        = "XCgsqzn0000055";
    x.avs_fs_convert_path                 = "XCgsqzn0000046";
    x.avs_fs_read                         = "XCgsqzn0000051";
    x.avs_fs_lseek                        = "XCgsqzn000004f";
    x.avs_fs_fstat                        = "XCgsqzn0000062";
    x.avs_fs_lstat                        = "XCgsqzn0000063";
    x.avs_fs_mount                        = "XCgsqzn000004b";
    x.mdigest_create                      = "XCgsqzn000013f";
    x.mdigest_update                      = "XCgsqzn0000141";
    x.mdigest_finish                      = "XCgsqzn0000142";
    x.mdigest_destroy                     = "XCgsqzn0000143";
    x.property_read_query_memsize         = "XCgsqzn00000b0";
    x.property_read_query_memsize_long    = "XCgsqzn00000b1";
    x.property_create                     = "XCgsqzn0000090";
    x.property_insert_read                = "XCgsqzn0000094";
    x.property_mem_write                  = "XCgsqzn00000b8";
    x.property_destroy                    = "XCgsqzn0000091";
    x.property_query_size                 = "XCgsqzn000009f";
    x.cstream_create                      = "XCgsqzn0000130";
    x.cstream_operate                     = "XCgsqzn0000132";
    x.cstream_finish                      = "XCgsqzn0000133";
    x.cstream_destroy                     = "XCgsqzn0000134";
    return x;
    }(),
};

#define AVS_FUNC_PTR(ret_type, name, ...) ret_type (* name )( __VA_ARGS__ );
FOREACH_AVS_FUNC(AVS_FUNC_PTR)
FOREACH_AVS_FUNC_OPTIONAL(AVS_FUNC_PTR)

/*void* (*avs_fs_mount)(char* mountpoint, char* fsroot, char* fstype, int a5);
void* hook_avs_fs_mount(char* mountpoint, char* fsroot, char* fstype, int a5) {
    log_misc("Mounting %s at %s with type %s", fsroot, mountpoint, fstype);
    return avs_fs_mount(mountpoint, fsroot, fstype, a5);
}*/

#define TEST_HOOK_AND_APPLY(func) if (MH_CreateHookApi(dll_name, avs_exports[i].func, (LPVOID)hook_ ## func, (LPVOID*)&func) != MH_OK || func == NULL) continue
#define LOAD_FUNC(func) if( (func = (decltype(func))GetProcAddress(mod_handle, avs_exports[i].func)) == NULL) continue
#define CHECK_UNIQUE(func) if( avs_exports[i].func != NULL && GetProcAddress(mod_handle, avs_exports[i].func) == NULL) continue

#define AVS_FUNC_LOAD(ret_type, name, ...) LOAD_FUNC(name);
#define AVS_FUNC_LOAD_OPTIONAL(ret_type, name, ...) name = (decltype(name))GetProcAddress(mod_handle, avs_exports[i].name);

bool init_avs(void) {
    bool success = false;

#ifdef _DEBUG
    for (int i = 0; i < lenof(avs_exports); i++) {
#define VERBOSE_EXPORT_CHECK(ret_type, name, ...) if(avs_exports[i]. ## name == NULL) log_warning("MISSING EXPORT %d: %s", i, #name);
        FOREACH_AVS_FUNC(VERBOSE_EXPORT_CHECK)
        FOREACH_AVS_FUNC_OPTIONAL(VERBOSE_EXPORT_CHECK)
    }
#endif

    // presumably we don't load more than 1 unique avs dll.
    // Please don't be proven wrong
    LPCWSTR dll_name = NULL;
    HMODULE mod_handle = NULL;
    for (size_t i = 0; i < lenof(dll_names); i++) {
        mod_handle = GetModuleHandleW(dll_names[i]);
        if (mod_handle != NULL) {
            dll_name = dll_names[i];
            break;
        }
    }

    if (mod_handle == NULL) {
        log_warning("Couldn't find AVS dll in memory");
        return false;
    }

    for (size_t i = 0; i < lenof(avs_exports); i++) {
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
        avs_loaded_dll_name = avs_exports[i].version_name;
        avs_loaded_version = avs_exports[i].version;

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
            log_warning("Couldn't get memsize %08X (%s)", memsize, get_prop_error_str(memsize));
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
        log_warning("Couldn't create prop (%s)", get_prop_error_str((int32_t)(size_t)prop));
        goto FAIL;
    }

    avs_fs_lseek(f, 0, SEEK_SET);
    ret = property_insert_read(prop, 0, avs_fs_read, f);
    avs_fs_close(f);

    if (ret < 0) {
        log_warning("Couldn't read prop (%s)", get_prop_error_str(ret));
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

property_t prop_from_file_path(string const&path) {
    AVS_FILE f = avs_fs_open(path.c_str(), avs_open_mode_read(), 420);
    if (f < 0) {
        log_warning("Couldn't open prop");
        return NULL;
    }

    return prop_from_file_handle(f);
}

char* prop_to_xml_string(property_t prop, rapidxml::xml_document<>& allocator) {
    auto prop_size = property_query_size(prop);
    char* xml = allocator.allocate_string(NULL, prop_size);

    auto written = property_mem_write(prop, xml, prop_size);
    if (written > 0) {
        xml[written] = '\0';
    }
    else {
        xml[0] = '\0';
        log_warning("property_mem_write failed (%s)", get_prop_error_str(written));
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

bool is_binary_prop(AVS_FILE f) {
    avs_fs_lseek(f, 0, SEEK_SET);
    unsigned char head;
    auto read = avs_fs_read(f, &head, 1);
    bool ret = (read == 1) && head == 0xA0;
    avs_fs_lseek(f, 0, SEEK_SET);
    //logf("detected binary: %s (read %d byte %02x)", ret ? "true": "false", read, head&0xff);
    return ret;
}

bool rapidxml_from_avs_filepath(
    string const& path,
    rapidxml::xml_document<>& doc,
    rapidxml::xml_document<>& doc_to_allocate_with
) {
    AVS_FILE f = avs_fs_open(path.c_str(), avs_open_mode_read(), 420);
    if (f < 0) {
        log_warning("Couldn't open prop");
        return false;
    }

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
        log_warning("Couldn't parse xml (%s byte %d)", e.what(), (int)(e.where<char>() - xml));
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

string md5_sum(const char* str) {
    uint8_t sum[MD5_LEN];
    char sum_str[MD5_LEN*2 + 1];
    auto digest = mdigest_create(MD5);
    mdigest_update(digest, str, (int)strlen(str));
    mdigest_finish(digest, sum, MD5_LEN);
    mdigest_destroy(digest);
    for (int i = 0; i < MD5_LEN; i++) {
        snprintf(sum_str + 2 * i, 3, "%02x", sum[i]);
    }
    return sum_str;
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
    auto to_add = std::max(length / 8, (size_t)1);
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

typedef struct {
    uint32_t code;
    const char* msg;
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

const char* get_prop_error_str(int32_t code) {
    static char ret[64];
    for (size_t i = 0; i < lenof(prop_error_list); i++) {
        if (prop_error_list[i].code == (uint32_t)code)
            return prop_error_list[i].msg;
    }
    snprintf(ret, sizeof(ret), "unknown (%X)", code);
    return ret;
}

const char* prop_data_to_str(int type, void* data) {
    static char ret[64];

    type &= 63;

    switch (type) {
        case PROP_TYPE_node:
            return "none - node element";
        case PROP_TYPE_str:
        case PROP_TYPE_attr:
            snprintf(ret, sizeof(ret), "%s", (char*)data);
            return ret;
        case PROP_TYPE_s8:
        case PROP_TYPE_u8:
        case PROP_TYPE_s16:
        case PROP_TYPE_u16:
        case PROP_TYPE_s32:
        case PROP_TYPE_u32:
        case PROP_TYPE_s64:
        case PROP_TYPE_u64:
        case PROP_TYPE_bin:
        case PROP_TYPE_ip4:
        case PROP_TYPE_time:
        case PROP_TYPE_float:
        case PROP_TYPE_double:
        case PROP_TYPE_2s8:
        case PROP_TYPE_2u8:
        case PROP_TYPE_2s16:
        case PROP_TYPE_2u16:
        case PROP_TYPE_2s32:
        case PROP_TYPE_2u32:
        case PROP_TYPE_2s64:
        case PROP_TYPE_2u64:
        case PROP_TYPE_2f:
        case PROP_TYPE_2d:
        case PROP_TYPE_3s8:
        case PROP_TYPE_3u8:
        case PROP_TYPE_3s16:
        case PROP_TYPE_3u16:
        case PROP_TYPE_3s32:
        case PROP_TYPE_3u32:
        case PROP_TYPE_3s64:
        case PROP_TYPE_3u64:
        case PROP_TYPE_3f:
        case PROP_TYPE_3d:
        case PROP_TYPE_4s8:
        case PROP_TYPE_4u8:
        case PROP_TYPE_4s16:
        case PROP_TYPE_4u16:
        case PROP_TYPE_4s32:
        case PROP_TYPE_4u32:
        case PROP_TYPE_4s64:
        case PROP_TYPE_4u64:
        case PROP_TYPE_4f:
        case PROP_TYPE_4d:
        case PROP_TYPE_attr_and_node:
        case PROP_TYPE_vs8:
        case PROP_TYPE_vu8:
        case PROP_TYPE_vs16:
        case PROP_TYPE_vu16:
        case PROP_TYPE_bool:
        case PROP_TYPE_2b:
        case PROP_TYPE_3b:
        case PROP_TYPE_4b:
        case PROP_TYPE_vb:
            snprintf(ret, sizeof(ret), "STR REP NOT IMPLEMENTED (%d)", type);
            return ret;
        default:
            return "UNKNOWN";
    }
}
