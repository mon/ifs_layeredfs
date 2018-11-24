#include "stdafx.h"
#include "hook.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

// all good code mixes C and C++, right?
using std::string;
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

#include "MinHook.h"
#pragma comment(lib, "minhook.lib")

#include "config.h"
#include "utils.h"
#include "avs.h"
#include "lodepng.h"
#include "stb_dxt.h"
#include "texture_packer.h"
#include "modpath_handler.h"
#include "GuillotineBinPack.h"

#define VER_STRING "1.6"

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

typedef struct {
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
std::unordered_map<string, image_t> ifs_textures;

typedef std::unordered_set<string> string_set;

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

bool add_images_to_list(string_set &extra_pngs, property_t &prop, string const&ifs_path, string const&ifs_mod_path, compress_type compress) {
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

	struct node_size node_size = {};
	auto old_size = property_node_query_stat(prop, NULL, &node_size);
	// big numbers really don't hit memory that badly, and smaller ones are too small :(
	const int per_canvas_size = 1024; //104;
	const int per_texture_size = 1024; //176;
	int new_size = old_size + per_canvas_size * (int)packed_textures.size();
	for (auto canvas = packed_textures.begin(); canvas != packed_textures.end(); canvas++) {
		new_size += (int)(*canvas)->bitmaps.size() * per_texture_size;
	}

	auto prop_buffer = malloc(new_size);
	// bit 16 enables append
	auto new_prop = property_create(PROP_CREATE_FLAGS, prop_buffer, new_size);
	if (!prop) {
		logf("Couldn't create new prop");
		free(prop_buffer);
		return false;
	}

	auto old_root = property_search(prop, NULL, "/");
	if (!property_node_clone(new_prop, NULL, old_root, 1)) {
		logf("Couldn't clone %s oldsize: %d, newsize: %d, error: %X (%s) %X",
			ifs_path.c_str(), old_size, new_size, new_prop->error_code, get_prop_error_str(new_prop->error_code), new_prop->has_error);
		prop_free(new_prop);
		return false;
	}

	auto texturelist_node = property_search(new_prop, NULL, "/");
	for (unsigned int i = 0; i < packed_textures.size(); i++) {
		Packer *canvas = packed_textures[i];
		char tex_name[8];
		snprintf(tex_name, 8, "ctex%03d", i);
		auto canvas_node = property_node_create(NULL, texturelist_node, PROP_TYPE_node, "texture");
		property_node_create(NULL, canvas_node, PROP_TYPE_attr, "format@", "argb8888rev");
		property_node_create(NULL, canvas_node, PROP_TYPE_attr, "mag_filter@", "nearest");
		property_node_create(NULL, canvas_node, PROP_TYPE_attr, "min_filter@", "nearest");
		property_node_create(NULL, canvas_node, PROP_TYPE_attr, "name@", tex_name);
		property_node_create(NULL, canvas_node, PROP_TYPE_attr, "wrap_s@", "clamp");
		property_node_create(NULL, canvas_node, PROP_TYPE_attr, "wrap_t@", "clamp");

		uint16_t size[2] = { (uint16_t)canvas->width, (uint16_t)canvas->height };
		property_node_create(NULL, canvas_node, PROP_TYPE_2u16, "size", size);

		for (unsigned int j = 0; j < canvas->bitmaps.size(); j++) {
			Bitmap *texture = canvas->bitmaps[j];
			auto tex_node = property_node_create(NULL, canvas_node, PROP_TYPE_node, "image");
			property_node_create(NULL, tex_node, PROP_TYPE_attr, "name@", (void*)texture->name.c_str());

			uint16_t coords[4];
			coords[0] = texture->packX * 2;
			coords[1] = (texture->packX + texture->width) * 2;
			coords[2] = texture->packY * 2;
			coords[3] = (texture->packY + texture->height) * 2;
			property_node_create(NULL, tex_node, PROP_TYPE_4u16, "imgrect", coords);
			coords[0] += 2;
			coords[1] -= 2;
			coords[2] += 2;
			coords[3] -= 2;
			property_node_create(NULL, tex_node, PROP_TYPE_4u16, "uvrect", coords);

			image_t image_info;
			image_info.name = texture->name;
			image_info.name_md5 = md5_sum(texture->name.c_str());
			image_info.format = ARGB8888REV;
			image_info.compression = compress;
			image_info.ifs_mod_path = ifs_mod_path;
			image_info.width = texture->width;
			image_info.height = texture->height;

			auto md5_path = ifs_path + "/tex/" + image_info.name_md5;
			ifs_textures[md5_path] = image_info;
		}
	}

	prop_free(prop);
	prop = new_prop;
	logf("Texture extend total time %d ms", time() - start);
	return true;
}

void parse_texturelist(string const&path, string const&norm_path, optional<string> &mod_path) {
	char tmp[128];
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
	auto prop = prop_from_file(path_to_open);
	if (!prop)
		return;

	auto extra_pngs = list_pngs(ifs_mod_path);

	auto compress = NONE;
	auto compress_node = property_search(prop, NULL, "compress@texturelist");
	if (compress_node) {
		property_node_read(compress_node, PROP_TYPE_attr, tmp, sizeof(tmp));
		if (!_stricmp(tmp, "avslz")) {
			compress = AVSLZ;
		} else {
			compress = UNSUPPORTED_COMPRESS;
		}
	}

	for (auto texture = property_search(prop, NULL, "texturelist/texture");
		 texture != NULL;
		 texture = property_node_traversal(texture, TRAVERSE_NEXT_SEARCH_RESULT)) {
		auto format = property_search(NULL, texture, "format@");
		if (!format) {
			logf("Texture missing format %s", path_to_open.c_str());
			continue;
		}

		//<size __type="2u16">128 128</size>
		auto size = property_search(NULL, texture, "size");
		if (!size) {
			logf("Texture missing size %s", path_to_open.c_str());
			continue;
		}

		auto format_type = UNSUPPORTED_FORMAT;
		property_node_read(format, PROP_TYPE_attr, tmp, sizeof(tmp));
		if (!_stricmp(tmp, "argb8888rev")) {
			format_type = ARGB8888REV;
		} else if (!_stricmp(tmp, "dxt5")) {
			format_type = DXT5;
		}

		for (auto image = property_search(NULL, texture, "image");
			 image != NULL;
			 image = property_node_traversal(image, TRAVERSE_NEXT_SEARCH_RESULT)) {
			auto name = property_search(NULL, image, "name@");
			if (!name) {
				logf("Texture missing name %s", path_to_open.c_str());
				continue;
			}
			property_node_read(name, PROP_TYPE_attr, tmp, sizeof(tmp));

			uint16_t dimensions[4];
			auto imgrect = property_search(NULL, image, "imgrect");
			auto uvrect = property_search(NULL, image, "uvrect");
			if (!imgrect || !uvrect) {
				logf("Texture missing dimensions %s", path_to_open.c_str());
				continue;
			}

			property_node_read(imgrect, PROP_TYPE_4u16, dimensions, sizeof(dimensions));

			//logf("Image '%s' compress %d format %d", tmp, compress, format_type);
			image_t image_info;
			image_info.name = tmp;
			image_info.name_md5 = md5_sum(tmp);
			image_info.format = format_type;
			image_info.compression = compress;
			image_info.ifs_mod_path = ifs_mod_path;
			image_info.width = (dimensions[1] - dimensions[0]) / 2;
			image_info.height = (dimensions[3] - dimensions[2]) / 2;

			auto md5_path = ifs_path + "/tex/" + image_info.name_md5;
			ifs_textures[md5_path] = image_info;

			extra_pngs.erase(image_info.name);
		}
	}

	logf_verbose("%d added PNGs", extra_pngs.size());
	if (extra_pngs.size() > 0) {
		if (add_images_to_list(extra_pngs, prop, ifs_path, ifs_mod_path, compress))
			prop_was_rewritten = true;
	}

	if (prop_was_rewritten) {
		string outfolder = CACHE_FOLDER "/" + ifs_mod_path;
		if (!mkdir_p(outfolder)) {
			logf("Couldn't create cache folder");
		}
		string outfile = outfolder + "/texturelist.xml";
		if (property_file_write(prop, outfile.c_str()) > 0) {
			mod_path = outfile;
		}
		else {
			logf("Couldn't write prop file");
		}
	}
	prop_free(prop);
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
	if (cache_time > 0 && cache_time >= png_time) {
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
	auto tex_search = ifs_textures.find(norm_path);
	if (tex_search == ifs_textures.end()) {
		return;
	}

	//logf("Mapped file %s is found!", norm_path.c_str());
	auto tex = tex_search->second;

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

void merge_xmls(string const& path, string const&norm_path, optional<string> &mod_path) {
	auto start = time();
	// initialize since we're GOTO-ing like naughty people
	string out;
	string out_folder;
	void* merged_prop_buff = NULL;
	property_t merged_prop = NULL;
	vector<property_t> props_to_merge;

	auto merge_path = norm_path;
	string_replace(merge_path, ".xml", ".merged.xml");
	auto to_merge = find_all_modfile(merge_path);
	// nothing to do...
	if (to_merge.size() == 0)
		return;

	auto starting = mod_path ? *mod_path : path;
	out = CACHE_FOLDER "/" + norm_path;

	auto time_out = file_time(out.c_str());
	// don't forget to take the input into account
	time_t newest = file_time(starting.c_str());
	for (auto &path : to_merge)
		newest = std::max(newest, file_time(path.c_str()));
	// no need to merge
	if(time_out >= newest) {
		mod_path = out;
		return;
	}

	auto starting_f = avs_fs_open(starting.c_str(), 1, 420);
	if (starting_f < 0)
		return;

	int size = property_read_query_memsize_long(avs_fs_read, starting_f, NULL, NULL, NULL);
	avs_fs_lseek(starting_f, 0, SEEK_SET);
	if (size < 0)
		goto CLEANUP;

	node_size size_dummy;
	for (auto &path : to_merge) {
		logf("Merging %s", path.c_str());
		auto prop = prop_from_file(path);
		if (prop == NULL) {
			logf("Couldn't merge (can't load xml) %s", path.c_str());
			goto CLEANUP;
		}
		props_to_merge.push_back(prop);
		size += property_node_query_stat(prop, NULL, &size_dummy);
	}
	logf("...into %s", starting.c_str());

	merged_prop_buff = malloc(size);
	merged_prop = property_create(PROP_CREATE_FLAGS, merged_prop_buff, size);
	if (merged_prop <= 0) {
		logf("Couldn't merge (can't create result prop, %s)", get_prop_error_str((int32_t)merged_prop));
		goto CLEANUP;
	}
	int first_insert = property_insert_read(merged_prop, NULL, avs_fs_read, starting_f);
	if (first_insert < 0) {
		logf("Couldn't merge (can't load first xml, %s)", get_prop_error_str(first_insert));
		goto CLEANUP;
	}
	//auto top = property_search(merged_prop, NULL, "/");
	for (auto prop : props_to_merge) {
		//auto root = property_search(prop, NULL, "/");
		if (!prop_merge_into(merged_prop, prop)) {
			logf("Couldn't merge");
			goto CLEANUP;
		}
		/*for(auto child = property_node_traversal(root, TRAVERSE_FIRST_CHILD);
				child;
				child = property_node_traversal(child, TRAVERSE_NEXT_SIBLING))
			if (!property_node_clone(NULL, top, child, 1)) {
				logf("Couldn't merge (can't clone) %X %X", merged_prop->error_code, merged_prop->has_error);
				goto CLEANUP;
			}*/
	}

	auto folder_terminator = out.rfind("/");
	out_folder = out.substr(0, folder_terminator);
	if (!mkdir_p(out_folder)) {
		logf("Couldn't create merged cache folder");
		goto CLEANUP;
	}

	property_file_write(merged_prop, out.c_str());
	mod_path = out;

	logf("Merge took %d ms", time() - start);

CLEANUP:
	if (merged_prop)
		property_destroy(merged_prop);
	if (merged_prop_buff)
		free(merged_prop_buff);
	avs_fs_close(starting_f);
	for (auto prop : props_to_merge)
		prop_free(prop);
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

AVS_FILE hook_avs_fs_open(const char* name, uint16_t mode, int flags) {
	if(name == NULL)
		return avs_fs_open(name, mode, flags);
	logf_verbose("opening %s mode %d flags %d", name, mode, flags);
	// only touch reads
	if (mode != 1) {
		return avs_fs_open(name, mode, flags);
	}
	string path = name;

	// can it be modded ie is it under /data ?
	auto _norm_path = normalise_path(path);
	if (!_norm_path)
		return avs_fs_open(name, mode, flags);
	// unpack success
	auto norm_path = *_norm_path;

	auto mod_path = find_first_modfile(norm_path);
	if (!mod_path) {
		// mod ifs paths use _ifs
		string_replace(norm_path, ".ifs", "_ifs");
		mod_path = find_first_modfile(norm_path);
	}

	if(string_ends_with(path.c_str(), ".xml")) {
		merge_xmls(path, norm_path, mod_path);
	}

	if (string_ends_with(path.c_str(), "texturelist.xml")) {
		parse_texturelist(path, norm_path, mod_path);
	}
	else {
		handle_texture(norm_path, mod_path);
	}

	if (mod_path) {
		logf("Using %s", mod_path->c_str());
	}

	auto to_open = mod_path ? *mod_path : path;
	auto ret = avs_fs_open(to_open.c_str(), mode, flags);
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