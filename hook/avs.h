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
	PROP_TYPE_4u16 = 39,
	PROP_TYPE_ATTR = 46,
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

// file functions
extern AVS_FILE(*avs_fs_open)(const char* name, uint16_t mode, int flags);
extern void(*avs_fs_close)(AVS_FILE f);
extern int(*avs_fs_fstat)(AVS_FILE f, struct avs_stat *st);
extern avs_reader_t avs_fs_read;
extern int(*avs_fs_lseek)(AVS_FILE f, long int offset, int origin);

// property handling
extern int32_t(*property_read_query_memsize)(avs_reader_t reader, AVS_FILE f, int* unk0, int* unk1);
extern property_t(*property_create)(int flags, void *buffer, uint32_t buffer_size);
extern int(*property_insert_read)(property_t prop, int unk0, avs_reader_t reader, AVS_FILE f);
extern property_t(*property_search)(property_t prop, node_t node, const char *path);
extern void(*property_node_read)(node_t node, property_type type, void* out, uint32_t out_size);
extern void(*property_destroy)(property_t prop);
extern node_t(*property_node_traversal)(node_t node, int flags);

// md5sum (and sha1 if needed)
extern mdigest_p(*mdigest_create)(mdigest_algs_t algorithm);
extern void(*mdigest_update)(mdigest_p digest, char* data, int size);
extern void(*mdigest_finish)(mdigest_p digest, uint8_t* hash, int size);
extern void(*mdigest_destroy)(mdigest_p digest);

// compression
extern cstream_t*(*cstream_create)(compression_type type);
extern cstream_t*(*cstream_create)(compression_type type);
extern bool(*cstream_operate)(cstream_t* compressor);
extern bool(*cstream_finish)(cstream_t* compressor);
extern bool(*cstream_destroy)(cstream_t* compressor);

string md5_sum(char* str);
bool init_avs(void);
unsigned char* lz_compress(unsigned char* input, size_t length, size_t *compressed_length);