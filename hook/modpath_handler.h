#pragma once

#include <string>
#include <optional>
#include <vector>

using std::string;
using std::optional;
using std::vector;

#define MOD_FOLDER "./data_mods"
#define CACHE_FOLDER MOD_FOLDER "/_cache"

vector<string> available_mods();
optional<string> normalise_path(const string &path);
optional<string> find_first_modfile(const string &norm_path);
optional<string> find_first_modfolder(const string &norm_path);
vector<string> find_all_modfile(const string &norm_path);
bool mkdir_p(string &path);