#ifndef VGUI_LAYOUT_H
#define VGUI_LAYOUT_H

#include "VGUI.h"

namespace vgui
{

class Panel;

class VGUIAPI Layout
{
public:
	Layout();

	virtual void performLayout( Panel* panel );

	Layout& operator=( const Layout& ) = default;
};

}

#endif
