#include "str_utils.h"

#include <stdio.h>
#include <string.h>

bool str_has_prefix( const char* str, const char* pre )
{
	size_t lenpre = strlen( pre ), lenstr = strlen( str );
	return lenstr < lenpre ? false : strncmp( pre, str, lenpre ) == 0;
}

void encode_hash_to_str( char dst[SHA_DIGEST_LENGTH_HEX], const char hash[SHA_DIGEST_LENGTH] )
{
	for( int i = 0; i < SHA_DIGEST_LENGTH; i++ ) {
		sprintf( dst + i * 2, "%02x", (unsigned char)hash[i] );
	}
}
