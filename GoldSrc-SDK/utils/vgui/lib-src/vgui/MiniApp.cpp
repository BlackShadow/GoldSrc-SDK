#include "VGUI_MiniApp.h"

#include <cstring>

namespace vgui
{

MiniApp::MiniApp()
{
	_name = (char*)"";
}

void MiniApp::getName( char* name, int nameLen )
{
	int len;

	if ( name == NULL )
	{
		return;
	}

	len = (int)strlen( _name ) + 1;
	if ( len > nameLen )
	{
		len = nameLen;
	}

	memcpy( name, _name, len - 1 );
	name[len - 1] = 0;
}

void MiniApp::setName( const char* name )
{
#if defined( _WIN32 )
	_name = _strdup( name );
#else
	_name = strdup( name );
#endif
}

}
