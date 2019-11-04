#include "lfm_binary_parser.h"

#include "lfm.h"
#include "my_malloc.h"

#include <assert.h>
#include <stdio.h>

int maxstrlen( const char* s, size_t n )
{
	int l = 0;
	for( ; l < n && s[l]; l++ )
		;
	return l;
}

int parse_binary_lfm( const char* s, size_t n, struct LFM** lfm )
{
	int res = 0;

	int i;
	int l;

	char* t = NULL;
	char* key = NULL;

	struct LFM* tmp_lfm = NULL;

	const char* end = s + n;
	ssize_t remaining;

	for( i = 0;; i++ ) {
		remaining = end - s;
		if( remaining <= 0 ) {
			break;
		}
		l = maxstrlen( s, remaining );
		t = my_malloc( l + 1 );
		memcpy( t, s, l );
		t[l] = '\0';

		if( i == 0 ) {
			tmp_lfm = lfm_new( t );
			t = NULL;
		}
		else {
			assert( tmp_lfm );

			if( i % 2 == 1 ) {
				key = t;
				t = NULL;
			}
			else {
				lfm_add_label_unsorted( tmp_lfm, key, t );
				key = NULL;
				t = NULL;
			}
		}

		s += l + 1;
	}

	//special case for empty metrics
	if( tmp_lfm == NULL ) {
		tmp_lfm = lfm_new( NULL );
	}

	if( key || t ) {
		// unequal number of key and values
		res = 1;
		goto error;
	}

	lfm_sort_labels( tmp_lfm );

	*lfm = tmp_lfm;
	tmp_lfm = NULL;

error:
	if( tmp_lfm ) {
		lfm_free( tmp_lfm );
	}
	if( t ) {
		my_free( t );
	}
	if( key ) {
		my_free( key );
	}

	return res;
}

void encode_binary_lfm( struct LFM* lfm, char** s, int* n )
{
	int l;
	int m = 0;
	char* t = NULL;

	// count num bytes needed
	if( lfm->name ) {
		m += strlen( lfm->name );
	}
	for( int i = 0; i < lfm->num_labels; i++ ) {
		m += 2;
		m += strlen( lfm->labels[i].key );
		m += strlen( lfm->labels[i].value );
	}

	if( m ) {
		*s = t = my_malloc( m );
	}
	else {
		*s = NULL;
	}
	*n = m;

	if( lfm->name ) {
		l = strlen( lfm->name );
		memcpy( t, lfm->name, l );
		t += l;
	}
	for( int i = 0; i < lfm->num_labels; i++ ) {
		*t = '\0';
		t++;

		l = strlen( lfm->labels[i].key );
		memcpy( t, lfm->labels[i].key, l );
		t += l;

		*t = '\0';
		t++;

		l = strlen( lfm->labels[i].value );
		memcpy( t, lfm->labels[i].value, l );
		t += l;
	}
}
