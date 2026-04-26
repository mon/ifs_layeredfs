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
#include <cstring>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <optional>

#include "3rd_party/hat-trie/htrie_map.h"

#include "arc.hpp"
#include "ramfs_demangler.h"
#include "log.hpp"
#include "utils.hpp"
#include "winxp_mutex.hpp"

using namespace std;

typedef struct {
	AVS_FILE handle;
	void* buffer;
	optional<string> ramfs_path;
	optional<string> link_path;
	optional<string> mounted_path;
} file_cleanup_info_t;

static map<string, file_cleanup_info_t, CaseInsensitiveCompare> cleanup_map;
static unordered_map<AVS_FILE, string> open_file_map;
static unordered_map<void*, string> ram_load_map;
// using tries for fast prefix matches on our mangled names
static tsl::htrie_map<char, string> ramfs_map;
static tsl::htrie_map<char, string> mangling_map;
// Set of original game-side paths whose .arc has been repacked uncompressed.
// Only these arcs get their inner .ifs files registered into ram_load_map.
static unordered_set<string> repacked_arcs;

static CriticalSectionLock mangling_mtx;

// since we call this from a function that is already taking the lock
static void ramfs_demangler_demangle_if_possible_nolock(std::string& raw_path);

void ramfs_demangler_on_fs_open(const std::string& path, AVS_FILE open_result) {
	if (open_result < 0
		|| (!string_ends_with(path.c_str(), ".ifs")
			&& !string_ends_with(path.c_str(), ".arc"))) {
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
		if (cleanup.link_path) {
			ramfs_map.erase(*cleanup.link_path);
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

void ramfs_demangler_register_arc_repack(const std::string& original_path) {
	mangling_mtx.lock();
	repacked_arcs.insert(original_path);
	mangling_mtx.unlock();
}

// Walk the directory of a repacked uncompressed arc and register each inner
// .ifs at (buffer + entry.file_offset) so a later ramfs mount with that base
// pointer is demangled to "<arc_path>/<inner.ifs>". Only inner files ending
// in .ifs are tracked; everything else is noise we'd never demangle.
static void register_arc_inner_ifs_nolock(const std::string& arc_path, void* dest, size_t nbytes) {
	if (dest == nullptr || nbytes < sizeof(ArcHeader)) {
		return;
	}
	auto* hdr = reinterpret_cast<const ArcHeader*>(dest);
	if (hdr->magic != ARC_MAGIC || hdr->compression != ARC_COMPRESSION_NONE) {
		log_warning("arc demangle: %s missing/compressed header (magic=%08x compression=%u), skipping",
			arc_path.c_str(), hdr->magic, hdr->compression);
		return;
	}

	size_t entries_end = sizeof(ArcHeader) + (size_t)hdr->filecount * sizeof(ArcEntry);
	if (entries_end > nbytes) {
		return;
	}

	auto* entries = reinterpret_cast<const ArcEntry*>(
		reinterpret_cast<const uint8_t*>(dest) + sizeof(ArcHeader));
	auto* base = reinterpret_cast<uint8_t*>(dest);

	for (uint32_t i = 0; i < hdr->filecount; i++) {
		const auto& e = entries[i];
		if (e.str_offset >= nbytes) continue;
		const char* name = reinterpret_cast<const char*>(base + e.str_offset);
		if (memchr(name, 0, nbytes - e.str_offset) == nullptr) continue;
		if (!string_ends_with(name, ".ifs")) continue;
		if ((size_t)e.file_offset + e.packed_size > nbytes) continue;

		void* inner = base + e.file_offset;
		string inner_path = arc_path + "/" + name;
		ram_load_map[inner] = inner_path;
		log_verbose("arc inner mapped %p -> %s", inner, inner_path.c_str());
	}
}

void ramfs_demangler_on_fs_read(AVS_FILE context, void* dest, size_t nbytes) {
	mangling_mtx.lock();

	auto find = open_file_map.find(context);
	if (find != open_file_map.end()) {
		auto path = find->second;
		ram_load_map[dest] = path;

		auto cleanup = cleanup_map.find(path);
		if (cleanup != cleanup_map.end()) {
			cleanup->second.buffer = dest;
		}

		if (repacked_arcs.count(path)) {
			register_arc_inner_ifs_nolock(path, dest, nbytes);
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
	else if (!strcmp(fstype, "link")) {
		auto find = ramfs_map.find(fsroot);
		if (find != ramfs_map.end()) {
			auto orig_path = *find;
			log_verbose("link mount mapped to %s", orig_path.c_str());
			ramfs_map[mountpoint] = orig_path;

			auto cleanup = cleanup_map.find(orig_path);
			if (cleanup != cleanup_map.end()) {
				cleanup->second.link_path = mountpoint;
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
