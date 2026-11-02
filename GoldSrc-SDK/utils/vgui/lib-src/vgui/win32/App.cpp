#include "VGUI_App.h"
#include "VGUI_SurfaceBase.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

#define REGISTRY_NAME_CAPACITY 256

namespace
{

bool SplitRegistryName( const char* key, char* path, char* valueName )
{
	if ( key == NULL )
	{
		return false;
	}

	int len = (int)strlen( key );
	if ( len <= 0 )
	{
		return false;
	}

	int valueStart = -1;
	for ( int i = len - 1; i >= 0; --i )
	{
		if ( key[i] == '\\' )
		{
			break;
		}
		valueStart = i;
	}
	if ( valueStart == -1 )
	{
		return false;
	}

	vgui::vgui_strcpy( path, valueStart + 1, key );
	vgui::vgui_strcpy( valueName, len + 1 - valueStart, key + valueStart );
	return true;
}

}

namespace vgui
{

void App::platTick()
{
	MSG message;
	while ( PeekMessageA( &message, NULL, 0, 0, PM_NOREMOVE ) )
	{
		const BOOL result = GetMessageA( &message, NULL, 0, 0 );
		if ( result == 0 )
		{
			break;
		}
		if ( result == -1 )
		{
			stop();
			return;
		}
		DispatchMessageA( &message );
	}
}

void App::setCursorPos( int, int )
{
}

void App::getCursorPos( int& x, int& y )
{
	_surfaceBaseDar[0]->GetMousePos( x, y );
}

void App::internalSetMouseArena( int x0, int y0, int x1, int y1, bool enabled )
{
	RECT bounds = { x0, y0, x1, y1 };
	ClipCursor( enabled ? &bounds : NULL );
}

long App::getTimeMillis()
{
#pragma warning( suppress : 28159 )
	return (long)GetTickCount();
}

void App::setClipboardText( const char* text, int textLen )
{
	if ( text == NULL || textLen <= 0 || !OpenClipboard( NULL ) )
	{
		return;
	}

	HGLOBAL storage = GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, textLen + 1 );
	if ( storage != NULL )
	{
		char* buffer = (char*)GlobalLock( storage );
		if ( buffer != NULL )
		{
			memset( buffer, 0, textLen + 1 );
			memcpy( buffer, text, textLen );
			GlobalUnlock( storage );
			SetClipboardData( CF_TEXT, storage );
		}
	}
	CloseClipboard();
}

int App::getClipboardTextCount()
{
	if ( !OpenClipboard( NULL ) )
	{
		return 0;
	}
	HANDLE storage = GetClipboardData( CF_TEXT );
	const int count = storage == NULL ? 0 : (int)GlobalSize( storage );
	CloseClipboard();
	return count;
}

int App::getClipboardText( int offset, char* buf, int bufLen )
{
	if ( buf == NULL || bufLen <= 0 || !OpenClipboard( NULL ) )
	{
		return 0;
	}

	int count = 0;
	HANDLE storage = GetClipboardData( CF_TEXT );
	if ( storage != NULL )
	{
		count = (int)GlobalSize( storage ) - offset;
		if ( count <= 0 )
		{
			CloseClipboard();
			return 0;
		}
		if ( count > bufLen )
		{
			count = bufLen;
		}
		const char* text = (const char*)GlobalLock( storage );
		if ( text != NULL )
		{
			memcpy( buf, text + offset, count );
			GlobalUnlock( storage );
		}
	}
	CloseClipboard();
	return count;
}

bool App::setRegistryString( const char* key, const char* value )
{
	char path[REGISTRY_NAME_CAPACITY];
	char valueName[REGISTRY_NAME_CAPACITY];
	HKEY handle;
	if ( SplitRegistryName( key, path, valueName ) &&
		RegCreateKeyExA( HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_NON_VOLATILE,
			KEY_WRITE, NULL, &handle, NULL ) == ERROR_SUCCESS )
	{
		if ( RegSetValueExA( handle, valueName, 0, REG_SZ, (const BYTE*)value,
			(DWORD)( strlen( value ) + 1 ) ) == ERROR_SUCCESS )
		{
			RegCloseKey( handle );
			return true;
		}
		RegCloseKey( handle );
	}
	return false;
}

bool App::getRegistryString( const char* key, char* buf, int bufLen )
{
	char path[REGISTRY_NAME_CAPACITY];
	char valueName[REGISTRY_NAME_CAPACITY];
	HKEY handle;
	if ( SplitRegistryName( key, path, valueName ) &&
		RegOpenKeyExA( HKEY_CURRENT_USER, path, 0, KEY_READ, &handle ) == ERROR_SUCCESS )
	{
		DWORD length = (DWORD)bufLen;
		if ( RegQueryValueExA( handle, valueName, NULL, NULL, (BYTE*)buf, &length ) == ERROR_SUCCESS )
		{
			RegCloseKey( handle );
			return true;
		}
		RegCloseKey( handle );
	}
	return false;
}

bool App::setRegistryInteger( const char* key, int value )
{
	char path[REGISTRY_NAME_CAPACITY];
	char valueName[REGISTRY_NAME_CAPACITY];
	HKEY handle;
	if ( SplitRegistryName( key, path, valueName ) &&
		RegCreateKeyExA( HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_NON_VOLATILE,
			KEY_WRITE, NULL, &handle, NULL ) == ERROR_SUCCESS )
	{
		if ( RegSetValueExA( handle, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof( value ) ) == ERROR_SUCCESS )
		{
			RegCloseKey( handle );
			return true;
		}
		RegCloseKey( handle );
	}
	return false;
}

bool App::getRegistryInteger( const char* key, int& value )
{
	char path[REGISTRY_NAME_CAPACITY];
	char valueName[REGISTRY_NAME_CAPACITY];
	HKEY handle;
	if ( SplitRegistryName( key, path, valueName ) &&
		RegOpenKeyExA( HKEY_CURRENT_USER, path, 0, KEY_READ, &handle ) == ERROR_SUCCESS )
	{
		DWORD length = sizeof( value );
		if ( RegQueryValueExA( handle, valueName, NULL, NULL, (BYTE*)&value, &length ) == ERROR_SUCCESS )
		{
			RegCloseKey( handle );
			return true;
		}
		RegCloseKey( handle );
	}
	return false;
}

App* App::getInstance()
{
	return _instance;
}

}
