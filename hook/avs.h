#pragma once

#include <stdint.h>

#include <string>
using std::string;

/*
For a random texture xml
property_read_query_memsize: 3856
property_node_query_stat: 3856
property_query_size: 1519
property_node_datasize(root node)  0
*/

typedef void* mdigest_p;
enum mdigest_algs_t {
	MD5 = 0,
};

struct avs_stat {
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
	// not actually sure how big theirs is
	struct stat padding;
};

typedef struct {
	unsigned char* output_buffer;
	unsigned char* input_buffer;
	uint32_t output_size;
	uint32_t input_size;
} cstream_t;

// want to add more? use kbinxml repo
enum property_type {
	PROP_TYPE_node = 1,
	PROP_TYPE_s8 = 2,
	PROP_TYPE_u8 = 3,
	PROP_TYPE_s16 = 4,
	PROP_TYPE_u16 = 5,
	PROP_TYPE_s32 = 6,
	PROP_TYPE_u32 = 7,
	PROP_TYPE_s64 = 8,
	PROP_TYPE_u64 = 9,
	PROP_TYPE_bin = 10,
	PROP_TYPE_str = 11,
	PROP_TYPE_ip4 = 12,
	PROP_TYPE_time = 13,
	PROP_TYPE_float = 14,
	PROP_TYPE_double = 15,
	PROP_TYPE_2s8 = 16,
	PROP_TYPE_2u8 = 17,
	PROP_TYPE_2s16 = 18,
	PROP_TYPE_2u16 = 19,
	PROP_TYPE_2s32 = 20,
	PROP_TYPE_2u32 = 21,
	PROP_TYPE_2s64 = 22,
	PROP_TYPE_2u64 = 23,
	PROP_TYPE_2f = 24,
	PROP_TYPE_2d = 25,
	PROP_TYPE_3s8 = 26,
	PROP_TYPE_3u8 = 27,
	PROP_TYPE_3s16 = 28,
	PROP_TYPE_3u16 = 29,
	PROP_TYPE_3s32 = 30,
	PROP_TYPE_3u32 = 31,
	PROP_TYPE_3s64 = 32,
	PROP_TYPE_3u64 = 33,
	PROP_TYPE_3f = 34,
	PROP_TYPE_3d = 35,
	PROP_TYPE_4s8 = 36,
	PROP_TYPE_4u8 = 37,
	PROP_TYPE_4s16 = 38,
	PROP_TYPE_4u16 = 39,
	PROP_TYPE_4s32 = 40,
	PROP_TYPE_4u32 = 41,
	PROP_TYPE_4s64 = 42,
	PROP_TYPE_4u64 = 43,
	PROP_TYPE_4f = 44,
	PROP_TYPE_4d = 45,
	PROP_TYPE_attr = 46,
	PROP_TYPE_attr_and_node = 47,
	PROP_TYPE_vs8 = 48,
	PROP_TYPE_vu8 = 49,
	PROP_TYPE_vs16 = 50,
	PROP_TYPE_vu16 = 51,
	PROP_TYPE_bool = 52,
	PROP_TYPE_2b = 53,
	PROP_TYPE_3b = 54,
	PROP_TYPE_4b = 55,
	PROP_TYPE_vb = 56,
};

enum prop_traverse_option {
	TRAVERSE_PARENT = 0,
	TRAVERSE_FIRST_CHILD = 1,
	TRAVERSE_FIRST_ATTR = 2,
	TRAVERSE_FIRST_SIBLING = 3,
	TRAVERSE_NEXT_SIBLING = 4,
	TRAVERSE_PREVIOUS_SIBLING = 5,
	TRAVERSE_LAST_SIBLING = 6,
	TRAVERSE_NEXT_SEARCH_RESULT = 7,
	TRAVERSE_PREV_SEARCH_RESULT = 8
};

enum prop_create_flag {
	PROP_XML                  = 0x000,
	PROP_READ                 = 0x001,
	PROP_WRITE                = 0x002,
	PROP_CREATE               = 0x004,
	PROP_BINARY               = 0x008,
	PROP_APPEND               = 0x010,
	PROP_DEBUG_VERBOSE        = 0x400,
	PROP_JSON                 = 0x800,
	PROP_BIN_PLAIN_NODE_NAMES = 0x1000,
};

// READ/WRITE/CREATE/APPEND : obvious
// BINARY: smaller file size in cache
// PLAIN_NODE_NAMES: lets us exceed 65535 node limit (SDVX breaches this)
const int PROP_CREATE_FLAGS = (PROP_READ | PROP_WRITE | PROP_CREATE | PROP_APPEND | PROP_BINARY | PROP_BIN_PLAIN_NODE_NAMES);

// for cstream_*
enum compression_type {
	AVS_DECOMPRESS_AVSLZ = 0,
	AVS_COMPRESS_AVSLZ = 1,
};

struct node_size {
	int nodes;
	int data;
	int unk1, unk2, unk3;
};

struct property_info {
	uint8_t blah[560];
	uint32_t error_code;
	uint32_t has_error;
	uint32_t unk;
	int8_t buffer_offset;
};

/*struct node_info { 
	DWORD dword0;
	BYTE gap4[8];
	unsigned __int16 unsignedC;
	unsigned __int16 unsignedE;
	unsigned __int16 unsigned10;
	unsigned __int16 unsigned12;
	unsigned __int16 unsigned14;
	unsigned __int16 unsigned16;
	DWORD dword18;
	__declspec(align(8)) unsigned __int16 unsigned20;
	unsigned __int16 unsigned22;
	unsigned __int16 unsigned24;
	unsigned __int16 unsigned26;
	unsigned __int16 unsigned28;
	unsigned __int16 unsigned2A;
	BYTE gap2C[3];
	BYTE byte2F;
};*/

struct node_info {
	uint8_t blah[47];
	property_type type;
};

typedef property_info* property_t;
typedef node_info* node_t;
typedef int32_t AVS_FILE;

typedef size_t(*avs_reader_t)(AVS_FILE context, void *bytes, size_t nbytes);

#define ELLIPSIS ...
// because the functions are repeated in 3 places, we format them with an X macro
// return value, name, va args...
#define AVS_FUNC_LIST \
/* file functions */ \
X(AVS_FILE,   avs_fs_open, const char* name, uint16_t mode, int flags) \
X(void,       avs_fs_close, AVS_FILE f) \
/*X(int,        avs_fs_fstat, AVS_FILE f, struct avs_stat *st)*/ \
X(int,        avs_fs_lstat, const char* path, struct avs_stat *st) \
X(int,        avs_fs_lseek, AVS_FILE f, long int offset, int origin) \
X(size_t,     avs_fs_read, AVS_FILE context, void *bytes, size_t nbytes) \
/*X(AVS_FILE,   avs_fs_opendir, const char* name)*/ \
/*X(int,        avs_fs_closedir, AVS_FILE f)*/ \
/*X(char*,      avs_fs_readdir, AVS_FILE f) */\
/* property handling */ \
/* property_read_query_memsize has a limit of 65535 nodes, which SDVX breaches. we must use the plain names (which requires memsize_long) */ \
/*X(int32_t,    property_read_query_memsize, avs_reader_t reader, AVS_FILE f, int* unk0, int* unk1)*/ \
X(int32_t,    property_read_query_memsize_long, avs_reader_t reader, AVS_FILE f, int* unk0, int* unk1, int* unk2) \
X(property_t, property_create, int flags, void *buffer, uint32_t buffer_size) \
X(void*,      property_desc_to_buffer, property_t prop) \
X(int,        property_insert_read, property_t prop, node_t node, avs_reader_t reader, AVS_FILE f) \
X(node_t,     property_search, property_t prop, node_t node, const char *path) \
X(bool,       property_node_clone, property_t dest_prop, node_t dest_node, node_t src_node, bool deep) \
X(node_t,     property_node_create, property_t prop, node_t node, property_type type, char* path, ELLIPSIS) \
X(int,        property_node_name, node_t node, char* buff, int buff_len) \
X(int,        property_node_datasize, node_t node) \
X(void,       property_node_read, node_t node, property_type type, void* out, uint32_t out_size) \
X(uint32_t,   property_node_write, node_t node, property_type type, void* in) \
X(property_type, property_node_type, node_t node) \
X(int,        property_file_write, property_t prop, const char* out_path) \
X(void,       property_destroy, property_t prop) \
X(node_t,     property_node_traversal, node_t node, int flags) \
X(int,        property_node_query_stat, property_t prop, node_t node, struct node_size *size) \
/*X(int,      property_query_size, property_t prop) unused */ \
/* md5sum *sha1 if needed) */ \
X(mdigest_p,  mdigest_create, mdigest_algs_t algorithm) \
X(void,       mdigest_update, mdigest_p digest, const char* data, int size) \
X(void,       mdigest_finish, mdigest_p digest, uint8_t* hash, int size) \
X(void,       mdigest_destroy, mdigest_p digest) \
/* compression */ \
X(cstream_t*, cstream_create, compression_type type) \
X(bool,       cstream_operate, cstream_t* compressor) \
X(bool,       cstream_finish, cstream_t* compressor) \
X(bool,       cstream_destroy, cstream_t* compressor) \

#undef X
#define X(ret_type, name, ...) extern ret_type (* name )( __VA_ARGS__ );
AVS_FUNC_LIST

void prop_free(property_t prop);
property_t prop_from_file(string const&path);
string md5_sum(const char* str);
bool init_avs(void);
unsigned char* lz_compress(unsigned char* input, size_t length, size_t *compressed_length);

bool prop_merge_into(property_t dest_prop, property_t src_prop);
const char* get_prop_error_str(int32_t code);