#include "tests.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk_storage.h"
#include "log.h"
#include "my_malloc.h"
#include "str_utils.h"

void write_data( struct disk_storage* store, size_t offset, int i )
{
	char buf[1024];
	sprintf( buf, "hello%d", i );
	assert( disk_storage_write_block(
				store, offset, DISK_STORAGE_BLOCK_STATUS_VALID, buf, strlen( buf ) ) == 0 );
}

void test_read_data( struct disk_storage* store, size_t offset, int i )
{
	int res;
	size_t n;
	uint8_t block_status;
	char buf[1024];
	char expected[1024];
	sprintf( expected, "hello%d", i );
	res = disk_storage_read_block( store, offset, &block_status, buf, &n, 1024 );
	assert( res == 0 );
	assert( n >= strlen( expected ) );
	assert( strncmp( buf, expected, strlen( expected ) ) == 0 );
}

void test_disk_storage( void )
{
	my_malloc_init();

	int res;
	char root_path[512];
	strcpy( root_path, "/tmp/menoetius-test-XXXXXX" );
	assert( mkdtemp( root_path ) );

	printf( "working out of %s\n", root_path );

	char path[1024];
	snprintf( path, 1024, "%s/test", root_path );

	struct disk_storage store;
	size_t block_size = 512;
	res = disk_storage_open( &store, path );
	CU_ASSERT( res == 0 );

	size_t offset[10];

	CU_ASSERT( disk_storage_reserve_block( &store, block_size, &offset[0] ) == 0 );
	write_data( &store, offset[0], 0 );

	CU_ASSERT( disk_storage_reserve_block( &store, block_size, &offset[1] ) == 0 );
	write_data( &store, offset[1], 1 );

	CU_ASSERT( disk_storage_reserve_block( &store, block_size, &offset[2] ) == 0 );
	write_data( &store, offset[2], 2 );

	// free write item 1
	disk_storage_release_block( &store, offset[1] );

	// test that released block is reused
	CU_ASSERT( disk_storage_reserve_block( &store, block_size, &offset[3] ) == 0 );
	write_data( &store, offset[3], 3 );

	CU_ASSERT( offset[1] == offset[3] );

	// test some reads
	test_read_data( &store, offset[0], 0 );
	// offset[1] was freed (and now points to 3)
	test_read_data( &store, offset[2], 2 );
	test_read_data( &store, offset[3], 3 );

	///////////////////////////////////////////////////////////////////////////////////
	// clean up
	char cleanup[1024];
	snprintf( cleanup, 1024, "rm -rf %s", root_path );

	// double check we're removing the correct path
	assert( str_has_prefix( cleanup, "rm -rf /tmp/menoetius-test" ) );
	printf( "running command: %s\n", cleanup );
	res = system( cleanup );
	if( res != 0 ) {
		printf( "failed to clean up; exit(%d) returned by rm command\n", res );
	}

	disk_storage_close( &store );

	my_malloc_assert_free();
	my_malloc_free();
}
