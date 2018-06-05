#include "modpath_handler.h"

#include "utils.h"
#include "avs.h"

#include <algorithm>

using std::nullopt;

optional<string> normalise_path(const string &path) {
	auto data_pos = path.find("data/");
	if (data_pos == string::npos)
		return nullopt;
	return path.substr(data_pos + strlen("data/"));
}

vector<string> available_mods() {
	vector<string> ret;
	string mod_root = MOD_FOLDER "/";
	auto mods = avs_fs_opendir(MOD_FOLDER);
	if (mods) {
		for (auto name = avs_fs_readdir(mods); name; name = avs_fs_readdir(mods)) {
			if (!strcmp(name, ".") ||
				!strcmp(name, "..") ||
				!strcmp(name, "_cache"))
				continue;
			auto subdir = mod_root + name;
			auto subdir_f = avs_fs_opendir(subdir.c_str());
			if (subdir_f > 0) {
				avs_fs_closedir(subdir_f);
				ret.push_back(subdir);
			}
		}
		avs_fs_closedir(mods);
	}
	std::sort(ret.begin(), ret.end());
	return ret;
}

bool mkdir_p(string &path) {
	/* Adapted from http://stackoverflow.com/a/2336245/119527 */
	const size_t len = strlen(path.c_str());
	char _path[MAX_PATH];
	char *p;

	errno = 0;

	/* Copy string so its mutable */
	if (len > sizeof(_path) - 1) {
		return false;
	}
	strcpy_s(_path, sizeof(_path), path.c_str());

	/* Iterate the string */
	for (p = _path + 1; *p; p++) {
		if (*p == '/') {
			/* Temporarily truncate */
			*p = '\0';

			if (!CreateDirectoryA(_path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
				return false;
			}

			*p = '/';
		}
	}

	if (!CreateDirectoryA(_path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
		return false;
	}

	return true;
}

optional<string> find_first_modfile(const string &norm_path) {
	for (auto &dir : available_mods()) {
		auto mod_path = dir + "/" + norm_path;
		if (file_exists(mod_path.c_str())) {
			return mod_path;
		}
	}
	return nullopt;
}

optional<string> find_first_modfolder(const string &norm_path) {
	for (auto &dir : available_mods()) {
		auto mod_path = dir + "/" + norm_path;
		if (folder_exists(mod_path.c_str())) {
			return mod_path;
		}
	}
	return nullopt;
}

vector<string> find_all_modfile(const string &norm_path) {
	vector<string> ret;
	for (auto &dir : available_mods()) {
		auto mod_path = dir + "/" + norm_path;
		if (file_exists(mod_path.c_str())) {
			ret.push_back(mod_path);
		}
	}
	return ret;
}