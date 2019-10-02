#pragma once

int env_parse_get_int( const char* name, int default_value );

// env_parse_get_str returns a newly allocated string; the caller must free() it.
char* env_parse_get_str( const char* name, const char* default_value );
