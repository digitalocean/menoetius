#define _GNU_SOURCE // for memmem

#include "escape_key.h"
#include "log.h"
#include "my_malloc.h"
#include "sglib.h"
#include "structured_stream.h"

#include <assert.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_ESCAPPED_KEY 1024 * 1024
#define MAX_POINTS_PEY_KEY 4000

typedef struct key_item
{
	char* key;
	size_t key_len;

	int i;
	int64_t t[MAX_POINTS_PEY_KEY];
	double y[MAX_POINTS_PEY_KEY];

	// for RB tree
	char color_field;
	struct key_item* left;
	struct key_item* right;
} KeyItem;

int key_item_cmp( KeyItem* a, KeyItem* b );

SGLIB_DEFINE_RBTREE_PROTOTYPES( KeyItem, left, right, color_field, key_item_cmp )
SGLIB_DEFINE_RBTREE_FUNCTIONS( KeyItem, left, right, color_field, key_item_cmp )

int key_item_cmp( KeyItem* a, KeyItem* b )
{
	if( a->key_len < b->key_len )
		return -1;
	if( a->key_len > b->key_len )
		return 1;
	return memcmp( a->key, b->key, a->key_len );
}

void track_point( KeyItem** root, const char* key, size_t key_len, int64_t t, double y )
{
	KeyItem* p;

	KeyItem x;
	x.key_len = key_len;
	x.key = (char*)key;

	p = sglib_KeyItem_find_member( *root, &x );
	if( p == NULL ) {
		// add item
		p = my_malloc( sizeof( KeyItem ) );

		p->key_len = key_len;
		p->key = my_malloc( key_len );
		p->i = 0;
		memcpy( p->key, key, key_len );

		sglib_KeyItem_add( root, p );
	}

	// save point
	assert( p->i < MAX_POINTS_PEY_KEY );
	p->t[p->i] = t;
	p->y[p->i] = y;
	p->i++;
}

void dump_data( KeyItem* root )
{
	struct structured_stream* ss;

	int i;
	int res;
	KeyItem* item;
	struct sglib_KeyItem_iterator itr;

	res = structured_stream_new( fileno( stdout ), 0, 1024, &ss );
	assert( res == 0 );

	for( item = sglib_KeyItem_it_init_inorder( &itr, root ); item != NULL;
		 item = sglib_KeyItem_it_next( &itr ) ) {
		assert( structured_stream_write_uint16_prefixed_bytes( ss, item->key, item->key_len ) ==
				0 );
		assert( structured_stream_write_uint16( ss, item->i ) == 0 );
		for( i = 0; i < item->i; i++ ) {
			assert( structured_stream_write_int64( ss, item->t[i] ) == 0 );
			assert( structured_stream_write_double( ss, item->y[i] ) == 0 );
		}
	}
	structured_stream_flush( ss );
}

int main( int argc, char** argv, char** env )
{
	int res;

	// HACK; need an opt-parser
	if( argc != 3 ) {
		LOG_ERROR( "bad args" );
		return 1;
	}
	const char* comp_path = argv[1];
	const char* filter_str = argv[2];

	char escapped_key[MAX_ESCAPPED_KEY];

	KeyItem* root = NULL;

	int fd = open( comp_path, O_RDONLY );
	if( fd < 0 ) {
		LOG_ERROR( "path=s failed to read", comp_path );
		return 1;
	}

	struct structured_stream* ss;

	res = structured_stream_new( fd, 1024 * 1024, 0, &ss );
	if( res != 0 ) {
		LOG_ERROR( "res=d failed to create structured stream", res );
		return 1;
	}

	char key[MAX_ESCAPPED_KEY];
	size_t n;
	double y;
	int64_t t;
	for( ;; ) {
		res = structured_stream_read_uint16_prefixed_bytes( ss, key, &n, MAX_ESCAPPED_KEY );
		if( res != 0 ) {
			LOG_ERROR( "res=d failed to read", res );
			break;
		}

		res = structured_stream_read_int64( ss, &t );
		if( res != 0 ) {
			LOG_ERROR( "res=d failed to read", res );
			break;
		}
		res = structured_stream_read_double( ss, &y );
		if( res != 0 ) {
			LOG_ERROR( "res=d failed to read", res );
			break;
		}
		if( memmem( key, n, filter_str, strlen( filter_str ) ) == NULL ) {
			continue;
		}

		assert( escape_key( key, n, escapped_key, MAX_ESCAPPED_KEY ) == 0 );
		// printf("got %s %ld %lf\n", escapped_key, t, y);
		track_point( &root, key, n, t, y );
	}
	dump_data( root );
}
