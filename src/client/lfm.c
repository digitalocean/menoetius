#include "lfm.h"

#include "my_malloc.h"

#include <assert.h>

int key_value_cmp( const void* a, const void* b )
{
	return strcmp( ( (struct KeyValue*)a )->key, ( (struct KeyValue*)b )->key );
}

void lfm_add_label_unsorted( struct LFM* lfm, char* key, char* value )
{
	if( lfm->num_labels >= lfm->max_num_labels ) {
		assert( lfm->max_num_labels > 0 );
		lfm->max_num_labels *= 2;
		lfm->labels = realloc( lfm->labels, lfm->max_num_labels );
	}
	lfm->labels[lfm->num_labels].key = key;
	lfm->labels[lfm->num_labels].value = value;
	lfm->num_labels++;
}

void lfm_sort_labels( struct LFM* lfm )
{
	qsort( lfm->labels, lfm->num_labels, sizeof( struct KeyValue ), key_value_cmp );
}

void lfm_free( struct LFM* lfm )
{
	for( int i = 0; i < lfm->num_labels; i++ ) {
		my_free( lfm->labels[i].key );
		my_free( lfm->labels[i].value );
	}
	if( lfm->name ) {
		my_free( lfm->name );
	}
	my_free( lfm->labels );
	my_free( lfm );
}

struct LFM* lfm_new( char* name )
{
	struct LFM* lfm = my_malloc( sizeof( struct LFM ) );

	lfm->name = name;
	lfm->max_num_labels = 64;
	lfm->num_labels = 0;
	lfm->labels = my_malloc( sizeof( struct KeyValue ) * lfm->max_num_labels );

	return lfm;
}
