#pragma once
#include <stdbool.h>

bool str_has_prefix( const char* str, const char* pre );

#ifndef SHA_DIGEST_LENGTH
#	define SHA_DIGEST_LENGTH 20
#endif

#define SHA_DIGEST_LENGTH_HEX 41
void encode_hash_to_str( char dst[SHA_DIGEST_LENGTH_HEX], const char hash[SHA_DIGEST_LENGTH] );
