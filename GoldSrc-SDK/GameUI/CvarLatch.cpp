#include "CvarLatch.h"
#include "GameUI_Interface.h"

#include "wrect.h"
#include "cl_dll.h"

#include <string.h>

CvarLatch::CvarLatch( const char* cvarName, const char* defaultValue )
{
	const char* name = cvarName ? cvarName : "";
	m_cvarName = new char[strlen( name ) + 1];
	strcpy( m_cvarName, name );

	const char* value = defaultValue;
	if ( engine->pfnGetCvarPointer( m_cvarName ) )
	{
		value = engine->pfnGetCvarString( m_cvarName );
		if ( !value )
			value = "";
	}

	m_oldValue = NULL;
	if ( value )
	{
		m_oldValue = new char[strlen( value ) + 1];
		strcpy( m_oldValue, value );
	}
}

CvarLatch::~CvarLatch()
{
	if ( m_oldValue )
	{
		engine->Cvar_Set( m_cvarName, m_oldValue );
		delete[] m_oldValue;
		m_oldValue = NULL;
	}

	delete[] m_cvarName;
#ifdef _WIN32
	m_cvarName = NULL;
#endif
}
