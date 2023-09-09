#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <inttypes.h>

#include "hook.h"

// all good code mixes C and C++, right?
using std::string;
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>

#include "3rd_party/MinHook.h"

#include "3rd_party/lodepng.h"
#include "3rd_party/stb_dxt.h"
#include "3rd_party/GuillotineBinPack.h"
#include "3rd_party/rapidxml_print.hpp"

#include "ramfs_demangler.h"
#include "config.hpp"
#include "log.hpp"
#include "texbin.hpp"
#include "utils.hpp"
#include "avs.h"
//#include "jubeat.h"
#include "texture_packer.h"
#include "modpath_handler.h"
#include "winxp_mutex.hpp"

// let me use the std:: version, damnit
#undef max
#undef min

#ifdef _DEBUG
#define DBG_VER_STRING "_DEBUG"
#else
#define DBG_VER_STRING
#endif

#define VERSION VER_STRING DBG_VER_STRING

// debugging
//#define ALWAYS_CACHE

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
    const string cache_folder() { return CACHE_FOLDER "/" + ifs_mod_path;  }
    const string cache_file() { return cache_folder() + "/" + name_md5; };
} image_t;

// ifs_textures["data/graphics/ver04/logo.ifs/tex/4f754d4f424f092637a49a5527ece9bb"] will be "konami"
static std::unordered_map<string, image_t> ifs_textures;
static CriticalSectionLock ifs_textures_mtx;

typedef std::unordered_set<string> string_set;

void rapidxml_dump_to_file(const string& out, const rapidxml::xml_document<> &xml) {
    std::ofstream out_file;
    out_file.open(out.c_str());

    // this is 3x faster than writing directly to the output file
    std::string s;
    print(std::back_inserter(s), xml, rapidxml::print_no_indenting);
    out_file << s;

    out_file.close();
}

void list_pngs_onefolder(string_set &names, string const& folder) {
    auto search_path = folder + "/*.png";
    const auto extension_len = strlen(".png");
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // read all (real) files in current folder
            // , delete '!' read other 2 default folder . and ..
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                fd.cFileName[strlen(fd.cFileName) - extension_len] = '\0';
                names.insert(fd.cFileName);
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }
}

string_set list_pngs(string const&folder) {
    string_set ret;

    for (auto &mod : available_mods()) {
        auto path = mod + "/" + folder;
        list_pngs_onefolder(ret, path);
        list_pngs_onefolder(ret, path + "/tex");
    }

    return ret;
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
            image_info.name_md5 = md5_sum(texture->name.c_str());
            image_info.format = ARGB8888REV;
            image_info.compression = compress;
            image_info.ifs_mod_path = ifs_mod_path;
            image_info.width = texture->width;
            image_info.height = texture->height;

            auto md5_path = ifs_path + "/tex/" + image_info.name_md5;
            ifs_textures_mtx.lock();
            ifs_textures[md5_path] = image_info;
            ifs_textures_mtx.unlock();
        }
    }

    log_misc("Texture extend total time %d ms", time() - start);
    return true;
}

void parse_texturelist(string const&path, string const&norm_path, optional<string> &mod_path) {
    bool prop_was_rewritten = false;

    // get a reasonable base path
    auto ifs_path = norm_path;
    // truncate
    ifs_path.resize(ifs_path.size() - strlen("/tex/texturelist.xml"));
    //log_misc("Reading ifs %s", ifs_path.c_str());
    auto ifs_mod_path = ifs_path;
    string_replace(ifs_mod_path, ".ifs", "_ifs");

    if (!find_first_modfolder(ifs_mod_path)) {
        log_verbose("mod folder doesn't exist, skipping");
        return;
    }

    // open the correct file
    auto path_to_open = mod_path ? *mod_path : path;
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

            //log_misc("Image '%s' compress %d format %d", tmp, compress, format_type);
            image_t image_info;
            image_info.name = name->value();
            image_info.name_md5 = md5_sum(name->value());
            image_info.format = format_type;
            image_info.compression = compress;
            image_info.ifs_mod_path = ifs_mod_path;
            image_info.width = (dimensions[1] - dimensions[0]) / 2;
            image_info.height = (dimensions[3] - dimensions[2]) / 2;

            auto md5_path = ifs_path + "/tex/" + image_info.name_md5;
            ifs_textures_mtx.lock();
            ifs_textures[md5_path] = image_info;
            ifs_textures_mtx.unlock();

            extra_pngs.erase(image_info.name);
        }
    }

    log_verbose("%d added PNGs", extra_pngs.size());
    if (extra_pngs.size() > 0) {
        if (add_images_to_list(extra_pngs, texturelist_node, ifs_path, ifs_mod_path, compress))
            prop_was_rewritten = true;
    }

    if (prop_was_rewritten) {
        string outfolder = CACHE_FOLDER "/" + ifs_mod_path;
        if (!mkdir_p(outfolder)) {
            log_warning("Couldn't create cache folder");
        }
        string outfile = outfolder + "/texturelist.xml";
        rapidxml_dump_to_file(outfile, texturelist);
        mod_path = outfile;
    }
}

bool cache_texture(string const&png_path, image_t &tex) {
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

void handle_texture(string const&norm_path, optional<string> &mod_path) {
    ifs_textures_mtx.lock();
    auto tex_search = ifs_textures.find(norm_path);
    if (tex_search == ifs_textures.end()) {
        ifs_textures_mtx.unlock();
        return;
    }

    //log_misc("Mapped file %s is found!", norm_path.c_str());
    auto tex = tex_search->second;
    ifs_textures_mtx.unlock(); // is it safe to unlock this early? Time will tell...

    // remove the /tex/, it's nicer to navigate
    auto png_path = find_first_modfile(tex.ifs_mod_path + "/" + tex.name + ".png");
    if (!png_path) {
        // but maybe they used it anyway
        png_path = find_first_modfile(tex.ifs_mod_path + "/tex/" + tex.name + ".png");
        if (!png_path)
            return;
    }

    if (tex.compression == UNSUPPORTED_COMPRESS) {
        log_warning("Unsupported compression for %s", png_path->c_str());
        return;
    }
    if (tex.format == UNSUPPORTED_FORMAT) {
        log_warning("Unsupported texture format for %s", png_path->c_str());
        return;
    }

    log_verbose("Mapped file %s found!", png_path->c_str());
    if (cache_texture(*png_path, tex)) {
        mod_path = tex.cache_file();
    }
    return;
}

void hash_filenames(vector<string> &filenames, uint8_t hash[MD5_LEN]) {
    auto digest = mdigest_create(MD5);

    for (auto &path : filenames) {
        mdigest_update(digest, path.c_str(), (int)path.length());
    }

    mdigest_finish(digest, hash, MD5_LEN);
    mdigest_destroy(digest);
}

void merge_xmls(string const& path, string const&norm_path, optional<string> &mod_path) {
    auto start = time();
    // initialize since we're GOTO-ing like naughty people
    string out;
    string out_folder;
    rapidxml::xml_document<> merged_xml;

    auto merge_path = norm_path;
    string_replace(merge_path, ".xml", ".merged.xml");
    auto to_merge = find_all_modfile(merge_path);
    // nothing to do...
    if (to_merge.size() == 0)
        return;

    auto starting = mod_path ? *mod_path : path;
    out = CACHE_FOLDER "/" + norm_path;
    auto out_hashed = out + ".hashed";

    uint8_t hash[MD5_LEN];
    hash_filenames(to_merge, hash);

    uint8_t cache_hash[MD5_LEN] = {0};
    FILE* cache_hashfile = fopen(out_hashed.c_str(), "rb");
    if (cache_hashfile) {
        fread(cache_hash, 1, sizeof(cache_hash), cache_hashfile);
        fclose(cache_hashfile);
    }

    auto time_out = file_time(out.c_str());
    // don't forget to take the input into account
    auto newest = file_time(starting.c_str());
    for (auto &path : to_merge)
        newest = std::max(newest, file_time(path.c_str()));
    // no need to merge - timestamps all up to date, dll not newer, files haven't been deleted
    if(time_out >= newest && time_out >= dll_time && memcmp(hash, cache_hash, sizeof(hash)) == 0) {
        mod_path = out;
        return;
    }

    auto first_result = rapidxml_from_avs_filepath(starting, merged_xml, merged_xml);
    if (!first_result) {
        log_warning("Couldn't merge (can't load first xml %s)", starting.c_str());
        return;
    }

    log_info("Merging into %s", starting.c_str());
    for (auto &path : to_merge) {
        log_info("  %s", path.c_str());
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
    cache_hashfile = fopen(out_hashed.c_str(), "wb");
    if (cache_hashfile) {
        fwrite(hash, 1, sizeof(hash), cache_hashfile);
        fclose(cache_hashfile);
    }
    mod_path = out;

    log_misc("Merge took %d ms", time() - start);
}

void handle_texbin(string const& path, string const&norm_path, optional<string> &mod_path) {
    auto start = time();

    auto bin_orig_path = mod_path ? *mod_path : path; // may have layered pre-built mod .bin with extra PNGs
    auto bin_mod_path = norm_path;
    // mod texbins strip the .bin off the end. This isn't consistent with the _ifs
    // used for ifs files, but it's consistent with gitadora-texbintool, the *only*
    // tool to extract .bin files currently.
    string_replace(bin_mod_path, ".bin", "");

    if (!find_first_modfolder(bin_mod_path)) {
        // log_verbose("texbin: mod folder doesn't exist, skipping");
        return;
    }

    auto pngs = list_pngs(bin_mod_path);

    //// This whole hashing section is just a tiny bit different from the XML
    //// hashing :(

    // convert the string_set to a vec for repeatable hashes
    vector<string> pngs_list;
    pngs_list.reserve(pngs.size());
    for (auto it = pngs.begin(); it != pngs.end(); ) {
        auto png = std::move(pngs.extract(it++).value());
        // todo: hacky as hell, list_pngs should do better
        png = png + ".png";
        str_tolower_inline(png);
        auto png_res = find_first_modfile(bin_mod_path + "/" + png);
        if(png_res) {
            pngs_list.push_back(*png_res);
        }
    }
    // sort
    std::sort(pngs_list.begin(), pngs_list.end());

    // nothing to do...
    if (pngs_list.size() == 0) {
        log_verbose("texbin: mod folder has no PNGs, skipping");
        return;
    }

    string out = CACHE_FOLDER "/" + norm_path;
    auto out_hashed = out + ".hashed";

    uint8_t hash[MD5_LEN];
    hash_filenames(pngs_list, hash);

    uint8_t cache_hash[MD5_LEN] = {0};
    FILE* cache_hashfile;
    cache_hashfile = fopen(out_hashed.c_str(), "rb");
    if (cache_hashfile) {
        fread(cache_hash, 1, sizeof(cache_hash), cache_hashfile);
        fclose(cache_hashfile);
    }

    auto time_out = file_time(out.c_str());
    auto newest = file_time(bin_orig_path.c_str());
    for (auto &path : pngs_list)
        newest = std::max(newest, file_time(path.c_str()));
    // no need to merge - timestamps all up to date, dll not newer, files haven't been deleted
    if(time_out >= newest && time_out >= dll_time && memcmp(hash, cache_hash, sizeof(hash)) == 0) {
        mod_path = out;
        log_misc("texbin cache up to date, skip");
        return;
    }
    // log_verbose("Regenerating cache");
    // log_verbose("  time_out >= newest == %d", time_out >= newest);
    // log_verbose("  time_out >= dll_time == %d", time_out >= dll_time);
    // log_verbose("  memcmp(hash, cache_hash, sizeof(hash)) == 0 == %d", memcmp(hash, cache_hash, sizeof(hash)) == 0);

    Texbin texbin;
    AVS_FILE f = avs_fs_open(bin_orig_path.c_str(), 1, 420);
    if (f >= 0) {
        auto orig_data = avs_file_to_vec(f);
        avs_fs_close(f);

        // one extra copy which *sucks* but whatever
        std::istringstream stream(string((char*)&orig_data[0], orig_data.size()));
        auto _texbin = Texbin::from_stream(stream);
        if(!_texbin) {
            log_warning("Texbin load failed, aborting modding");
            return;
        }
        texbin = *_texbin;
    } else {
        log_info("Found texbin mods but no original file, creating from scratch: \"%s\"", norm_path.c_str());
    }

    auto folder_terminator = out.rfind("/");
    auto out_folder = out.substr(0, folder_terminator);
    if (!mkdir_p(out_folder)) {
        log_warning("Texbin: Couldn't create output cache folder");
        return;
    }

    for (auto &path : pngs_list) {
        // I have yet to see a texbin with allcaps names for textures
        auto tex_name = basename_without_extension(path);
        str_toupper_inline(tex_name);
        texbin.add_or_replace_image(tex_name.c_str(), path.c_str());
    }

    if(!texbin.save(out.c_str())) {
        log_warning("Texbin: Couldn't create output");
        return;
    }

    cache_hashfile = fopen(out_hashed.c_str(), "wb");
    if (cache_hashfile) {
        fwrite(hash, 1, sizeof(hash), cache_hashfile);
        fclose(cache_hashfile);
    }
    mod_path = out;

    log_misc("Texbin generation took %d ms", time() - start);
}

int hook_avs_fs_lstat(const char* name, struct avs_stat *st) {
    if (name == NULL)
        return avs_fs_lstat(name, st);

    log_verbose("statting %s", name);
    string path = name;

    // can it be modded?
    auto norm_path = normalise_path(path);
    if(!norm_path)
        return avs_fs_lstat(name, st);

    auto mod_path = find_first_modfile(*norm_path);

    if (mod_path) {
        log_verbose("Overwriting lstat");
        return avs_fs_lstat(mod_path->c_str(), st);
    }
    // called prior to avs_fs_open
    if(string_ends_with(path.c_str(), ".bin")) {
        handle_texbin(name, *norm_path, mod_path);
        if(mod_path) {
            log_verbose("Overwriting texbin lstat");
            return avs_fs_lstat(mod_path->c_str(), st);
        }
    }
    return avs_fs_lstat(name, st);
}

int hook_avs_fs_convert_path(char dest_name[256], const char *name) {
    if (name == NULL)
        return avs_fs_convert_path(dest_name, name);

    log_verbose("convert_path %s", name);
    string path = name;

    // can it be modded?
    auto norm_path = normalise_path(path);
    if (!norm_path)
        return avs_fs_convert_path(dest_name, name);

    auto mod_path = find_first_modfile(*norm_path);

    if (mod_path) {
        log_verbose("Overwriting convert_path");
        return avs_fs_convert_path(dest_name, mod_path->c_str());
    }
    return avs_fs_convert_path(dest_name, name);
}

int hook_avs_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* args) {
    log_verbose("mounting %s to %s with type %s and args %s", fsroot, mountpoint, fstype, args);
    ramfs_demangler_on_fs_mount(mountpoint, fsroot, fstype, args);

    return avs_fs_mount(mountpoint, fsroot, fstype, args);
}

size_t hook_avs_fs_read(AVS_FILE context, void* bytes, size_t nbytes) {
    ramfs_demangler_on_fs_read(context, bytes);
    return avs_fs_read(context, bytes, nbytes);
}

AVS_FILE hook_avs_fs_open(const char* name, uint16_t mode, int flags) {
    if(name == NULL)
        return avs_fs_open(name, mode, flags);
    log_verbose("opening %s mode %d flags %d", name, mode, flags);
    // only touch reads - new AVS has bitflags (R=1,W=2), old AVS has enum (R=0,W=1,RW=2)
    if ((avs_loaded_version >= 1400 && mode != 1) || (avs_loaded_version < 1400 && mode != 0)) {
        return avs_fs_open(name, mode, flags);
    }
    string path = name;
    string orig_path = name;

    // can it be modded ie is it under /data ?
    auto _norm_path = normalise_path(path);
    if (!_norm_path)
        return avs_fs_open(name, mode, flags);
    // unpack success
    auto norm_path = *_norm_path;

    auto mod_path = find_first_modfile(norm_path);
    // mod ifs paths use _ifs, go one at a time for ifs-inside-ifs
    while (!mod_path && string_replace_first(norm_path, ".ifs", "_ifs")) {
        mod_path = find_first_modfile(norm_path);
    }

    if(string_ends_with(path.c_str(), ".xml")) {
        merge_xmls(orig_path, norm_path, mod_path);
    }

    if(string_ends_with(path.c_str(), ".bin")) {
        handle_texbin(orig_path, norm_path, mod_path);
    }

    if (string_ends_with(path.c_str(), "texturelist.xml")) {
        parse_texturelist(orig_path, norm_path, mod_path);
    }
    else {
        handle_texture(norm_path, mod_path);
    }

    if (mod_path) {
        log_info("Using %s", mod_path->c_str());
    }

    auto to_open = mod_path ? *mod_path : orig_path;
    auto ret = avs_fs_open(to_open.c_str(), mode, flags);
    ramfs_demangler_on_fs_open(to_open, ret);
    // log_verbose("(returned %d)", ret);
    return ret;
}

extern "C" {
    __declspec(dllexport) int init(void) {
        // all logs up until init_avs succeeds will go to a file for debugging purposes

        // find out where we're logging to
        load_config();

        if (MH_Initialize() != MH_OK) {
            log_fatal("Couldn't initialize MinHook");
            return 1;
        }

        if (!init_avs()) {
            log_fatal("Couldn't find ifs operations in dll. Send avs dll (libavs-winxx.dll or avs2-core.dll) to mon.");
            return 2;
        }

        // re-route to AVS logs if no external file specified
        if(!config.logfile) {
            imp_log_body_fatal = log_body_fatal;
            imp_log_body_warning = log_body_warning;
            imp_log_body_info = log_body_info;
            imp_log_body_misc = log_body_misc;
        }

        // now we can say hello!
        log_info("IFS layeredFS v" VERSION " init");
        log_info("AVS DLL detected: %s", avs_loaded_dll_name);
        print_config();

        cache_mods();

        //jb_texhook_init();

        if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
            log_warning("Couldn't enable hooks");
            return 2;
        }
        log_info("Hook DLL init success");

        log_info("Detected mod folders:");
        for (auto &p : available_mods()) {
            log_info("%s", p.c_str());
        }

        return 0;
    }
}
