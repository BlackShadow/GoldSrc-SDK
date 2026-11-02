#include "interface.h"
#include "vgui_internal.h"
#include "vgui_key_translation.h"
#include "KeyValues.h"
#include "vgui/ISystem.h"
#include "vgui/IInputInternal.h"

#ifdef _WIN32
#include "utf8_windows.h"
#endif

#include "SDL_clipboard.h"
#include "SDL_stdinc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/statvfs.h>
#include <sys/time.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#undef GetCurrentTime
#undef ShellExecute
#endif

using namespace vgui2;

static int Q_UnicodeToUTF8( const wchar_t* pUnicode, char* pUTF8, int cubDestSizeInBytes )
{
#ifdef _WIN32
	return windows_utf8::FromUnicode( pUnicode, pUTF8, cubDestSizeInBytes );
#else
	pUTF8[0] = 0;
	int cchResult = wcstombs( pUTF8, pUnicode, cubDestSizeInBytes );
	pUTF8[cubDestSizeInBytes - 1] = 0;
	return cchResult;
#endif
}

static int Q_UTF8ToUnicode( const char* pUTF8, wchar_t* pwchDest, int cubDestSizeInBytes )
{
#ifdef _WIN32
	int result = windows_utf8::ToUnicode( pUTF8, pwchDest, cubDestSizeInBytes );
	return result < 0 ? result : result + 1;
#else
	pwchDest[0] = 0;
	int cchResult = mbstowcs( pwchDest, pUTF8, cubDestSizeInBytes / sizeof( wchar_t ) );
	if ( cchResult >= 0 )
	{
		cchResult++;
	}
	pwchDest[( cubDestSizeInBytes / sizeof( wchar_t ) ) - 1] = 0;
	return cchResult;
#endif
}

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable: 4121 )
#pragma pack( push, 4 )
#endif
class CSystem : public ISystem
{
public:
	CSystem();
	~CSystem() override;

	virtual void Shutdown() override;
	virtual void RunFrame() override;
	virtual void ShellExecute( const char* command, const char* file ) override;

	virtual double GetFrameTime() override;
	virtual double GetCurrentTime() override;
	virtual long GetTimeMillis() override;

	virtual int GetClipboardTextCount() override;
	virtual void SetClipboardText( const char* text, int textLen ) override;
	virtual void SetClipboardText( const wchar_t* text, int textLen ) override;
	virtual int GetClipboardText( int offset, char* buf, int bufLen ) override;
	virtual int GetClipboardText( int offset, wchar_t* buf, int bufLen ) override;

	virtual bool SetRegistryString( const char* key, const char* value ) override;
	virtual bool GetRegistryString( const char* key, char* value, int valueLen ) override;
	virtual bool SetRegistryInteger( const char* key, int value ) override;
	virtual bool GetRegistryInteger( const char* key, int& value ) override;

	virtual KeyValues* GetUserConfigFileData( const char* dialogName, int dialogID ) override;
	virtual void SetUserConfigFile( const char* fileName, const char* pathName ) override;
	virtual void SaveUserConfigFile() override;

	virtual bool SetWatchForComputerUse( bool state ) override;
	virtual double GetTimeSinceLastUse() override;
	virtual int GetAvailableDrives( char* buf, int bufLen ) override;

	virtual bool CommandLineParamExists( const char* paramName ) override;
	virtual const char* GetFullCommandLine() override;
	virtual bool GetCurrentTimeAndDate( int* year, int* month, int* dayOfWeek,
		int* day, int* hour, int* minute, int* second ) override;

	virtual double GetFreeDiskSpace( const char* path ) override;

	virtual bool CreateShortcut( const char* linkFileName, const char* targetPath,
		const char* arguments, const char* workingDirectory, const char* iconFile ) override;
	virtual bool GetShortcutTarget( const char* linkFileName, char* targetPath,
		char* arguments, int destBufferSizes ) override;
	virtual bool ModifyShortcutTarget( const char* linkFileName, const char* targetPath,
		const char* arguments, const char* workingDirectory ) override;

	virtual bool GetCommandLineParamValue( const char* paramName, char* value, int valueBufferSize ) override;
	virtual bool DeleteRegistryKey( const char* keyName ) override;

	virtual const char* GetDesktopFolderPath() override;
	virtual KeyCode KeyCode_VirtualKeyToVGUI( int keyCode ) override;
	virtual int KeyCode_VGUIToVirtualKey( KeyCode keyCode ) override;

	virtual const char* GetStartMenuFolderPath();
	virtual const char* GetAllUserDesktopFolderPath();
	virtual const char* GetAllUserStartMenuFolderPath();

private:
	bool		m_bStaticWatchForComputerUse;
	double		m_StaticLastComputerUseTime;
	int			m_iStaticMouseOldX;
	int			m_iStaticMouseOldY;

	double		m_flFrameTime;

	KeyValues*	m_pUserConfigData;
	char		m_szFileName[4096];
	char		m_szPathID[4096];
};
#ifdef _WIN32
#pragma pack( pop )
#pragma warning( pop )
#endif

CSystem g_System;
namespace vgui2
{
ISystem* g_pSystem = &g_System;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CSystem, ISystem, "VGUI_System009", g_System );

CSystem::CSystem()
{
	m_bStaticWatchForComputerUse = false;
	m_StaticLastComputerUseTime = 0.0;
	m_iStaticMouseOldX = 0;
	m_iStaticMouseOldY = 0;
	m_flFrameTime = 0.0;
	m_pUserConfigData = NULL;
	memset( m_szFileName, 0, sizeof( m_szFileName ) );
	memset( m_szPathID, 0, sizeof( m_szPathID ) );
}

CSystem::~CSystem()
{
}

void CSystem::Shutdown()
{
	if ( m_pUserConfigData )
	{
		m_pUserConfigData->deleteThis();
	}
}

void CSystem::RunFrame()
{
	int x;
	int y;

	m_flFrameTime = GetCurrentTime();

	if ( !m_bStaticWatchForComputerUse )
	{
		return;
	}

	g_pInput->GetCursorPos( x, y );

	if ( abs( ( x + y ) - ( m_iStaticMouseOldX + m_iStaticMouseOldY ) ) > 50 )
	{
		m_StaticLastComputerUseTime = GetTimeMillis() / 1000;
		m_iStaticMouseOldX = x;
		m_iStaticMouseOldY = y;
	}
}

double CSystem::GetFrameTime()
{
	return m_flFrameTime;
}

double CSystem::GetCurrentTime()
{
#ifdef _WIN32
	FILETIME fileTime;
	ULARGE_INTEGER ticks;
	GetSystemTimeAsFileTime( &fileTime );
	ticks.LowPart = fileTime.dwLowDateTime;
	ticks.HighPart = fileTime.dwHighDateTime;

	const unsigned long long ticksPerSecond = 10000000;
	const unsigned long long unixEpochSeconds = 11644473600;
	long long seconds = (long long)( ticks.QuadPart / ticksPerSecond ) - unixEpochSeconds;
	unsigned long microseconds = (unsigned long)( ( ticks.QuadPart % ticksPerSecond ) / 10 );
	static long long secbase;
	if ( !secbase )
	{
		secbase = seconds;
	}

	return ( seconds - secbase ) + microseconds / 1000000.0f;
#else
	struct timeval tp;
	static int secbase;

	gettimeofday( &tp, NULL );

	if ( !secbase )
	{
		secbase = tp.tv_sec;
		return tp.tv_usec / 1000000.0f;
	}

	return ( tp.tv_sec - secbase ) + tp.tv_usec / 1000000.0f;
#endif
}

long CSystem::GetTimeMillis()
{
	return (long)( GetCurrentTime() * 1000.0f );
}

void CSystem::ShellExecute( const char* command, const char* file )
{
#ifdef _WIN32
	::ShellExecuteA( NULL, command, file, NULL, NULL, SW_SHOWNORMAL );
#else
	char szRealCommand[1024];

	snprintf( szRealCommand, sizeof( szRealCommand ), "xdg-open %s", file );
	system( szRealCommand );
#endif
}

void CSystem::SetClipboardText( const char* text, int )
{
	SDL_SetClipboardText( text );
}

void CSystem::SetClipboardText( const wchar_t* text, int )
{
	char szText[2048];

	Q_UnicodeToUTF8( text, szText, sizeof( szText ) );
	SetClipboardText( szText, strlen( szText ) );
}

int CSystem::GetClipboardTextCount()
{
	int len;
	char* clip = SDL_GetClipboardText();

	if ( !clip )
	{
		return 0;
	}

	len = strlen( clip ) + 1;
	SDL_free( clip );
	return len;
}

int CSystem::GetClipboardText( int, char* buf, int bufLen )
{
	if ( !buf || bufLen <= 0 )
		return 0;
	buf[0] = 0;
	char* clip = SDL_GetClipboardText();

	if ( !clip )
	{
		return 0;
	}

	strncpy( buf, clip, bufLen - 1 );
	buf[bufLen - 1] = 0;
	SDL_free( clip );
	return strlen( buf );
}

int CSystem::GetClipboardText( int offset, wchar_t* buf, int bufLen )
{
	char szText[2048];

	GetClipboardText( offset, szText, sizeof( szText ) );
	return Q_UTF8ToUnicode( szText, buf, bufLen );
}

bool CSystem::SetRegistryString( const char*, const char* )
{
	return false;
}

bool CSystem::GetRegistryString( const char*, char*, int )
{
	return false;
}

bool CSystem::SetRegistryInteger( const char*, int )
{
	return false;
}

bool CSystem::GetRegistryInteger( const char*, int& )
{
	return false;
}

bool CSystem::DeleteRegistryKey( const char* )
{
	return false;
}

bool CSystem::SetWatchForComputerUse( bool state )
{
	if ( m_bStaticWatchForComputerUse != state )
	{
		m_bStaticWatchForComputerUse = state;
	}

	return true;
}

double CSystem::GetTimeSinceLastUse()
{
	if ( m_bStaticWatchForComputerUse )
	{
		return ( GetCurrentTime() / 1000.0f - m_StaticLastComputerUseTime ) / 1000.0f;
	}

	return 0.0;
}

int CSystem::GetAvailableDrives( char*, int )
{
	return 0;
}

double CSystem::GetFreeDiskSpace( const char* path )
{
#ifdef _WIN32
	ULARGE_INTEGER freeBytes;
	if ( !GetDiskFreeSpaceExA( path, NULL, NULL, &freeBytes ) )
	{
		return 0.0;
	}

	return (double)freeBytes.QuadPart;
#else
	struct statvfs buf;
	int ret;

	ret = statvfs( path, &buf );
	if ( ret < 0 )
	{
		return 0.0;
	}

	return (double)buf.f_bsize * (double)buf.f_bfree;
#endif
}

KeyValues* CSystem::GetUserConfigFileData( const char* dialogName, int dialogID )
{
	char buf[256];

	if ( !m_pUserConfigData )
	{
		return NULL;
	}

	if ( dialogID )
	{
		snprintf( buf, sizeof( buf ), "%s_%d", dialogName, dialogID );
		dialogName = buf;
	}

	return m_pUserConfigData->FindKey( dialogName, true );
}

void CSystem::SetUserConfigFile( const char* fileName, const char* pathName )
{
	if ( m_pUserConfigData )
	{
		m_pUserConfigData->Clear();
	}
	else
	{
		m_pUserConfigData = new KeyValues( "UserConfigData" );
	}

	strncpy( m_szFileName, fileName, sizeof( m_szFileName ) );
	strncpy( m_szPathID, pathName, sizeof( m_szPathID ) );
	m_pUserConfigData->LoadFromFile( g_pFileSystem, m_szFileName, m_szPathID );
}

void CSystem::SaveUserConfigFile()
{
	if ( m_pUserConfigData )
	{
		m_pUserConfigData->SaveToFile( g_pFileSystem, m_szFileName, m_szPathID );
	}
}

bool CSystem::CommandLineParamExists( const char* )
{
	return false;
}

bool CSystem::GetCommandLineParamValue( const char*, char*, int )
{
	return true;
}

const char* CSystem::GetFullCommandLine()
{
	return "";
}

bool CSystem::GetCurrentTimeAndDate( int*, int*, int*, int*, int*, int*, int* )
{
	return true;
}

bool CSystem::CreateShortcut( const char*, const char*, const char*, const char*, const char* )
{
	return false;
}

bool CSystem::GetShortcutTarget( const char*, char*, char*, int )
{
	return false;
}

bool CSystem::ModifyShortcutTarget( const char*, const char*, const char*, const char* )
{
	return false;
}

const char* CSystem::GetDesktopFolderPath()
{
	return NULL;
}

const char* CSystem::GetStartMenuFolderPath()
{
	return NULL;
}

const char* CSystem::GetAllUserDesktopFolderPath()
{
	return NULL;
}

const char* CSystem::GetAllUserStartMenuFolderPath()
{
	return NULL;
}

KeyCode CSystem::KeyCode_VirtualKeyToVGUI( int keyCode )
{
	return ::KeyCode_VirtualKeyToVGUI( keyCode );
}

int CSystem::KeyCode_VGUIToVirtualKey( KeyCode keyCode )
{
	return ::KeyCode_VGUIToVirtualKey( keyCode );
}
