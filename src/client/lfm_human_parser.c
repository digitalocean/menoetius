#include "lfm_human_parser.h"

#include "lfm.h"

#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "my_malloc.h"
#include "time_parser.h"

#define EOF_TOKEN 1
#define IDENTIFIER_TOKEN 2
#define QUOTED_STRING_TOKEN 3
#define OPEN_BRACE_TOKEN 4
#define CLOSE_BRACE_TOKEN 5
#define COMMA_TOKEN 6
#define EQUAL_TOKEN 7
#define AT_TOKEN 8
#define INT_TOKEN 9
#define FLOAT_TOKEN 10
#define TIMESTAMP_TOKEN 11

int scanIdent( const char** s, char** lit )
{
	int i = 0;
	if( !isalpha( s[0][i] ) ) {
		return 1;
	}
	i++;

	for( ; isalpha( s[0][i] ) || isdigit( s[0][i] ) || s[0][i] == '_'; i++ )
		;

	*lit = my_malloc( i + 1 );
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

	t = my_malloc( strlen( *s ) + 1 );

	for( ;; ) {
		c = s[0][i];
		if( c == '\0' ) {
			my_free( t );
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
	return 0;
}

// looking for:
//          14
//          14.234
//          YYYY-MM-DDTHH:MM:SS
int scanNumberOrTimestamp( const char** s, int* tok, char** lit )
{
	char c;
	int i = 0;
	bool decimal_found = false;
	int hyphens_found = 0;
	bool t_found = false;
	int colons_found = 0;
	for( ;; ) {
		c = s[0][i];
		if( isdigit( c ) ) {
			i++;
			continue;
		}

		if( c == '.' ) {
			if( decimal_found ) {
				return 1;
			}
			if( hyphens_found ) {
				return 1;
			}
			decimal_found = true;
		}

		if( c == '-' ) {
			if( decimal_found ) {
				return 1;
			}
			switch( hyphens_found ) {
			case 0:
				if( i != 4 ) {
					return 1;
				}
				break;

			case 1:
				if( i != 7 ) {
					return 1;
				}
				break;

			default:
				return 1;
			}
			hyphens_found++;
		}

		if( c == 'T' ) {
			if( t_found ) {
				return 1;
			}
			if( i != 10 ) {
				return 1;
			}
			if( hyphens_found != 2 ) {
				return 1;
			}
			t_found = true;
		}

		if( c == ':' ) {
			if( decimal_found ) {
				return 1;
			}
			if( !t_found ) {
				return 1;
			}
			switch( colons_found ) {
			case 0:
				if( i != 13 ) {
					return 1;
				}
				break;

			case 1:
				if( i != 16 ) {
					return 1;
				}
				break;

			default:
				return 1;
			}
			colons_found++;
		}

		if( c == ' ' || c == '\0' || c == '@' ) {
			break;
		}

		i++;
	}

	if( decimal_found ) {
		*tok = FLOAT_TOKEN;
	}
	else if( hyphens_found || t_found || colons_found ) {
		if( hyphens_found != 2 || !t_found || colons_found != 2 ) {
			return 1;
		}
		if( i != 19 ) {
			return 1;
		}
		*tok = TIMESTAMP_TOKEN;
	}
	else {
		*tok = INT_TOKEN;
	}

	*lit = my_malloc( i + 1 );
	memcpy( *lit, *s, i );
	lit[0][i] = '\0';

	s[0] += i;

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

	if( isalpha( c ) ) {
		*tok = IDENTIFIER_TOKEN;
		return scanIdent( s, lit );
	}

	if( isdigit( c ) ) {
		return scanNumberOrTimestamp( s, tok, lit );
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

	case '@':
		*tok = AT_TOKEN;
		break;

	default:
		return 1;
	}
	( *s )++;
	return 0;
}

int parse_human_lfm_helper( const char** s, struct LFM** lfm )
{
	struct LFM* lfm_tmp = NULL;
	int res = 0;
	int token;
	char* lit = NULL;

	bool done = false;

	char* key = NULL;
	char* metric_name = NULL;

	if( ( res = scan( s, &token, &lit ) ) ) {
		goto error;
	}
	switch( token ) {
	case IDENTIFIER_TOKEN:
		metric_name = lit;
		break;
	case OPEN_BRACE_TOKEN:
		// metric with no name
		break;
	default:
		res = 1;
		goto error;
	}

	if( metric_name ) {
		if( ( res = scan( s, &token, &lit ) ) ) {
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
	else {
		metric_name = my_strdup( "" );
	}

	lfm_tmp = lfm_new( metric_name );
	metric_name = NULL;

	while( !done ) {
		// get key
		if( ( res = scan( s, &token, &lit ) ) ) {
			goto error;
		}
		switch( token ) {
		case CLOSE_BRACE_TOKEN:
			done = true;
			break;
		case IDENTIFIER_TOKEN:
			// metric name only, no labels.
			key = lit;
			break;
		default:
			res = 1;
			goto error;
		}

		// get equal
		if( ( res = scan( s, &token, &lit ) ) ) {
			goto error;
		}
		if( token != EQUAL_TOKEN ) {
			res = 1;
			goto error;
		}

		// get val
		if( ( res = scan( s, &token, &lit ) ) ) {
			goto error;
		}
		if( token != QUOTED_STRING_TOKEN ) {
			res = 1;
			goto error;
		}

		// steal the references
		lfm_add_label_unsorted( lfm_tmp, key, lit );
		key = NULL;
		lit = NULL;

		// get comma
		if( ( res = scan( s, &token, &lit ) ) ) {
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

	lfm_sort_labels( lfm_tmp );

	// steal refernce (and prevent tmp from being cleaned up)
	*lfm = lfm_tmp;
	lfm_tmp = NULL;

	assert( res == 0 );

error:
	if( lfm_tmp ) {
		lfm_free( lfm_tmp );
	}
	if( key ) {
		my_free( key );
	}
	if( lit ) {
		my_free( lit );
	}
	if( metric_name ) {
		my_free( metric_name );
	}

	return res;
}

int parse_human_lfm( const char* s, struct LFM** lfm )
{
	return parse_human_lfm_helper( &s, lfm );
}

int parse_human_lfm_and_value(
	const char* s, struct LFM** lfm, double* y, time_t* t, bool* valid_t )
{
	int res = 0;
	struct LFM* lfm_tmp = NULL;
	int token;
	char* lit = NULL;

	double yy;

	bool search_for_timestamp = true;

	// lfm
	if( ( res = parse_human_lfm_helper( &s, &lfm_tmp ) ) ) {
		goto error;
	}

	// =
	if( ( res = scan( &s, &token, &lit ) ) ) {
		goto error;
	}
	if( token != EQUAL_TOKEN ) {
		res = 1;
		goto error;
	}

	// <float>
	if( ( res = scan( &s, &token, &lit ) ) ) {
		goto error;
	}
	if( token != FLOAT_TOKEN && token != INT_TOKEN ) {
		res = 1;
		goto error;
	}
	yy = strtod( lit, NULL );
	my_free( lit );
	lit = NULL;

	// @
	if( ( res = scan( &s, &token, &lit ) ) ) {
		goto error;
	}
	switch( token ) {
	case EOF_TOKEN:
		search_for_timestamp = false;
		break;
	case AT_TOKEN:
		break;
	default:
		res = 1;
		goto error;
	}

	if( search_for_timestamp ) {
		// <timestamp>
		if( ( res = scan( &s, &token, &lit ) ) ) {
			goto error;
		}
		if( token != TIMESTAMP_TOKEN ) {
			res = 1;
			goto error;
		}

		if( ( res = parse_time( lit, t ) ) ) {
			goto error;
		}
		my_free( lit );
		lit = NULL;
		*valid_t = true;
	}

	assert( res == 0 );

	*lfm = lfm_tmp;
	lfm_tmp = NULL;
	*y = yy;

error:
	if( lfm_tmp ) {
		lfm_free( lfm_tmp );
	}
	if( lit ) {
		my_free( lit );
	}

	return res;
}

void encode_human_lfm( struct LFM* lfm, char** s )
{
	int m = 0;
	char* t = NULL;

	// count num bytes needed
	if( lfm->name ) {
		m += strlen( lfm->name );
	}
	for( int i = 0; i < lfm->num_labels; i++ ) {
		m += 10; //more than needed
		m += strlen( lfm->labels[i].key );
		m += strlen( lfm->labels[i].value );
	}

	m++;
	*s = t = my_malloc( m );
	t[0] = '\0';

	if( lfm->name ) {
		sprintf( t + strlen( t ), "%s", lfm->name );
	}
	if( lfm->num_labels > 0 ) {
		for( int i = 0; i < lfm->num_labels; i++ ) {
			if( i == 0 ) {
				sprintf( t + strlen( t ), "{" );
			}
			else {
				sprintf( t + strlen( t ), "," );
			}
			sprintf( t + strlen( t ),
					 "%s=\"%s\"",
					 lfm->labels[i].key,
					 lfm->labels[i].value ); // TODO escape strings (and adjust buffer size)
		}
		sprintf( t + strlen( t ), "}" );
	}
}
