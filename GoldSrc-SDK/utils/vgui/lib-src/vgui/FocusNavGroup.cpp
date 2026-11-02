#include "VGUI_FocusNavGroup.h"
#include "VGUI_Dar.h"
#include "VGUI.h"
#include "VGUI_Panel.h"

using namespace vgui;

void FocusNavGroup::requestFocusPrev()
{
	if ( _panelDar.getCount() == 0 )
	{
		return;
	}

	_currentIndex--;
	if ( _currentIndex < 0 )
	{
		_currentIndex = _panelDar.getCount() - 1;
	}

	_panelDar[_currentIndex]->requestFocus();
}

void FocusNavGroup::requestFocusNext()
{
	if ( _panelDar.getCount() == 0 )
	{
		return;
	}

	_currentIndex++;
	if ( _currentIndex >= _panelDar.getCount() )
	{
		_currentIndex = 0;
	}

	_panelDar[_currentIndex]->requestFocus();
}

void FocusNavGroup::setCurrentPanel( Panel* panel )
{
	for ( int i = 0; i < _panelDar.getCount(); i++ )
	{
		if ( _panelDar[i] == panel )
		{
			_currentIndex = i;
			return;
		}
	}
}

void FocusNavGroup::addPanel( Panel* panel )
{
	_panelDar.putElement( panel );
}

FocusNavGroup::FocusNavGroup()
{
	_currentIndex = 0;
}
