#include "avs_standalone.hpp"

#include <windows.h>

#include "avs.hpp"
#include "fmt/format.h"
#include "log.hpp"

namespace avs_standalone {

static LONG WINAPI exc_handler(_EXCEPTION_POINTERS* ExceptionInfo);
static size_t read_boot_cfg(int32_t context, void* dst_buf, size_t count);
static void log_writer(const char* chars, uint32_t nchars, void* ctx);
static void log_writer_lt_1500(void* ctx, const char* chars, uint32_t nchars);

static bool g_print_logs = true;
static void reset_boot_cfg_reader();

#define DEFAULT_HEAP_SIZE 16777216

bool preboot() {
    AddVectoredExceptionHandler(1, exc_handler);

    log_to_stdout();

    if (!load_dll()) {
        log_fatal("DLL load failed");
        return false;
    }

    return true;
}

bool boot(bool _print_logs) {
    auto avs_heap = malloc(DEFAULT_HEAP_SIZE);

    reset_boot_cfg_reader();
    int prop_len = property_read_query_memsize(read_boot_cfg, 0, 0, 0);
    if (prop_len <= 0) {
        log_fatal("error reading config (size <= 0)");
        return false;
    }
    auto buffer = malloc(prop_len);
    auto avs_config =
        property_create(PROP_READ | PROP_WRITE | PROP_CREATE | PROP_APPEND, buffer, prop_len);
    if (!avs_config) {
        log_fatal("cannot create property");
        return false;
    }
    reset_boot_cfg_reader();
    if (!property_insert_read(avs_config, 0, read_boot_cfg, 0)) {
        log_fatal("avs-core", "cannot read property");
        return false;
    }

    auto avs_config_root = property_search(avs_config, 0, "/config");
    if (!avs_config_root) {
        log_fatal("no root config node");
        return false;
    }

    bool was_print = std::exchange(g_print_logs, _print_logs);

    if (avs_loaded_version >= 1600) {
        avs_boot_gte_1600(avs_config_root, avs_heap, DEFAULT_HEAP_SIZE, NULL, log_writer, NULL);
    } else {
        auto std_heap = malloc(DEFAULT_HEAP_SIZE);
        auto logger   = avs_loaded_version >= 1500
                            ? reinterpret_cast<const void*>(log_writer)
                            : reinterpret_cast<const void*>(log_writer_lt_1500);
        avs_boot_lt_1600(avs_config_root, std_heap, DEFAULT_HEAP_SIZE, avs_heap, DEFAULT_HEAP_SIZE,
            logger, NULL);
    }

    g_print_logs = was_print;

    return true;
}

void shutdown() { avs_shutdown(); }

bool load_dll() {
    auto avs = LoadLibraryA("libavs-win32.dll");
    if (!avs)
        avs = LoadLibraryA("libavs-win64.dll");
    if (!avs)
        avs = LoadLibraryA("avs2-core.dll");

    if (!avs) {
        log_fatal("Playpen: Couldn't load avs dll");
        return false;
    }

    return true;
}

void log_writer(const char* chars, uint32_t nchars, void* ctx) {
    if (!g_print_logs)
        return;

    std::string_view msg(chars, nchars);

    // lol, ancient avs will emit a bonus NUL when adding the newline
    if (avs_loaded_version <= 1300 && msg.ends_with('\0'))
        msg.remove_suffix(1);

    // don't print noisy shutdown logs
    if (msg.starts_with("[----/--/-- --:--:--] "))
        return;

    fmt::print(stderr, "{}", msg);
}

void log_writer_lt_1500(void* ctx, const char* chars, uint32_t nchars) {
    log_writer(chars, nchars, ctx);
}

static std::string_view boot_cfg_reader;

static void reset_boot_cfg_reader() {
    static std::string cfg;

    auto log_level = avs_loaded_version >= 1400 ? "<level __type=\"str\">misc</level>"
                                                : "<level __type=\"u32\">4</level>";
    auto ea_on     = avs_loaded_version >= 1400 ? "<ea_on __type=\"bool\">0</ea_on>"
                                                : "<ea_on __type=\"u8\">0</ea_on>";
    auto mounts    = avs_loaded_version >= 1500 && avs_loaded_version < 1630 ? R"(
        <raw>
            <fstype>fs</fstype>
            <device>dev/raw</device>
        </raw>
        <nvram>
            <fstype>fs</fstype>
            <device>dev/nvram</device>
            <option>posix=1</option>
        </nvram>
        )"
                                                                             : R"(
        <mounttable>
            <vfs name="boot" fstype="fs" src="dev/raw" dst="/dev/raw" opt="vf=1,posix=1"/>
            <vfs name="boot" fstype="fs" src="dev/nvram" dst="/dev/nvram" opt="vf=0,posix=1"/>
        </mounttable>
    )";

    cfg = fmt::format(R"(<?xml version="1.0" encoding="SHIFT_JIS"?>
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
        {}
    </fs>
    <log>
        {}
        <!-- 2.14.x and later -->
        <output_delay __type="u16">0</output_delay>
    </log>
    <sntp>
        {}
        <servers/>
    </sntp>
</config>
)",
        mounts, log_level, ea_on);

    boot_cfg_reader = cfg;
}

static size_t read_boot_cfg(int32_t context, void* dst_buf, size_t count) {
    count = std::min(count, boot_cfg_reader.size());
    memcpy(dst_buf, boot_cfg_reader.data(), count);
    boot_cfg_reader.remove_prefix(count);
    return count;
}

LONG WINAPI exc_handler(_EXCEPTION_POINTERS* ExceptionInfo) {
    switch (ExceptionInfo->ExceptionRecord->ExceptionCode) {
        case DBG_PRINTEXCEPTION_C:
            break;
        default:
            log_warning("Unhandled exception code {:#x} at {:p}",
                ExceptionInfo->ExceptionRecord->ExceptionCode,
                ExceptionInfo->ExceptionRecord->ExceptionAddress);
            break;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

} // namespace avs_standalone
