#pragma once

#define ENSURE_RES_OK( r )                                                                         \
	res = ( r );                                                                                   \
	if( res ) {                                                                                    \
		goto error;                                                                                \
	}

#define MUST_WRITE( fd, s, n )                                                                     \
	do {                                                                                           \
		ssize_t _must_write_remaining_n_ = n;                                                      \
		ssize_t _must_write_result_;                                                               \
		const void* _must_write_data_ = s;                                                         \
		while( _must_write_remaining_n_ > 0 ) {                                                    \
			_must_write_result_ = maybe_write( fd, _must_write_data_, _must_write_remaining_n_ );  \
			if( _must_write_result_ < 0 ) {                                                        \
				LOG_ERROR( "err=s failed to write", strerror( errno ) );                           \
				abort();                                                                           \
			}                                                                                      \
			if( _must_write_result_ == 0 ) {                                                       \
				LOG_WARN( "write returned 0" );                                                    \
				sleep( 1 );                                                                        \
			}                                                                                      \
			_must_write_remaining_n_ -= _must_write_result_;                                       \
			_must_write_data_ += _must_write_result_;                                              \
		}                                                                                          \
	} while( 0 );

#define MUST_READ( fd, buf, n )                                                                    \
	do {                                                                                           \
		ssize_t _must_read_remaining_n_ = n;                                                       \
		ssize_t _must_read_result_;                                                                \
		void* _must_read_data_ = buf;                                                              \
		while( _must_read_remaining_n_ > 0 ) {                                                     \
			_must_read_result_ = read( fd, _must_read_data_, _must_read_remaining_n_ );            \
			if( _must_read_result_ < 0 ) {                                                         \
				LOG_ERROR( "err=s failed to read", strerror( errno ) );                            \
				abort();                                                                           \
			}                                                                                      \
			if( _must_read_result_ == 0 ) {                                                        \
				LOG_ERROR( "read returned 0" );                                                    \
				abort();                                                                           \
			}                                                                                      \
			_must_read_remaining_n_ -= _must_read_result_;                                         \
			_must_read_data_ += _must_read_result_;                                                \
		}                                                                                          \
	} while( 0 );
