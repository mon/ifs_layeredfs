#include <windows.h>
#include <stdio.h>

#include "avs.h"
#include "hook.h"
#include "MinHook.h"
#include "utils.h"

typedef struct {
	LPCWSTR dll_name;
	char
		*version_name,
		*unique_check // for IIDX vs SDVX cloud, almost all funcs are identical
#undef X
#define X(ret_type, name, ...) , * name
		AVS_FUNC_LIST
	;
} avs_exports_t;

const avs_exports_t avs_exports[] = {
	[&] { avs_exports_t x;
	x.version_name =				"normal";
	x.dll_name =					L"libavs-win32.dll";
	x.unique_check =				NULL;
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
	x.property_node_write =			"property_node_write";
	x.property_file_write =			"property_file_write";
	x.property_destroy =			"property_destroy";
	x.property_node_traversal =		"property_node_traversal";
	x.cstream_create =				"cstream_create";
	x.cstream_operate =				"cstream_operate";
	x.cstream_finish =				"cstream_finish";
	x.cstream_destroy =				"cstream_destroy";
	return x;
	}(),
	[&] { avs_exports_t x;
	x.version_name =				"sdvx";
	x.dll_name =					L"libavs-win32.dll";
	x.unique_check =				NULL;
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
	x.property_node_write =			"XCd229cc00002d";
	x.property_file_write =			"XCd229cc000052";
	x.property_destroy =			"XCd229cc00013c";
	x.property_node_traversal =		"XCd229cc000046";
	x.cstream_create =				"XCd229cc000141";
	x.cstream_operate =				"XCd229cc00008c";
	x.cstream_finish =				"XCd229cc000025";
	x.cstream_destroy =				"XCd229cc0000e3";
	return x;
	}(),
	[&] { avs_exports_t x; // sdvx cloud
	x.version_name =				"sdvx_cloud";
	x.dll_name =					L"libavs-win32.dll";
	x.unique_check =				"XCnbrep700013c";
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
	x.property_node_write =			"XCnbrep70000ac";
	x.property_file_write =			"XCnbrep70000b6";
	x.property_destroy =			"XCnbrep7000091";
	x.property_node_traversal =		"XCnbrep70000a6";
	x.cstream_create =				"XCnbrep7000130";
	x.cstream_operate =				"XCnbrep7000132";
	x.cstream_finish =				"XCnbrep7000133";
	x.cstream_destroy =				"XCnbrep7000134";
	return x;
	}(),
	[&] { avs_exports_t x; // IIDX
	x.version_name =				"iidx";
	x.dll_name =					L"libavs-win32.dll";
	x.unique_check =				NULL;
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
	x.property_node_write =			"XCnbrep7000097";
	x.property_file_write =			"XCnbrep70000a1";
	x.property_destroy =			"XCnbrep700007c";
	x.property_node_traversal =		"XCnbrep7000091";
	x.cstream_create =				"XCnbrep7000124";
	x.cstream_operate =				"XCnbrep7000126";
	x.cstream_finish =				"XCnbrep7000127";
	x.cstream_destroy =				"XCnbrep7000128";
	return x;
	}(),
};

#undef X
#define X(ret_type, name, ...) ret_type (* name )( __VA_ARGS__ );
AVS_FUNC_LIST

/*void* (*avs_fs_mount)(char* mountpoint, char* fsroot, char* fstype, int a5);
void* hook_avs_fs_mount(char* mountpoint, char* fsroot, char* fstype, int a5) {
	logf("Mounting %s at %s with type %s", fsroot, mountpoint, fstype);
	return avs_fs_mount(mountpoint, fsroot, fstype, a5);
}*/

#define TEST_HOOK_AND_APPLY(func) if (MH_CreateHookApi(avs_exports[i].dll_name, avs_exports[i]. ## func, hook_ ## func, (LPVOID*) &func) != MH_OK || func == NULL) continue
#define LOAD_FUNC(func) if( (func = (decltype(func))GetProcAddress(mod_handle, avs_exports[i]. ## func)) == NULL) continue
#define CHECK_UNIQUE(func) if( avs_exports[i]. ## func != NULL && GetProcAddress(mod_handle, avs_exports[i]. ## func) == NULL) continue

bool init_avs(void) {
	bool success = false;
	for (int i = 0; i < lenof(avs_exports); i++) {
		auto mod_handle = GetModuleHandle(avs_exports[i].dll_name);
		CHECK_UNIQUE(unique_check);
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
		LOAD_FUNC(property_node_write);
		LOAD_FUNC(property_file_write);
		LOAD_FUNC(property_destroy);
		LOAD_FUNC(property_node_traversal);

		LOAD_FUNC(cstream_create);
		LOAD_FUNC(cstream_operate);
		LOAD_FUNC(cstream_finish);
		LOAD_FUNC(cstream_destroy);
		success = true;
		logf("Detected dll: %s", avs_exports[i].version_name);
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
	// worst case, for every 8 bytes there will be an extra flag byte
    auto to_add = max(length / 8, 1);
	auto compress_size = length + to_add;
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