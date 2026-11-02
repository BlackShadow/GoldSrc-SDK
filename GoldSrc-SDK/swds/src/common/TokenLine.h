// TokenLine.h -- command text and parsed token storage

#if defined( _WIN32 )

#include "../../../HL25-Engine/engine/TokenLine.h"

#else

#ifndef TOKENLINE_H
#define TOKENLINE_H

//-----------------------------------------------------------------------------
// Purpose: retain command text while splitting a separate copy into tokens.
//-----------------------------------------------------------------------------
class TokenLine
{
public:
	TokenLine();
	TokenLine( char* string );
	virtual ~TokenLine();

	bool SetLine( const char* newLine );
	char* GetLine();
	char* GetToken( int i );
	char* CheckToken( char* parm );
	int CountToken();
	char* GetRestOfLine( int i );

	char	m_tokenBuffer[2048];
	char	m_fullLine[2048];
	char*	m_token[128];
	int		m_tokenNumber;
};

#endif

#endif
