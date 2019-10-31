#include "tests.h"

#include "my_malloc.h"
#include "lfm.h"
#include "lfm_binary_parser.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_lfm_new_and_free( void )
{
	my_malloc_init();

	struct LFM *lfm = lfm_new(my_strdup("hello"));

	lfm_free(lfm);

	my_malloc_assert_free();
	my_malloc_free();
}

void test_lfm_new_and_free_with_labels( void )
{
	my_malloc_init();

	struct LFM *lfm = lfm_new(my_strdup("hello"));

	lfm_add_label_unsorted( lfm, my_strdup("mykey"), my_strdup("myval") );
	lfm_sort_labels( lfm );

	lfm_free(lfm);

	my_malloc_assert_free();
	my_malloc_free();
}

void test_lfm_binary_parser( void )
{
	my_malloc_init();

	int res;

	struct LFM *lfm;
	res = parse_binary_lfm( "myname\x00mykey\x00myvalue", 20, &lfm );
	CU_ASSERT_EQUAL_FATAL( res, 0 );

	CU_ASSERT_STRING_EQUAL( lfm->name, "myname" );
	CU_ASSERT_EQUAL_FATAL( lfm->num_labels, 1 );

	CU_ASSERT_STRING_EQUAL( lfm->labels[0].key, "mykey" );
	CU_ASSERT_STRING_EQUAL( lfm->labels[0].value, "myvalue" );

	lfm_free( lfm );

	my_malloc_assert_free();
	my_malloc_free();
}

void test_lfm_binary_parser2( void )
{
	my_malloc_init();

	int res;

	struct LFM *lfm;
	// ensure the remaining ___ bytes are not used since they are not included in the 20 char limit
	res = parse_binary_lfm( "myname\x00mykey\x00myvalue___", 20, &lfm );
	CU_ASSERT_EQUAL_FATAL( res, 0 );

	CU_ASSERT_STRING_EQUAL( lfm->name, "myname" );
	CU_ASSERT_EQUAL_FATAL( lfm->num_labels, 1 );

	CU_ASSERT_STRING_EQUAL( lfm->labels[0].key, "mykey" );
	CU_ASSERT_STRING_EQUAL( lfm->labels[0].value, "myvalue" );

	lfm_free( lfm );

	my_malloc_assert_free();
	my_malloc_free();
}

void test_lfm_binary_encoder( void )
{
	my_malloc_init();

	struct LFM *lfm = lfm_new( my_strdup("thename") );
	lfm_add_label_unsorted( lfm, my_strdup("thekey"), my_strdup("thevalue") );
	lfm_sort_labels( lfm );


	char *binary_lfm = NULL;
	int binary_lfm_len = 0;
	encode_binary_lfm( lfm, &binary_lfm, &binary_lfm_len );

	lfm_free( lfm );

	CU_ASSERT( binary_lfm_len == 7 + 1 + 6 + 1 + 8 );
	CU_ASSERT( memcmp( binary_lfm, "thename\x00thekey\x00thevalue", binary_lfm_len ) == 0 );

	my_free( binary_lfm );

	my_malloc_assert_free();
	my_malloc_free();
}
