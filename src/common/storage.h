#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <openssl/sha.h>

//#include "sglib.h"

void test_tree( void );

#define INTERNAL_ID_INDEX 0
#define LFM_INDEX 1
#define NAME_INDEX 2
#define NUM_SPECIAL_INDICES 3

struct disk_storage;

struct offset_and_time
{
	uint64_t offset;
	int64_t start_time;
};

typedef struct storage_item
{
	// used for pagination; this ID will NOT be constant between replication pairs
	uint64_t internal_id;

	uint16_t key_len;
	char* key;
	struct tsc_header* data;

	// points to where the LFM->internal ID is stored on disk
	uint64_t key_storage_offset;

	// points to all disk-locations of data
	int num_data_offsets; // num of valid elements
	int cap_data_offsets; // available capacity
	struct offset_and_time* data_offsets;

	// for RB trees (each element is a different tree keyed by different data)
	char* color_field;
	struct storage_item** left;
	struct storage_item** right;

	// name and user-defined indices
	const char** index_val;

	// doubly-linked list (used for full-scans + cleaning)
	struct storage_item* next;
	struct storage_item* prev;
} StorageItem;

int storage_item_cmp( const StorageItem* a, const StorageItem* b, int index );

struct sglib_StorageItem_iterator
{
	StorageItem* currentelem;
	char pass[128];
	StorageItem* path[128];
	short int pathi;
	short int order;
	StorageItem* equalto;
	int ( *subcomparator )( StorageItem*, StorageItem* );
};

extern void sglib_StorageItem_add( StorageItem** tree, StorageItem* elem, int index );
extern int sglib_StorageItem_add_if_not_member( StorageItem** tree,
												StorageItem* elem,
												StorageItem** memb,
												int index );

extern void sglib_StorageItem_delete( StorageItem** tree, StorageItem* elem, int index );
extern int sglib_StorageItem_delete_if_member( StorageItem** tree,
											   StorageItem* elem,
											   StorageItem** memb,
											   int index );

extern int sglib_StorageItem_is_member( StorageItem* t, StorageItem* elem, int index );
extern StorageItem* sglib_StorageItem_find_member( StorageItem* t, StorageItem* elem, int index );

extern StorageItem*
sglib_StorageItem_find_member_or_nearest_lesser( StorageItem* t, StorageItem* elem, int index );

extern void sglib___StorageItem_consistency_check( StorageItem* t, int index );
extern int sglib_StorageItem_len( StorageItem* t, int index );

extern StorageItem*
sglib_StorageItem_it_init( struct sglib_StorageItem_iterator* it, StorageItem* tree, int index );
extern StorageItem* sglib_StorageItem_it_init_preorder( struct sglib_StorageItem_iterator* it,
														StorageItem* tree,
														int index );
extern StorageItem* sglib_StorageItem_it_init_inorder( struct sglib_StorageItem_iterator* it,
													   StorageItem* tree,
													   int index );
extern StorageItem* sglib_StorageItem_it_init_postorder( struct sglib_StorageItem_iterator* it,
														 StorageItem* tree,
														 int index );
extern StorageItem* sglib_StorageItem_it_init_on_equal( struct sglib_StorageItem_iterator* it,
														StorageItem* tree,
														int ( *subcomparator )( StorageItem*,
																				StorageItem* ),
														StorageItem* equalto,
														int index );
extern StorageItem* sglib_StorageItem_it_current( struct sglib_StorageItem_iterator* it,
												  int index );
extern StorageItem* sglib_StorageItem_it_next( struct sglib_StorageItem_iterator* it, int index );
;

struct the_storage
{
	size_t initial_tsc_buf_size;

	pthread_mutex_t lock;
	pthread_t trimmer_thread;
	int retention_seconds;
	int target_cleaning_loop_microseconds;
	int cleaning_sleep_multiplier;
	int max_duration;
	int64_t next_internal_id;

	StorageItem* list_root;
	StorageItem* list_root_last;

	// LFM tree
	int num_indices; // 1 + <num of user defined>
	const char** indices; // 0 = name, 1.. = user defined
	StorageItem** root; // 0 = lfm; 1 = name; 2.. = user defined

	struct disk_storage* key_disk_store;
	struct disk_storage* data_disk_store;

	// cluster config
	char cluster_config_hash[SHA_DIGEST_LENGTH];
	char* cluster_config;
	char* cluster_config_path;
};

int storage_init( struct the_storage* store,
				  int retention_seconds,
				  int cleaning_sleep,
				  size_t initial_tsc_buf_size,
				  int num_indices,
				  const char** indices,
				  const char* disk_storage_path );

void storage_free( struct the_storage* store );

int storage_start_thread( struct the_storage* store );

int storage_join_thread( struct the_storage* store );

int storage_write( struct the_storage* store,
				   const char* key,
				   uint16_t key_len,
				   int64_t* t,
				   double* y,
				   size_t num_pts );

int storage_get( struct the_storage* store,
				 const char* key,
				 uint16_t key_len,
				 int64_t* t,
				 double* y,
				 size_t max_num_pts,
				 size_t* num_pts );

int storage_lookup_without_index( struct the_storage* store,
								  const char* matchers,
								  int num_matchers,
								  char* results,
								  size_t* results_n,
								  size_t max_results_size,
								  uint64_t* last_internal_id,
								  uint64_t limit );

int save_cluster_config_to_disk( struct the_storage* store );

// when deadlock = true, the lock will never be released, thus preventing future writes
// this is used when shutting down the server to ensure no future writes are acked back to the client.
// TODO: ideally we would disconnect all clients first and stop accepting connections before syncing,
// but this was easier.
int storage_sync( struct the_storage* store, bool deadlock );

int storage_lookup( struct the_storage* store,
					int index_num,
					const char* label_value,
					const char* extra_matchers,
					int num_extra_matchers,
					char* results,
					size_t* results_n,
					size_t max_results_size,
					uint64_t offset,
					uint64_t limit );

const char* get_key_value( const char* lfm, size_t lfm_len, const char* key );

void clean_items( struct the_storage* store );

// ROUND_UP to nearest 8
#define ROUND_UP( x ) ( ( ( x ) + 7 ) & ( ~( 7 ) ) )
