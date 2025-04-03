#include "avs_standalone.hpp"

#include <windows.h>

#include "avs.h"
#include "hook.h"
#include "log.hpp"

namespace avs_standalone
{

typedef void (*avs_log_writer_t)(const char *chars, uint32_t nchars, void *ctx);

static LONG WINAPI exc_handler(_EXCEPTION_POINTERS *ExceptionInfo);
static size_t read_str(int32_t context, void *dst_buf, size_t count);
static void log_writer(const char *chars, uint32_t nchars, void *ctx);

#define FOREACH_EXTRA_FUNC(X)                                                                                                                              \
    X("XCgsqzn0000129", void, avs_boot, node_t config, void *com_heap, size_t sz_com_heap, void *reserved, avs_log_writer_t log_writer, void *log_context) \
    X("XCgsqzn000012a", void, avs_shutdown)                                                                                                                \
    X("XCgsqzn00000a1", node_t, property_search, property_t prop, node_t node, const char *path)                                                           \
    X("XCgsqzn0000048", int, avs_fs_addfs, void *filesys)                                                                                                  \
    X("XCgsqzn0000159", void *, avs_filesys_ramfs)

#define AVS_FUNC_PTR(obfus_name, ret_type, name, ...) ret_type (*name)(__VA_ARGS__);
    FOREACH_EXTRA_FUNC(AVS_FUNC_PTR)

static bool g_print_logs = true;
static size_t g_boot_cfg_offset;

#define DEFAULT_HEAP_SIZE 16777216

bool boot(bool _print_logs) {
    AddVectoredExceptionHandler(1, exc_handler);

    log_to_stdout();

    if(!load_dll()) {
        log_fatal("DLL load failed");
        return false;
    }

    init_avs(); // fails because of Minhook not being initialised, don't care

    auto avs_heap = malloc(DEFAULT_HEAP_SIZE);

    g_boot_cfg_offset = 0;
    int prop_len = property_read_query_memsize(read_str, 0, 0, 0);
    if (prop_len <= 0) {
        log_fatal("error reading config (size <= 0)");
        return false;
    }
    auto buffer = malloc(prop_len);
    auto avs_config = property_create(PROP_READ | PROP_WRITE | PROP_CREATE | PROP_APPEND, buffer, prop_len);
    if (!avs_config) {
        log_fatal("cannot create property");
        return false;
    }
    g_boot_cfg_offset = 0;
    if (!property_insert_read(avs_config, 0, read_str, 0)) {
        log_fatal("avs-core", "cannot read property");
        return false;
    }

    auto avs_config_root = property_search(avs_config, 0, "/config");
    if(!avs_config_root) {
        log_fatal("no root config node");
        return false;
    }

    bool was_print = g_print_logs;
    g_print_logs = _print_logs;
    avs_boot(avs_config_root, avs_heap, DEFAULT_HEAP_SIZE, NULL, log_writer, NULL);
    g_print_logs = was_print;

    return true;
}

void shutdown(void) {
    avs_shutdown();
}

#define LOAD_FUNC(obfus_name, ret_type, name, ...)                 \
    if (!(name = (decltype(name))GetProcAddress(avs, obfus_name))) \
    {                                                              \
        log_fatal("avs_standalone: couldn't get " #name);          \
        return false;                                              \
    }

bool load_dll(void)
{
    auto avs = LoadLibraryA("avs2-core.dll");
    if (!avs)
    {
        log_fatal("Playpen: Couldn't load avs dll");
        return false;
    }

    FOREACH_EXTRA_FUNC(LOAD_FUNC);

    return true;
}

void log_writer(const char *chars, uint32_t nchars, void *ctx)
{
    // don't print noisy shutdown logs
    auto prefix = "[----/--/-- --:--:--] ";
    auto len = strlen(prefix);
    if (strncmp(chars, prefix, len) == 0)
    {
        return;
    }

    if (g_print_logs)
    {
        fprintf(stderr, "%.*s", nchars, chars);
    }
}

static const char *boot_cfg = R"(<?xml version="1.0" encoding="SHIFT_JIS"?>
<config>
    <fs>
    <nr_filesys __type="u16">16</nr_filesys>
    <nr_mountpoint __type="u16">1024</nr_mountpoint>
    <nr_mounttable __type="u16">32</nr_mounttable>
    <nr_filedesc __type="u16">4096</nr_filedesc>
    <link_limit __type="u16">8</link_limit>
    <root>
        <device>.</device>
        <!--<option>posix=1</option>-->
    </root>
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

static size_t read_str(int32_t context, void *dst_buf, size_t count)
{
    memcpy(dst_buf, &boot_cfg[g_boot_cfg_offset], count);
    g_boot_cfg_offset += count;
    return count;
}

LONG WINAPI exc_handler(_EXCEPTION_POINTERS *ExceptionInfo) {
    switch(ExceptionInfo->ExceptionRecord->ExceptionCode) {
        case DBG_PRINTEXCEPTION_C:
            break;
        default:
            fprintf(stderr, "Unhandled exception %lX\n", ExceptionInfo->ExceptionRecord->ExceptionCode);
            break;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
}
