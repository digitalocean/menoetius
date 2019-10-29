#include "lfm_parser.h"

#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define EOF_TOKEN 1
#define IDENTIFIER_TOKEN 2
#define QUOTED_STRING_TOKEN 3
#define OPEN_BRACE_TOKEN 4
#define CLOSE_BRACE_TOKEN 5
#define COMMA_TOKEN 6
#define EQUAL_TOKEN 7

int key_value_cmp( const void* a, const void* b )
{
	return strcmp( ( (struct KeyValue*)a )->key, ( (struct KeyValue*)b )->key );
}

int scanIdent( const char** s, char** lit )
{
	int i = 0;
	if( !isalpha( s[0][i] ) ) {
		return 1;
	}
	i++;

	for( ; isalpha( s[0][i] ) || isdigit( s[0][i] ) || s[0][i] == '_'; i++ )
		;

	*lit = malloc( i + 1 );
	memcpy( *lit, s[0], i );
	( *lit )[i] = '\0';

	*s += i;
	return 0;
}

int scanQuotedString( const char** s, char** lit )
{
	char c;
	char* t;
	int i = 0;
	int j = 0;
	if( s[0][0] != '"' ) {
		return 1;
	}
	i++;

	t = malloc( strlen( *s ) + 1 );

	for( ;; ) {
		c = s[0][i];
		if( c == '\0' ) {
			free( t );
			return 1;
		}
		if( c == '"' ) {
			i++;
			break;
		}
		if( c == '\\' ) {
			i++;
			t[j++] = s[0][i];
			i++;
			continue;
		}
		t[j++] = c;
		i++;
	}
	t[j] = '\0';
	*s += i;
	*lit = t;
	//printf("quoted val is %s, remain is %s\n", t, s[0]);
	return 0;
}

int scan( const char** s, int* tok, char** lit )
{
	char c;
	*lit = NULL;

	for( ;; ) {
		c = **s;
		if( c != ' ' ) {
			break;
		}
		( *s )++;
	}
	//printf("char is '%c' %d\n", c, c);

	if( isalpha( c ) ) {
		*tok = IDENTIFIER_TOKEN;
		return scanIdent( s, lit );
	}

	if( c == '"' ) {
		*tok = QUOTED_STRING_TOKEN;
		return scanQuotedString( s, lit );
	}

	switch( c ) {
	case '\0':
		*tok = EOF_TOKEN;
		break;

	case '{':
		*tok = OPEN_BRACE_TOKEN;
		break;

	case '}':
		*tok = CLOSE_BRACE_TOKEN;
		break;

	case ',':
		*tok = COMMA_TOKEN;
		break;

	case '=':
		*tok = EQUAL_TOKEN;
		break;

	default:
		return 1;
	}
	( *s )++;
	return 0;
}

int parse_lfm( const char* s,
			   const char** metric_name,
			   struct KeyValue* metric_labels,
			   int* num_labels,
			   int max_key_value_pairs )
{
	int res = 0;
	int token;
	char* lit;

	bool done = false;

	const char* metric_name_ = NULL;
	int i = 0;

	if( ( res = scan( &s, &token, &lit ) ) ) {
		goto error;
	}
	switch( token ) {
	case IDENTIFIER_TOKEN:
		metric_name_ = lit;
		break;
	case OPEN_BRACE_TOKEN:
		// metric with no name
		break;
	default:
		res = 1;
		goto error;
	}

	if( metric_name_ ) {
		if( ( res = scan( &s, &token, &lit ) ) ) {
			goto error;
		}
		switch( token ) {
		case OPEN_BRACE_TOKEN:
			break;
		case EOF_TOKEN:
			// metric name only, no labels.
			done = true;
			break;
		default:
			res = 1;
			goto error;
		}
	}

	while( !done ) {
		// get key
		if( ( res = scan( &s, &token, &lit ) ) ) {
			goto error;
		}
		switch( token ) {
		case CLOSE_BRACE_TOKEN:
			done = true;
			break;
		case IDENTIFIER_TOKEN:
			// metric name only, no labels.
			metric_labels[i].key = lit;
			break;
		default:
			res = 1;
			goto error;
		}

		// get equal
		if( ( res = scan( &s, &token, &lit ) ) ) {
			goto error;
		}
		if( token != EQUAL_TOKEN ) {
			res = 1;
			goto error;
		}

		// get val
		if( ( res = scan( &s, &token, &lit ) ) ) {
			goto error;
		}
		if( token != QUOTED_STRING_TOKEN ) {
			res = 1;
			goto error;
		}
		metric_labels[i].value = lit;
		i++;
		if( i == max_key_value_pairs ) {
			res = 2;
			goto error;
		}

		// get comma
		if( ( res = scan( &s, &token, &lit ) ) ) {
			goto error;
		}
		switch( token ) {
		case CLOSE_BRACE_TOKEN:
			done = true;
			break;
		case COMMA_TOKEN:
			break;
		default:
			res = 1;
			goto error;
		}
	}

	qsort( metric_labels, i, sizeof( struct KeyValue ), key_value_cmp );

	*metric_name = metric_name_;
	*num_labels = i;

	assert( res == 0 );

error:
	return res;
}
