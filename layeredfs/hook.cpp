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
#include <fstream>

#include "3rd_party/MinHook.h"
#pragma comment(lib, "minhook.lib")

#include "3rd_party/lodepng.h"
#include "3rd_party/stb_dxt.h"
#include "3rd_party/GuillotineBinPack.h"
#include "3rd_party/rapidxml_print.hpp"

#include "ramfs_demangler.h"
#include "config.hpp"
#include "utils.h"
#include "avs.h"
//#include "jubeat.h"
#include "texture_packer.h"
#include "modpath_handler.h"
#include "winxp_mutex.hpp"

// let me use the std:: version, damnit
#undef max
#undef min

#define VER_STRING "2.2"

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
        logf_verbose("New image: %s", it->c_str());

        string png_tex = *it + ".png";
        auto png_loc = find_first_modfile(ifs_mod_path + "/" + png_tex);
        if(!png_loc)
            png_loc = find_first_modfile(ifs_mod_path + "/tex/" + png_tex);
        if (!png_loc)
            continue;

        FILE* f;
        fopen_s(&f, png_loc->c_str(), "rb");
        if (!f) // shouldn't happen but check anyway
            continue;

        unsigned char header[33];
        // this may read less bytes than expected but lodepng will die later anyway
        fread(header, 1, 33, f);
        fclose(f);

        unsigned width, height;
        LodePNGState state = {};
        if (lodepng_inspect(&width, &height, &state, header, 33)) {
            logf("couldn't inspect png");
            continue;
        }

        textures.push_back(new Bitmap(*it, width, height));
    }

    auto pack_start = time();
    vector<Packer*> packed_textures;
    if (!pack_textures(textures, packed_textures)) {
        logf("Couldn't pack textures :(");
        return false;
    }
    logf("Texture packing %d ms", time() - pack_start);

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

    logf("Texture extend total time %d ms", time() - start);
    return true;
}

void parse_texturelist(string const&path, string const&norm_path, optional<string> &mod_path) {
    bool prop_was_rewritten = false;

    // get a reasonable base path
    auto ifs_path = norm_path;
    // truncate
    ifs_path.resize(ifs_path.size() - strlen("/tex/texturelist.xml"));
    //logf("Reading ifs %s", ifs_path.c_str());
    auto ifs_mod_path = ifs_path;
    string_replace(ifs_mod_path, ".ifs", "_ifs");

    if (!find_first_modfolder(ifs_mod_path)) {
        logf_verbose("mod folder doesn't exist, skipping");
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
        logf("texlist has no texturelist node");
        return;
    }

    auto extra_pngs = list_pngs(ifs_mod_path);

    auto compress = NONE;
    rapidxml::xml_attribute<> *compress_node;
    if (compress_node = texturelist_node->first_attribute("compress")) {
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
            logf("Texture missing format %s", path_to_open.c_str());
            continue;
        }

        //<size __type="2u16">128 128</size>
        auto size = texture->first_node("size");
        if (!size) {
            logf("Texture missing size %s", path_to_open.c_str());
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
                logf("Texture missing name %s", path_to_open.c_str());
                continue;
            }

            uint16_t dimensions[4];
            auto imgrect = image->first_node("imgrect");
            auto uvrect = image->first_node("uvrect");
            if (!imgrect || !uvrect) {
                logf("Texture missing dimensions %s", path_to_open.c_str());
                continue;
            }

            // it's a 4u16
            sscanf_s(imgrect->value(), "%" SCNu16 " %" SCNu16 " %" SCNu16 " %" SCNu16, &dimensions[0], &dimensions[1], &dimensions[2], &dimensions[3]);

            //logf("Image '%s' compress %d format %d", tmp, compress, format_type);
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

    logf_verbose("%d added PNGs", extra_pngs.size());
    if (extra_pngs.size() > 0) {
        if (add_images_to_list(extra_pngs, texturelist_node, ifs_path, ifs_mod_path, compress))
            prop_was_rewritten = true;
    }

    if (prop_was_rewritten) {
        string outfolder = CACHE_FOLDER "/" + ifs_mod_path;
        if (!mkdir_p(outfolder)) {
            logf("Couldn't create cache folder");
        }
        string outfile = outfolder + "/texturelist.xml";
        rapidxml_dump_to_file(outfile, texturelist);
        mod_path = outfile;
    }
}

bool cache_texture(string const&png_path, image_t &tex) {
    string cache_path = tex.cache_folder();
    if (!mkdir_p(cache_path)) {
        logf("Couldn't create texture cache folder");
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
        logf("can't load png %u: %s\n", error, lodepng_error_text(error));
        return false;
    }

    if (width != tex.width || height != tex.height) {
        logf("Loaded png (%dx%d) doesn't match texturelist.xml (%dx%d), ignoring", width, height, tex.width, tex.height);
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

        /*FILE* f;
        fopen_s(&f, "dxt_debug.bin", "wb");
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
            logf("Couldn't compress");
            return false;
        }
        image = compressed;
        image_size = compressed_size;
    }

    fopen_s(&cache, cache_file.c_str(), "wb");
    if (!cache) {
        logf("can't open cache for writing");
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

    //logf("Mapped file %s is found!", norm_path.c_str());
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
        logf("Unsupported compression for %s", png_path->c_str());
        return;
    }
    if (tex.format == UNSUPPORTED_FORMAT) {
        logf("Unsupported texture format for %s", png_path->c_str());
        return;
    }

    logf_verbose("Mapped file %s found!", png_path->c_str());
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
    FILE* cache_hashfile;
    fopen_s(&cache_hashfile, out_hashed.c_str(), "rb");
    if (cache_hashfile) {
        fread(cache_hash, 1, sizeof(cache_hash), cache_hashfile);
        fclose(cache_hashfile);
    }

    auto time_out = file_time(out.c_str());
    // don't forget to take the input into account
    time_t newest = file_time(starting.c_str());
    for (auto &path : to_merge)
        newest = std::max(newest, file_time(path.c_str()));
    // no need to merge - timestamps all up to date, dll not newer, files haven't been deleted
    if(time_out >= newest && time_out >= dll_time && memcmp(hash, cache_hash, sizeof(hash)) == 0) {
        mod_path = out;
        return;
    }

    auto first_result = rapidxml_from_avs_filepath(starting, merged_xml, merged_xml);
    if (!first_result) {
        logf("Couldn't merge (can't load first xml %s)", starting.c_str());
        return;
    }

    logf("Merging into %s", starting.c_str());
    for (auto &path : to_merge) {
        logf("  %s", path.c_str());
        rapidxml::xml_document<> rapid_to_merge;
        auto merge_load_result = rapidxml_from_avs_filepath(path, rapid_to_merge, merged_xml);
        if (!merge_load_result) {
            logf("Couldn't merge (can't load xml) %s", path.c_str());
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
        logf("Couldn't create merged cache folder");
    }

    rapidxml_dump_to_file(out, merged_xml);
    fopen_s(&cache_hashfile, out_hashed.c_str(), "wb");
    if (cache_hashfile) {
        fwrite(hash, 1, sizeof(hash), cache_hashfile);
        fclose(cache_hashfile);
    }
    mod_path = out;

    logf("Merge took %d ms", time() - start);
}

int hook_avs_fs_lstat(const char* name, struct avs_stat *st) {
    if (name == NULL)
        return avs_fs_lstat(name, st);

    logf_verbose("statting %s", name);
    string path = name;

    // can it be modded?
    auto norm_path = normalise_path(path);
    if(!norm_path)
        return avs_fs_lstat(name, st);

    auto mod_path = find_first_modfile(*norm_path);

    if (mod_path) {
        logf_verbose("Overwriting lstat");
        return avs_fs_lstat(mod_path->c_str(), st);
    }
    return avs_fs_lstat(name, st);
}

int hook_avs_fs_convert_path(char dest_name[256], const char *name) {
    if (name == NULL)
        return avs_fs_convert_path(dest_name, name);

    logf_verbose("convert_path %s", name);
    string path = name;

    // can it be modded?
    auto norm_path = normalise_path(path);
    if (!norm_path)
        return avs_fs_convert_path(dest_name, name);

    auto mod_path = find_first_modfile(*norm_path);

    if (mod_path) {
        logf_verbose("Overwriting convert_path");
        return avs_fs_convert_path(dest_name, mod_path->c_str());
    }
    return avs_fs_convert_path(dest_name, name);
}

int hook_avs_fs_mount(const char* mountpoint, const char* fsroot, const char* fstype, const char* args) {
    logf_verbose("mounting %s to %s with type %s and args %s", fsroot, mountpoint, fstype, args);
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
    logf_verbose("opening %s mode %d flags %d", name, mode, flags);
    // only touch reads
    if (mode != 1) {
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

    if (string_ends_with(path.c_str(), "texturelist.xml")) {
        parse_texturelist(orig_path, norm_path, mod_path);
    }
    else {
        handle_texture(norm_path, mod_path);
    }

    if (mod_path) {
        logf("Using %s", mod_path->c_str());
    }

    auto to_open = mod_path ? *mod_path : orig_path;
    auto ret = avs_fs_open(to_open.c_str(), mode, flags);
    ramfs_demangler_on_fs_open(to_open, ret);
    // logf("returned %d", ret);
    return ret;
}

void avs_playpen() {
    /*string path = "testcases.xml";
    void* prop_buffer = NULL;
    property_t prop = NULL;

    auto f = avs_fs_open(path.c_str(), 1, 420);
    if (f < 0)
        return;

    auto memsize = property_read_query_memsize(avs_fs_read, f, NULL, NULL);
    if (memsize < 0) {
        logf("Couldn't get memsize %08X", memsize);
        goto FAIL;
    }

    // who knows
    memsize *= 10;

    prop_buffer = malloc(memsize);
    prop = property_create(PROP_READ|PROP_WRITE|PROP_APPEND|PROP_CREATE|PROP_JSON, prop_buffer, memsize);
    if (!prop) {
        logf("Couldn't create prop");
        goto FAIL;
    }

    avs_fs_lseek(f, 0, SEEK_SET);
    property_insert_read(prop, 0, avs_fs_read, f);
    avs_fs_close(f);

    property_file_write(prop, "testcases.json");

FAIL:
    if (f)
        avs_fs_close(f);
    if (prop)
        property_destroy(prop);
    if (prop_buffer)
        free(prop_buffer);*/

    /*auto d = avs_fs_opendir(MOD_FOLDER);
    if (!d) {
        logf("couldn't d");
        return;
    }
    for (char* n = avs_fs_readdir(d); n; n = avs_fs_readdir(d))
        logf("dir %s", n);
    avs_fs_closedir(d);*/
    //char name[64];
    //auto playpen = prop_from_file("playpen.xml");
    //auto node = property_search(playpen, NULL, "/");
    //auto start = property_node_traversal(node, TRAVERSE_FIRST_CHILD);
    //auto end = property_node_traversal(start, TRAVERSE_LAST_SIBLING);
    //print_node(node);
    //print_node(start);
    //print_node(end);
    /*for (int i = 0; i <= 8; i++) {
        if (i == 6 || i == 3) continue;
        logf("Traverse: %d", i);
        auto node = property_search(playpen, NULL, "/root/t2");
        auto nnn = property_node_traversal(node, 8);
        auto nna = property_node_traversal(nnn, TRAVERSE_FIRST_ATTR);
        property_node_name(nna, name, 64);
        logf("bloop %s", name);
        for (;node;node = property_node_traversal(node, i)) {
            if (!property_node_name(node, name, 64)) {
                logf("    %s", name);
            }
        }
    }*/
    //prop_free(playpen);
}

extern "C" {
    __declspec(dllexport) int init(void) {
        logf("IFS layeredFS v" VERSION " DLL init...");
        if (MH_Initialize() != MH_OK) {
            logf("Couldn't initialize MinHook");
            return 1;
        }

        load_config();
        cache_mods();

        logf("Hooking ifs operations");
        if (!init_avs()) {
            logf("Couldn't find ifs operations in dll. Send avs dll (libavs-winxx.dll or avs2-core.dll) to mon.");
            return 2;
        }

        //jb_texhook_init();

        if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
            logf("Couldn't enable hooks");
            return 2;
        }
        logf("Hook DLL init success");

        logf("Detected mod folders:");
        for (auto &p : available_mods()) {
            logf("%s", p.c_str());
        }

        return 0;
    }
}
