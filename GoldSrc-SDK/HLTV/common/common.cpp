#include "HLTV/common/common.h"

#include "HLTV/common/mem.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined( _WIN32 )
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#endif

extern "C"
{
	bool s_com_token_unget;
	char com_token[1024];
}

extern "C" void* Mem_ZeroMalloc( unsigned int size )
{
	void* p = malloc( size );
	if ( p )
		memset( p, 0, size );
	return p;
}

int COM_BuildNumber()
{
	static int b;
	const char* date;
	const char* mon[12];
	int mond[12];
	int month;
	int days;
	int year;

	if ( b != 0 )
		return b;

	date = __DATE__;
	mon[0] = "Jan";
	mon[1] = "Feb";
	mon[2] = "Mar";
	mon[3] = "Apr";
	mon[4] = "May";
	mon[5] = "Jun";
	mon[6] = "Jul";
	mon[7] = "Aug";
	mon[8] = "Sep";
	mon[9] = "Oct";
	mon[10] = "Nov";
	mon[11] = "Dec";
	mond[0] = 31;
	mond[1] = 28;
	mond[2] = 31;
	mond[3] = 30;
	mond[4] = 31;
	mond[5] = 30;
	mond[6] = 31;
	mond[7] = 31;
	mond[8] = 30;
	mond[9] = 31;
	mond[10] = 30;
	mond[11] = 31;

	month = 0;
	days = 0;
	while ( month < 11 )
	{
		if ( strncmp( date, mon[month], 3 ) == 0 )
			break;
		days += mond[month];
		month++;
	}

	days += atoi( date + 4 ) - 1;
	year = atoi( date + 7 ) - 1900;
	days += (int)( ( year - 1 ) * 365.25 );
	if ( ( year % 4 ) == 0 && month > 1 )
		days++;

	b = days - 34995;
	return b;
}

char* COM_TimeString()
{
	static char timedate[16];
	time_t ltime;

	time( &ltime );
	strftime( timedate, sizeof( timedate ), "%y%m%d%H%M", localtime( &ltime ) );
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
	char* scan;
	char c;

	last = pathname;
	c = *pathname;
	if ( c != '\0' )
	{
		scan = pathname + 1;
		do
		{
			if ( c == '/' || c == '\\' )
				last = scan;
			c = *scan++;
		}
		while ( c != '\0' );
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
	int length;
	int end;
	int start;
	int count;

	length = (int)strlen( in );
	end = length - 1;
	while ( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;

	if ( in[end] != '.' )
		end = length - 1;
	else
		end--;

	start = length - 1;
	while ( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if ( start < 0 )
		start = 0;
	else
		start++;

	count = end - start + 1;
	strncpy( out, &in[start], count );
	out[count] = '\0';
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

	com_token[0] = '\0';
	if ( data == NULL )
		return NULL;

	for ( ;; )
	{
		c = *data;
		while ( c <= ' ' )
		{
			if ( c == '\0' )
				return NULL;
			data++;
			c = *data;
		}

		if ( c != '/' )
			break;
		if ( data[1] != '/' )
			break;

		while ( data[1] != '\0' && data[1] != '\n' )
			data++;
		data++;
	}

	if ( c == '"' )
	{
		data++;
		len = 0;
		for ( ;; )
		{
			c = *data++;
			if ( c == '"' || c == '\0' )
			{
				com_token[len] = '\0';
				return data;
			}
			com_token[len] = (char)c;
			len++;
		}
	}

	if ( c == '{' || c == '}' || c == '(' || c == ')' || c == '\'' || c == ',' || c == ':' )
	{
		com_token[0] = (char)c;
		com_token[1] = '\0';
		return data + 1;
	}

	len = 0;
	for ( ;; )
	{
		com_token[len] = (char)c;
		data++;
		len++;
		c = *data;
		if ( c == '{' || c == '}' || c == '(' || c == ')' || c == '\'' || c == ',' || c == ':' )
			break;
		if ( c <= ' ' )
			break;
	}
	com_token[len] = '\0';
	return data;
}

char* COM_VarArgs( char* format, ... )
{
	static char string[1024];
	va_list argptr;

	va_start( argptr, format );
	_vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );
	return string;
}

unsigned char COM_Nibble( char c )
{
	unsigned char value;

	value = (unsigned char)( c - '0' );
	if ( value > 9 )
	{
		if ( (unsigned char)( c - 'A' ) <= 5 )
			return (unsigned char)( c - 'A' + 10 );

		value = '0';
		if ( (unsigned char)( c - 'a' ) <= 5 )
			return (unsigned char)( c - 'a' + 10 );
	}
	return value;
}

void COM_HexConvert( const char* pszInput, int nInputLength, unsigned char* pOutput )
{
	int i;
	unsigned char high;
	unsigned char low;

	if ( nInputLength <= 0 )
		return;

	i = 0;
	for ( ;; )
	{
		high = COM_Nibble( pszInput[i] );
		low = COM_Nibble( pszInput[i + 1] );
		i += 2;
		*pOutput++ = (unsigned char)( ( high << 4 ) | low );
		if ( nInputLength <= i )
			return;
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
	unsigned char c;
	char szChunk[10];
	int i;

	memset( szReturn, 0, sizeof( szReturn ) );

	for ( i = 0; i < nLen; i++ )
	{
		c = buf[i];
		_snprintf( szChunk, sizeof( szChunk ), "%02x", c );
		strncat( szReturn, szChunk, sizeof( szReturn ) - strlen( szReturn ) - 1 );
	}

	return szReturn;
}

char* COM_FormatTime( float seconds )
{
	static char time[32];
	int whole;
	int hours;
	int minutes;
	int secs;
	float remain;

	if ( seconds < 3600.0f )
	{
		whole = (int)seconds;
		minutes = (int)( seconds / 60.0f );
		secs = whole % 60;
		_snprintf( time, sizeof( time ), "%02i:%02i", minutes, secs );
	}
	else
	{
		hours = (int)seconds / 3600;
		remain = seconds - (float)( hours * 3600 );
		whole = (int)remain;
		minutes = (int)( remain / 60.0f );
		secs = whole % 60;
		_snprintf( time, sizeof( time ), "%02i:%2i:%02i", hours, minutes, secs );
	}
	return time;
}

void COM_RemoveEvilChars( char* string )
{
	unsigned char c;

	if ( string == NULL )
		return;

	while ( ( c = (unsigned char)*string ) != 0 )
	{
		if ( c == ';' || c == '%' || (unsigned char)( c - 32 ) > 0x5E )
			*string = ' ';
		string++;
	}
}

int COM_FileNameCmp( const char* file1, const char* file2 )
{
	char c1;
	char c2;

	for ( ;; )
	{
		c1 = *file1++;
		c2 = *file2++;

		if ( ( c1 == '\\' && c2 == '/' ) || ( c1 == '/' && c2 == '\\' ) )
			continue;

		if ( tolower( c1 ) != tolower( c2 ) )
			return -1;
		if ( c1 == '\0' || c2 == '\0' )
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
	int length;
	int start;
	int end;
	int count;
	const char* src;

	length = (int)strlen( source );
	start = 0;
	src = source;
	if ( *source != '\0' )
	{
		while ( COM_IsWhiteSpace( *src ) )
		{
			src++;
			start++;
			if ( *src == '\0' )
				break;
		}
	}

	end = length - 1;
	if ( end > 0 )
	{
		while ( COM_IsWhiteSpace( source[end] ) )
		{
			if ( --end == 0 )
				break;
		}
	}

	count = end - start + 1;
	if ( count > 0 )
	{
		strncpy( dest, src, count );
		dest[count] = '\0';
	}
	else
	{
		*dest = '\0';
	}
}

void COM_UnpackRGB( int& r, int& g, int& b, unsigned int ulRGB )
{
	r = ( ulRGB & 0xFF0000 ) >> 16;
	g = ( ulRGB >> 8 ) & 0xFF;
	b = ulRGB & 0xFF;
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

	while ( *in != '\0' && *in != '.' )
		in++;

	if ( *in == '\0' )
		return "";

	in++;
	for ( i = 0; i < 7 && *in != '\0'; i++, in++ )
		exten[i] = *in;
	exten[i] = '\0';
	return exten;
}

char* COM_LastFileExtension( char* in )
{
	char* scan;
	int last;

	if ( in == NULL || *in == '\0' )
		return "";

	last = -1;
	scan = in;
	do
	{
		if ( *scan == '.' )
			last = (int)( scan - in );
		scan++;
	}
	while ( *scan != '\0' );

	if ( last < 0 )
		return "";

	return COM_FileExtension( in + last );
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension( char* path, char* extension )
{
	size_t length;
	char* src;

	length = strlen( path );
	src = &path[length - 1];
	if ( path != src )
	{
		while ( *src != '/' )
		{
			if ( *src == '.' )
				return;
			src--;
			if ( path == src )
				break;
		}
	}

	strcpy( &path[length], extension );
}
