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
	//x.avs_fs_opendir =				"avs_fs_opendir";
	//x.avs_fs_closedir =				"avs_fs_closedir";
	//x.avs_fs_readdir =				"avs_fs_readdir";
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
	//x.avs_fs_opendir =				"XCd229cc0000f0";
	//x.avs_fs_closedir =				"XCd229cc0000b8";
	//x.avs_fs_readdir =				"XCd229cc0000bb";
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
	//x.avs_fs_opendir =				"XCnbrep700005c";
	//x.avs_fs_closedir =				"XCnbrep700005e";
	//x.avs_fs_readdir =				"XCnbrep700005d";
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
	//x.avs_fs_opendir =				"XCnbrep7000047";
	//x.avs_fs_closedir =				"XCnbrep7000049";
	//x.avs_fs_readdir =				"XCnbrep7000048";
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
	[&] { avs_exports_t x = { 0 }; // avs 64 bit, pretty much
	x.version_name =				"museca/IIDX 25+";
	x.dll_name =					L"avs2-core.dll";
	x.unique_check =				"XCgsqzn000013c";
	x.avs_fs_open =					"XCgsqzn000004e";
	x.avs_fs_close =				"XCgsqzn0000055";
	x.avs_fs_read =					"XCgsqzn0000051";
	x.avs_fs_lseek =				"XCgsqzn000004f";
	x.avs_fs_lstat =				"XCgsqzn0000063";
	//x.avs_fs_opendir =				"XCgsqzn000005c";
	//x.avs_fs_closedir =				"XCgsqzn000005e";
	//x.avs_fs_readdir =				"XCgsqzn000005d";
	x.mdigest_create =				"XCgsqzn000013f";
	x.mdigest_update =				"XCgsqzn0000141";
	x.mdigest_finish =				"XCgsqzn0000142";
	x.mdigest_destroy =				"XCgsqzn0000143";
	x.property_read_query_memsize = "XCgsqzn00000b0";
	x.property_create =				"XCgsqzn0000090";
	x.property_insert_read =		"XCgsqzn0000094";
	x.property_search =				"XCgsqzn00000a1";
	x.property_node_clone =			"XCgsqzn00000a4";
	x.property_node_create =		"XCgsqzn00000a2";
	x.property_desc_to_buffer =		"XCgsqzn0000092";
	x.property_node_read =			"XCgsqzn00000ab";
	x.property_node_write =			"XCgsqzn00000ac";
	x.property_node_type =			"XCgsqzn00000a8";
	x.property_node_name =			"XCgsqzn00000a7";
	x.property_node_datasize =		"XCgsqzn00000aa";
	x.property_file_write =			"XCgsqzn00000b6";
	x.property_destroy =			"XCgsqzn0000091";
	x.property_node_traversal =		"XCgsqzn00000a6";
	x.property_node_query_stat =	"XCgsqzn00000c5";
	x.cstream_create =				"XCgsqzn0000130";
	x.cstream_operate =				"XCgsqzn0000132";
	x.cstream_finish =				"XCgsqzn0000133";
	x.cstream_destroy =				"XCgsqzn0000134";
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
		// make sure this is the right DLL
		CHECK_UNIQUE(unique_check);

		// load all our imports, fail if any cannot be found
#undef X
#define X(ret_type, name, ...) LOAD_FUNC(name);
		AVS_FUNC_LIST

		// apply hooks
		TEST_HOOK_AND_APPLY(avs_fs_open);
		TEST_HOOK_AND_APPLY(avs_fs_lstat);

		success = true;
		logf("Detected dll: %s", avs_exports[i].version_name);
		break;
	}
	return success;
}

property_t prop_from_file(string const&path) {
	void* prop_buffer = NULL;
	property_t prop = NULL;
    AVS_FILE f = 0;

	f = avs_fs_open(path.c_str(), 1, 420);
	if (f < 0) {
        logf("Couldn't open prop");
		goto FAIL;
    }

	auto memsize = property_read_query_memsize(avs_fs_read, f, NULL, NULL);
	if (memsize < 0) {
		logf("Couldn't get memsize %08X (%s)", memsize, get_prop_error_str(memsize));
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
	mdigest_update(digest, str, (int)strlen(str));
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
	compressor->input_size = (uint32_t)length;
	// worst case, for every 8 bytes there will be an extra flag byte
    auto to_add = max(length / 8, 1);
	auto compress_size = length + to_add;
	auto compress_buffer = (unsigned char*)malloc(compress_size);
	compressor->output_buffer = compress_buffer;
	compressor->output_size = (uint32_t)compress_size;
	
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

typedef struct {
	uint32_t code;
	char* msg;
} prop_error_info_t;

const prop_error_info_t prop_error_list[73] = {
	{ 0x80092000, "invalid type" },
	{ 0x80092001, "type cannot use as array" },
	{ 0x80092002, "invalid" },
	{ 0x80092003, "too large data size" },
	{ 0x80092004, "too small buffer size" },
	{ 0x80092005, "passcode 0 is not allowed" },
	{ 0x80092040, "invalid node name" },
	{ 0x80092041, "invalid attribute name" },
	{ 0x80092042, "reserved attribute name" },
	{ 0x80092043, "cannot find node/attribute" },
	{ 0x80092080, "cannot allocate node" },
	{ 0x80092081, "cannot allocate node value" },
	{ 0x80092082, "cannot allocate mdigest for finger-print" },
	{ 0x80092083, "cannot allocate nodename" },
	{ 0x800920C0, "node type differs" },
	{ 0x800920C1, "node type is VOID" },
	{ 0x800920C2, "node is array" },
	{ 0x800920C3, "node is not array" },
	{ 0x80092100, "node is create-disabled" },
	{ 0x80092101, "node is read-disabled" },
	{ 0x80092102, "node is write-disabled" },
	{ 0x80092103, "flag is already locked" },
	{ 0x80092104, "passcode differs" },
	{ 0x80092105, "insert_read() is applied to attribute" },
	{ 0x80092106, "part_write() is applied to attribute" },
	{ 0x80092107, "MODE_EXTEND flag differs" },
	{ 0x80092140, "root node already exists" },
	{ 0x80092141, "attribute cannot have children" },
	{ 0x80092142, "node/attribute already exists" },
	{ 0x80092143, "number of nodes exceeds 65535" },
	{ 0x80092144, "cannot interpret as number" },
	{ 0x80092145, "property is empty" },
	{ 0x80092180, "I/O error" },
	{ 0x80092181, "unexpected EOF" },
	{ 0x80092182, "unknown format" },
	{ 0x800921C0, "broken magic" },
	{ 0x800921C1, "broken metadata" },
	{ 0x800921C2, "broken databody" },
	{ 0x800921C3, "invalid type" },
	{ 0x800921C4, "too large data size" },
	{ 0x800921C5, "too long node/attribute name" },
	{ 0x800921C6, "attribute name is too long" },
	{ 0x800921C7, "node/attribute already exists" },
	{ 0x80092200, "invalid encoding " },
	{ 0x80092201, "invalid XML token" },
	{ 0x80092202, "XML syntax error" },
	{ 0x80092203, "STag/ETag mismatch" },
	{ 0x80092204, "too large node data" },
	{ 0x80092205, "too deep node tree" },
	{ 0x80092206, "invalid type" },
	{ 0x80092207, "invalid size" },
	{ 0x80092208, "invalid count" },
	{ 0x80092209, "invalid value" },
	{ 0x8009220A, "invalid node name" },
	{ 0x8009220B, "invalid attribute name" },
	{ 0x8009220C, "reserved attribute name" },
	{ 0x8009220D, "node/attribute already exists" },
	{ 0x8009220E, "too many elements in node data" },
	{ 0x80092240, "JSON syntax error" },
	{ 0x80092241, "invalid JSON literal" },
	{ 0x80092242, "invalid JSON number" },
	{ 0x80092243, "invalid JSON string" },
	{ 0x80092244, "invalid JSON object name" },
	{ 0x80092245, "object name already exists" },
	{ 0x80092246, "too long JSON object name" },
	{ 0x80092247, "too deep JSON object/array nesting" },
	{ 0x80092248, "cannot convert JSON array to property" },
	{ 0x80092249, "cannot convert empty JSON object to property" },
	{ 0x8009224A, "root node already exists" },
	{ 0x8009224B, "cannot convert root node to TYPE_ARRAY" },
	{ 0x8009224C, "name represents reserved attribute" },
	{ 0x80092280, "finger-print differs" },
	{ 0x800922C0, "operation is not supported" },
};

const char* get_prop_error_str(int32_t code) {
	for (int i = 0; i < lenof(prop_error_list); i++) {
		if (prop_error_list[i].code == code)
			return prop_error_list[i].msg;
	}
	return "unknown";
}