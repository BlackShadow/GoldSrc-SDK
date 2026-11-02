#pragma once

#include <cstdlib>

class CUtlString
{
public:
	CUtlString() : m_pchString( NULL )
	{
	}

	~CUtlString()
	{
		std::free( m_pchString );
	}

	char* m_pchString;
};
