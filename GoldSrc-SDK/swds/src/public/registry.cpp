// registry.cpp -- engine settings stored in the Windows registry

#include "iregistry.h"

#ifdef _WIN32
#include <windows.h>
#include <string.h>
#endif

//-----------------------------------------------------------------------------
// Purpose: maintains the handle used for engine setting queries
//-----------------------------------------------------------------------------
class CRegistry : public IRegistry
{
public:
	CRegistry();
	virtual ~CRegistry();

	void Init() override;
	void Shutdown() override;
	int ReadInt( const char* key, int defaultValue ) override;
	void WriteInt( const char* key, int value ) override;

	const char* ReadString( const char* key, const char* defaultValue ) override;
	void WriteString( const char* key, const char* value ) override;

private:
	bool	m_bValid;
#ifdef _WIN32
	HKEY	m_hKey;
#else
	int		m_hKey;
#endif
};

static CRegistry g_Registry;
IRegistry* registry = &g_Registry;

CRegistry::CRegistry() : m_bValid( false ), m_hKey( 0 )
{
}

CRegistry::~CRegistry()
{
}

#ifdef _WIN32
void CRegistry::Init()
{
	char subKey[1024];
	DWORD disposition;

	wsprintfA( subKey, "Software\\Valve\\%s\\Settings\\", "Half-Life" );
	m_bValid = RegCreateKeyExA( HKEY_CURRENT_USER, subKey, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &m_hKey, &disposition ) == ERROR_SUCCESS;
}

void CRegistry::Shutdown()
{
	if ( !m_bValid )
		return;

	m_bValid = false;
	RegCloseKey( m_hKey );
}

int CRegistry::ReadInt( const char* key, int defaultValue )
{
	if ( !m_bValid )
		return defaultValue;

	int value;
	DWORD type;
	DWORD size = sizeof( value );

	if ( RegQueryValueExA( m_hKey, key, NULL, &type, (BYTE*)&value, &size ) != ERROR_SUCCESS
		|| type != REG_DWORD )
		return defaultValue;

	return value;
}

void CRegistry::WriteInt( const char* key, int value )
{
	if ( !m_bValid )
		return;

	RegSetValueExA( m_hKey, key, 0, REG_DWORD, (const BYTE*)&value, sizeof( value ) );
}

const char* CRegistry::ReadString( const char* key, const char* defaultValue )
{
	static char value[512];
	DWORD type;
	DWORD size = sizeof( value );

	value[0] = '\0';
	if ( !m_bValid )
		return defaultValue;

	if ( RegQueryValueExA( m_hKey, key, NULL, &type, (BYTE*)value, &size ) != ERROR_SUCCESS
		|| type != REG_SZ )
		return defaultValue;

	return value;
}

void CRegistry::WriteString( const char* key, const char* value )
{
	if ( !m_bValid )
		return;

	RegSetValueExA( m_hKey, key, 0, REG_SZ, (const BYTE*)value, (DWORD)( strlen( value ) + 1 ) );
}
#else
void CRegistry::Init()
{
}

void CRegistry::Shutdown()
{
}

int CRegistry::ReadInt( const char*, int defaultValue )
{
	return defaultValue;
}

void CRegistry::WriteInt( const char*, int )
{
}

const char* CRegistry::ReadString( const char*, const char* defaultValue )
{
	return defaultValue;
}

void CRegistry::WriteString( const char*, const char* )
{
}
#endif
