#pragma once

#include <string>
#include <vector>

#if 0
#include <experimental/optional>
using std::experimental::optional;
#else
#include <optional>
using std::optional;
#endif

using std::string;
using std::vector;

void init_modpath_handler(void);
void cache_mods(void);
vector<string> available_mods();
// mutates source string to be all lowercase
optional<string> normalise_path(const string &path);
optional<string> find_first_modfile(const string &norm_path);
optional<string> find_first_modfolder(const string &norm_path);
vector<string> find_all_modfile(const string &norm_path);
bool mkdir_p(const string &path);
