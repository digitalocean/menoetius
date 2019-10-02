#pragma once

#define TEST_LFM_MAX_SIZE 128

// generate a lfm key "hello\x00user_id\00<user_id>" and store it in s
// lfm_len will be the length of the key, there is no guarentee that the
// lfm will have a final NULL byte.
void get_test_lfm( char s[TEST_LFM_MAX_SIZE], int* lfm_len, int user_id );

double get_test_pt( int seed );
