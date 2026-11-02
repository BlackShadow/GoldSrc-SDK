#ifndef VGUI_FLOWLAYOUT_H
#define VGUI_FLOWLAYOUT_H

#include "VGUI.h"
#include "VGUI_Layout.h"

namespace vgui
{

class Panel;

class VGUIAPI FlowLayout : public Layout
{
public:
	FlowLayout( int hgap );

	virtual void performLayout( Panel* panel ) override;

private:
	int	_hgap;

public:
	FlowLayout& operator=( const FlowLayout& ) = default;
};

}

#endif
