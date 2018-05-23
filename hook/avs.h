#pragma once

#include <stdint.h>

#include <string>
using std::string;

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
	size_t output_size;
	size_t input_size;
} cstream_t;

// want to add more? use kbinxml repo
enum property_type {
	PROP_TYPE_void = 1,
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
	PROP_TYPE_ATTR = 46,
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

// for cstream_*
enum compression_type {
	AVS_DECOMPRESS_AVSLZ = 0,
	AVS_COMPRESS_AVSLZ = 1,
};

typedef void* property_t;
typedef void* node_t;
typedef int32_t AVS_FILE;

typedef int(*avs_reader_t)(uint32_t* context, void *bytes, size_t nbytes);

// because the functions are repeated in 3 places, we format them with an X macro
// return value, name, va args...
#define AVS_FUNC_LIST \
/* file functions */ \
X(AVS_FILE,   avs_fs_open, const char* name, uint16_t mode, int flags) \
X(void,       avs_fs_close, AVS_FILE f) \
X(int,        avs_fs_fstat, AVS_FILE f, struct avs_stat *st) \
X(int,        avs_fs_lseek, AVS_FILE f, long int offset, int origin) \
X(int,        avs_fs_read, uint32_t* context, void *bytes, size_t nbytes) \
/* property handling */ \
X(int32_t,    property_read_query_memsize, avs_reader_t reader, AVS_FILE f, int* unk0, int* unk1) \
X(property_t, property_create, int flags, void *buffer, uint32_t buffer_size) \
X(int,        property_insert_read, property_t prop, int unk0, avs_reader_t reader, AVS_FILE f) \
X(property_t, property_search, property_t prop, node_t node, const char *path) \
X(void,       property_node_read, node_t node, property_type type, void* out, uint32_t out_size) \
X(uint32_t,   property_node_write, node_t node, property_type type, void* in) \
X(int,        property_file_write, property_t prop, const char* out_path) \
X(void,       property_destroy, property_t prop) \
X(node_t,     property_node_traversal, node_t node, int flags) \
/* md5sum *sha1 if needed) */ \
X(mdigest_p,  mdigest_create, mdigest_algs_t algorithm) \
X(void,       mdigest_update, mdigest_p digest, char* data, int size) \
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

string md5_sum(char* str);
bool init_avs(void);
unsigned char* lz_compress(unsigned char* input, size_t length, size_t *compressed_length);