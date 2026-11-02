// TokenLine.cpp -- parse command text into tokens

#include "TokenLine.h"

#include <cstring>

TokenLine::TokenLine()
{
}

TokenLine::TokenLine( char* string )
{
	SetLine( string );
}

TokenLine::~TokenLine()
{
}

bool TokenLine::SetLine( const char* newLine )
{
	m_tokenNumber = 0;
	if ( !newLine || strlen( newLine ) > sizeof( m_fullLine ) - 2 )
	{
		memset( m_fullLine, 0, sizeof( m_fullLine ) );
		memset( m_tokenBuffer, 0, sizeof( m_tokenBuffer ) );
		return false;
	}

	strncpy( m_fullLine, newLine, sizeof( m_fullLine ) - 1 );
	m_fullLine[sizeof( m_fullLine ) - 1] = '\0';
	strncpy( m_tokenBuffer, newLine, sizeof( m_tokenBuffer ) - 1 );
	m_tokenBuffer[sizeof( m_tokenBuffer ) - 1] = '\0';

	char* cursor = m_tokenBuffer;
	const int maxTokens = sizeof( m_token ) / sizeof( m_token[0] );
	while ( *cursor && m_tokenNumber < maxTokens )
	{
		while ( *cursor && ( *cursor < '!' || *cursor > '~' ) )
			cursor++;

		if ( !*cursor )
			break;

		m_token[m_tokenNumber] = cursor;
		if ( *cursor == '"' )
		{
			m_token[m_tokenNumber] = ++cursor;
			while ( *cursor && *cursor != '"' )
				cursor++;
		}
		else
		{
			while ( *cursor >= '!' && *cursor <= '~' )
				cursor++;
		}

		m_tokenNumber++;
		if ( !*cursor )
			break;

		*cursor++ = '\0';
	}

	return m_tokenNumber != maxTokens;
}

char* TokenLine::GetLine()
{
	return m_fullLine;
}

char* TokenLine::GetToken( int i )
{
	if ( i >= m_tokenNumber )
		return NULL;

	return m_token[i];
}

char* TokenLine::CheckToken( char* parm )
{
	for ( int i = 0; i < m_tokenNumber; i++ )
	{
		if ( !m_token[i] || strcmp( parm, m_token[i] ) )
			continue;

		if ( i + 1 == m_tokenNumber )
			return (char*)"";

		return m_token[i + 1];
	}

	return NULL;
}

int TokenLine::CountToken()
{
	int count = 0;
	for ( int i = 0; i < m_tokenNumber; i++ )
	{
		if ( m_token[i] )
			count++;
	}

	return count;
}

char* TokenLine::GetRestOfLine( int i )
{
	if ( i >= m_tokenNumber )
		return NULL;

	return m_fullLine + ( m_token[i] - m_tokenBuffer );
}
