#include "VGUI_BorderPair.h"

namespace vgui
{

void BorderPair::doPaint( Panel* panel )
{
	if ( _border[0] != NULL )
	{
		_border[0]->doPaint( panel );
	}

	if ( _border[1] != NULL )
	{
		_border[1]->doPaint( panel );
	}
}

void BorderPair::paint( Panel* panel )
{
	if ( _border[0] != NULL )
	{
		_border[0]->paint( panel );
	}

	if ( _border[1] != NULL )
	{
		_border[1]->paint( panel );
	}
}

BorderPair::BorderPair( Border* border0, Border* border1 )
{
	_border[0] = border0;
	_border[1] = border1;
}

}
