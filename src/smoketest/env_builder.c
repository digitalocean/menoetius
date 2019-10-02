#include "env_builder.h"

#include "my_malloc.h"
#include "sds.h"

#include <assert.h>
#include <stdio.h>

void env_builder_init( struct env_builder* eb )
{
	eb->i = 0;
	eb->cap = 1024;
	eb->envs = my_malloc( sizeof( char* ) * eb->cap );
	eb->envs[0] = NULL;
}

void env_builder_free( struct env_builder* eb )
{
	for( int i = 0; i < eb->i; i++ ) {
		sdsfree( eb->envs[i] );
	}
}

void env_builder_addf( struct env_builder* eb, const char* fmt, ... )
{

	sds s = sdsnew( "" );

	va_list args;
	va_start( args, fmt );
	s = sdscatvprintf( s, fmt, args );
	va_end( args );

	assert( ( eb->i + 1 ) < eb->cap );
	eb->envs[eb->i++] = s;
	eb->envs[eb->i] = NULL;
}
