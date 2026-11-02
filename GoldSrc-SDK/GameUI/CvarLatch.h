#pragma once

class CvarLatch
{
public:
	CvarLatch( const char* cvarName, const char* defaultValue );
	~CvarLatch();

private:
	char*	m_cvarName;
	char*	m_oldValue;
};
