#include "test_storage_utils.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disk_storage.h"
#include "log.h"
#include "my_malloc.h"
#include "str_utils.h"

void delete_tmp_dir( void )
{
	int res;
	char cleanup[1024];
	snprintf( cleanup, 1024, "rm -rf /tmp/menoetius-test-will-be-deleted*" );

	// double check we're removing the correct path
	assert( str_has_prefix( cleanup, "rm -rf /tmp/menoetius-test" ) );
	printf( "running command: %s\n", cleanup );
	res = system( cleanup );
	if( res != 0 ) {
		printf( "failed to clean up; exit(%d) returned by rm command\n", res );
	}
}

void init_test_storage( const char* name, struct disk_storage** store )
{
	int res;
	char root_path[512];
	sprintf( root_path, "/tmp/menoetius-test-will-be-deleted-XXXXXX" );
	assert( mkdtemp( root_path ) );
	printf( "working out of %s\n", root_path );

	char path[1024];
	snprintf( path, 1024, "%s/%s", root_path, name );

	*store = my_malloc( sizeof( struct disk_storage ) );

	res = disk_storage_open( *store, path );
	assert( res == 0 );
}

void destroy_test_storage( struct disk_storage* store )
{
	delete_tmp_dir();
	my_free( store );
}

char root_path[512];
const char* get_test_storage_path( void )
{
	sprintf( root_path, "/tmp/menoetius-test-will-be-deleted-XXXXXX" );
	assert( mkdtemp( root_path ) );
	printf( "working out of %s\n", root_path );
	return root_path;
}

void delete_test_storage( void )
{
	delete_tmp_dir();
}
