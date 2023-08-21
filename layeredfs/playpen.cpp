// The layeredfs playpen is for making sure shit works without having to boot
// the game a million times. BYO copy of AVS 2.17.x

#include <windows.h>

#include "hook.h"
#include "log.hpp"

#include <fstream>

void boot_avs(void);
bool load_dll(void);
LONG WINAPI exc_handler(_EXCEPTION_POINTERS *ExceptionInfo);

typedef void (*avs_log_writer_t)(const char *chars, uint32_t nchars, void *ctx);

#define FOREACH_EXTRA_FUNC(X) \
X("XCgsqzn0000129", void,   avs_boot, node_t config, void *com_heap, size_t sz_com_heap, void *reserved, avs_log_writer_t log_writer, void *log_context) \
X("XCgsqzn000012a", void,   avs_shutdown) \
X("XCgsqzn00000a1", node_t, property_search, property_t prop, node_t node, const char *path) \

#define AVS_FUNC_PTR(obfus_name, ret_type, name, ...) ret_type (* name )( __VA_ARGS__ );
FOREACH_EXTRA_FUNC(AVS_FUNC_PTR)

static bool print_logs = true;

#define QUIET_BOOT

#include "texbin.hpp"

// decompressed_length MUST be set and will be updated on finish
unsigned char* lz_decompress(unsigned char* input, size_t length, size_t *decompressed_length) {
    auto compressor = cstream_create(AVS_DECOMPRESS_AVSLZ);
    if (!compressor) {
        log_warning("Couldn't create");
        return NULL;
    }
    compressor->input_buffer = input;
    compressor->input_size = (uint32_t)length; // may be -1 to auto-finish
    auto decompress_buffer = (unsigned char*)malloc(*decompressed_length);
    compressor->output_buffer = decompress_buffer;
    compressor->output_size = (uint32_t)*decompressed_length;

    cstream_operate(compressor);
    compressor->input_buffer = NULL;
    compressor->input_size = -1;
    bool ret = cstream_operate(compressor);
    if (!ret) {
        log_warning("Couldn't operate");
        return NULL;
    }
    if (cstream_finish(compressor)) {
        log_warning("Couldn't finish");
        return NULL;
    }
    *decompressed_length = *decompressed_length - compressor->output_size;
    cstream_destroy(compressor);
    return decompress_buffer;
}

void lz_unfuck(uint8_t *buf, size_t len) {
    int repl = 0;
    uint8_t *end = buf + len;
    while(buf < end) {
        uint8_t flag = *buf++;
        for(auto i = 0; i < 8; i++) {
            if(!(flag & 1) && (buf+1) < end) {
                uint8_t hi = buf[0];
                uint8_t lo = buf[1];

                *buf++ = (hi >> 4) | (lo & 0xF0);
                *buf++ = (hi << 4) | (lo & 0x0F);
                repl++;
            } else {
                buf++;
            }

            flag >>= 1;
        }
    }

    log_info("unfucked %d bytes", repl);
}

optional<std::vector<uint8_t>> readFile(const char* filename)
{
    // open the file:
    std::streampos fileSize;
    std::ifstream file(filename, std::ios::binary);

    if(!file) {
        log_warning("Couldn't open %s", filename);
        return nullopt;
    }

    // get its size:
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // read the data:
    std::vector<uint8_t> fileData(fileSize);
    file.read((char*) &fileData[0], fileSize);
    return fileData;
}

void avs_playpen() {
    // log_info("loading file");
    // auto _debug = readFile("debug.bin");
    // if(!_debug) {
    //     return;
    // }
    // auto debug = *_debug;
    // log_info("Loaded %d bytes", debug.size());

    // // lz_unfuck(&debug[8], comp_sz);
    // // auto decomp = lz_decompress(&debug[8], comp_sz, &decomp_sz);
    // auto decomp = texbin_lz77_decompress(debug);
    // log_info("Decomp to %u", decomp.size());
    // auto comp = texbin_lz77_compress(decomp);
    // log_info("Comp again to %u", comp.size());
    // auto decomp2 = texbin_lz77_decompress(comp);
    // log_info("Final decomp to %u", decomp2.size());

    // auto f = fopen("debug.out.bin", "wb");
    // fwrite(&decomp[0], 1, decomp.size(), f);
    // fclose(f);

    // return;


    // auto _tex = Texbin::from_path("tex_l44qb_smc_sm.bin");
    auto tex = Texbin::from_path("tex_custom.bin");
    if(!tex) {
        return;
    }

#ifdef TEXBIN_VERBOSE
    tex->debug();
#endif

    tex->add_or_replace_image("AAA_NEW_IMAGE", "new image test.png");
    tex->add_or_replace_image("SSM_000_T1000", "replace image test.png");

    tex->save("tex_custom_modified.bin");
    // tex.save("tex_l44qb_smc_sm_modified.bin");

    /*string path = "testcases.xml";
    void* prop_buffer = NULL;
    property_t prop = NULL;

    auto f = avs_fs_open(path.c_str(), 1, 420);
    if (f < 0)
        return;

    auto memsize = property_read_query_memsize(avs_fs_read, f, NULL, NULL);
    if (memsize < 0) {
        log_warning("Couldn't get memsize %08X", memsize);
        goto FAIL;
    }

    // who knows
    memsize *= 10;

    prop_buffer = malloc(memsize);
    prop = property_create(PROP_READ|PROP_WRITE|PROP_APPEND|PROP_CREATE|PROP_JSON, prop_buffer, memsize);
    if (!prop) {
        log_warning("Couldn't create prop");
        goto FAIL;
    }

    avs_fs_lseek(f, 0, SEEK_SET);
    property_insert_read(prop, 0, avs_fs_read, f);
    avs_fs_close(f);

    property_file_write(prop, "testcases.json");

FAIL:
    if (f)
        avs_fs_close(f);
    if (prop)
        property_destroy(prop);
    if (prop_buffer)
        free(prop_buffer);*/

    /*auto d = avs_fs_opendir(MOD_FOLDER);
    if (!d) {
        log_warning("couldn't d");
        return;
    }
    for (char* n = avs_fs_readdir(d); n; n = avs_fs_readdir(d))
        log_info("dir %s", n);
    avs_fs_closedir(d);*/
    //char name[64];
    //auto playpen = prop_from_file("playpen.xml");
    //auto node = property_search(playpen, NULL, "/");
    //auto start = property_node_traversal(node, TRAVERSE_FIRST_CHILD);
    //auto end = property_node_traversal(start, TRAVERSE_LAST_SIBLING);
    //print_node(node);
    //print_node(start);
    //print_node(end);
    /*for (int i = 0; i <= 8; i++) {
        if (i == 6 || i == 3) continue;
        log_info("Traverse: %d", i);
        auto node = property_search(playpen, NULL, "/root/t2");
        auto nnn = property_node_traversal(node, 8);
        auto nna = property_node_traversal(nnn, TRAVERSE_FIRST_ATTR);
        property_node_name(nna, name, 64);
        log_info("bloop %s", name);
        for (;node;node = property_node_traversal(node, i)) {
            if (!property_node_name(node, name, 64)) {
                log_info("    %s", name);
            }
        }
    }*/
    //prop_free(playpen);
}

int main(int argc, char** argv) {
    AddVectoredExceptionHandler(1, exc_handler);
    log_to_stdout();
    if(!load_dll()) {
        log_fatal("DLL load failed");
        return 1;
    }
    init_avs(); // fails because of Minhook not being initialised, don't care
#ifdef QUIET_BOOT
    print_logs = false;
    boot_avs();
    print_logs = true;
#else
    boot_avs();
#endif

    init(); // this double-hooks some AVS funcs, don't care

    avs_playpen();

    avs_shutdown();

    return 0;
}

#define DEFAULT_HEAP_SIZE 16777216

void log_writer(const char *chars, uint32_t nchars, void *ctx) {
    // don't print noisy shutdown logs
    auto prefix = "[----/--/-- --:--:--] ";
    auto len = strlen(prefix);
    if(strncmp(chars, prefix, len) == 0) {
        return;
    }

    if(print_logs) {
        fprintf(stderr, "%.*s", nchars, chars);
    }
}

static const char *boot_cfg = R"(<?xml version="1.0" encoding="SHIFT_JIS"?>
<config>
  <fs>
    <root><device>.</device></root>
    <mounttable>
      <vfs name="boot" fstype="fs" src="dev/raw" dst="/dev/raw" opt="vf=1,posix=1"/>
      <vfs name="boot" fstype="fs" src="dev/nvram" dst="/dev/nvram" opt="vf=0,posix=1"/>
    </mounttable>
  </fs>
  <log><level>misc</level></log>
  <sntp>
    <ea_on __type="bool">0</ea_on>
    <servers></servers>
  </sntp>
</config>
)";

static size_t read_str(int32_t context, void *dst_buf, size_t count) {
    memcpy(dst_buf, boot_cfg, count);
    return count;
}

#define LOAD_FUNC(obfus_name, ret_type, name, ...) \
    if(!(name = (decltype(name))GetProcAddress(avs, obfus_name))) {\
        log_fatal("Playpen: couldn't get " #name); \
        return false; \
    }

bool load_dll(void) {
    auto avs = LoadLibraryA("avs2-core.dll");
    if(!avs) {
        log_fatal("Playpen: Couldn't load avs dll");
        return false;
    }

    FOREACH_EXTRA_FUNC(LOAD_FUNC);

    return true;
}

void boot_avs(void) {
    auto avs_heap = malloc(DEFAULT_HEAP_SIZE);

    int prop_len = property_read_query_memsize(read_str, 0, 0, 0);
    if (prop_len <= 0) {
        log_fatal("error reading config (size <= 0)");
        return;
    }
    auto buffer = malloc(prop_len);
    auto avs_config = property_create(PROP_READ | PROP_WRITE | PROP_CREATE | PROP_APPEND, buffer, prop_len);
    if (!avs_config) {
        log_fatal("cannot create property");
        return;
    }
    if (!property_insert_read(avs_config, 0, read_str, 0)) {
        log_fatal("avs-core", "cannot read property");
        return;
    }

    auto avs_config_root = property_search(avs_config, 0, "/config");
    if(!avs_config_root) {
        log_fatal("no root config node");
        return;
    }

    avs_boot(avs_config_root, avs_heap, DEFAULT_HEAP_SIZE, NULL, log_writer, NULL);
}

LONG WINAPI exc_handler(_EXCEPTION_POINTERS *ExceptionInfo) {
    fprintf(stderr, "Unhandled exception %X\n", ExceptionInfo->ExceptionRecord->ExceptionCode);

    return EXCEPTION_CONTINUE_SEARCH;
}
