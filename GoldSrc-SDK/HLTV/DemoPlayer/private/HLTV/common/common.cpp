#include "HLTV/common/mem.h"
#include "public/steam/steamtypes.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUILD_EPOCH_YEAR		1901
#define BUILD_EPOCH_OFFSET	34995

#if defined( _WIN32 )
#define COMPATIBILITY_BUILD_DATE	"Oct  7 2024"
#else
#define COMPATIBILITY_BUILD_DATE	"Oct  8 2024"
#endif

char com_token[1024];
bool s_com_token_unget;

void* Mem_ZeroMalloc( unsigned int size )
{
	void* p;

#if defined( _WIN32 )
	p = malloc( size );
	if ( p )
		memset( p, 0, size );
#else
	p = calloc( size, 1 );
#endif
	return p;
}

int COM_BuildNumber()
{
	int m;
	int d;
	int y;
	static int b = 0;

	if ( !b )
	{
		const char* date = COMPATIBILITY_BUILD_DATE;
		const char* mon[12] =
		{
			"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
		};
		const int mond[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

		for ( m = 0; m < 11; m++ )
		{
			if ( strncmp( date, mon[m], 3 ) == 0 )
				break;
		}

		d = atoi( date + 4 ) - 1;
		y = atoi( date + 7 );
		b = d + mond[m] + (int)( ( y - BUILD_EPOCH_YEAR ) * 365.25 );

		if ( ( y % 4 ) == 0 && m > 1 )
			b += 1;

		b -= BUILD_EPOCH_OFFSET;
	}

	return b;
}

char* COM_TimeString()
{
	static char timedate[16];
	time_t ltime;
	struct tm* now;

	time( &ltime );
#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
	now = localtime( &ltime );
#if defined( _MSC_VER )
#pragma warning( pop )
#endif
	strftime( timedate, sizeof( timedate ), "%y%m%d%H%M", now );
	return timedate;
}

/*
============
COM_SkipPath
============
*/
char* COM_SkipPath( char* pathname )
{
	char* last;

	last = pathname;

	while ( *pathname )
	{
		if ( *pathname == '/' || *pathname == '\\' )
			last = pathname + 1;

		pathname++;
	}

	return last;
}

/*
============
COM_FileBase
============
*/
void COM_FileBase( char* in, char* out )
{
	int len;
	int start;
	int end;

	len = (int)strlen( in );
	end = len - 1;

	while ( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;

	if ( in[end] != '.' )
		end = len - 1;
	else
		end--;

	start = len - 1;

	while ( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if ( start >= 0 && ( in[start] == '/' || in[start] == '\\' ) )
		start++;
	else
		start = 0;

	len = end - start + 1;
#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
	strncpy( out, &in[start], len );
#if defined( _MSC_VER )
#pragma warning( pop )
#endif
	out[len] = 0;
}

void COM_UngetToken()
{
	s_com_token_unget = true;
}

char* COM_Parse( char* data )
{
	int c;
	int len;

	if ( s_com_token_unget )
	{
		s_com_token_unget = false;
		return data;
	}

	len = 0;
	com_token[0] = 0;

	if ( !data )
		return NULL;

skipwhite:
	while ( ( c = *data ) <= ' ' )
	{
		if ( c == 0 )
			return NULL;

		data++;
	}

	if ( c == '/' && data[1] == '/' )
	{
		while ( *data && *data != '\n' )
			data++;

		goto skipwhite;
	}

	if ( c == '\"' )
	{
		data++;

		while ( 1 )
		{
			c = *data++;

			if ( c == '\"' || !c )
			{
				com_token[len] = 0;
				return data;
			}

			com_token[len] = (char)c;
			len++;
		}
	}

	if ( c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',' || c == ':' )
	{
		com_token[len] = (char)c;
		len++;
		com_token[len] = 0;
		return data + 1;
	}

	do
	{
		com_token[len] = (char)c;
		data++;
		len++;
		c = *data;

		if ( c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',' || c == ':' )
			break;
	}
	while ( c > ' ' );

	com_token[len] = 0;
	return data;
}

char* COM_VarArgs( char* format, ... )
{
	va_list argptr;
	static char string[1024];

	va_start( argptr, format );
	vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );
	return string;
}

unsigned char COM_Nibble( char c )
{
	if ( (unsigned char)( c - '0' ) <= 9 )
		return (unsigned char)( c - '0' );

	if ( (unsigned char)( c - 'A' ) <= 5 )
		return (unsigned char)( c - 'A' + 10 );

	if ( (unsigned char)( c - 'a' ) <= 5 )
		return (unsigned char)( c - 'a' + 10 );

	return '0';
}

void COM_HexConvert( const char* pszInput, int nInputLength, unsigned char* pOutput )
{
	unsigned char* p;
	int i;
	const char* pIn;

	p = pOutput;
	pIn = pszInput;

	for ( i = 0; i < nInputLength; i += 2 )
	{
		*p++ = (unsigned char)( ( COM_Nibble( pIn[0] ) << 4 ) | COM_Nibble( pIn[1] ) );
		pIn += 2;
	}
}

/*
==============================
COM_BinPrintf

==============================
*/
char* COM_BinPrintf( unsigned char* buf, int nLen )
{
	static char szReturn[4096];
	static const char digits[] = "0123456789abcdef";
	size_t position = 0;
	int i;

	memset( szReturn, 0, sizeof( szReturn ) );

	for ( i = 0; i < nLen; i++ )
	{
		unsigned char value = buf[i];
		if ( position < sizeof( szReturn ) - 1 )
			szReturn[position++] = digits[value >> 4];
		if ( position < sizeof( szReturn ) - 1 )
			szReturn[position++] = digits[value & 0x0F];
	}

	return szReturn;
}

char* COM_FormatTime( float seconds )
{
	int hours;
	static char time[32];

	if ( seconds < 3600.0f )
	{
		snprintf( time, sizeof( time ), "%02i:%02i", (int)( seconds / 60 ), (int)seconds % 60 );
	}
	else
	{
		float left;

		hours = (int)seconds / 3600;
		left = seconds - (float)( hours * 3600 );
		snprintf( time, sizeof( time ), "%02i:%2i:%02i", hours, (int)( left / 60 ), (int)left % 60 );
	}

	return time;
}

void COM_RemoveEvilChars( char* string )
{
	while ( string )
	{
		char c;

		c = *string;

		if ( !c )
			break;

		if ( c == ';' || c == '%' || c < ' ' || c > '~' )
			*string = ' ';

		string++;
	}
}

int COM_FileNameCmp( const char* file1, const char* file2 )
{
	for ( ;; )
	{
		char c1;
		char c2;

		c1 = *file1++;
		c2 = *file2++;

		if ( ( c1 == '/' && c2 == '\\' ) || ( c1 == '\\' && c2 == '/' ) )
			continue;

		if ( tolower( c1 ) != tolower( c2 ) )
			return -1;

		if ( c1 == 0 || c2 == 0 )
			return 0;
	}
}

bool COM_IsWhiteSpace( char space )
{
	if ( space == ' ' || space == '\t' )
		return true;

	return space == '\r' || space == '\n';
}

void COM_TrimSpace( const char* source, char* dest )
{
	int start;
	int end;
	int length;

	start = 0;

	while ( source[start] && COM_IsWhiteSpace( source[start] ) )
		start++;

	end = (int)strlen( source ) - 1;

	while ( end > 0 && COM_IsWhiteSpace( source[end] ) )
		end--;

	length = end + 1 - start;

	if ( length <= 0 )
	{
		*dest = 0;
	}
	else
	{
#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
		strncpy( dest, source + start, length );
#if defined( _MSC_VER )
#pragma warning( pop )
#endif
		dest[length] = 0;
	}
}

void COM_UnpackRGB( int& r, int& g, int& b, uint32 ulRGB )
{
	r = (int)( ( ulRGB & 0xFF0000 ) >> 16 );
	g = (int)( ( ulRGB >> 8 ) & 0xFF );
	b = (int)( ulRGB & 0xFF );
}

unsigned int COM_PackRGB( int r, int g, int b )
{
	return ( (unsigned char)r << 16 ) | ( (unsigned char)g << 8 ) | (unsigned char)b;
}

/*
============
COM_FileExtension
============
*/
char* COM_FileExtension( char* in )
{
	static char exten[8];
	int i;

	while ( *in && *in != '.' )
		in++;

	if ( !*in )
		return (char*)"";

	in++;

	for ( i = 0; i < 7 && in[i]; i++ )
		exten[i] = in[i];

	exten[i] = 0;
	return exten;
}

char* COM_LastFileExtension( char* in )
{
	char* cur;
	int i;

	if ( !in || !*in )
		return (char*)"";

	i = -1;

	for ( cur = in; *cur; cur++ )
	{
		if ( *cur == '.' )
			i = (int)( cur - in );
	}

	if ( i < 0 )
		return (char*)"";

	return COM_FileExtension( in + i );
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension( char* path, char* extension )
{
	char* src;

	src = path + strlen( path ) - 1;

	while ( *src != '/' && src != path )
	{
		if ( *src == '.' )
			return;

		src--;
	}

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4996 )
#endif
	strcat( path, extension );
#if defined( _MSC_VER )
#pragma warning( pop )
#endif
}
