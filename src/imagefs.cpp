#include "imagefs.hpp"

#include <inttypes.h>
#include <map>
#include <fstream>
#include <memory>
#include <sstream>

#include "3rd_party/lodepng.h"
#include "3rd_party/stb_dxt.h"
#include "3rd_party/GuillotineBinPack.h"
#include "3rd_party/rapidxml_print.hpp"
#include "3rd_party/md5.h"

#include "avs.h"
#include "log.hpp"
#include "modpath_handler.h"
#include "texture_packer.h"
#include "utils.hpp"
#include "winxp_mutex.hpp"

using std::string;

enum img_format {
    ARGB8888REV,
    DXT5,
    UNSUPPORTED_FORMAT,
};

enum compress_type {
    NONE,
    AVSLZ,
    UNSUPPORTED_COMPRESS,
};

typedef struct image {
    string name;
    string name_md5;
    img_format format;
    compress_type compression;
    string ifs_mod_path;
    int width;
    int height;
    string cache_folder() const { return CACHE_FOLDER + "/" + ifs_mod_path;  }
    string cache_file() const { return cache_folder() + "/" + name_md5; };
} image_t;

typedef struct afp {
    string mod_path;
} afp_t;

// ifs_textures["data/graphics/ver04/logo.ifs/tex/4f754d4f424f092637a49a5527ece9bb"] will be "konami"
static std::map<string, std::shared_ptr<image_t>, CaseInsensitiveCompare> ifs_textures;
static CriticalSectionLock ifs_textures_mtx;

static std::map<std::string, std::shared_ptr<afp_t>, CaseInsensitiveCompare> afp_md5_names;
static CriticalSectionLock afp_md5_names_mtx;


void rapidxml_dump_to_file(const string& out, const rapidxml::xml_document<> &xml) {
    std::ofstream out_file;
    out_file.open(out.c_str());

    // this is 3x faster than writing directly to the output file
    std::string s;
    print(std::back_inserter(s), xml, rapidxml::print_no_indenting);
    out_file << s;

    out_file.close();
}

#define FMT_4U16(arr) "%" PRIu16 " %" PRIu16 " %" PRIu16 " %" PRIu16, (arr)[0],(arr)[1],(arr)[2],(arr)[3]
#define FMT_2U16(arr) "%" PRIu16 " %" PRIu16, (arr)[0],(arr)[1]

rapidxml::xml_node<>* allocate_node_and_attrib(
        rapidxml::xml_document<> *doc,
        const char* node_name,
        const char* node_value,
        const char* attr_name,
        const char* attr_value) {
    auto node = doc->allocate_node(rapidxml::node_element, node_name, node_value);
    node->append_attribute(doc->allocate_attribute(attr_name, attr_value));
    return node;
}

bool add_images_to_list(string_set &extra_pngs, rapidxml::xml_node<> *texturelist_node, string const&ifs_path, string const&ifs_mod_path, compress_type compress) {
    auto start = time();
    vector<Bitmap*> textures;

    for (auto it = extra_pngs.begin(); it != extra_pngs.end(); ++it) {
        log_verbose("New image: %s", it->c_str());

        string png_tex = *it + ".png";
        auto png_loc = find_first_modfile(ifs_mod_path + "/" + png_tex);
        if(!png_loc)
            png_loc = find_first_modfile(ifs_mod_path + "/tex/" + png_tex);
        if (!png_loc)
            continue;

        FILE* f = fopen(png_loc->c_str(), "rb");
        if (!f) // shouldn't happen but check anyway
            continue;

        unsigned char header[33];
        // this may read less bytes than expected but lodepng will die later anyway
        fread(header, 1, 33, f);
        fclose(f);

        unsigned width, height;
        LodePNGState state = {};
        if (lodepng_inspect(&width, &height, &state, header, 33)) {
            log_warning("couldn't inspect png");
            continue;
        }

        textures.push_back(new Bitmap(*it, width, height));
    }

    auto pack_start = time();
    vector<Packer*> packed_textures;
    if (!pack_textures(textures, packed_textures)) {
        log_warning("Couldn't pack textures :(");
        return false;
    }
    log_misc("Texture packing %d ms", time() - pack_start);

    // because the property API, being
    // a) written by Konami
    // b) not documented since lol DLLs
    // is absolutely garbage to work with for merging XMLs, we use rapidxml instead
    // thus I hope my sanity is restored.

    auto document = texturelist_node->document();
    for (unsigned int i = 0; i < packed_textures.size(); i++) {
        Packer *canvas = packed_textures[i];
        char tex_name[8];
        snprintf(tex_name, 8, "ctex%03d", i);
        auto canvas_node = document->allocate_node(rapidxml::node_element, "texture");
        texturelist_node->append_node(canvas_node);
        canvas_node->append_attribute(document->allocate_attribute("format", "argb8888rev"));
        canvas_node->append_attribute(document->allocate_attribute("mag_filter", "nearest"));
        canvas_node->append_attribute(document->allocate_attribute("min_filter", "nearest"));
        canvas_node->append_attribute(document->allocate_attribute("name", document->allocate_string(tex_name)));
        canvas_node->append_attribute(document->allocate_attribute("wrap_s", "clamp"));
        canvas_node->append_attribute(document->allocate_attribute("wrap_t", "clamp"));

        char tmp[64];

        uint16_t size[2] = { (uint16_t)canvas->width, (uint16_t)canvas->height };

        snprintf(tmp, sizeof(tmp), FMT_2U16(size));
        canvas_node->append_node(allocate_node_and_attrib(document, "size", document->allocate_string(tmp), "__type", "2u16"));

        for (unsigned int j = 0; j < canvas->bitmaps.size(); j++) {
            Bitmap *texture = canvas->bitmaps[j];
            auto tex_node = document->allocate_node(rapidxml::node_element, "image");
            canvas_node->append_node(tex_node);
            tex_node->append_attribute(document->allocate_attribute("name", document->allocate_string(texture->name.c_str())));

            uint16_t coords[4];
            coords[0] = texture->packX * 2;
            coords[1] = (texture->packX + texture->width) * 2;
            coords[2] = texture->packY * 2;
            coords[3] = (texture->packY + texture->height) * 2;
            snprintf(tmp, sizeof(tmp), FMT_4U16(coords));
            tex_node->append_node(allocate_node_and_attrib(document, "imgrect", document->allocate_string(tmp), "__type", "4u16"));
            coords[0] += 2;
            coords[1] -= 2;
            coords[2] += 2;
            coords[3] -= 2;
            snprintf(tmp, sizeof(tmp), FMT_4U16(coords));
            tex_node->append_node(allocate_node_and_attrib(document, "uvrect", document->allocate_string(tmp), "__type", "4u16"));

            image_t image_info;
            image_info.name = texture->name;
            MD5 md5;
            image_info.name_md5 = md5(texture->name);
            image_info.format = ARGB8888REV;
            image_info.compression = compress;
            image_info.ifs_mod_path = ifs_mod_path;
            image_info.width = texture->width;
            image_info.height = texture->height;

            auto md5_path = ifs_path + "/tex/" + image_info.name_md5;
            ifs_textures_mtx.lock();
            ifs_textures[md5_path] = std::make_shared<image_t>(std::move(image_info));
            ifs_textures_mtx.unlock();
        }
    }

    log_misc("Texture extend total time %d ms", time() - start);
    return true;
}

void parse_texturelist(HookFile &file) {
    bool prop_was_rewritten = false;

    // get a reasonable base path
    auto ifs_path = file.norm_path;
    // truncate
    ifs_path.resize(ifs_path.size() - strlen("/tex/texturelist.xml"));
    // log_misc("Reading ifs %s", ifs_path.c_str());
    auto ifs_mod_path = ifs_path;
    string_replace(ifs_mod_path, ".ifs", "_ifs");

    if (!find_first_modfolder(ifs_mod_path)) {
        log_verbose("mod folder doesn't exist, skipping");
        return;
    }

    // open the correct file
    auto path_to_open = file.get_path_to_open();
    rapidxml::xml_document<> texturelist;
    auto success = rapidxml_from_avs_filepath(path_to_open, texturelist, texturelist);
    if (!success)
        return;

    auto texturelist_node = texturelist.first_node("texturelist");

    if (!texturelist_node) {
        log_warning("texlist has no texturelist node");
        return;
    }

    auto extra_pngs = list_pngs(ifs_mod_path);

    auto compress = NONE;
    rapidxml::xml_attribute<> *compress_node;
    if ((compress_node = texturelist_node->first_attribute("compress"))) {
        if (!_stricmp(compress_node->value(), "avslz")) {
            compress = AVSLZ;
        }
        else {
            compress = UNSUPPORTED_COMPRESS;
        }
    }

    for(auto texture = texturelist_node->first_node("texture");
            texture;
            texture = texture->next_sibling("texture")) {

        auto format = texture->first_attribute("format");
        if (!format) {
            log_warning("Texture missing format %s", path_to_open.c_str());
            continue;
        }

        //<size __type="2u16">128 128</size>
        auto size = texture->first_node("size");
        if (!size) {
            log_warning("Texture missing size %s", path_to_open.c_str());
            continue;
        }

        auto format_type = UNSUPPORTED_FORMAT;
        if (!_stricmp(format->value(), "argb8888rev")) {
            format_type = ARGB8888REV;
        } else if (!_stricmp(format->value(), "dxt5")) {
            format_type = DXT5;
        }

        for(auto image = texture->first_node("image");
            image;
            image = image->next_sibling("image")) {
            auto name = image->first_attribute("name");
            if (!name) {
                log_warning("Texture missing name %s", path_to_open.c_str());
                continue;
            }

            uint16_t dimensions[4];
            auto imgrect = image->first_node("imgrect");
            auto uvrect = image->first_node("uvrect");
            if (!imgrect || !uvrect) {
                log_warning("Texture missing dimensions %s", path_to_open.c_str());
                continue;
            }

            // it's a 4u16
            sscanf(imgrect->value(), "%" SCNu16 " %" SCNu16 " %" SCNu16 " %" SCNu16, &dimensions[0], &dimensions[1], &dimensions[2], &dimensions[3]);

            // log_misc("Image '%s' compress %d format %d", name->value(), compress, format_type);
            image_t image_info;
            image_info.name = name->value();
            MD5 md5;
            image_info.name_md5 = md5(name->value());
            image_info.format = format_type;
            image_info.compression = compress;
            image_info.ifs_mod_path = ifs_mod_path;
            image_info.width = (dimensions[1] - dimensions[0]) / 2;
            image_info.height = (dimensions[3] - dimensions[2]) / 2;

            extra_pngs.erase(image_info.name);

            auto md5_path = ifs_path + "/tex/" + image_info.name_md5;
            ifs_textures_mtx.lock();
            ifs_textures[md5_path] = std::make_shared<image_t>(std::move(image_info));
            ifs_textures_mtx.unlock();
        }
    }

    log_verbose("%d added PNGs", extra_pngs.size());
    if (extra_pngs.size() > 0) {
        if (add_images_to_list(extra_pngs, texturelist_node, ifs_path, ifs_mod_path, compress))
            prop_was_rewritten = true;
    }

    if (prop_was_rewritten) {
        string outfolder = CACHE_FOLDER + "/" + ifs_mod_path;
        if (!mkdir_p(outfolder)) {
            log_warning("Couldn't create cache folder");
        }
        string outfile = outfolder + "/texturelist.xml";
        rapidxml_dump_to_file(outfile, texturelist);
        file.mod_path = outfile;
    }
}

bool cache_texture(string const&png_path, image_t const&tex) {
    string cache_path = tex.cache_folder();
    if (!mkdir_p(cache_path)) {
        log_warning("Couldn't create texture cache folder");
        return false;
    }

    string cache_file = tex.cache_file();
    auto cache_time = file_time(cache_file.c_str());
    auto png_time = file_time(png_path.c_str());

    // the cache is fresh, don't do the same work twice
#ifndef ALWAYS_CACHE
    if (cache_time > 0 && cache_time >= dll_time && cache_time >= png_time) {
        return true;
    }
#endif

    // make the cache
    FILE *cache;

    unsigned error;
    unsigned char* image;
    unsigned width, height; // TODO use these to check against xml

    error = lodepng_decode32_file(&image, &width, &height, png_path.c_str());
    if (error) {
        log_warning("can't load png %u: %s\n", error, lodepng_error_text(error));
        return false;
    }

    if ((int)width != tex.width || (int)height != tex.height) {
        log_warning("Loaded png (%dx%d) doesn't match texturelist.xml (%dx%d), ignoring", width, height, tex.width, tex.height);
        return false;
    }

    size_t image_size = 4 * width * height;

    switch (tex.format) {
    case ARGB8888REV:
        for (size_t i = 0; i < image_size; i += 4) {
            // swap r and b
            auto tmp = image[i];
            image[i] = image[i + 2];
            image[i + 2] = tmp;
        }
        break;
    case DXT5: {
        size_t dxt5_size = image_size / 4;
        unsigned char* dxt5_image = (unsigned char*)malloc(dxt5_size);
        rygCompress(dxt5_image, image, width, height, 1);
        free(image);
        image = dxt5_image;
        image_size = dxt5_size;

        // the data has swapped endianness for every WORD
        for (size_t i = 0; i < image_size; i += 2) {
            auto tmp = image[i];
            image[i] = image[i + 1];
            image[i + 1] = tmp;
        }

        /*FILE* f = fopen("dxt_debug.bin", "wb");
        fwrite(dxt5_image, 1, dxt5_size, f);
        fclose(f);*/
        break;
    }
    default:
        break;
    }
    auto uncompressed_size = image_size;

    if (tex.compression == AVSLZ) {
        size_t compressed_size;
        auto compressed = lz_compress(image, image_size, &compressed_size);
        free(image);
        if (compressed == NULL) {
            log_warning("Couldn't compress");
            return false;
        }
        image = compressed;
        image_size = compressed_size;
    }

    cache = fopen(cache_file.c_str(), "wb");
    if (!cache) {
        log_warning("can't open cache for writing");
        return false;
    }
    if (tex.compression == AVSLZ) {
        uint32_t uncomp_sz = _byteswap_ulong((uint32_t)uncompressed_size);
        uint32_t comp_sz = _byteswap_ulong((uint32_t)image_size);
        fwrite(&uncomp_sz, 4, 1, cache);
        fwrite(&comp_sz, 4, 1, cache);
    }
    fwrite(image, 1, image_size, cache);
    fclose(cache);
    free(image);
    return true;
}

void parse_afplist(HookFile &file) {
    // get a reasonable base path
    auto ifs_path = file.norm_path;
    // truncate
    ifs_path.resize(ifs_path.size() - strlen("/tex/afplist.xml"));
    // log_misc("Reading ifs %s", ifs_path.c_str());
    auto ifs_mod_path = ifs_path;
    string_replace(ifs_mod_path, ".ifs", "_ifs");

    if (!find_first_modfolder(ifs_mod_path)) {
        // hide this print - the texturelist.xml will catch it
        // log_verbose("mod folder doesn't exist, skipping");
        return;
    }

    // open the correct file
    auto path_to_open = file.get_path_to_open();
    rapidxml::xml_document<> afplist;
    auto success = rapidxml_from_avs_filepath(path_to_open, afplist, afplist);
    if (!success)
        return;

    auto afplist_node = afplist.first_node("afplist");

    if (!afplist_node) {
        log_warning("afplist has no afplist node");
        return;
    }

    int mapped = 0;

    for(auto afp = afplist_node->first_node("afp");
            afp;
            afp = afp->next_sibling("afp")) {

        auto name = afp->first_attribute("name");
        if (!name) {
            log_warning("AFP missing name %s", path_to_open.c_str());
            continue;
        }

        // <geo __type="u16" __count="5">5 8 11 16 19</geo>
        auto geo = afp->first_node("geo");
        if (!geo) {
            log_warning("AFP missing geo %s", path_to_open.c_str());
            continue;
        }

        auto add_mapping = [&](std::string folder, std::string file) {
            auto md5_path = ifs_path + folder + MD5()(file);
            afp_md5_names[md5_path] = std::make_shared<afp_t>(afp_t {
                .mod_path = ifs_mod_path + folder + file,
            });
            mapped++;
            // log_info("AFP %s -> %s", md5_path.c_str(), (ifs_mod_path + folder + file).c_str());
        };

        afp_md5_names_mtx.lock();

        add_mapping("/afp/", name->value());
        add_mapping("/afp/bsi/", name->value());

        // iterate geos
        std::string index;
        std::stringstream ss(geo->value());
        while(ss >> index) {
            add_mapping("/geo/", std::string(name->value()) + "_shape" + index);
        }

        afp_md5_names_mtx.unlock();
    }

    log_misc("Mapped %d AFP filenames", mapped);
}

std::optional<std::tuple<std::string, std::shared_ptr<image_t>>> lookup_png_from_md5(HookFile &file) {
    ifs_textures_mtx.lock();
    auto tex_search = ifs_textures.find(file.norm_path);
    if (tex_search == ifs_textures.end()) {
        ifs_textures_mtx.unlock();
        return std::nullopt;
    }

    //log_misc("Mapped file %s is found!", norm_path.c_str());
    auto tex = tex_search->second;
    ifs_textures_mtx.unlock(); // is it safe to unlock this early? Time will tell...

    // remove the /tex/, it's nicer to navigate
    auto png_path = find_first_modfile(tex->ifs_mod_path + "/" + tex->name + ".png");
    if (!png_path) {
        // but maybe they used it anyway
        png_path = find_first_modfile(tex->ifs_mod_path + "/tex/" + tex->name + ".png");
        if (!png_path)
            return std::nullopt;
    }

    return std::make_tuple(*png_path, tex);
}

void handle_texture(HookFile &file) {
    auto lookup = lookup_png_from_md5(file);
    if(!lookup)
        return;

    auto &[png_path, tex] = *lookup;

    if (tex->compression == UNSUPPORTED_COMPRESS) {
        log_warning("Unsupported compression for %s", png_path.c_str());
        return;
    }
    if (tex->format == UNSUPPORTED_FORMAT) {
        log_warning("Unsupported texture format for %s", png_path.c_str());
        return;
    }

    log_verbose("Mapped file %s found!", png_path.c_str());
    if (cache_texture(png_path, *tex)) {
        file.mod_path = tex->cache_file();
    }
    return;
}

std::optional<std::string> lookup_afp_from_md5(HookFile &file) {
    afp_md5_names_mtx.lock();
    auto afp_search = afp_md5_names.find(file.norm_path);
    if (afp_search == afp_md5_names.end()) {
        afp_md5_names_mtx.unlock();
        return std::nullopt;
    }

    //log_misc("Mapped file %s is found!", norm_path.c_str());
    auto afp = afp_search->second;
    afp_md5_names_mtx.unlock(); // is it safe to unlock this early? Time will tell...

    return find_first_modfile(afp->mod_path);
}

void handle_afp(HookFile &file) {
    auto lookup = lookup_afp_from_md5(file);
    if(!lookup)
        return;

    log_verbose("Mapped file %s found!", lookup->c_str());
    file.mod_path = *lookup;
    return;
}

void merge_xmls(HookFile &file) {
    auto start = time();
    // initialize since we're GOTO-ing like naughty people
    string out;
    string out_folder;
    rapidxml::xml_document<> merged_xml;

    auto merge_path = file.norm_path;
    string_replace(merge_path, ".xml", ".merged.xml");
    auto to_merge = find_all_modfile(merge_path);
    if (to_merge.size() == 0) {
        // handle merging XML inside .ifs
        string_replace(merge_path, ".ifs", "_ifs");
        to_merge = find_all_modfile(merge_path);

        // nothing to do...
        if (to_merge.size() == 0)
            return;
    }

    auto starting = file.get_path_to_open();
    out = CACHE_FOLDER + "/" + file.norm_path;
    auto out_hashed = out + ".hashed";
    auto cache_hasher = CacheHasher(out_hashed);

    cache_hasher.add(starting); // don't forget to take the input into account
    log_info("Merging into %s", starting.c_str());
    for (auto &path : to_merge) {
        log_info("  %s", path.c_str());
        cache_hasher.add(path);
    }
    cache_hasher.finish();

    // no need to merge - timestamps all up to date, dll not newer, files haven't been deleted
    if(cache_hasher.matches()) {
        log_info("Merged XML up to date");
        file.mod_path = out;
        return;
    }

    auto first_result = rapidxml_from_avs_filepath(starting, merged_xml, merged_xml);
    if (!first_result) {
        log_warning("Couldn't merge (can't load first xml %s)", starting.c_str());
        return;
    }

    for (auto &path : to_merge) {
        rapidxml::xml_document<> rapid_to_merge;
        auto merge_load_result = rapidxml_from_avs_filepath(path, rapid_to_merge, merged_xml);
        if (!merge_load_result) {
            log_warning("Couldn't merge (can't load xml) %s", path.c_str());
            return;
        }

        // toplevel nodes include doc declaration and mdb node
        // getting the last node grabs the mdb node
        // document -> mdb entry -> music entry
        for (rapidxml::xml_node<> *node = rapid_to_merge.last_node()->first_node(); node; node = node->next_sibling()) {
            merged_xml.last_node()->append_node(merged_xml.clone_node(node));
        }
    }

    auto folder_terminator = out.rfind("/");
    out_folder = out.substr(0, folder_terminator);
    if (!mkdir_p(out_folder)) {
        log_warning("Couldn't create merged cache folder");
    }

    rapidxml_dump_to_file(out, merged_xml);
    cache_hasher.commit();
    file.mod_path = out;

    log_misc("Merge took %d ms", time() - start);
}
