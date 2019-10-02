#include "dump.h"

#include <stdio.h>

void dump_mem( const char* buf, size_t n )
{
	for( size_t i = 0; i < n; i++ ) {
		printf( "%02hhX ", buf[i] );
	}
	printf( "\n" );
}

void dump_mem_bits( const char* buf, size_t n )
{
	for( size_t i = 0; i < n; i++ ) {
		for( int j = 7; j >= 0; j-- ) {
			int m = ( 1U << j );
			if( buf[i] & m ) {
				printf( "1" );
			}
			else {
				printf( "0" );
			}
		}
		printf( " " );
	}
	printf( "\n" );
}

void dump_mem_bits_strange( const char* buf, size_t n )
{
	for( size_t i = 0; i < n; i++ ) {
		for( int j = 0; j < 8; j++ ) {
			int m = ( 1U << j );
			if( buf[i] & m ) {
				printf( "1" );
			}
			else {
				printf( "0" );
			}
		}
		printf( " " );
	}
	printf( "\n" );
}
