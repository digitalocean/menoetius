#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>

struct LFM;

// NOTE returned lfm must be freed via free_lfm
int parse_human_lfm_and_value( const char* s, struct LFM** lfm, double* y, time_t* t, bool* valid_t );

// NOTE returned lfm must be freed via free_lfm
int parse_human_lfm( const char* s, struct LFM** lfm );

void encode_human_lfm( struct LFM* lfm, char** s );
