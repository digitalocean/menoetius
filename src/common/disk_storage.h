#pragma once

#ifndef _LARGEFILE64_SOURCE
#	define _LARGEFILE64_SOURCE
#endif // _LARGEFILE64_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// On disk format:
//  - stream of blocks
//  - each block is a multiple of 1MB
//     - read the following at each block offset:
//       - uint8; version (should be 0)
//       - uint8; block size (in MB)
//       - uint16; num-elements in block
//         user must then calculate element size as ROUND_DOWN(block_size*1024*1024/num_elements)
//     - each elmenent in block:
//       - uint8; 0 = free/invalid data; 1 = in-use/valid data
//       - remaining data is left to the user to do whatever with.
//

#define NUM_DISK_POOLS 20
#define MIN_DISK_POW_TWO 7 // round all sizes to 2^X

// empty (data is invalid)
#define DISK_STORAGE_BLOCK_STATUS_EMPTY 0

// reserved, but never written to, data is invalid
#define DISK_STORAGE_BLOCK_STATUS_RESERVED 1

// reserved and contains valid data (either a key, or fully-compressed tsc data)
#define DISK_STORAGE_BLOCK_STATUS_VALID 2

// reserved and contains valid but not-fully-compressed tsc data
#define DISK_STORAGE_BLOCK_STATUS_ACTIVE 3

// this is used as an upperbound, if this value or anything higher is found
// then the data has been corrupted.
#define DISK_STORAGE_BLOCK_NUM_STATUSES 4

typedef struct disk_storage
{
	int fd;
	int scratch_fd;
	char* path;
	char* scratch_path;
	size_t eof_offset;
	size_t next_free_block[NUM_DISK_POOLS]; // when 0, that signals none are avilable; 0 would point
		// to begining of file which stores the version number
} DiskStorage;

int disk_storage_open( struct disk_storage* store, const char* path );

void disk_storage_close( struct disk_storage* store );

int disk_storage_read_blocks_from_start( struct disk_storage* store,
										 bool initial,
										 size_t max_n,
										 char* buf,
										 size_t* n,
										 size_t* offset,
										 uint8_t* block_status );

int disk_storage_write_block(
	struct disk_storage* store, size_t offset, uint8_t block_status, char* buf, size_t n );

int disk_storage_read_block( struct disk_storage* store,
							 size_t offset,
							 uint8_t* block_status,
							 char* buf,
							 size_t* n,
							 size_t max_n );

int disk_storage_reserve_and_write_block(
	struct disk_storage* store, uint8_t block_status, char* buf, size_t n, size_t* offset );

int disk_storage_reserve_block( struct disk_storage* store, size_t n, size_t* offset );

int disk_storage_release_block( struct disk_storage* store, size_t offset );
