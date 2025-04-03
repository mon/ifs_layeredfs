/*
	IIDX mounts sounds files annoyingly, for example
	/data/sound/XXX.ifs -> /sdXXX/XXX/
	If avs_fs_mount is called with a known good source but known bad dest, save the mapping
	so it can be demangled later.

	Several games load IFS files into RAM for faster loads.
	Some games (SDVX, mostly) will load these into paths which contain their source, which is fine.
	Some games (Nostalgia with some IFS files) will use a random/different path.

	How do we detect and override this?
	The sequence is as follows:
		avs_fs_lstat to get the size (we ignore this)
		avs_fs_open the ifs file
		avs_fs_read its entire contents into a buffer just alloced
		avs_fs_mount with type ramfs
			options including base=%p, pointing at the buffer fs_read used
			path join mountpoint + root to get the virtual filename
		avs_fs_mount root=ram path, mountpoint = something dumb, type = imagefs

	So,
		avs_fs_open - save mapping from handle -> filename
		avs_fs_read - save mapping from buffer address -> (handle ->) filename
		avs_fs_mount - save mapping from ramfs filename -> (buffer -> handle ->) filename
		avs_fs_mount - save mapping from imagefs filename -> (ramfs -> buffer -> handle ->) filename
		normalise_path - map imagefs filename to real filename before checking mods folder

		As long as the mappings are unique (filename -> x instead of x -> filename)
		we won't leak memory as there is a finite number of filenames to map to.
		At worst, maybe a meg of memory will be lost to saving filename mappings.
*/

#include <algorithm>
#include <map>
#include <unordered_map>
#include <optional>

#include "3rd_party/hat-trie/htrie_map.h"

#include "ramfs_demangler.h"
#include "log.hpp"
#include "utils.hpp"
#include "winxp_mutex.hpp"

using namespace std;

typedef struct {
	AVS_FILE handle;
	void* buffer;
	optional<string> ramfs_path;
	optional<string> mounted_path;
} file_cleanup_info_t;

static map<string, file_cleanup_info_t, CaseInsensitiveCompare> cleanup_map;
static unordered_map<AVS_FILE, string> open_file_map;
static unordered_map<void*, string> ram_load_map;
// using tries for fast prefix matches on our mangled names
static tsl::htrie_map<char, string> ramfs_map;
static tsl::htrie_map<char, string> mangling_map;

static CriticalSectionLock mangling_mtx;

// since we call this from a function that is already taking the lock
static void ramfs_demangler_demangle_if_possible_nolock(std::string& raw_path);

void ramfs_demangler_on_fs_open(const std::string& path, AVS_FILE open_result) {
	if (open_result < 0 || !string_ends_with(path.c_str(), ".ifs")) {
		return;
	}

	mangling_mtx.lock();

	auto existing_info = cleanup_map.find(path);
	if (existing_info != cleanup_map.end()) {
		file_cleanup_info_t cleanup = existing_info->second;

		open_file_map.erase(cleanup.handle);
		if (cleanup.buffer != NULL) {
			ram_load_map.erase(cleanup.buffer);
		}
		if (cleanup.ramfs_path) {
			ramfs_map.erase(*cleanup.ramfs_path);
		}
		if (cleanup.mounted_path) {
			mangling_map.erase(*cleanup.mounted_path);
		}
		cleanup_map.erase(existing_info);
	}
	file_cleanup_info_t cleanup = {
		open_result,
		NULL,
		nullopt,
		nullopt
	};
	cleanup_map[path] = cleanup;
	open_file_map[open_result] = path;

	mangling_mtx.unlock();
}

void ramfs_demangler_on_fs_read(AVS_FILE context, void* dest) {
	mangling_mtx.lock();

	auto find = open_file_map.find(context);
	if (find != open_file_map.end()) {
		auto path = find->second;
		// even this is too verbose
		//log_verbose("Mapped %p to %s", dest, path.c_str());
		ram_load_map[dest] = path;

		auto cleanup = cleanup_map.find(path);
		if (cleanup != cleanup_map.end()) {
			cleanup->second.buffer = dest;
		}
	}

	mangling_mtx.unlock();
}

void ramfs_demangler_on_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* flags) {
	mangling_mtx.lock();

	if (!strcmp(fstype, "ramfs")) {
		void* buffer;

		if (!flags) {
			log_verbose("ramfs has no flags?");
			mangling_mtx.unlock();
			return;
		}
		const char* baseptr = strstr(flags, "base=");
		if (!baseptr) {
			log_verbose("ramfs has no base pointer?");
			mangling_mtx.unlock();
			return;
		}

		buffer = (void*)strtoull(baseptr + strlen("base="), NULL, 0);

		auto find = ram_load_map.find(buffer);
		if (find != ram_load_map.end()) {
			auto orig_path = find->second;
			log_verbose("ramfs mount mapped to %s", orig_path.c_str());
			string mount_path = (string)mountpoint + "/" + fsroot;
			ramfs_map[mount_path.c_str()] =  orig_path;

			auto cleanup = cleanup_map.find(orig_path);
			if (cleanup != cleanup_map.end()) {
				cleanup->second.ramfs_path = mount_path;
			}
		}
	}
	else if (!strcmp(fstype, "imagefs")) {
		auto find = ramfs_map.longest_prefix(fsroot);
		if (find != ramfs_map.end()) {
			auto orig_path = *find;
			log_verbose("imagefs mount mapped to %s", orig_path.c_str());
			mangling_map[mountpoint] = orig_path;

			auto cleanup = cleanup_map.find(orig_path);
			if (cleanup != cleanup_map.end()) {
				cleanup->second.mounted_path = mountpoint;
			}
		}
		else if(string_ends_with(fsroot, ".ifs")) {
			// this fixes ifs-inside-ifs by demangling the root location too
			string root = (string)fsroot;
			ramfs_demangler_demangle_if_possible_nolock(root);
			log_verbose("imagefs mount mapped to %s", root.c_str());
			mangling_map[mountpoint] = root;
		}
	}

	mangling_mtx.unlock();
}

void ramfs_demangler_demangle_if_possible(std::string& raw_path) {
	mangling_mtx.lock();

	auto search = mangling_map.longest_prefix(raw_path);
	if (search != mangling_map.end()) {
		// log_verbose("can demangle %s to %s", search.key().c_str(), search->c_str());
		string_replace(raw_path, search.key().c_str(), search->c_str());
	}

	mangling_mtx.unlock();
}

static void ramfs_demangler_demangle_if_possible_nolock(std::string& raw_path) {
	auto search = mangling_map.longest_prefix(raw_path);
	if (search != mangling_map.end()) {
		string_replace(raw_path, search.key().c_str(), search->c_str());
	}
}
