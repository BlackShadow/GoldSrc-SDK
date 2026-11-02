#include <stdio.h>
#include <stdarg.h>
#include <string.h>

namespace vgui
{

int vgui_dprintf( const char* format, ... )
{
	char buf[2048];
	va_list argList;
	int ret;

	va_start( argList, format );
	ret = vsnprintf( buf, sizeof( buf ), format, argList );
	va_end( argList );
	fputs( buf, stderr );
	return ret;
}

int vgui_dprintf2( const char* format, ... )
{
	static int ctr;
	char buf[2048];
	va_list argList;
	int ret;

	snprintf( buf, sizeof( buf ), "%d:", ctr++ );
	va_start( argList, format );
	const size_t prefix = strlen( buf );
	ret = vsnprintf( buf + prefix, sizeof( buf ) - prefix, format, argList );
	va_end( argList );
	fputs( buf, stderr );
	return ret;
}

}
