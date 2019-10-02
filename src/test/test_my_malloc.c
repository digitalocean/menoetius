#include "tests.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "my_malloc.h"

void test_my_malloc( void )
{
	my_malloc_init();

	void* p = my_malloc( 10 );

	my_free( p );

	my_malloc_assert_free();
	my_malloc_free();
}

void test_my_malloc_2( void )
{
	my_malloc_init();

	void* p1 = my_malloc( 10 );
	void* p2 = my_malloc( 10 );

	my_free( p1 );

	void* p3 = my_malloc( 10 );

	assert( p3 == p1 );
	my_free( p2 );
	my_free( p3 );

	void* p4 = my_malloc( 10 );
	assert( p4 == p3 );

	void* p5 = my_malloc( 10 );
	assert( p5 == p2 );

	my_free( p4 );
	my_free( p5 );

	my_malloc_assert_free();
	my_malloc_free();
}

void test_my_realloc( void )
{
	my_malloc_init();

	void* p = my_malloc( 10 );
	strcpy( p, "hello" );

	void* q = my_realloc( p, 1024 * 50 );
	assert( p != q );
	assert( strcmp( q, "hello" ) == 0 );

	my_free( q );

	my_malloc_assert_free();
	my_malloc_free();
}
