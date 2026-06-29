#include <filesystem>
#include <stdint.h>
#include <system_error>
#include <windows.h>
#include <winternl.h>

#include "hook.hpp"

// all good code mixes C and C++, right?
#include <unordered_set>
#include <algorithm>
#include <map>
#include <ranges>
#include <set>
#include <sstream>

#include "3rd_party/MinHook.h"

#include "ramfs_demangler.hpp"
#include "buildconfig.hpp"
#include "config.hpp"
#include "log.hpp"
#include "imagefs.hpp"
#include "texbin.hpp"
#include "arc.hpp"
#include "utils.hpp"
#include "avs.hpp"
#include "modpath_handler.hpp"

#ifdef _DEBUG
#define DBG_VER_STRING "_DEBUG"
#else
#define DBG_VER_STRING
#endif

#define VERSION VER_STRING DBG_VER_STRING

// debugging
//#define ALWAYS_CACHE

// used by pkfs hooks - we don't want to hook avs_fs_open if we just hooked pkfs
thread_local static bool inside_pkfs_hook;

unsigned int (*pkfs_fs_open)(const char *path);
unsigned int (*pkfs_fs_fstat)(unsigned int f, struct avs_stat *stat);
unsigned int (*pkfs_fs_read)(unsigned int f, void *buf, int sz);
unsigned int (*pkfs_fs_close)(unsigned int f);
void (*pkfs_clear_hdd_error)();

class PkfsHookFile final : public HookFile {
    public:
    PkfsHookFile(istring &&path, NormPath &&norm_path)
        : HookFile(std::move(path), std::move(norm_path))
    {}

    bool ramfs_demangle() override {return false;};

    uint32_t call_real() override {
        log_if_modfile();
        auto ret = pkfs_fs_open(get_path_to_open().c_str());
        if(ret == 0) {
            log_verbose("pkfs_fs_open({}) failed in call_real", get_path_to_open());
        }
        return ret;
    }

    std::optional<std::vector<uint8_t>> load_to_vec() override {
        AVS_FILE f = pkfs_fs_open(get_path_to_open().c_str());
        if (f != 0) {
            avs_stat stat = {0}; // stat type is shared!
            pkfs_fs_fstat(f, &stat);
            std::vector<uint8_t> ret;
            ret.resize(stat.filesize);
            pkfs_fs_read(f, &ret[0], stat.filesize);
            pkfs_fs_close(f);
            return ret;
        } else {
            // failed pkfs_fs_open will set an HDD read error, which is actually
            // checked during boot. Usually a non-issue, since every read is
            // preceded by an avs_fs_fstat, which creates the cached .bin.
            //
            // This of course is racey, so if there is a *real* error in another
            // thread, this resets it. But it's a tight race, and I'll take that
            // chance.
            log_verbose("pkfs_open({}) failed in load_to_vec, clearing HDD error", get_path_to_open());
            pkfs_clear_hdd_error();
            return std::nullopt;
        }
    }
};

// this should probably be part of the modpath h/cpp
void list_pngs_onefolder(std::unordered_set<istring> &names, istring const&folder) {
    std::error_code err;
    for (const auto& entry : std::filesystem::directory_iterator(folder, err)) {
        if (!entry.is_regular_file())
            continue;

        istring ext(entry.path().extension().string());
        if (ext != ".png")
            continue;

        names.emplace(istring(entry.path().stem().string()));
    }
}

std::unordered_set<istring> list_pngs(NormPath const&folder) {
    std::unordered_set<istring> ret;

    for (auto &mod : available_mods()) {
        auto path = mod / folder;
        list_pngs_onefolder(ret, path);
        list_pngs_onefolder(ret, path / "tex");
    }

    return ret;
}

struct ArcModScan {
    // Per-entry overrides. Keyed by relative path within the arc, e.g. "shader/foo.bin".
    // Tuple of actual path / norm path
    std::map<istring, std::pair<istring, NormPath>> files;
    // Inner-ifs paths inside the arc, derived from "*_ifs" mod subdirs. Paths
    // are arc-relative with the dot restored: a mod dir "sub/inner_ifs/" yields
    // "sub/inner.ifs". These get registered with the demangler so the game's
    // ramfs mount of the extracted inner ifs maps back to the arc-qualified
    // path our mod-folder lookup expects. Set, so multiple mods declaring the
    // same inner ifs dedupe.
    std::set<istring> inner_ifs_paths;
};

static void scan_arc_mod_onefolder(ArcModScan &out, istring const& mod_folder, NormPath const& folder, istring const& rel_prefix = "") {
    // don't do a recursive iterator, makes it more awkward to skip the _ifs folders
    std::error_code err;
    for (const auto& entry : std::filesystem::directory_iterator(mod_folder / folder, err)) {
        istring name(entry.path().filename().string());
        if (entry.is_directory()) {
            if (name.ends_with("_ifs")) {
                name.replace(name.size() - 4, 4, ".ifs");
                out.inner_ifs_paths.insert(rel_prefix + name);
                continue;
            }

            scan_arc_mod_onefolder(out,
                mod_folder,
                folder / name,
                rel_prefix + name + "/");
        } else if (entry.is_regular_file()) {
            istring rel = rel_prefix + name;
            if (out.files.find(rel) == out.files.end())
                out.files[rel] = std::make_pair(
                    mod_folder / folder / name,
                    folder / name
                );
        }
    }
}

static ArcModScan scan_arc_mod_folder(NormPath const& folder) {
    ArcModScan ret;
    for (auto &mod : available_mods()) {
        scan_arc_mod_onefolder(ret, mod, folder);
    }
    return ret;
}

void handle_arc(HookFile &file) {
    Timer timer;

    auto arc_mod_path = file.norm_path;
    arc_mod_path.replace_suffix(".arc", "_arc");

    if (!find_first_modfolder(arc_mod_path)) {
        return;
    }

    auto scan = scan_arc_mod_folder(arc_mod_path);
    if (scan.files.empty() && scan.inner_ifs_paths.empty()) {
        log_verbose("arc: mod folder has no files, skipping");
        return;
    }

    for (auto const& inner_rel : scan.inner_ifs_paths) {
        auto pos = inner_rel.rfind('/');
        istring basename = (pos == std::string::npos) ? inner_rel : istring(inner_rel.substr(pos + 1));
        istring demangled = istring("data") / arc_mod_path / inner_rel;
        ramfs_demangler_register_arc_inner_ifs(basename.downcast_ref(), demangled.downcast_ref());
    }

    if (scan.files.empty()) {
        return;
    }

    auto out = config.get_cache_folder() / file.norm_path;
    auto cache_hasher = CacheHasher(out + ".hashed");

    auto starting = file.get_path_to_open();
    cache_hasher.add(starting);
    for (auto &[_, path] : scan.files) {
        cache_hasher.add(path.first);
    }
    cache_hasher.finish();

    if (cache_hasher.matches()) {
        file.mod_path = out;
        log_verbose("arc cache up to date, skip");
        return;
    }
    log_verbose("arc: regenerating cache");

    ArcArchive arc;
    auto _orig_data = file.load_to_vec();
    if (_orig_data) {
        auto &orig_data = *_orig_data;
        std::istringstream stream(std::string((char*)orig_data.data(), orig_data.size()));
        auto _arc = ArcArchive::from_stream(stream);
        if (!_arc) {
            log_warning("arc: load failed, aborting modding");
            return;
        }
        arc = std::move(*_arc);
    } else {
        log_info("arc: no original file, creating from scratch: \"{}\"", file.norm_path);
    }

    auto out_folder = out.substr(0, out.rfind("/"));
    if (!mkdir_p(out_folder)) {
        log_warning("arc: couldn't create output cache folder");
        return;
    }

    // TODO: this is a terrible hack really. XML merging assumes the file
    // exists on disk, so we need to make it so if it only exists inside of
    // the original .arc file
    for (const auto &arc_file : arc.files) {
        if (!arc_file.first.ends_with(".xml")) {
            continue;
        }

        // do we have an overlaid base xml? If so, nothing to do
        bool found = false;
        for (auto &[name, path] : scan.files) {
            // TODO: more hacks, make these paths less insane
            auto parent_pos = path.second.find("/");
            if (parent_pos == path.second.npos) {
                continue;
            }
            auto path_noparent = path.second.substr(parent_pos + 1);

            if (_stricmp(arc_file.first.c_str(), path_noparent.c_str())) {
                continue;
            }
            found = true;
        }

        if (found)
            continue;


        // Do we have XMLs to merge? Only then, extract to cache and add a fake
        // extra entry. We don't need to add this particular file to the cache
        // because it gets its key off the original .arc file
        auto merged_fname = arc_file.first;
        merged_fname.replace_suffix(".xml", ".merged.xml");
        decltype(scan.files) extra_files;
        for (auto &[name, path] : scan.files) {
            // TODO: more hacks, make these paths less insane
            auto parent_pos = path.second.find("/");
            if (parent_pos == path.second.npos) {
                continue;
            }
            auto path_noparent = path.second.substr(parent_pos + 1);
            auto path_justparent = path.second.substr(0, parent_pos);

            if (_stricmp(merged_fname.c_str(), path_noparent.c_str())) {
                continue;
            }

            auto xml_orig = config.get_cache_folder() / file.norm_path / (arc_file.first + ".orig");
            auto xml_orig_norm = file.norm_path / arc_file.first;
            xml_orig.replace_suffix_foreach_path_component(".arc", "_arc");
            xml_orig_norm.replace_suffix_foreach_path_component(".arc", "_arc");

            auto out_folder = xml_orig.substr(0, xml_orig.rfind("/"));
            if (!mkdir_p(out_folder)) {
                log_warning("Couldn't create arc xml cache folder {}", out_folder);
                continue;
            }

            if (!write_bytes(xml_orig, arc_file.second)) {
                log_warning("Couldn't create arc xml base at {}", xml_orig);
                continue;
            }
            extra_files.emplace(arc_file.first, std::make_pair(xml_orig, xml_orig_norm));
        }
        for (auto &[name, path] : extra_files)
            scan.files.emplace(name, std::move(path));
    }

    for (auto &[name, path] : scan.files) {
        if (name.ends_with(".merged.xml"))
            continue;

        // can't move, so clone
        AvsOpenHookFile f(istring(path.first), NormPath(path.second), 0, 0);

        if (name.ends_with(".xml"))
            merge_xmls(f);

        auto data = f.load_to_vec();
        if (!data) {
            log_warning("arc: couldn't load mod file '{}'", path.first);
            continue;
        }
        arc.add_or_replace(name, std::move(*data));
    }

    if (!arc.save(out.c_str())) {
        log_warning("arc: couldn't write output");
        return;
    }

    cache_hasher.commit();
    file.mod_path = out;

    log_misc("arc generation took {}", timer.elapsed());
}

void handle_texbin(HookFile &file) {
    Timer timer;

    auto bin_mod_path = file.norm_path;
    // mod texbins strip the .bin off the end. This isn't consistent with the _ifs
    // used for ifs files, but it's consistent with gitadora-texbintool, the *only*
    // tool to extract .bin files currently.
    bin_mod_path.replace_suffix(".bin", "");

    if (!find_first_modfolder(bin_mod_path)) {
        // log_verbose("texbin: mod folder doesn't exist, skipping");
        return;
    }

    auto pngs = list_pngs(bin_mod_path);

    //// This whole hashing section is just a tiny bit different from the XML
    //// hashing :(

    // convert the string set to a vec for repeatable hashes
    // pairs of texname / mod path
    std::vector<std::pair<istring, istring>> pngs_list;
    pngs_list.reserve(pngs.size());
    for (auto it = pngs.begin(); it != pngs.end(); ) {
        auto png = std::move(pngs.extract(it++).value());
        auto png_res = find_first_modfile(bin_mod_path / (png + ".png"));
        if(png_res) {
            // I have yet to see a texbin without allcaps names for textures
            for (auto &c : png)
                c = toupper_ascii(c);

            pngs_list.emplace_back(std::move(png), std::move(*png_res));
        }
    }
    // sort by actual filename
    std::sort(pngs_list.begin(), pngs_list.end(), [](auto &left, auto &right) {
        return left.second < right.second;
    });

    // nothing to do...
    if (pngs_list.size() == 0) {
        log_verbose("texbin: mod folder has no PNGs, skipping");
        return;
    }

    auto starting = file.get_path_to_open();
    auto out = config.get_cache_folder() / file.norm_path;
    auto out_hashed = out + ".hashed";
    auto cache_hasher = CacheHasher(out_hashed);

    cache_hasher.add(starting);
    for (auto &[_, path] : pngs_list) {
        cache_hasher.add(path);
    }
    cache_hasher.finish();

    // no need to merge - timestamps all up to date, dll not newer, files haven't been deleted
    if(cache_hasher.matches()) {
        file.mod_path = out;
        log_verbose("texbin cache up to date, skip");
        return;
    }
    log_verbose("Regenerating cache");

    Texbin texbin;
    auto _orig_data = file.load_to_vec();
    if (_orig_data) {
        auto orig_data = *_orig_data;
        // one extra copy which *sucks* but whatever
        std::istringstream stream(std::string((char*)&orig_data[0], orig_data.size()));
        auto _texbin = Texbin::from_stream(stream);
        if(!_texbin) {
            log_warning("Texbin load failed, aborting modding");
            return;
        }
        texbin = *_texbin;
    } else {
        log_info("Found texbin mods but no original file, creating from scratch: \"{}\"", file.norm_path);
    }

    auto folder_terminator = out.rfind("/");
    auto out_folder = out.substr(0, folder_terminator);
    if (!mkdir_p(out_folder)) {
        log_warning("Texbin: Couldn't create output cache folder");
        return;
    }

    for (auto &[tex_name, path] : pngs_list) {
        texbin.add_or_replace_image(tex_name.c_str(), path.c_str());
    }

    if(!texbin.save(out.c_str())) {
        log_warning("Texbin: Couldn't create output");
        return;
    }

    cache_hasher.commit();
    file.mod_path = out;

    log_misc("Texbin generation took {}", timer.elapsed());
}

uint32_t handle_file_open(HookFile &file) {
    auto norm_copy = file.norm_path;
    file.mod_path = find_first_modfile(norm_copy);

    if (!file.mod_path) {
        // mod ifs paths use _ifs, go one at a time for ifs-inside-ifs
        for (auto el : std::views::split(norm_copy, std::string_view("/"))) {
            istring_view segment(el);
            if (!segment.ends_with(".ifs"))
                continue;

            el[el.size() - 4] = '_';

            file.mod_path = find_first_modfile(norm_copy);
            if (file.mod_path)
                break;
        }
    }

    if(file.path.ends_with(".xml")) {
        merge_xmls(file);
    }

    if(file.path.ends_with(".bin")) {
        handle_texbin(file);
    }

    if(file.path.ends_with(".arc")) {
        handle_arc(file);
    }

    if (file.path.ends_with("texturelist.xml")) {
        parse_texturelist(file);
    } else if(file.path.ends_with("afplist.xml")) {
        parse_afplist(file);
    } else {
        handle_texture(file);
        handle_afp(file);
    }

    auto ret = file.call_real();
    if(file.ramfs_demangle()) {
        ramfs_demangler_on_fs_open(file.path.downcast_string(), ret);
    }
    // log_verbose("(returned {})", ret);
    return ret;
}

int hook_avs_fs_lstat(const char* name, struct avs_stat *st) {
    if (name == NULL)
        return avs_fs_lstat(name, st);

    std::string path = name;
    log_verbose("statting {}", name);

    // can it be modded ie is it under /data ?
    auto norm_path = normalise_path(path);
    if (!norm_path)
        return avs_fs_lstat(name, st);
    // unpack success
    AvsLstatHookFile file(std::move(path), std::move(*norm_path), st);

    return handle_file_open(file);
}

int hook_avs_fs_convert_path(char dest_name[256], const char *name) {
    if (name == NULL)
        return avs_fs_convert_path(dest_name, name);

    std::string path = name;
    log_verbose("convert_path {}", name);

    // can it be modded ie is it under /data ?
    auto norm_path = normalise_path(path);
    if (!norm_path)
        return avs_fs_convert_path(dest_name, name);
    // unpack success
    AvsConvertPathHookFile file(std::move(path), std::move(*norm_path), dest_name);

    return handle_file_open(file);
}

int hook_avs_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* _args) {
    std::optional<std::string_view> args = _args ? std::optional(_args) : std::nullopt;
    log_verbose("mounting {} to {} with type {} and args {}", fsroot, mountpoint, fstype, args);
    ramfs_demangler_on_fs_mount(mountpoint, fsroot, fstype, args);

    // In new jubeat, a modded IFS file will be loaded as such:
    // pkfs_open data/music/xxxx/bsc.eve
    // mounting /data/ifs_pack/xxxx/yyyy_msc.ifs to /data/imagefs/msc/xxxx with type imagefs and args (NULL)
    // avs_fs_open /data/ifs_pack/xxxx/yyyy_msc.ifs
    // ... Because of this, we have to go back to hooking avs_fs_open when a mount is seen.
    // It shouldn't affect old-beat (the reason pkfs hooks exist at all) because
    // they use .bin files instead of .ifs files.
    inside_pkfs_hook = false;

    return avs_fs_mount(mountpoint, fsroot, fstype, _args);
}

size_t hook_avs_fs_read(AVS_FILE context, void* bytes, size_t nbytes) {
    ramfs_demangler_on_fs_read(context, bytes);
    return avs_fs_read(context, bytes, nbytes);
}

static DWORD (WINAPI *real_GetLongPathNameA)(LPCSTR lpszShortPath, LPSTR lpszLongPath, DWORD cchBuffer);

DWORD WINAPI hook_GetLongPathNameA(LPCSTR lpszShortPath, LPSTR lpszLongPath, DWORD cchBuffer) {
    // have seen massive paths in DDR World ifs-in-arc cases, e.g:
    // ./data_mods/_cache/arc/custom/background/background_0001_arc/data/custom/background/background_0001_ifs/ebb521b5f25bf88e09dbd3aa19a48c60
    //
    // AVS has a 128 char buffer that it gives to GetLongPathNameA:
    // If it succeeds, it does a strcmp against the original path (sure, I guess?).
    // If it fails, it compares against ERROR_FILE_NOT_FOUND and
    // ERROR_PATH_NOT_FOUND and if it's one of those it just falls through to
    // CreateFileA with the original arg. So because we're succeeding with a buffer
    // too large, the strcmp fails, so we pretend it failed and do the fallthru.

    DWORD ret = real_GetLongPathNameA(lpszShortPath, lpszLongPath, cchBuffer);

    if(cchBuffer == 0x80 && ret > cchBuffer && strstr(lpszShortPath, config.get_mod_folder_native().c_str()) != nullptr) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return 0;
    }

    return ret;
}

AVS_FILE hook_avs_fs_open(const char* name, uint16_t mode, int flags) {
    if(name == NULL || inside_pkfs_hook)
        return avs_fs_open(name, mode, flags);
    log_verbose("opening {} mode {} flags {}", name, mode, flags);
    // only touch reads
    if (mode != avs_open_mode_read()) {
        return avs_fs_open(name, mode, flags);
    }
    std::string path = name;

    // can it be modded ie is it under /data ?
    auto norm_path = normalise_path(path);
    if (!norm_path)
        return avs_fs_open(name, mode, flags);
    // unpack success
    AvsOpenHookFile file(std::move(path), std::move(*norm_path), mode, flags);

    return handle_file_open(file);
}

unsigned int hook_pkfs_open(const char *name) {
    log_verbose("pkfs_open {}", name);

    std::string path = name;

    // can it be modded ie is it under /data ?
    auto norm_path = normalise_path(path);
    if (!norm_path) {
        log_verbose("pkfs_open falling back to real (no norm)");
        return pkfs_fs_open(name);
    }
    // unpack success
    PkfsHookFile file(std::move(path), std::move(*norm_path));

    // note that this also hides the avs_fs_open of the pakfile holding a
    // particular file - acceptable compromise IMO
    inside_pkfs_hook = true;

    if (g_build_config.pkfs_unpack) {
        std::filesystem::path pakdump_loc(fmt::format("./data_unpak/{}", file.norm_path));
        if(!std::filesystem::is_regular_file(pakdump_loc)) {
            auto data = file.load_to_vec();
            if(data) {
                auto out_folder = pakdump_loc.parent_path();
                if(mkdir_p(out_folder)) {
                    if(write_bytes(pakdump_loc, *data)) {
                        log_info("Dumped new pkfs file!");
                    } else {
                        log_warning("Pakdump: Couldn't open output file {}", pakdump_loc);
                    }
                } else {
                    log_warning("Pakdump: Couldn't create output folder {}", out_folder);
                }
            }
        }
    }

    auto ret = handle_file_open(file);
    inside_pkfs_hook = false;
    return ret;
}

static void dump_loaded_dll_info() {
    log_verbose("DLLs loaded:");
    auto peb = NtCurrentTeb()->ProcessEnvironmentBlock;
    auto head = peb->Ldr->InMemoryOrderModuleList.Flink;
    bool first = true;
    for(auto mod = head; mod && (first || mod != head); mod = mod->Flink) {
        auto ldr = reinterpret_cast<PLDR_DATA_TABLE_ENTRY>(mod);
        std::string narrow_dll_name(ldr->FullDllName.Buffer, ldr->FullDllName.Buffer + ldr->FullDllName.Length);
        log_verbose("  {}", narrow_dll_name);
        first = false;
    }
}

extern "C" {
    __declspec(dllexport) int init() {
        // all logs up until init_avs succeeds will go to a file for debugging purposes

        // find out where we're logging to
        load_config();

        if (MH_Initialize() != MH_OK) {
            dump_loaded_dll_info();
            log_fatal("Couldn't initialize MinHook");
            return 1;
        }

        if (!init_avs()) {
            dump_loaded_dll_info();
            log_fatal("Couldn't find ifs operations in dll. Send avs dll (libavs-winxx.dll or avs2-core.dll) to mon.");
            return 2;
        }

        // re-route to AVS logs if no external file specified
        if(!config.logfile) {
            imp_log_body_fatal = log_body_fatal;
            imp_log_body_warning = log_body_warning;
            imp_log_body_info = log_body_info;
            imp_log_body_misc = log_body_misc;
        }

        // now we can say hello!
        log_info("IFS layeredFS v" VERSION " init");
        if (g_build_config.special_ver)
            log_info("Build config: {}", *g_build_config.special_ver);
        log_info("AVS DLL detected: {}", avs_loaded_dll_name);
        print_config();
        if (g_build_config.pkfs_unpack)
            log_info(".pak dumper mode enabled");

        dump_loaded_dll_info();

        init_modpath_handler();
        cache_mods();

        // hook pkfs, not big enough to be its own file
        if(MH_CreateHookApi(L"libpackfs.dll", "?pkfs_fs_open@@YAIPBD@Z", (LPVOID)&hook_pkfs_open, (LPVOID*)&pkfs_fs_open) == MH_OK) {
            auto mod = GetModuleHandleA("libpackfs.dll");
            pkfs_fs_fstat = (decltype(pkfs_fs_fstat))GetProcAddress(mod, "?pkfs_fs_fstat@@YAEIPAUT_AVS_FS_STAT@@@Z");
            pkfs_fs_read = (decltype(pkfs_fs_read))GetProcAddress(mod, "?pkfs_fs_read@@YAHIPAXH@Z");
            pkfs_fs_close = (decltype(pkfs_fs_close))GetProcAddress(mod, "?pkfs_fs_close@@YAHI@Z");
            pkfs_clear_hdd_error = (decltype(pkfs_clear_hdd_error))GetProcAddress(mod, "?pkfs_clear_hdd_error@@YAXXZ");
        } else if(MH_CreateHookApi(L"pkfs.dll", "pkfs_fs_open", (LPVOID)&hook_pkfs_open, (LPVOID*)&pkfs_fs_open) == MH_OK) {
            // jubeat DLL has no mangling - only one of these will succeed (if at all)
            auto mod = GetModuleHandleA("pkfs.dll");
            pkfs_fs_fstat = (decltype(pkfs_fs_fstat))GetProcAddress(mod, "pkfs_fs_fstat");
            pkfs_fs_read = (decltype(pkfs_fs_read))GetProcAddress(mod, "pkfs_fs_read");
            pkfs_fs_close = (decltype(pkfs_fs_close))GetProcAddress(mod, "pkfs_fs_close");
            pkfs_clear_hdd_error = (decltype(pkfs_clear_hdd_error))GetProcAddress(mod, "pkfs_clear_hdd_error");
        }

        if(pkfs_fs_open) {
            if(pkfs_fs_fstat && pkfs_fs_read && pkfs_fs_close && pkfs_clear_hdd_error) {
                log_info("pkfs hooks activated");
            } else {
                log_fatal("Couldn't fully init pkfs hook - open an issue!");
            }
        }

        if (MH_CreateHookApi(L"kernel32.dll", "GetLongPathNameA", (LPVOID)&hook_GetLongPathNameA, (LPVOID*)&real_GetLongPathNameA) != MH_OK) {
            log_warning("Couldn't hook GetLongPathNameA");
        }

        if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
            log_warning("Couldn't enable hooks");
            return 2;
        }
        log_info("Hook DLL init success");

        log_info("Detected mod folders:");
        for (auto &p : available_mods()) {
            log_info("{}", p);
        }

        return 0;
    }
}
