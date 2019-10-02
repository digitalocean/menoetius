#define _LARGEFILE64_SOURCE

#include "disk_storage.h"

#include "crash_test.h"
#include "log.h"
#include "lseek_utils.h"
#include "metrics.h"
#include "my_utils.h"
#include "tsc.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <my_malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define STORAGE_INITIAL_RESERVED 64

// Each block starts with a header; each header element is a uint8
// header[0] is the block element status
// header[1] is the pool the block is part of (which determins the size of the block)
#define BLOCK_HEADER_STATUS_INDEX 0
#define BLOCK_HEADER_POOL_INDEX 1
#define BLOCK_HEADER_SIZE 2

// first 8 bytes should be this value stored in little endian (which works out to be acea035d00000000)
const uint64_t version = 1560537772;

int disk_storage_recover_scratch( struct disk_storage* store );

static inline void num_disk_reserve_counter( int i )
{
	switch( i ) {
	case 0:
		INCR_METRIC( num_disk_reserve_pool_0 );
		break;
	case 1:
		INCR_METRIC( num_disk_reserve_pool_1 );
		break;
	case 2:
		INCR_METRIC( num_disk_reserve_pool_2 );
		break;
	case 3:
		INCR_METRIC( num_disk_reserve_pool_3 );
		break;
	case 4:
		INCR_METRIC( num_disk_reserve_pool_4 );
		break;
	case 5:
		INCR_METRIC( num_disk_reserve_pool_5 );
		break;
	case 6:
		INCR_METRIC( num_disk_reserve_pool_6 );
		break;
	case 7:
		INCR_METRIC( num_disk_reserve_pool_7 );
		break;
	case 8:
		INCR_METRIC( num_disk_reserve_pool_8 );
		break;
	case 9:
		INCR_METRIC( num_disk_reserve_pool_9 );
		break;
	default:
		INCR_METRIC( num_disk_reserve_pool_10 );
	}
}

static inline void increment_disk_release_counter( int i )
{
	switch( i ) {
	case 0:
		INCR_METRIC( num_disk_release_pool_0 );
		break;
	case 1:
		INCR_METRIC( num_disk_release_pool_1 );
		break;
	case 2:
		INCR_METRIC( num_disk_release_pool_2 );
		break;
	case 3:
		INCR_METRIC( num_disk_release_pool_3 );
		break;
	case 4:
		INCR_METRIC( num_disk_release_pool_4 );
		break;
	case 5:
		INCR_METRIC( num_disk_release_pool_5 );
		break;
	case 6:
		INCR_METRIC( num_disk_release_pool_6 );
		break;
	case 7:
		INCR_METRIC( num_disk_release_pool_7 );
		break;
	case 8:
		INCR_METRIC( num_disk_release_pool_8 );
		break;
	case 9:
		INCR_METRIC( num_disk_release_pool_9 );
		break;
	default:
		INCR_METRIC( num_disk_release_pool_10 );
	}
}

static inline void increment_disk_reuse_counter( int i )
{
	switch( i ) {
	case 0:
		INCR_METRIC( num_disk_reuse_pool_0 );
		break;
	case 1:
		INCR_METRIC( num_disk_reuse_pool_1 );
		break;
	case 2:
		INCR_METRIC( num_disk_reuse_pool_2 );
		break;
	case 3:
		INCR_METRIC( num_disk_reuse_pool_3 );
		break;
	case 4:
		INCR_METRIC( num_disk_reuse_pool_4 );
		break;
	case 5:
		INCR_METRIC( num_disk_reuse_pool_5 );
		break;
	case 6:
		INCR_METRIC( num_disk_reuse_pool_6 );
		break;
	case 7:
		INCR_METRIC( num_disk_reuse_pool_7 );
		break;
	case 8:
		INCR_METRIC( num_disk_reuse_pool_8 );
		break;
	case 9:
		INCR_METRIC( num_disk_reuse_pool_9 );
		break;
	default:
		INCR_METRIC( num_disk_reuse_pool_10 );
	}
}

// rounds n up to the nearest pool size, and sets the size of the pooled item
uint8_t get_pool_id( size_t n, size_t* pool_size )
{
	long pow = ( 64 - __builtin_clzll( n - 1L ) );
	if( pow < MIN_DISK_POW_TWO ) {
		pow = MIN_DISK_POW_TWO;
	}
	assert( 0 <= pow && pow < 255 );
	*pool_size = 1L << pow;
	assert( n <= *pool_size );
	return (uint8_t)pow - MIN_DISK_POW_TWO;
}

size_t get_pool_size( uint8_t pool_id )
{
	return 1L << ( pool_id + MIN_DISK_POW_TWO );
}

int disk_storage_open( struct disk_storage* store, const char* path )
{
	int err = 0;
	ssize_t n;

	// Ensure _LARGEFILE64_SOURCE is working.
	// Our code uses these types interchangably.
	assert( sizeof( size_t ) == sizeof( uint64_t ) );

	// init store
	memset( store, 0, sizeof( struct disk_storage ) );
	store->fd = -1;
	store->scratch_fd = -1;
	store->path = my_strdup( path );
	store->scratch_path = my_malloc( strlen( path ) + 1000 );
	sprintf( store->scratch_path, "%s.scratch", path );

	store->fd = open( path, O_RDWR | O_CREAT | O_LARGEFILE, 0644 );
	if( store->fd < 0 ) {
		LOG_ERROR( "err=s path=s failed to open disk storage", strerror( errno ), path );
		err = 1;
		goto error;
	}

	off_t end_position = lseek( store->fd, 0, SEEK_END );
	if( end_position < 0 ) {
		LOG_ERROR( "err=s path=s failed to seek to end of file", strerror( errno ), path );
		err = 1;
		goto error;
	}
	if( lseek( store->fd, 0, SEEK_SET ) != 0 ) {
		LOG_ERROR( "err=s path=s failed to seek to start of file", strerror( errno ), path );
		err = 1;
		goto error;
	}

	uint64_t actual_version = 0;
	n = read( store->fd, &actual_version, sizeof( uint64_t ) );
	if( n == 0 ) {
		// ensure file is actually empty
		assert( end_position == 0 );

		// file is empty, initialize it.
		MUST_WRITE( store->fd, &version, sizeof( uint64_t ) );
		assert( STORAGE_INITIAL_RESERVED > sizeof( uint64_t ) );
		store->eof_offset = STORAGE_INITIAL_RESERVED;

		// ensure we pad the file; so that we can read the last full block
		// (even when it has no data)
		must_lseek( store->fd, store->eof_offset - 1, SEEK_SET );
		MUST_WRITE( store->fd, "\x00", 1 );

		LOG_INFO( "path=s initializing storage", path );
	}
	else {
		if( n != sizeof( uint64_t ) ) {
			LOG_ERROR( "path=s, failed to read version", path );
			err = 1;
			goto error;
		}
		if( actual_version != version ) {
			LOG_ERROR(
				"path=s got=d want=d file version missmatch", path, actual_version, version );
			err = 1;
			goto error;
		}
		store->eof_offset = must_lseek( store->fd, 0, SEEK_END );
		LOG_TRACE( "path=s offset=d seek to end", path, store->eof_offset );
		assert( store->eof_offset >= STORAGE_INITIAL_RESERVED );
	}

	store->scratch_fd = open( store->scratch_path, O_RDWR | O_CREAT | O_LARGEFILE, 0644 );
	if( store->scratch_fd < 0 ) {
		LOG_ERROR( "err=s path=s failed to open disk storage scratch",
				   strerror( errno ),
				   store->scratch_path );
		err = 1;
		goto error;
	}

	err = disk_storage_recover_scratch( store );
	if( err ) {
		LOG_WARN( "err=s path=s failed to recover from scratch file",
				  strerror( errno ),
				  store->scratch_path );
		err = 0; // TODO have an option to treat this as an error
	}
	else {
		LOG_INFO( "path=s recovered scratch file", store->scratch_path );
	}

error:
	if( err ) {
		disk_storage_close( store );
	}

	return err;
}

void disk_storage_close( struct disk_storage* store )
{
	if( store->path ) {
		my_free( store->path );
		store->path = NULL;
	}
	if( store->scratch_path ) {
		my_free( store->scratch_path );
		store->scratch_path = NULL;
	}
	if( store->fd >= 0 ) {
		close( store->fd );
		store->fd = -1;
	}
	if( store->scratch_fd >= 0 ) {
		close( store->scratch_fd );
		store->scratch_fd = -1;
	}
}

int disk_storage_read_blocks_from_start( struct disk_storage* store,
										 bool initial,
										 size_t max_n,
										 char* buf,
										 size_t* n,
										 size_t* offset,
										 uint8_t* block_status )
{
	ssize_t nn;
	uint8_t header[BLOCK_HEADER_SIZE];
	off_t off;
	if( initial ) {
		must_lseek( store->fd, STORAGE_INITIAL_RESERVED, SEEK_SET );
	}
	for( ;; ) {
		off = must_lseek( store->fd, 0, SEEK_CUR );
		LOG_TRACE( "path=s offset=d reading data", store->path, off );
		assert( off > 0 );
		nn = read( store->fd, &header, BLOCK_HEADER_SIZE );
		if( nn == 0 ) {
			// reached end of file
			LOG_TRACE( "path=s offset=d end of file", store->path, off );
			*n = 0;
			return 0;
		}
		if( nn != BLOCK_HEADER_SIZE ) {
			LOG_ERROR(
				"path=s offset=d nn=d failed to read complete header", store->path, off, nn );
			return 1;
		}
		if( header[BLOCK_HEADER_STATUS_INDEX] >= DISK_STORAGE_BLOCK_NUM_STATUSES ||
			header[BLOCK_HEADER_POOL_INDEX] >= NUM_DISK_POOLS ) {
			LOG_ERROR( "path=s offset=d header0=d header1=d corrupt header",
					   store->path,
					   off,
					   header[BLOCK_HEADER_STATUS_INDEX],
					   header[BLOCK_HEADER_POOL_INDEX] );
			return 1;
		}

		if( nn != BLOCK_HEADER_SIZE ) {
			LOG_ERROR( "path=s bytes=d unexpected number of bytes", store->path, nn );
			return 1;
		}

		// enough room?
		size_t block_size = get_pool_size( header[BLOCK_HEADER_POOL_INDEX] );
		if( block_size > max_n ) {
			LOG_ERROR( "path=s pool_id=d block_size=d max_size=d offset=d buffer too small",
					   store->path,
					   header[BLOCK_HEADER_POOL_INDEX],
					   block_size,
					   max_n,
					   off );
			return 2;
		}

		// is active?
		if( header[BLOCK_HEADER_STATUS_INDEX] == DISK_STORAGE_BLOCK_STATUS_VALID ||
			header[BLOCK_HEADER_STATUS_INDEX] == DISK_STORAGE_BLOCK_STATUS_ACTIVE ) {
			nn = read( store->fd, buf, block_size );
			assert( nn == block_size );
			*n = nn;
			*offset = off;
			*block_status = header[BLOCK_HEADER_STATUS_INDEX];
			LOG_TRACE( "path=s block=d offset=d status=d read block",
					   store->path,
					   block_size,
					   off,
					   header[BLOCK_HEADER_STATUS_INDEX] );
			return 0;
		}
		else {
			LOG_TRACE( "path=s offset=d status=d skipping block",
					   store->path,
					   off,
					   header[BLOCK_HEADER_STATUS_INDEX] );
			off = must_lseek( store->fd, block_size, SEEK_CUR );
		}
	}
	*n = 0;
	return 0;
}

int disk_storage_read_block( struct disk_storage* store,
							 size_t offset,
							 uint8_t* block_status,
							 char* buf,
							 size_t* n,
							 size_t max_n )
{
	int nn;
	uint8_t header[BLOCK_HEADER_SIZE];
	must_lseek( store->fd, offset, SEEK_SET );
	nn = read( store->fd, &header, 2 );
	assert( nn == BLOCK_HEADER_SIZE );

	// is active?
	if( header[BLOCK_HEADER_STATUS_INDEX] == 0 ) {
		return 1;
	}

	// enough room?
	size_t block_size = get_pool_size( header[BLOCK_HEADER_POOL_INDEX] );
	if( block_size > max_n ) {
		return 2;
	}

	nn = read( store->fd, buf, block_size );
	assert( nn == block_size );
	*n = nn;
	*block_status = header[BLOCK_HEADER_STATUS_INDEX];

	INCR_METRIC( num_disk_reads );

	return 0;
}

void disk_storage_write_block_for_real( struct disk_storage* store,
										size_t offset,
										char* buf,
										size_t n )
{
#ifdef DEBUG_BUILD
	assert( n >= BLOCK_HEADER_SIZE );
	size_t payload_size = n - BLOCK_HEADER_SIZE;

	assert( buf[BLOCK_HEADER_STATUS_INDEX] < DISK_STORAGE_BLOCK_NUM_STATUSES );
	assert( buf[BLOCK_HEADER_POOL_INDEX] < NUM_DISK_POOLS );

	// ensure we don't overwrite the next block
	size_t block_size = get_pool_size( buf[BLOCK_HEADER_POOL_INDEX] );
	if( payload_size > block_size ) {
		LOG_ERROR( "payload_size=d block_size=d buffer overflow", payload_size, block_size );
		assert( 0 );
	}

	if( strstr( store->path, "data.store" ) ) {
		switch( buf[BLOCK_HEADER_STATUS_INDEX] ) {
		case DISK_STORAGE_BLOCK_STATUS_VALID: {
			// when it is complete, the tsc_header is not added.
			const char* data_buf = buf + BLOCK_HEADER_SIZE + sizeof( uint64_t );
			assert( TSC_NUM_PTS( data_buf ) > 0 );
			assert( TSC_START_TIME( data_buf ) > 0 );
		} break;
		case DISK_STORAGE_BLOCK_STATUS_ACTIVE: {
			const char* serialized_tsc_header = buf + BLOCK_HEADER_SIZE + sizeof( uint64_t );
			const char* data_buf = serialized_tsc_header + TSC_HEADER_SERIALIZATION_SIZE;

			struct tsc_header h;
			tsc_deserialize_header( &h, serialized_tsc_header );

			assert( TSC_NUM_PTS( data_buf ) > 0 );
			assert( TSC_START_TIME( data_buf ) > 0 );
		} break;
		case DISK_STORAGE_BLOCK_STATUS_EMPTY: {
			// ignore
			assert( n == ( BLOCK_HEADER_SIZE + sizeof( uint64_t ) ) );
		} break;
		default:
			LOG_ERROR( "status=d bad status code", buf[BLOCK_HEADER_STATUS_INDEX] );
			assert( 0 );
		}
	}
	else if( strstr( store->path, "key.store" ) ) {
		//pass
	}
	else {
		LOG_WARN( "path=s unhandled path, not validating contents", store->path );
	}

#endif // DEBUG_BUILD
	// write the actual data
	must_lseek( store->fd, offset, SEEK_SET );
	CRASH_TEST( MUST_WRITE( store->fd, buf, n ), CRASH_TEST_LOCATION_2 );
}

uint64_t disk_storage_get_checksum( size_t offset, const char* buf, size_t n )
{
	uint64_t hash = ~offset;
	//
	// I think....
	// when we write we don't fill the entire block
	// but when we read we read the entire block
	//
	//const char* end = buf + n;
	//while( buf < end ) {
	//	hash = hash ^ *(uint64_t*)buf;
	//	buf += 8;
	//}
	//while( buf < end ) {
	//	hash += *(uint8_t*)buf;
	//	buf++;
	//}
	return hash;
}

int disk_storage_recover_scratch( struct disk_storage* store )
{
	size_t invalid_offset = 0xFFFFFFFFFFFFFFFF;
	size_t scratch_header[2];
	uint64_t actual_version = 0;
	size_t n;

	size_t eof = must_lseek( store->scratch_fd, 0, SEEK_END );
	assert( must_lseek( store->scratch_fd, 0, SEEK_SET ) == 0 );

	n = read( store->scratch_fd, &actual_version, sizeof( uint64_t ) );
	if( n == 0 ) {
		//init empty file
		MUST_WRITE( store->scratch_fd, &version, sizeof( uint64_t ) );
		LOG_TRACE( "path=s creating scratch", store->scratch_path );
		return 0;
	}
	if( actual_version != version ) {
		LOG_ERROR(
			"path=s want=d got=d version missmatch", store->scratch_path, version, actual_version );
		return 1;
	}

	if( eof == sizeof( uint64_t ) ) {
		// only the version was written
		LOG_TRACE( "path=s scratch is empty", store->scratch_path );
		return 0;
	}

	n = read( store->scratch_fd, scratch_header, sizeof( size_t ) * 2 );
	if( n != sizeof( size_t ) * 2 ) {
		LOG_ERROR( "path=s n=d scratch file header is imcomplete", store->scratch_path, n );
		return 1;
	}
	if( scratch_header[0] == invalid_offset ) {
		LOG_TRACE( "path=s scratch is invalid", store->scratch_path );
		return 0;
	}

	size_t offset = scratch_header[0];
	size_t actual_checksum = scratch_header[1];

	size_t buf_n;

	n = read( store->scratch_fd, &buf_n, sizeof( size_t ) );
	if( n != sizeof( size_t ) ) {
		LOG_ERROR( "path=s n=d scratch file missing size data", store->scratch_path, n );
		return 1;
	}

	char* buf = (char*)my_malloc( buf_n );
	n = read( store->scratch_fd, buf, buf_n );
	if( n != buf_n ) {
		LOG_ERROR( "path=s n=d scratch file missing data", store->scratch_path, buf_n );
		return 1;
	}

	size_t expected_checksum = disk_storage_get_checksum( offset, buf, buf_n );
	if( actual_checksum != expected_checksum ) {
		LOG_ERROR( "path=s offset=d offset_checksum=d expected=d n=d offset checksum invalid",
				   store->scratch_path,
				   offset,
				   actual_checksum,
				   expected_checksum,
				   buf_n );
		return 1;
	}

	// write the recovered data
	LOG_TRACE( "path=s offset=d checksum=d bytes=d restoring data",
			   store->scratch_path,
			   offset,
			   actual_checksum,
			   buf_n );
	disk_storage_write_block_for_real( store, offset, buf, buf_n );
	LOG_TRACE( "path=s offset=d checksum=d bytes=d restoring data complete",
			   store->scratch_path,
			   offset,
			   actual_checksum,
			   buf_n );

	my_free( buf );

	// finally mark scratch file as recovered
	must_lseek( store->scratch_fd, sizeof( uint64_t ), SEEK_SET );

	scratch_header[0] = invalid_offset;
	scratch_header[1] = invalid_offset;
	MUST_WRITE( store->scratch_fd, scratch_header, sizeof( size_t ) * 2 );

	return 0;
}

int disk_storage_write_scratch( struct disk_storage* store, size_t offset, char* buf, size_t n )
{
	size_t invalid_offset = 0xFFFFFFFFFFFFFFFF;
	size_t checksum = disk_storage_get_checksum( offset, buf, n );
	size_t scratch_header[2];

	LOG_TRACE( "offset=d checksum=d n=d writing scratch", offset, checksum, n );

	assert( must_lseek( store->scratch_fd, sizeof( uint64_t ), SEEK_SET ) == sizeof( uint64_t ) );

	// intenionally write this twice
	scratch_header[0] = invalid_offset;
	scratch_header[1] = invalid_offset;
	CRASH_TEST( MUST_WRITE( store->scratch_fd, scratch_header, sizeof( size_t ) * 2 ),
				CRASH_TEST_LOCATION_1 );

	// write data
	MUST_WRITE( store->scratch_fd, &n, sizeof( size_t ) );
	MUST_WRITE( store->scratch_fd, buf, n );

	// finally mark it as complete with the correct offset
	must_lseek( store->scratch_fd, sizeof( uint64_t ), SEEK_SET );
	scratch_header[0] = offset;
	scratch_header[1] = checksum;
	MUST_WRITE( store->scratch_fd, scratch_header, sizeof( size_t ) * 2 );

	LOG_TRACE( "path=s offset=d checksum=d n=d writing scratch done",
			   store->scratch_path,
			   offset,
			   checksum,
			   n );

	return 0;
}

int disk_storage_write_block(
	struct disk_storage* store, size_t offset, uint8_t block_status, char* buf, size_t n )
{
	int res = 0;
	size_t nn;
	uint8_t header[BLOCK_HEADER_SIZE];

	LOG_TRACE( "bytes=d call to write data", n );

	// first bit of the file is always reserved, so this case is not possible
	assert( offset > 0 );

	// offset must be aligned to an appropriate block, if not then it's
	//#assert( (offset - STORAGE_INITIAL_RESERVED) % ( 1 << MIN_DISK_POW_TWO) == 0 );

	assert( block_status == DISK_STORAGE_BLOCK_STATUS_VALID ||
			block_status == DISK_STORAGE_BLOCK_STATUS_ACTIVE );

	// check existing block size is correct
	must_lseek( store->fd, offset, SEEK_SET );
	nn = read( store->fd, &header, BLOCK_HEADER_SIZE );
	assert( nn == BLOCK_HEADER_SIZE );
	size_t block_size = get_pool_size( header[BLOCK_HEADER_POOL_INDEX] );
	assert( n <= block_size );

	//update status
	header[BLOCK_HEADER_STATUS_INDEX] = block_status;

	// write to scratch file (so we can replay this if it fails, to recover corrupted data)
	int scratch_buf_n = BLOCK_HEADER_SIZE + n;
	char* scratch_buf = (char*)my_malloc( scratch_buf_n );
	memcpy( scratch_buf, header, BLOCK_HEADER_SIZE );
	memcpy( scratch_buf + BLOCK_HEADER_SIZE, buf, n );
	LOG_TRACE( "path=s offset=d bytes=d writing data", store->path, offset, scratch_buf_n );
	res = disk_storage_write_scratch( store, offset, scratch_buf, scratch_buf_n );
	if( res ) {
		goto error;
	}

	// write the actual data
	LOG_TRACE( "n=d start_time=d number of points",
			   TSC_NUM_PTS( scratch_buf + BLOCK_HEADER_SIZE + sizeof( uint64_t ) ),
			   TSC_START_TIME( scratch_buf + BLOCK_HEADER_SIZE + sizeof( uint64_t ) ) );
	disk_storage_write_block_for_real( store, offset, scratch_buf, scratch_buf_n );

	INCR_METRIC( num_disk_writes );

	LOG_TRACE( "done" );

error:
	my_free( scratch_buf );
	return res;
}

int disk_storage_reserve_block( struct disk_storage* store, size_t n, size_t* offset )
{
	size_t block_size;
	uint8_t pool_id = get_pool_id( n, &block_size );

	if( pool_id >= NUM_DISK_POOLS ) {
		return 1;
	}

	if( store->next_free_block[pool_id] ) {
		increment_disk_reuse_counter( pool_id );
		*offset = store->next_free_block[pool_id];
		assert( *offset >= STORAGE_INITIAL_RESERVED );
		must_lseek( store->fd, *offset + BLOCK_HEADER_SIZE, SEEK_SET );
		MUST_READ( store->fd, &( store->next_free_block[pool_id] ), sizeof( size_t ) );
	}
	else {
		num_disk_reserve_counter( pool_id );
		*offset = store->eof_offset;
		assert( *offset >= STORAGE_INITIAL_RESERVED );
		store->eof_offset += BLOCK_HEADER_SIZE + block_size; // reserve 2 bytes per block
		must_lseek( store->fd, *offset, SEEK_SET );
		uint8_t buf[BLOCK_HEADER_SIZE];
		buf[BLOCK_HEADER_STATUS_INDEX] = DISK_STORAGE_BLOCK_STATUS_RESERVED;
		buf[BLOCK_HEADER_POOL_INDEX] = pool_id;
		MUST_WRITE( store->fd, buf, BLOCK_HEADER_SIZE );

		// ensure we pad the file; so that we can read the last full block
		// (even when it has no data)
		must_lseek( store->fd, store->eof_offset - 1, SEEK_SET );
		MUST_WRITE( store->fd, "\x00", 1 );
	}
	return 0;
}

// TODO optimize this function
int disk_storage_reserve_and_write_block(
	struct disk_storage* store, uint8_t block_status, char* buf, size_t n, size_t* offset )
{
	int res;
	res = disk_storage_reserve_block( store, n, offset );
	if( res ) {
		return res;
	}

	return disk_storage_write_block( store, *offset, block_status, buf, n );
}

int disk_storage_release_block( struct disk_storage* store, size_t offset )
{
	assert( offset >= STORAGE_INITIAL_RESERVED );

	int res;
	uint8_t pool_id;
	char buf[BLOCK_HEADER_SIZE + sizeof( uint64_t )];
	size_t nn = BLOCK_HEADER_SIZE + sizeof( uint64_t );
	must_lseek( store->fd, offset, SEEK_SET );
	MUST_READ( store->fd, &buf, BLOCK_HEADER_SIZE );

	if( buf[BLOCK_HEADER_STATUS_INDEX] >= DISK_STORAGE_BLOCK_NUM_STATUSES ||
		buf[BLOCK_HEADER_POOL_INDEX] >= NUM_DISK_POOLS ) {
		LOG_ERROR( "path=s offset=d header0=d header1=d corrupt header",
				   store->path,
				   offset,
				   buf[BLOCK_HEADER_STATUS_INDEX],
				   buf[BLOCK_HEADER_POOL_INDEX] );
		return 1;
	}
	if( buf[BLOCK_HEADER_STATUS_INDEX] == DISK_STORAGE_BLOCK_STATUS_EMPTY ) {
		LOG_ERROR( "path=s offset=d freeing already free block", store->path, offset );
		assert( 0 );
	}
	assert( buf[BLOCK_HEADER_STATUS_INDEX] < DISK_STORAGE_BLOCK_NUM_STATUSES );

	pool_id = (uint8_t)buf[BLOCK_HEADER_POOL_INDEX];
	assert( pool_id < NUM_DISK_POOLS );
	increment_disk_release_counter( pool_id );

	buf[BLOCK_HEADER_STATUS_INDEX] = 0;
	memcpy( &buf[2], &( store->next_free_block[pool_id] ), sizeof( uint64_t ) );

	// write to scratch file (so we can replay this if it fails, to recover corrupted data)
	res = disk_storage_write_scratch( store, offset, buf, nn );
	if( res ) {
		return res;
	}

	// write to actual file
	disk_storage_write_block_for_real( store, offset, buf, nn );
	store->next_free_block[pool_id] = offset;

	return 0;
}
