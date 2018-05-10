#include "stdafx.h"
#include <windows.h>
#include <stdio.h>

#include "avs.h"
#include "hook.h"
#include "MinHook.h"
#include "utils.h"

typedef struct {
	LPCWSTR dll_name;
	char
		*avs_fs_open,
		*avs_fs_close,
		*avs_fs_read,
		*avs_fs_lseek,
		*mdigest_create,
		*mdigest_update,
		*mdigest_finish,
		*mdigest_destroy,
		*property_read_query_memsize,
		*property_create,
		*property_insert_read,
		*property_search,
		*property_node_read,
		*property_destroy,
		*property_node_traversal,
		*cstream_create,
		*cstream_operate,
		*cstream_finish,
		*cstream_destroy
	;
} avs_exports_t;

const avs_exports_t avs_exports[] = {
	[&] { avs_exports_t x; // unobfuscated
	x.dll_name =					L"libavs-win32.dll";
	x.avs_fs_open =					"avs_fs_open";
	x.avs_fs_close =				"avs_fs_close";
	x.avs_fs_read =					"avs_fs_read";
	x.avs_fs_lseek =				"avs_fs_lseek";
	x.mdigest_create =				"mdigest_create";
	x.mdigest_update =				"mdigest_update";
	x.mdigest_finish =				"mdigest_finish";
	x.mdigest_destroy =				"mdigest_destroy";
	x.property_read_query_memsize = "property_read_query_memsize";
	x.property_create =				"property_create";
	x.property_insert_read =		"property_insert_read";
	x.property_search =				"property_search";
	x.property_node_read =			"property_node_read";
	x.property_destroy =			"property_destroy";
	x.property_node_traversal =		"property_node_traversal";
	x.cstream_create =				"cstream_create";
	x.cstream_operate =				"cstream_operate";
	x.cstream_finish =				"cstream_finish";
	x.cstream_destroy =				"cstream_destroy";
	return x;
	}(),
	[&] { avs_exports_t x; // sdvx arcade
	x.dll_name =					L"libavs-win32.dll";
	x.avs_fs_open =					"XCd229cc000090";
	x.avs_fs_close =				"XCd229cc00011f";
	x.avs_fs_read =					"XCd229cc00010d";
	x.avs_fs_lseek =				"XCd229cc00004d";
	x.mdigest_create =				"XCd229cc00003d";
	x.mdigest_update =				"XCd229cc000157";
	x.mdigest_finish =				"XCd229cc000015";
	x.mdigest_destroy =				"XCd229cc000050";
	x.property_read_query_memsize = "XCd229cc0000ff";
	x.property_create =				"XCd229cc000126";
	x.property_insert_read =		"XCd229cc00009a";
	x.property_search =				"XCd229cc00012e";
	x.property_node_read =			"XCd229cc0000f3";
	x.property_destroy =			"XCd229cc00013c";
	x.property_node_traversal =		"XCd229cc000046";
	x.cstream_create =				"XCd229cc000141";
	x.cstream_operate =				"XCd229cc00008c";
	x.cstream_finish =				"XCd229cc000025";
	x.cstream_destroy =				"XCd229cc0000e3";
	return x;
	}(),
	[&] { avs_exports_t x; // sdvx cloud
	x.dll_name =					L"libavs-win32.dll";
	x.avs_fs_open =					"XCnbrep700004e";
	x.avs_fs_close =				"XCnbrep7000055";
	x.avs_fs_read =					"XCnbrep7000051";
	x.avs_fs_lseek =				"XCnbrep700004f";
	x.mdigest_create =				"XCnbrep700013f";
	x.mdigest_update =				"XCnbrep7000141";
	x.mdigest_finish =				"XCnbrep7000142";
	x.mdigest_destroy =				"XCnbrep7000143";
	x.property_read_query_memsize = "XCnbrep70000b0";
	x.property_create =				"XCnbrep7000090";
	x.property_insert_read =		"XCnbrep7000094";
	x.property_search =				"XCnbrep70000a1";
	x.property_node_read =			"XCnbrep70000ab";
	x.property_destroy =			"XCnbrep7000091";
	x.property_node_traversal =		"XCnbrep70000a6";
	x.cstream_create =				"XCnbrep7000130";
	x.cstream_operate =				"XCnbrep7000132";
	x.cstream_finish =				"XCnbrep7000133";
	x.cstream_destroy =				"XCnbrep7000134";
	return x;
	}(),
	[&] { avs_exports_t x; // IIDX
	x.dll_name =					L"libavs-win32.dll";
	x.avs_fs_open =					"XCnbrep7000039";
	x.avs_fs_close =				"XCnbrep7000040";
	x.avs_fs_read =					"XCnbrep700003c";
	x.avs_fs_lseek =				"XCnbrep700003a";
	x.mdigest_create =				"XCnbrep7000133";
	x.mdigest_update =				"XCnbrep7000135";
	x.mdigest_finish =				"XCnbrep7000136";
	x.mdigest_destroy =				"XCnbrep7000137";
	x.property_read_query_memsize = "XCnbrep700009b";
	x.property_create =				"XCnbrep700007b";
	x.property_insert_read =		"XCnbrep700007f";
	x.property_search =				"XCnbrep700008c";
	x.property_node_read =			"XCnbrep7000096";
	x.property_destroy =			"XCnbrep700007c";
	x.property_node_traversal =		"XCnbrep7000091";
	x.cstream_create =				"XCnbrep7000124";
	x.cstream_operate =				"XCnbrep7000126";
	x.cstream_finish =				"XCnbrep7000127";
	x.cstream_destroy =				"XCnbrep7000128";
	return x;
	}(),
};

// file functions
AVS_FILE(*avs_fs_open)(const char* name, uint16_t mode, int flags);
void(*avs_fs_close)(AVS_FILE f);
int(*avs_fs_fstat)(AVS_FILE f, struct avs_stat *st);
avs_reader_t avs_fs_read;
int(*avs_fs_lseek)(AVS_FILE f, long int offset, int origin);

// property handling
int32_t(*property_read_query_memsize)(avs_reader_t reader, AVS_FILE f, int* unk0, int* unk1);
property_t(*property_create)(int flags, void *buffer, uint32_t buffer_size);
int(*property_insert_read)(property_t prop, int unk0, avs_reader_t reader, AVS_FILE f);
property_t(*property_search)(property_t prop, node_t node, const char *path);
void(*property_node_read)(node_t node, property_type type, void* out, uint32_t out_size);
void(*property_destroy)(property_t prop);
node_t(*property_node_traversal)(node_t node, int flags);

// md5sum (and sha1 if needed)
mdigest_p(*mdigest_create)(mdigest_algs_t algorithm);
void(*mdigest_update)(mdigest_p digest, char* data, int size);
void(*mdigest_finish)(mdigest_p digest, uint8_t* hash, int size);
void(*mdigest_destroy)(mdigest_p digest);

// compression
cstream_t*(*cstream_create)(compression_type type);
bool(*cstream_operate)(cstream_t* compressor);
bool(*cstream_finish)(cstream_t* compressor);
bool(*cstream_destroy)(cstream_t* compressor);

/*void* (*avs_fs_mount)(char* mountpoint, char* fsroot, char* fstype, int a5);
void* hook_avs_fs_mount(char* mountpoint, char* fsroot, char* fstype, int a5) {
	logf("Mounting %s at %s with type %s", fsroot, mountpoint, fstype);
	return avs_fs_mount(mountpoint, fsroot, fstype, a5);
}*/

#define TEST_HOOK_AND_APPLY(func) if (MH_CreateHookApi(avs_exports[i].dll_name, avs_exports[i]. ## func, hook_ ## func, (LPVOID*) &func) != MH_OK || func == NULL) continue
#define LOAD_FUNC(func) if( (func = (decltype(func))GetProcAddress(mod_handle, avs_exports[i]. ## func)) == NULL) continue

bool init_avs(void) {
	bool success = false;
	for (int i = 0; i < lenof(avs_exports); i++) {
		auto mod_handle = GetModuleHandle(avs_exports[i].dll_name);
		TEST_HOOK_AND_APPLY(avs_fs_open);
		//TEST_HOOK_AND_APPLY(avs_fs_mount);
		LOAD_FUNC(avs_fs_close);
		LOAD_FUNC(avs_fs_read);
		LOAD_FUNC(avs_fs_lseek);

		LOAD_FUNC(mdigest_create);
		LOAD_FUNC(mdigest_update);
		LOAD_FUNC(mdigest_finish);
		LOAD_FUNC(mdigest_destroy);

		LOAD_FUNC(property_read_query_memsize);
		LOAD_FUNC(property_create);
		LOAD_FUNC(property_insert_read);
		LOAD_FUNC(property_search);
		LOAD_FUNC(property_node_read);
		LOAD_FUNC(property_destroy);
		LOAD_FUNC(property_node_traversal);

		LOAD_FUNC(cstream_create);
		LOAD_FUNC(cstream_operate);
		LOAD_FUNC(cstream_finish);
		LOAD_FUNC(cstream_destroy);
		success = true;
		break;
	}
	return success;
}

string md5_sum(char* str) {
	uint8_t sum[16];
	char sum_str[33];
	auto digest = mdigest_create(MD5);
	mdigest_update(digest, str, strlen(str));
	mdigest_finish(digest, sum, 16);
	mdigest_destroy(digest);
	for (int i = 0; i < 16; i++) {
		snprintf(sum_str + 2 * i, 3, "%02x", sum[i]);
	}
	return sum_str;
}

unsigned char* lz_compress(unsigned char* input, size_t length, size_t *compressed_length) {
	auto compressor = cstream_create(AVS_COMPRESS_AVSLZ);
	if (!compressor) {
		logf("Couldn't create");
		return NULL;
	}
	compressor->input_buffer = input;
	compressor->input_size = length;
	// worst case, for every 16 bytes there will be an extra flag byte
	auto compress_size = length + length / 16;
	auto compress_buffer = (unsigned char*)malloc(compress_size);
	compressor->output_buffer = compress_buffer;
	compressor->output_size = compress_size;
	
	bool ret;
	ret = cstream_operate(compressor);
	if (!ret && !compressor->input_size) {
		compressor->input_buffer = NULL;
		compressor->input_size = -1;
		ret = cstream_operate(compressor);
	}
	if (!ret) {
		logf("Couldn't operate");
		return NULL;
	}
	if (cstream_finish(compressor)) {
		logf("Couldn't finish");
		return NULL;
	}
	*compressed_length = compress_size - compressor->output_size;
	cstream_destroy(compressor);
	return compress_buffer;
}