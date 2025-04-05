#include <memory>
#include <optional>
#include <tuple>

#include "hook.h"

void handle_texture(HookFile &file);
void handle_afp(HookFile &file);
void parse_texturelist(HookFile &file);
void parse_afplist(HookFile &file);
void merge_xmls(HookFile &file);
// only exported to test the MD5 lookup machinery
struct image;
std::optional<std::tuple<std::string, std::shared_ptr<struct image>>> lookup_png_from_md5(HookFile &file);
std::optional<std::string> lookup_afp_from_md5(HookFile &file);
