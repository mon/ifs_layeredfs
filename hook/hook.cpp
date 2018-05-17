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

#include "MinHook.h"
#pragma comment(lib, "minhook.lib")

#include "utils.h"
#include "avs.h"
#include "lodepng.h"
#include "stb_dxt.h"

#define VERSION "0.3"
#define MOD_FOLDER "./data_mod"

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
	const string cache_folder() { return ifs_mod_path + "/_cache/";  }
	const string cache_file() { return cache_folder() + name_md5; };
} image_t;

// ifs_textures["data/graphics/ver04/logo.ifs/tex/4f754d4f424f092637a49a5527ece9bb"] will be "konami"
std::unordered_map<std::string, image_t> ifs_textures;

void parse_texturelist(string const&path, string const&norm_path, string const&mod_path, bool mod_path_valid, int mode, int flags) {
	char tmp[128];
	void* prop_buffer = NULL;
	property_t prop = NULL;

	// open the correct file
	auto path_to_open = mod_path_valid ? mod_path : path;

	auto texlist = avs_fs_open(path_to_open.c_str(), mode, flags);
	if (texlist < 0)
		return;

	// get a reasonable base path
	auto ifs_path = norm_path;
	// truncate
	ifs_path.resize(ifs_path.size() - strlen("/tex/texturelist.xml"));
	//logf("Reading ifs %s", ifs_path.c_str());
	auto ifs_mod_path = MOD_FOLDER "/" + ifs_path;
	string_replace(ifs_mod_path, ".ifs", "_ifs");

	auto memsize = property_read_query_memsize(avs_fs_read, texlist, NULL, NULL);
	if (memsize < 0) {
		logf("Couldn't get memsize %08X", memsize);
		goto CLEANUP;
	}
	
	prop_buffer = malloc(memsize);
	prop = property_create(17, prop_buffer, memsize);
	if (!prop) {
		logf("Couldn't create prop");
		goto CLEANUP;
	}

	avs_fs_lseek(texlist, 0, SEEK_SET);
	property_insert_read(prop, 0, avs_fs_read, texlist);
	avs_fs_close(texlist);
	texlist = NULL;

	auto compress = NONE;
	auto compress_node = property_search(prop, NULL, "compress@texturelist");
	if (compress_node) {
		property_node_read(compress_node, PROP_TYPE_ATTR, tmp, sizeof(tmp));
		if (!_stricmp(tmp, "avslz")) {
			compress = AVSLZ;
		} else {
			compress = UNSUPPORTED_COMPRESS;
		}
	}

	for (auto texture = property_search(prop, NULL, "texturelist/texture");
		 texture != NULL;
		 texture = property_node_traversal(texture, 7)) { // 7 = next node
		auto format = property_search(NULL, texture, "format@");
		if (!format) {
			logf("Texture missing format %s", path_to_open.c_str());
			continue;
		}

		auto format_type = UNSUPPORTED_FORMAT;
		property_node_read(format, PROP_TYPE_ATTR, tmp, sizeof(tmp));
		if (!_stricmp(tmp, "argb8888rev")) {
			format_type = ARGB8888REV;
		} else if (!_stricmp(tmp, "dxt5")) {
			format_type = DXT5;
		}

		for (auto image = property_search(NULL, texture, "image");
			 image != NULL;
			 image = property_node_traversal(image, 7)) { // 7 = next node
			auto name = property_search(NULL, image, "name@");
			if (!name) {
				logf("Texture missing name %s", path_to_open.c_str());
				continue;
			}
			property_node_read(name, PROP_TYPE_ATTR, tmp, sizeof(tmp));

			uint16_t dimensions[4];
			auto imgrect = (property_search(NULL, image, "imgrect"));
			if (!imgrect) {
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
		}
	}

CLEANUP:
	if(texlist)
		avs_fs_close(texlist);
	if(prop)
		property_destroy(prop);
	if(prop_buffer)
		free(prop_buffer);
}

bool cache_texture(string const&png_path, image_t &tex) {
	string cache_path = tex.cache_folder();
	if (_mkdir(cache_path.c_str()) && errno != EEXIST) {
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
		auto uncomp_sz = _byteswap_ulong(uncompressed_size);
		auto comp_sz = _byteswap_ulong(image_size);
		fwrite(&uncomp_sz, 4, 1, cache);
		fwrite(&comp_sz, 4, 1, cache);
	}
	fwrite(image, 1, image_size, cache);
	fclose(cache);
	free(image);
	return true;

	return false;
}

void handle_texture(string const&norm_path, string &mod_path) {
	auto tex_search = ifs_textures.find(norm_path);
	if (tex_search == ifs_textures.end()) {
		return;
	}

	//logf("Mapped file %s is found!", norm_path.c_str());
	auto tex = tex_search->second;
	auto png_path = tex.ifs_mod_path + "/tex/" + tex.name + ".png";
	if (!file_exists(png_path.c_str())) {
		// remove the /tex/, it's nicer to navigate
		png_path = tex.ifs_mod_path + "/" + tex.name + ".png";
		if (!file_exists(png_path.c_str())) {
			return;
		}
	}

	if (tex.compression == UNSUPPORTED_COMPRESS) {
		logf("Unsupported compression for %s", png_path.c_str());
		return;
	}
	if (tex.format == UNSUPPORTED_FORMAT) {
		logf("Unsupported texture format for %s", png_path.c_str());
		return;
	}

	logf("Mapped file %s found!", png_path.c_str());
	if (cache_texture(png_path, tex)) {
		mod_path = tex.cache_file();
	}
	return;
}

AVS_FILE hook_avs_fs_open(const char* name, uint16_t mode, int flags) {
	if(name == NULL)
		return avs_fs_open(name, mode, flags);
#ifdef _DEBUG
	//logf("opening %s mode %d flags %d", name, mode, flags);
#endif
	string path = name;

	// can it be modded?
	auto data_pos = path.find("data/");
	if (data_pos == string::npos)
		return avs_fs_open(name, mode, flags);
	auto norm_path = path.substr(data_pos + strlen("data/"));

	string mod_path = MOD_FOLDER "/" + norm_path;
	// mod ifs paths use _ifs
	string_replace(mod_path, ".ifs", "_ifs");
	
	auto mod_path_valid = file_exists(mod_path.c_str());

	if (string_ends_with(path.c_str(), "texturelist.xml")) {
		parse_texturelist(path, norm_path, mod_path, mod_path_valid, mode, flags);
	}

	handle_texture(norm_path, mod_path);

	// since texture handling can update this
	// TODO: this is a bit messy
	mod_path_valid = file_exists(mod_path.c_str());
	if (mod_path_valid) {
		logf("Using %s", mod_path.c_str());
	}

	auto to_open = mod_path_valid ? mod_path : path;
	auto ret = avs_fs_open(to_open.c_str(), mode, flags);
	// logf("returned %d", ret);
	return ret;
}

extern "C" {
	__declspec(dllexport) int init(void) {
		// reset contents for fresh run
		fopen_s(&logfile, "ifs_hook.log", "w");
		if(logfile)
			fclose(logfile);
		logf("IFS layeredFS v" VERSION " DLL init...");
		if (MH_Initialize() != MH_OK) {
			logf("Couldn't initialize MinHook");
			return 1;
		}

		logf("Hooking ifs operations");
		if (!init_avs()) {
			logf("Couldn't find ifs operations in dll. Send avs dll to mon.");
			return 2;
		}

		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
			logf("Couldn't enable hooks");
			return 2;
		}
		logf("Hook DLL init success");
		return 0;
	}
}