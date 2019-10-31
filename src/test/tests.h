#include "CUnit/Basic.h"

// list all of our tests here, so that we can register them in the main test runner

char _assert_buf[1024];

#define ASSERT_DOUBLE_EQUAL( actual, expected )                                                    \
	{                                                                                              \
		sprintf( _assert_buf, "got %f; want %f", actual, expected );                               \
		CU_assertImplementation(                                                                   \
			( ( actual ) == ( expected ) ), __LINE__, _assert_buf, __FILE__, "", CU_FALSE );       \
	}

#define ASSERT_LONG_EQUAL( actual, expected )                                                      \
	{                                                                                              \
		sprintf( _assert_buf, "got %ld; want %ld", actual, expected );                             \
		CU_assertImplementation(                                                                   \
			( ( actual ) == ( expected ) ), __LINE__, _assert_buf, __FILE__, "", CU_FALSE );       \
	}

/*
//#define REG_TEST( r )                                                                     \
//	do {                                                                                           \
//		int ENSURE_NETWORK_IO_res = ( r );                                                         \
//		switch( ENSURE_NETWORK_IO_res ) {                                                          \
//		case STRUCTURED_STREAM_BUF_NEED_MORE:                                                      \
//			LOG_DEBUG( "EAGAIN" );                                                                 \
//			return -1;                                                                             \
//		case 0:                                                                                    \
//			break;                                                                                 \
//		default:                                                                                   \
//			LOG_ERROR( "res=d unhandled socket error", ENSURE_NETWORK_IO_res );                    \
//			return 1;                                                                              \
//		}                                                                                          \
//	} while( 0 );
//	*/

// from test_bitstream.c
void test_bitstream_signed( void );
void test_bitstream_unsigned( void );
void test_bitstream_signed_bounds_checking( void );
void test_bitstream_unsigned_bounds_checking( void );

// from test_tsc.c
void test_tsc( void );
void test_tsc_write_read( void );
void test_tsc_grow( void );
void test_tsc_trim( void );
void test_tsc_trim_and_write( void );

// from test_my_malloc.c
void test_my_malloc( void );
void test_my_malloc_2( void );
void test_my_realloc( void );

// from test_storage
void test_storage( void );
void test_get_key_value( void );
void test_nearest_pagination( void );
void test_antoi( void );
void test_storage_with_writes( void );

void test_storage_stress1( void );
void test_storage_stress2( void );

void test_storage_reload( void );
void test_storage_reload2( void );

// from test_disk_storage
void test_disk_storage( void );

// from test_client_lfm_parser
void test_lfm_new_and_free( void );
void test_lfm_new_and_free_with_labels( void );
void test_lfm_binary_parser( void );
void test_lfm_binary_parser2( void );
void test_lfm_binary_encoder( void );
