#include "generate_test_data.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "my_malloc.h"
#include "storage.h"
#include "time_utils.h"

void get_test_lfm( char* s, int* lfm_len, int user_id )
{
	memcpy( s, "hello\x00user_id\x00", 14 );
	sprintf( s + 14, "%d", user_id );
	*lfm_len = strlen( s + 14 ) + 14;

	// LFMs are NOT NULL terminated, fill up the rest of the string
	// to prevent errors
	for( int i = *lfm_len; i < TEST_LFM_MAX_SIZE; i++ ) {
		s[i] = '?';
	}
}

double get_test_pt( int seed )
{
	int fast_seed = ( 214013 * seed + 2531011 );
	return ( (double)( ( fast_seed >> 16 ) & 0x7FFF ) ) / 1000.0;
}
