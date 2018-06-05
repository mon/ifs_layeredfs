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
	[&] { avs_exports_t x = { 0 };
	x.version_name =				"normal";
	x.dll_name =					L"libavs-win32.dll";
	x.unique_check =				NULL;
	x.avs_fs_open =					"avs_fs_open";
	x.avs_fs_close =				"avs_fs_close";
	x.avs_fs_read =					"avs_fs_read";
	x.avs_fs_lseek =				"avs_fs_lseek";
	x.avs_fs_lstat =				"avs_fs_lstat";
	x.avs_fs_opendir =				"avs_fs_opendir";
	x.avs_fs_closedir =				"avs_fs_closedir";
	x.avs_fs_readdir =				"avs_fs_readdir";
	x.mdigest_create =				"mdigest_create";
	x.mdigest_update =				"mdigest_update";
	x.mdigest_finish =				"mdigest_finish";
	x.mdigest_destroy =				"mdigest_destroy";
	x.property_read_query_memsize = "property_read_query_memsize";
	x.property_create =				"property_create";
	x.property_desc_to_buffer =		"property_desc_to_buffer";
	x.property_insert_read =		"property_insert_read";
	x.property_search =				"property_search";
	x.property_node_clone =			"property_node_clone";
	x.property_node_create =		"property_node_create";
	x.property_node_read =			"property_node_read";
	x.property_node_write =			"property_node_write";
	x.property_node_type =			"property_node_type";
	x.property_node_name =			"property_node_name";
	x.property_node_datasize =		"property_node_datasize";
	x.property_file_write =			"property_file_write";
	x.property_destroy =			"property_destroy";
	x.property_node_traversal =		"property_node_traversal";
	x.property_node_query_stat =	"property_node_query_stat";
	x.cstream_create =				"cstream_create";
	x.cstream_operate =				"cstream_operate";
	x.cstream_finish =				"cstream_finish";
	x.cstream_destroy =				"cstream_destroy";
	return x;
	}(),
	[&] { avs_exports_t x = { 0 };
	x.version_name =				"sdvx";
	x.dll_name =					L"libavs-win32.dll";
	x.unique_check =				NULL;
	x.avs_fs_open =					"XCd229cc000090";
	x.avs_fs_close =				"XCd229cc00011f";
	x.avs_fs_read =					"XCd229cc00010d";
	x.avs_fs_lseek =				"XCd229cc00004d";
	x.avs_fs_lstat =				"XCd229cc0000c0";
	x.avs_fs_opendir =				"XCd229cc0000f0";
	x.avs_fs_closedir =				"XCd229cc0000b8";
	x.avs_fs_readdir =				"XCd229cc0000bb";
	x.mdigest_create =				"XCd229cc00003d";
	x.mdigest_update =				"XCd229cc000157";
	x.mdigest_finish =				"XCd229cc000015";
	x.mdigest_destroy =				"XCd229cc000050";
	x.property_read_query_memsize = "XCd229cc0000ff";
	x.property_create =				"XCd229cc000126";
	x.property_desc_to_buffer =		"XCd229cc0000fd";
	x.property_insert_read =		"XCd229cc00009a";
	x.property_search =				"XCd229cc00012e";
	x.property_node_clone =			"XCd229cc00010a";
	x.property_node_create =		"XCd229cc00002c";
	x.property_node_read =			"XCd229cc0000f3";
	x.property_node_write =			"XCd229cc00002d";
	x.property_node_type =			"XCd229cc000071";
	x.property_node_name =			"XCd229cc000049";
	x.property_node_datasize =		"XCd229cc000083";
	x.property_file_write =			"XCd229cc000052";
	x.property_destroy =			"XCd229cc00013c";
	x.property_node_traversal =		"XCd229cc000046";
	x.property_node_query_stat =	"XCd229cc0000b1";
	x.cstream_create =				"XCd229cc000141";
	x.cstream_operate =				"XCd229cc00008c";
	x.cstream_finish =				"XCd229cc000025";
	x.cstream_destroy =				"XCd229cc0000e3";
	return x;
	}(),
	[&] { avs_exports_t x = { 0 }; // sdvx cloud
	x.version_name =				"sdvx_cloud";
	x.dll_name =					L"libavs-win32.dll";
	x.unique_check =				"XCnbrep700013c";
	x.avs_fs_open =					"XCnbrep700004e";
	x.avs_fs_close =				"XCnbrep7000055";
	x.avs_fs_read =					"XCnbrep7000051";
	x.avs_fs_lseek =				"XCnbrep700004f";
	x.avs_fs_lstat =				"XCnbrep7000063";
	x.avs_fs_opendir =				"XCnbrep700005c";
	x.avs_fs_closedir =				"XCnbrep700005e";
	x.avs_fs_readdir =				"XCnbrep700005d";
	x.mdigest_create =				"XCnbrep700013f";
	x.mdigest_update =				"XCnbrep7000141";
	x.mdigest_finish =				"XCnbrep7000142";
	x.mdigest_destroy =				"XCnbrep7000143";
	x.property_read_query_memsize = "XCnbrep70000b0";
	x.property_create =				"XCnbrep7000090";
	x.property_insert_read =		"XCnbrep7000094";
	x.property_search =				"XCnbrep70000a1";
	x.property_node_clone =			"XCnbrep70000a4";
	x.property_node_create =		"XCnbrep70000a2";
	x.property_desc_to_buffer =		"XCnbrep7000092";
	x.property_node_read =			"XCnbrep70000ab";
	x.property_node_write =			"XCnbrep70000ac";
	x.property_node_type =			"XCnbrep70000a8";
	x.property_node_name =			"XCnbrep70000a7";
	x.property_node_datasize =		"XCnbrep70000aa";
	x.property_file_write =			"XCnbrep70000b6";
	x.property_destroy =			"XCnbrep7000091";
	x.property_node_traversal =		"XCnbrep70000a6";
	x.property_node_query_stat =	"XCnbrep70000c5";
	x.cstream_create =				"XCnbrep7000130";
	x.cstream_operate =				"XCnbrep7000132";
	x.cstream_finish =				"XCnbrep7000133";
	x.cstream_destroy =				"XCnbrep7000134";
	return x;
	}(),
	[&] { avs_exports_t x = { 0 }; // IIDX
	x.version_name =				"iidx";
	x.dll_name =					L"libavs-win32.dll";
	x.unique_check =				NULL;
	x.avs_fs_open =					"XCnbrep7000039";
	x.avs_fs_close =				"XCnbrep7000040";
	x.avs_fs_read =					"XCnbrep700003c";
	x.avs_fs_lseek =				"XCnbrep700003a";
	x.avs_fs_lstat =				"XCnbrep700004e";
	x.avs_fs_opendir =				"XCnbrep7000047";
	x.avs_fs_closedir =				"XCnbrep7000049";
	x.avs_fs_readdir =				"XCnbrep7000048";
	x.mdigest_create =				"XCnbrep7000133";
	x.mdigest_update =				"XCnbrep7000135";
	x.mdigest_finish =				"XCnbrep7000136";
	x.mdigest_destroy =				"XCnbrep7000137";
	x.property_read_query_memsize = "XCnbrep700009b";
	x.property_create =				"XCnbrep700007b";
	x.property_insert_read =		"XCnbrep700007f";
	x.property_search =				"XCnbrep700008c";
	x.property_node_clone =			"XCnbrep700008f";
	x.property_node_create =		"XCnbrep700008d";
	x.property_desc_to_buffer =		"XCnbrep700007d";
	x.property_node_read =			"XCnbrep7000096";
	x.property_node_write =			"XCnbrep7000097";
	x.property_node_type =			"XCnbrep7000093";
	x.property_node_name =			"XCnbrep7000092";
	x.property_node_datasize =		"XCnbrep7000095";
	x.property_file_write =			"XCnbrep70000a1";
	x.property_destroy =			"XCnbrep700007c";
	x.property_node_traversal =		"XCnbrep7000091";
	x.property_node_query_stat =	"XCnbrep70000b0";
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

#ifdef _DEBUG
	for (int i = 0; i < lenof(avs_exports); i++) {
#undef X
#define X(ret_type, name, ...) if(avs_exports[i]. ## name == NULL) logf("MISSING EXPORT %d: %s", i, #name);
		AVS_FUNC_LIST
	}
#endif

	for (int i = 0; i < lenof(avs_exports); i++) {
		auto mod_handle = GetModuleHandle(avs_exports[i].dll_name);
		CHECK_UNIQUE(unique_check);
		TEST_HOOK_AND_APPLY(avs_fs_open);
		TEST_HOOK_AND_APPLY(avs_fs_lstat);
		//TEST_HOOK_AND_APPLY(avs_fs_mount);
		LOAD_FUNC(avs_fs_close);
		LOAD_FUNC(avs_fs_read);
		LOAD_FUNC(avs_fs_lseek);

		LOAD_FUNC(avs_fs_opendir);
		LOAD_FUNC(avs_fs_readdir);
		LOAD_FUNC(avs_fs_closedir);

		LOAD_FUNC(mdigest_create);
		LOAD_FUNC(mdigest_update);
		LOAD_FUNC(mdigest_finish);
		LOAD_FUNC(mdigest_destroy);

		LOAD_FUNC(property_read_query_memsize);
		LOAD_FUNC(property_create);
		LOAD_FUNC(property_desc_to_buffer);
		LOAD_FUNC(property_insert_read);
		LOAD_FUNC(property_search);
		LOAD_FUNC(property_node_clone);
		LOAD_FUNC(property_node_create);
		LOAD_FUNC(property_node_read);
		LOAD_FUNC(property_node_write);
		LOAD_FUNC(property_file_write);
		LOAD_FUNC(property_destroy);
		LOAD_FUNC(property_node_traversal);
		LOAD_FUNC(property_node_query_stat);
		LOAD_FUNC(property_node_type);

		LOAD_FUNC(property_node_name);
		LOAD_FUNC(property_node_datasize);

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

property_t prop_from_file(string const&path) {
	void* prop_buffer = NULL;
	property_t prop = NULL;

	auto f = avs_fs_open(path.c_str(), 1, 420);
	if (f < 0)
		return NULL;

	auto memsize = property_read_query_memsize(avs_fs_read, f, NULL, NULL);
	if (memsize < 0) {
		logf("Couldn't get memsize %08X", memsize);
		goto FAIL;
	}

	prop_buffer = malloc(memsize);
	prop = property_create(31, prop_buffer, memsize);
	if (!prop) {
		logf("Couldn't create prop");
		goto FAIL;
	}

	avs_fs_lseek(f, 0, SEEK_SET);
	property_insert_read(prop, 0, avs_fs_read, f);
	avs_fs_close(f);

	return prop;

FAIL:
	if (f)
		avs_fs_close(f);
	if (prop)
		property_destroy(prop);
	if (prop_buffer)
		free(prop_buffer);
	return NULL;
}

void prop_free(property_t prop) {
	if (!prop)
		return;
	auto buffer = property_desc_to_buffer(prop);
	property_destroy(prop);
	free(buffer);
}

void inline scratch_embiggen(int **scratch, int *scratch_len, node_t node) {
	auto size = property_node_datasize(node);
	if (size > *scratch_len) {
		*scratch = (int*)realloc(*scratch, size);
		*scratch_len = size;
	}
}

bool prop_node_merge_into(node_t dest, node_t src) {
	bool ret = true;
	int *scratch = NULL;
	int scratch_len = 0;
	char name[64];

	auto type = property_node_type(src);
	auto masked_type = type & 63;
	bool is_array = type & 64;

	property_node_name(src, name, sizeof(name));
	node_t dst_copy;

	if ((type & 63) == PROP_TYPE_node) {
		dst_copy = property_node_create(NULL, dest, type, name);
	} else {
		scratch_embiggen(&scratch, &scratch_len, src);
		property_node_read(src, type, scratch, scratch_len);
		if(property_node_datasize(src) <= 4 &&
			!is_array &&
			(masked_type < 10
			 || masked_type == PROP_TYPE_float
			 || masked_type == PROP_TYPE_bool))
			dst_copy = property_node_create(NULL, dest, type, name, *scratch);
		else
			dst_copy = property_node_create(NULL, dest, type, name, scratch);
	}
	if (!dst_copy) {
		logf("Can't copy %s node", name);
		ret = false;
	}

	// attributes
	for (auto attr = property_node_traversal(src, TRAVERSE_FIRST_ATTR);
		attr;
		attr = property_node_traversal(attr, TRAVERSE_NEXT_SIBLING)) {
		property_node_name(attr, name, sizeof(name));
		auto end = strlen(name);
		name[end] = '@';
		name[end + 1] = '\0';
		
		//type = property_node_type(attr);
		scratch_embiggen(&scratch, &scratch_len, attr);
		property_node_read(attr, PROP_TYPE_attr, scratch, scratch_len);
		auto r = property_node_create(NULL, dst_copy, PROP_TYPE_attr, name, scratch);
		if (!r) {
			logf("Can't copy %s node", name);
			ret = false;
		}
	}

	// child, he will do his own siblings
	auto child = property_node_traversal(src, TRAVERSE_FIRST_CHILD);
	if (child)
		ret = ret && prop_node_merge_into(dst_copy, child);

	// sibling
	auto sibling = property_node_traversal(src, TRAVERSE_NEXT_SIBLING);
	if (sibling)
		ret = ret && prop_node_merge_into(dest, sibling);

	if (scratch)
		free(scratch);
	return ret;
}

bool prop_merge_into(property_t dest_prop, property_t src_prop) {
	auto dest_root = property_search(dest_prop, NULL, "/");
	auto src_root = property_search(src_prop, NULL, "/");

	auto node = property_node_traversal(src_root, TRAVERSE_FIRST_CHILD);
	if(node)
		return prop_node_merge_into(dest_root, node);
	return false;
}

string md5_sum(const char* str) {
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