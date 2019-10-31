#include "CUnit/Basic.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "tests.h"

#define REGISTER_TEST( func_name )                                                                 \
	do {                                                                                           \
		ok = ok && CU_add_test( pSuite, #func_name, func_name ) != NULL;                           \
	} while( 0 );

int register_tests( CU_pSuite pSuite )
{
	int ok = 1;

	// bitstream tests
	REGISTER_TEST( test_bitstream_signed );
	REGISTER_TEST( test_bitstream_unsigned );
	REGISTER_TEST( test_bitstream_signed_bounds_checking );
	REGISTER_TEST( test_bitstream_unsigned_bounds_checking );

	// tsc tests
	REGISTER_TEST( test_tsc );
	REGISTER_TEST( test_tsc_grow );
	REGISTER_TEST( test_tsc_write_read );
	REGISTER_TEST( test_tsc_trim );
	REGISTER_TEST( test_tsc_trim_and_write );

	// my_malloc tests
	REGISTER_TEST( test_my_malloc );
	REGISTER_TEST( test_my_malloc_2 );
	REGISTER_TEST( test_my_realloc );

	// storage tests
	REGISTER_TEST( test_storage );
	REGISTER_TEST( test_get_key_value );
	REGISTER_TEST( test_nearest_pagination );
	REGISTER_TEST( test_antoi );
	REGISTER_TEST( test_storage_with_writes );

	REGISTER_TEST( test_storage_stress1 );
	REGISTER_TEST( test_storage_stress2 );

	REGISTER_TEST( test_storage_reload );
	REGISTER_TEST( test_storage_reload2 );

	// disk storage tests
	REGISTER_TEST( test_disk_storage );

	// lfm parser tests
	REGISTER_TEST( test_lfm_new_and_free );
	REGISTER_TEST( test_lfm_new_and_free_with_labels );
	REGISTER_TEST( test_lfm_binary_parser );
	REGISTER_TEST( test_lfm_binary_parser2 );
	REGISTER_TEST( test_lfm_binary_encoder );
	REGISTER_TEST( test_lfm_binary_parser_missing_value );
	REGISTER_TEST( test_lfm_binary_parser_empty );
	REGISTER_TEST( test_lfm_human_encoder );
	REGISTER_TEST( test_lfm_human_encoder2 );
	REGISTER_TEST( test_lfm_human_parser );
	REGISTER_TEST( test_lfm_human_parser_no_name );
	REGISTER_TEST( test_lfm_binary_encoder_no_name );

	return ok;
}

int main( int argc, const char** argv, const char** env )
{
	CU_pSuite pSuite = NULL;
	CU_pTest pTest = NULL;
	const char* test_name = NULL;
	bool list_tests = false;

	set_log_level_from_env_variables( env );

	if( argc == 2 ) {
		if( strcmp( argv[1], "--list" ) == 0 ) {
			list_tests = true;
		}
		else {
			test_name = argv[1];
		}
	}
	else if( argc > 2 ) {
		fprintf( stderr, "usage: %s [TEST NAME]\n", argv[0] );
		return 1;
	}

	/* initialize the CUnit test registry */
	if( CUE_SUCCESS != CU_initialize_registry() )
		return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite( "default", NULL, NULL );
	if( NULL == pSuite ) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if( !register_tests( pSuite ) ) {
		printf( "failed to register all tests\n" );
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_pTestRegistry reg = CU_get_registry();

	if( list_tests ) {
		for( pSuite = reg->pSuite; pSuite; pSuite = pSuite->pNext ) {
			for( pTest = pSuite->pTest; pTest; pTest = pTest->pNext ) {
				// printf("%s::%s\n", pSuite->pName, pTest->pName);
				printf( "%s\n", pTest->pName );
			}
		}
		return 0;
	}

	CU_basic_set_mode( CU_BRM_VERBOSE );

	if( test_name != NULL ) {
		bool found = false;
		for( pSuite = reg->pSuite; pSuite; pSuite = pSuite->pNext ) {
			for( pTest = pSuite->pTest; pTest; pTest = pTest->pNext ) {
				if( strcmp( pTest->pName, test_name ) == 0 ) {
					found = true;
				}
				else {
					pTest->fActive = false;
				}
			}
		}
		if( !found ) {
			printf( "failed to find test \"%s\"; use --list to show all tests\n", test_name );
			return 1;
		}
	}

	CU_basic_run_tests();

	if( CU_get_number_of_suites_failed() || CU_get_number_of_tests_failed() ||
		CU_get_number_of_failures() ) {
		return 1;
	}
	return 0;
}
