//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "info.h"
#include "common.h"
#include "conprint.h"
#include "unicode_strtools.h"

#include <cctype>
#include <cstdio>
#include <cstring>

#if !defined( _WIN32 )
#ifndef _snprintf
#define _snprintf snprintf
#endif
#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif
#endif

static char info_values[INFO_VALUE_BUFFERS][MAX_INFO_VALUE];
static int info_value_index;

static char info_largest_key[MAX_INFO_KEY];

qboolean Info_IsValid( const char* s )
{
	int count;

	if ( *s == '\\' )
		s++;

	while ( *s )
	{
		count = 0;
		while ( *s != '\\' )
		{
			if ( count >= MAX_INFO_KEY - 1 )
				return 0;
			s++;
			count++;
			if ( !*s )
				return 0;
		}
		s++;

		count = 0;
		while ( *s && *s != '\\' )
		{
			if ( count >= MAX_INFO_VALUE - 1 )
				return 0;
			s++;
			count++;
		}
		if ( !count )
			return 0;

		if ( *s )
			s++;
	}

	return 1;
}

qboolean Info_IsKeyImportant( const char* key )
{
	return key[0] == '*'
		|| !Q_strcmp( key, "name" )
		|| !Q_strcmp( key, "model" )
		|| !Q_strcmp( key, "rate" )
		|| !Q_strcmp( key, "topcolor" )
		|| !Q_strcmp( key, "bottomcolor" )
		|| !Q_strcmp( key, "cl_updaterate" )
		|| !Q_strcmp( key, "cl_lw" )
		|| !Q_strcmp( key, "cl_lc" )
		|| !Q_strcmp( key, "*hltv" )
		|| !Q_strcmp( key, "*sid" );
}

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
const char* Info_ValueForKey( const char* s, const char* key )
{
	char pkey[MAX_INFO_KEY];
	char* value;
	int count;

	info_value_index = ( info_value_index + 1 ) % INFO_VALUE_BUFFERS;

	if ( *s == '\\' )
		s++;

	while ( 1 )
	{
		count = 0;
		while ( *s != '\\' )
		{
			if ( count >= MAX_INFO_KEY - 1 )
				break;
			if ( !*s )
				return com_empty_string;
			pkey[count++] = *s++;
		}
		pkey[count] = 0;

		s++;

		value = info_values[info_value_index];
		count = 0;
		while ( *s && *s != '\\' )
		{
			if ( count >= MAX_INFO_VALUE - 1 )
				break;
			value[count++] = *s++;
		}
		value[count] = 0;

		if ( !Q_strcmp( key, pkey ) )
			return value;

		if ( !*s )
			return com_empty_string;
		s++;
	}
}

void Info_RemoveKey( char* s, const char* key )
{
	char* start;
	char pkey[MAX_INFO_KEY];
	int len, count;

	len = (int)strlen( key );
	if ( len > MAX_INFO_KEY - 1 )
		len = MAX_INFO_KEY - 1;

	if ( Q_strstr( key, "\\" ) )
	{
		Con_Printf( "Can't use a key with a \\\n" );
		return;
	}

	while ( 1 )
	{
		start = s;
		if ( *s == '\\' )
			s++;

		count = 0;
		while ( *s != '\\' )
		{
			if ( count >= MAX_INFO_KEY - 1 )
				break;
			if ( !*s )
				return;
			pkey[count++] = *s++;
		}
		pkey[count] = 0;
		s++;

		count = 0;
		while ( *s != '\\' )
		{
			if ( !*s || count >= MAX_INFO_VALUE - 1 )
				break;
			s++;
			count++;
		}

		if ( !Q_strncmp( key, pkey, len ) )
		{
			memmove( start, s, Q_strlen( s ) + 1 ); // remove this part
			return;
		}

		if ( !*s )
			return;
	}
}

void Info_RemovePrefixedKeys( char* start, char prefix )
{
	char* s;
	char pkey[MAX_INFO_KEY];
	int count;

	s = start;

	do
	{
		if ( *s == '\\' )
			s++;

		count = 0;
		while ( *s != '\\' )
		{
			if ( count >= MAX_INFO_KEY - 1 )
				break;
			if ( !*s )
				return;
			pkey[count++] = *s++;
		}
		pkey[count] = 0;
		s++;

		count = 0;
		while ( *s != '\\' )
		{
			if ( !*s || count >= MAX_INFO_VALUE - 1 )
				break;
			s++;
			count++;
		}

		if ( pkey[0] == prefix )
		{
			Info_RemoveKey( start, pkey );
			s = start;
		}
	} while ( *s );
}

static const char* Info_FindLargestKey( const char* s )
{
	char key[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	int count, size, keysize, largestsize;

	info_largest_key[0] = 0;
	largestsize = 0;

	if ( *s == '\\' )
		s++;

	while ( *s )
	{
		count = 0;
		while ( *s && *s != '\\' )
		{
			if ( count >= MAX_INFO_KEY - 1 )
				break;
			key[count++] = *s++;
		}
		key[count] = 0;
		keysize = Q_strlen( key );

		if ( !*s )
			break;
		s++;

		count = 0;
		while ( *s && *s != '\\' )
		{
			if ( count >= MAX_INFO_VALUE - 1 )
				break;
			value[count++] = *s++;
		}
		value[count] = 0;

		if ( *s )
			s++;

		size = Q_strlen( value ) + keysize;
		if ( size > largestsize && !Info_IsKeyImportant( key ) )
		{
			largestsize = size;
			Q_strncpy( info_largest_key, key, MAX_INFO_KEY - 1 );
			info_largest_key[MAX_INFO_KEY - 1] = 0;
		}
	}

	return info_largest_key;
}

static qboolean Info_HasRoom( const char* s, const char* pair, int maxsize )
{
	return Q_strlen( s ) + Q_strlen( pair ) < maxsize;
}

void Info_SetValueForStarKey( char* s, const char* key, const char* value, int maxsize )
{
	char newpair[1024];
	const char* largekey;
	const char* v;
	char* o;
	int c;

	if ( Q_strstr( key, "\\" ) || Q_strstr( value, "\\" ) )
	{
		Con_Printf( "Can't use keys or values with a \\\n" );
		return;
	}

	if ( Q_strstr( key, ".." ) || Q_strstr( value, ".." ) )
		return;

	if ( Q_strstr( key, "\"" ) || Q_strstr( value, "\"" ) )
	{
		Con_Printf( "Can't use keys or values with a \"\n" );
		return;
	}

	if ( Q_strlen( key ) <= 0 || Q_strlen( key ) > MAX_INFO_KEY - 1 || Q_strlen( value ) > MAX_INFO_VALUE - 1 )
	{
		Con_Printf( "Keys and values must be < %i characters and > 0.\n", MAX_INFO_KEY );
		return;
	}

	if ( !Q_UnicodeValidate( value ) )
	{
		Con_Printf( "Values must be valid utf8 text\n" );
		return;
	}

	while ( *Info_ValueForKey( s, key ) )
		Info_RemoveKey( s, key );

	if ( !value || !Q_strlen( value ) )
		return;

	_snprintf( newpair, sizeof( newpair ), "\\%s\\%s", key, value );

	if ( !Info_HasRoom( s, newpair, maxsize ) )
	{
		// no more room in buffer to add key/value
		if ( !Info_IsKeyImportant( key ) )
		{
			Con_Printf( "Info string length exceeded\n" );
			return;
		}

		while ( 1 )
		{
			largekey = Info_FindLargestKey( s );

			Info_RemoveKey( s, largekey );

			if ( Info_HasRoom( s, newpair, maxsize ) )
				break;

			if ( !largekey[0] )
			{
				Con_Printf( "Info string length exceeded\n" );
				return;
			}
		}

		if ( !info_largest_key[0] )
		{
			Con_Printf( "Info string length exceeded\n" );
			return;
		}
	}

	o = s + Q_strlen( s );
	for ( v = newpair; *v; v++ )
	{
		c = (unsigned char)*v;
		if ( !Q_strcasecmp( key, "team" ) )
			c = tolower( c );
		*o++ = (char)c;
	}
	*o = 0;
}

void Info_SetValueForKey( char* s, const char* key, const char* value, int maxsize )
{
	if ( key[0] == '*' )
	{
		Con_Printf( "Can't set * keys\n" );
		return;
	}

	Info_SetValueForStarKey( s, key, value, maxsize );
}

void Info_Print( const char* s )
{
	char key[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	int count;

	if ( *s == '\\' )
		s++;

	while ( *s )
	{
		count = 0;
		while ( *s && *s != '\\' && count < MAX_INFO_KEY - 1 )
			key[count++] = *s++;

		if ( count < 20 )
		{
			Q_memset( key + count, ' ', 20 - count );
			key[20] = 0;
		}
		else
			key[count] = 0;
		Con_Printf( "%s", key );

		if ( !*s )
		{
			Con_Printf( "MISSING VALUE\n" );
			return;
		}
		s++;

		count = 0;
		while ( *s && *s != '\\' && count < MAX_INFO_VALUE - 1 )
			value[count++] = *s++;
		value[count] = 0;

		if ( *s )
			s++;
		Con_Printf( "%s\n", value );
	}
}
