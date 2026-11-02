#include "VGUI_App.h"

#include "VGUI_SurfaceBase.h"

#include <sys/time.h>

using namespace vgui;

void App::platTick()
{
}

void App::setCursorPos( int, int )
{
}

void App::getCursorPos( int& x, int& y )
{
	x = y = 0;
	if ( _surfaceBaseDar[0] != NULL )
	{
		_surfaceBaseDar[0]->GetMousePos( x, y );
	}
}

void App::internalSetMouseArena( int, int, int, int, bool )
{
}

long App::getTimeMillis()
{
	timeval tp;
	static time_t secbase;

	gettimeofday( &tp, NULL );
	if ( secbase != 0 )
	{
		return (long)(long long)( (long double)( 1000 * ( tp.tv_sec - secbase ) )
			+ (long double)tp.tv_usec / 1000.0 );
	}
	secbase = tp.tv_sec;
	return (long)(double)( (long double)tp.tv_usec / 1000000.0 );
}

void App::setClipboardText( const char*, int )
{
}

int App::getClipboardTextCount()
{
	return 0;
}

int App::getClipboardText( int, char*, int )
{
	return 0;
}

int App::getRegistryString( const char*, char*, int )
{
	return 0;
}

int App::setRegistryString( const char*, const char* )
{
	return 0;
}

int App::setRegistryInteger( const char*, int )
{
	return 0;
}

int App::getRegistryInteger( const char*, int& )
{
	return 0;
}

App* App::getInstance()
{
	return _instance;
}
