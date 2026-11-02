#include "TokenLine.h"

#include <cstring>

static bool IsTokenCharacter( char character )
{
	return character >= '!' && character <= '~';
}

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
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
	char* cursor;

	m_tokenNumber = 0;
	if ( !newLine || strlen( newLine ) > sizeof( m_fullLine ) - 2 )
	{
		memset( m_fullLine, 0, sizeof( m_fullLine ) );
		memset( m_tokenBuffer, 0, sizeof( m_tokenBuffer ) );
		return false;
	}

	const size_t length = strlen( newLine );
	memcpy( m_fullLine, newLine, length );
	memset( m_fullLine + length, 0, sizeof( m_fullLine ) - length );
	memcpy( m_tokenBuffer, newLine, length );
	memset( m_tokenBuffer + length, 0, sizeof( m_tokenBuffer ) - length );

	cursor = m_tokenBuffer;
	const int maxTokens = sizeof( m_token ) / sizeof( m_token[0] );
	while ( *cursor && m_tokenNumber < maxTokens )
	{
		while ( *cursor && !IsTokenCharacter( *cursor ) )
			cursor++;

		if ( !*cursor )
			break;

		if ( *cursor == '"' )
		{
			cursor++;
			m_token[m_tokenNumber] = cursor;
			while ( *cursor && *cursor != '"' )
				cursor++;
		}
		else
		{
			m_token[m_tokenNumber] = cursor;
			while ( IsTokenCharacter( *cursor ) )
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
	int i;

	for ( i = 0; i < m_tokenNumber; i++ )
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
	int count;
	int i;

	count = 0;
	for ( i = 0; i < m_tokenNumber; i++ )
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
