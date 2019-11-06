#include "my_malloc.h"

#include "log.h"
#include "metrics.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define min_size 32
#define min_size_pow 5
#define num_pools 14
#define max_size min_size* num_pools

// WARNING: even though we assume the pool number can be a uint16
// we MUST store it as a uint64 (assuming 64bit system); otherwise the struct
// will NOT be properly aligned. To further complicate this matter, one can see a
// "The futex facility returned an unexpected error code" error if it's not properly aligned.
#define NO_POOL 65535 // 2^16-1

void** pools = NULL;

#ifdef DEBUG_BUILD
size_t items_currently_allocated[num_pools + 1];
size_t items_currently_in_pool[num_pools];

static void init_mem_trackers( void )
{
	memset( items_currently_allocated, 0, ( num_pools + 1 ) * sizeof( size_t ) );
	memset( items_currently_in_pool, 0, num_pools * sizeof( size_t ) );
}

#endif // DEBUG_BUILD

static inline void increment_malloc_counter( int i )
{
	switch( i ) {
	case 0:
		INCR_METRIC( num_malloc_pool_0 );
		break;
	case 1:
		INCR_METRIC( num_malloc_pool_1 );
		break;
	case 2:
		INCR_METRIC( num_malloc_pool_2 );
		break;
	case 3:
		INCR_METRIC( num_malloc_pool_3 );
		break;
	case 4:
		INCR_METRIC( num_malloc_pool_4 );
		break;
	case 5:
		INCR_METRIC( num_malloc_pool_5 );
		break;
	case 6:
		INCR_METRIC( num_malloc_pool_6 );
		break;
	case 7:
		INCR_METRIC( num_malloc_pool_7 );
		break;
	case 8:
		INCR_METRIC( num_malloc_pool_8 );
		break;
	case 9:
		INCR_METRIC( num_malloc_pool_9 );
		break;
	case 10:
		INCR_METRIC( num_malloc_pool_10 );
		break;
	case 11:
		INCR_METRIC( num_malloc_pool_11 );
		break;
	case 12:
		INCR_METRIC( num_malloc_pool_12 );
		break;
	case 13:
		INCR_METRIC( num_malloc_pool_13 );
		break;
	case 14:
		INCR_METRIC( num_malloc_pool_14 );
		break;
	case NO_POOL:
		INCR_METRIC( num_malloc_no_pool );
		break;
	default:
		printf( "failed to incr pool %d\n", i );
	}
}

static inline void increment_free_counter( int i )
{
	switch( i ) {
	case 0:
		INCR_METRIC( num_free_pool_0 );
		break;
	case 1:
		INCR_METRIC( num_free_pool_1 );
		break;
	case 2:
		INCR_METRIC( num_free_pool_2 );
		break;
	case 3:
		INCR_METRIC( num_free_pool_3 );
		break;
	case 4:
		INCR_METRIC( num_free_pool_4 );
		break;
	case 5:
		INCR_METRIC( num_free_pool_5 );
		break;
	case 6:
		INCR_METRIC( num_free_pool_6 );
		break;
	case 7:
		INCR_METRIC( num_free_pool_7 );
		break;
	case 8:
		INCR_METRIC( num_free_pool_8 );
		break;
	case 9:
		INCR_METRIC( num_free_pool_9 );
		break;
	case 10:
		INCR_METRIC( num_free_pool_10 );
		break;
	case 11:
		INCR_METRIC( num_free_pool_11 );
		break;
	case 12:
		INCR_METRIC( num_free_pool_12 );
		break;
	case 13:
		INCR_METRIC( num_free_pool_13 );
		break;
	case 14:
		INCR_METRIC( num_free_pool_14 );
		break;
	case NO_POOL:
		INCR_METRIC( num_free_no_pool );
		break;
	default:
		printf( "failed to incr pool %d\n", i );
	}
}

static inline void increment_reuse_counter( int i )
{
	switch( i ) {
	case 0:
		INCR_METRIC( num_reuse_pool_0 );
		break;
	case 1:
		INCR_METRIC( num_reuse_pool_1 );
		break;
	case 2:
		INCR_METRIC( num_reuse_pool_2 );
		break;
	case 3:
		INCR_METRIC( num_reuse_pool_3 );
		break;
	case 4:
		INCR_METRIC( num_reuse_pool_4 );
		break;
	case 5:
		INCR_METRIC( num_reuse_pool_5 );
		break;
	case 6:
		INCR_METRIC( num_reuse_pool_6 );
		break;
	case 7:
		INCR_METRIC( num_reuse_pool_7 );
		break;
	case 8:
		INCR_METRIC( num_reuse_pool_8 );
		break;
	case 9:
		INCR_METRIC( num_reuse_pool_9 );
		break;
	case 10:
		INCR_METRIC( num_reuse_pool_10 );
		break;
	case 11:
		INCR_METRIC( num_reuse_pool_11 );
		break;
	case 12:
		INCR_METRIC( num_reuse_pool_12 );
		break;
	case 13:
		INCR_METRIC( num_reuse_pool_13 );
		break;
	case 14:
		INCR_METRIC( num_reuse_pool_14 );
		break;
	default:
		printf( "failed to incr pool %d\n", i );
	}
}

void my_malloc_init()
{
	assert( num_pools < NO_POOL );
	assert( pools == NULL );

#ifdef DEBUG_BUILD
	init_mem_trackers();
#endif // DEBUG_BUILD

	pools = malloc( sizeof( void* ) * num_pools );
	memset( pools, 0, sizeof( void* ) * num_pools );
}

char* my_strdup( const char* s )
{
	int n = strlen( s );
	char* t = my_malloc( n + 1 );
	memcpy( t, s, n + 1 );
	return t;
}

void my_malloc_free()
{
	void* p;
	void* q;
	for( int i = 0; i < num_pools; i++ ) {
		p = pools[i];
		while( p ) {
			q = p - sizeof( uint64_t );
			p = *( (void**)p );

			free( q );
#ifdef DEBUG_BUILD
			items_currently_in_pool[i]--;
			items_currently_allocated[i]--;
#endif // DEBUG_BUILD
		}
	}
	free( pools );
	pools = NULL;

#ifdef DEBUG_BUILD
	for( int i = 0; i <= num_pools; i++ ) {
		assert( items_currently_allocated[i] == 0 ); // memory leak otherwise
	}
#endif // DEBUG_BUILD
}

void my_malloc_assert_free()
{
#ifdef DEBUG_BUILD
	// test non-pooled items were correctly freed
	if( items_currently_allocated[num_pools] ) {
		LOG_ERROR( "memory leak: not all items were freed" );
		assert( 0 );
	};

	// test all items were returned to the pool
	for( int i = 0; i < num_pools; i++ ) {
		if( items_currently_allocated[i] != items_currently_in_pool[i] ) {
			LOG_ERROR( "memory leak: not all items were freed" );
			assert( 0 );
		}
	}
#endif // DEBUG_BUILD
}

void* my_malloc( size_t n )
{
#ifdef DEBUG_BUILD
	// ensure these hold, but deliberately define them separately in case the compiler
	// didn't pre-compute it.
	assert( min_size == ( 1 << min_size_pow ) );
#endif // DEBUG_BUILD

	int i;
	void* p;

	//LOG_DEBUG("n=d malloc requested", n );
	n += sizeof( uint64_t ); // save room for which pool to return this memory to
	if( n < min_size ) {
		n = min_size;
	}

	// round n to the nearest power of 2
	long pow = ( 64 - __builtin_clzll( n - 1L ) );
	n = 1L << pow;

	// determine which pool it is in
	i = pow - min_size_pow;

	if( i >= num_pools ) {
		i = NO_POOL;
		//LOG_DEBUG("n=d malloc non-pool item", n );
		p = malloc( n );
		if( p == NULL ) {
			LOG_CRITICAL( "n=d, malloc failed", n );
		}
		*( (uint64_t*)p ) = i;
		p += sizeof( uint64_t );
#ifdef DEBUG_BUILD
		items_currently_allocated[num_pools]++;
#endif // DEBUG_BUILD
#ifdef SERVER_BUILD
		INCR_METRIC( num_malloc_no_pool )
#endif // SERVER_BUILD
	}
	else {
		if( pools[i] ) {
			p = pools[i];
			pools[i] = *( (void**)p );
			increment_reuse_counter( i );
#ifdef DEBUG_BUILD
			items_currently_in_pool[i]--;
#endif // DEBUG_BUILD
		}
		else {
			//LOG_DEBUG("n=d malloc pool item", n );
			p = malloc( n );
			if( p == NULL ) {
				LOG_CRITICAL( "n=d, malloc failed", n );
			}
			*( (uint64_t*)p ) = i;
			p += sizeof( uint64_t );
#ifdef DEBUG_BUILD
			items_currently_allocated[i]++;
#endif // DEBUG_BUILD
			increment_malloc_counter( i );
		}
	}

#ifdef DEBUG_BUILD
	if( p ) {
		memset( p, 0xCC, n );
	}
#endif // DEBUG_BUILD
	LOG_DEBUG( "p=p my_malloc", p );
	return p;
}

void* my_realloc( void* p, size_t n )
{
	void* q;

	void* pp = p - sizeof( uint64_t );
	int i = *( (uint64_t*)pp );
	if( i >= num_pools ) {
		q = realloc( pp, n + sizeof( uint64_t ) );
		if( q == NULL ) {
			LOG_CRITICAL( "n=d realloc failed", n );
		}
		q += sizeof( uint64_t );
		return q;
	}

	size_t old_size = 1 << ( min_size_pow + i );

	q = my_malloc( n );
	assert( q );
	memcpy( q, p, old_size );

	my_free( p );

	return q;
}

void my_free( void* p )
{
	LOG_DEBUG( "p=p my_free", p );
	void* q = p - sizeof( uint64_t );
	int i = *( (uint64_t*)q );

	if( i >= num_pools ) {
		assert( i == NO_POOL );
		free( q );
#ifdef DEBUG_BUILD
		items_currently_allocated[num_pools]--;
#endif // DEBUG_BUILD
		INCR_METRIC( num_free_no_pool )
		return;
	}

#ifdef DEBUG_BUILD
	// overwrite data (to help find problems)
	memset( p, 0xDD, 1 << ( min_size_pow + i ) );
#endif // DEBUG_BUILD

	// return item to pool
	void** qq = (void**)p;
	*qq = pools[i];
	pools[i] = p;

	increment_free_counter( i );
#ifdef DEBUG_BUILD
	items_currently_in_pool[i]++;
#endif // DEBUG_BUILD
}
