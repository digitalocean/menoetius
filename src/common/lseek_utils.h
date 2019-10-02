#pragma once

#ifndef _LARGEFILE64_SOURCE
#	define _LARGEFILE64_SOURCE
#endif // _LARGEFILE64_SOURCE

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"

static inline off_t must_lseek( int fd, off_t offset, int whence )
{
	off_t o = lseek( fd, offset, whence );
	if( o < 0 ) {
		LOG_ERROR( "err=s failed to seek", strerror( errno ) );
		assert( 0 );
	}
	return o;
}
