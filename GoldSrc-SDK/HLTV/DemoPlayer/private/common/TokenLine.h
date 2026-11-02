#ifndef TOKENLINE_H
#define TOKENLINE_H

class TokenLine
{
	static const int BufferSize = 2048;
	static const int MaxTokens = 128;
	static const int MaxLineLength = BufferSize - 2;

public:
	TokenLine();
	TokenLine( char* string );
	virtual ~TokenLine();

	char* GetRestOfLine( int i );
	int CountToken();
	char* CheckToken( char* parm );
	char* GetToken( int i );
	char* GetLine();
	bool SetLine( const char* newLine );

	char			m_tokenBuffer[BufferSize];
	char			m_fullLine[BufferSize];
	char*			m_token[MaxTokens];
	int				m_tokenNumber;
};

#endif
