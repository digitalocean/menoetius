#include "env_parse.h"

#include <stdlib.h>
#include <string.h>

int env_parse_get_int( const char* name, int default_value )
{
	const char* s = getenv( name );
	if( !s ) {
		return default_value;
	}

	if( strcmp( s, "0" ) == 0 ) {
		return 0;
	}

	int x = atoi( s );
	if( x == 0 ) {
		return default_value;
	}

	return x;
}

char* env_parse_get_str( const char* name, const char* default_value )
{
	const char* s = getenv( name );
	if( !s ) {
		return strdup( default_value );
	}

	return strdup( s );
}
