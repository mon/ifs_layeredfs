#pragma once

#include <string>
#include <vector>
#include <optional>

void init_modpath_handler(void);
void modpath_debug_add_folder(const std::string &folder);
void cache_mods(void);
std::vector<std::string> available_mods();
// mutates source std::string to be all lowercase
std::optional<std::string> normalise_path(const std::string &path, bool demangle = true);
std::optional<std::string> find_first_modfile(const std::string &norm_path);
std::optional<std::string> find_first_modfolder(const std::string &norm_path);
std::vector<std::string> find_all_modfile(const std::string &norm_path);
bool mkdir_p(const std::string &path);
