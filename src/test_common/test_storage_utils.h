#pragma once

#define MAX_TEST_STORAGE_PATH 1024

struct disk_storage;

void init_test_storage( const char* name, struct disk_storage** store );

void destroy_test_storage( struct disk_storage* store );

const char* get_test_storage_path( void );

void delete_test_storage( void );
