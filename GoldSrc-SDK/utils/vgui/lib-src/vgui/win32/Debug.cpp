#include "VGUI.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int vgui::vgui_dprintf( const char* format, ... )
{
	char buffer[2048] = {};
	va_list args;
	va_start( args, format );
	const int length = vsnprintf( buffer, sizeof( buffer ), format, args );
	va_end( args );
	OutputDebugStringA( buffer );
	return length;
}

int vgui::vgui_dprintf2( const char* format, ... )
{
	static int counter;
	char buffer[2048] = {};
	snprintf( buffer, sizeof( buffer ), "%d:", counter++ );
	const size_t prefix = strlen( buffer );
	va_list args;
	va_start( args, format );
	const int length = vsnprintf( buffer + prefix, sizeof( buffer ) - prefix, format, args );
	va_end( args );
	OutputDebugStringA( buffer );
	return length;
}
