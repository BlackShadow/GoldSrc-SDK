#pragma once

class TokenLine
{
public:
	static constexpr int BufferSize = 2048;
	static constexpr int MaxTokens = 128;

	TokenLine();
	TokenLine( char* string );
	virtual ~TokenLine();

	bool SetLine( const char* newLine );

	char* GetLine();
	char* GetToken( int i );
	char* CheckToken( char* parm );
	int CountToken();
	char* GetRestOfLine( int i );

	char m_tokenBuffer[BufferSize];
	char m_fullLine[BufferSize];
	char* m_token[MaxTokens];
	int m_tokenNumber;
};
