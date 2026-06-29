#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "utils.hpp"

// Regardless of how many prefixes, extraneous slashes, back/forward
// slashes, the normalised path is the canonical game-folder-relative path
// used to search for mods eg:
//   graphics/ver03/cmn_sys.ifs
//   data2/graphics/whatever.ifs
//
// newtype so we actually know when we expect a normalised path, and so a modded
// path isn't accidentally re-normalised (because that is invalid)
class NormPath : public istring {
  public:
    NormPath() = default;
    // want the conversion step to be very deliberate
    explicit NormPath(istring&& s)
        : istring(std::move(s)) {}
    // but for tests, which are the only place to use string literals, it's fine
    NormPath(const char* s)
        : istring(s) {}

    NormPath operator/(const char* component) const {
        return NormPath(istring::operator/(component));
    }
    NormPath operator/(std::string_view component) const {
        return NormPath(istring::operator/(component));
    }
    NormPath operator/(istring_view component) const {
        return NormPath(istring::operator/(component));
    }
};

void init_modpath_handler();
void modpath_debug_add_folder(std::string_view folder);
void cache_mods();
std::vector<istring> available_mods();
std::optional<NormPath> normalise_path(std::string path, bool demangle = true);
// norm_path is used as a lookup key so can't be a view :(
std::optional<istring> find_first_modfile(const NormPath& norm_path);
std::optional<istring> find_first_modfolder(const NormPath& norm_path);
std::vector<istring> find_all_modfile(const NormPath& norm_path);
