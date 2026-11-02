#ifndef VGUI_STACKLAYOUT_H
#define VGUI_STACKLAYOUT_H

#include "VGUI.h"
#include "VGUI_Layout.h"

namespace vgui
{

class Panel;

class VGUIAPI StackLayout : public Layout
{
public:
	StackLayout( int vgap, bool fitWide );

	virtual void performLayout( Panel* panel ) override;

private:
	int		_vgap;
	bool	_fitWide;

public:
	StackLayout& operator=( const StackLayout& ) = default;
};

}

#endif
