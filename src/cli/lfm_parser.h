#pragma once

struct KeyValue
{
	char* key;
	char* value;
};

struct LFM
{
	char* name;
	struct KeyValue* labels;
	int num_labels;
	int max_num_labels;
};

// NOTE returned lfm must be freed via free_lfm
int parse_lfm( const char* s, struct LFM** lfm );

// NOTE: reference to name is stolen, it must exist on the heap
// returned lfm must be freed via free_lfm
struct LFM* new_lfm( char *name );

void free_lfm( struct LFM *lfm );

// NOTE reference to key and value strings are stolen
void lfm_add_label_unsorted( struct LFM *lfm, char *key, char *value );

void lfm_sort_labels( struct LFM *lfm );
