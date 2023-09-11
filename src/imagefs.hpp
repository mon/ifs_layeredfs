#include <string>
#include <optional>

void handle_texture(
    std::string const&norm_path,
    std::optional<std::string> &mod_path
);
void parse_texturelist(
    std::string const&path,
    std::string const&norm_path,
    std::optional<std::string> &mod_path
);
void merge_xmls(
    std::string const& path,
    std::string const&norm_path,
    std::optional<std::string> &mod_path);
