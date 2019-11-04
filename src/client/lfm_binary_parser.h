#pragma once

#include <stddef.h>
#include <unistd.h>

struct LFM;

// s must be freed
void encode_binary_lfm( struct LFM* lfm, char** s, int* n );

// NOTE returned lfm must be freed via free_lfm
int parse_binary_lfm( const char* s, size_t n, struct LFM** lfm );
