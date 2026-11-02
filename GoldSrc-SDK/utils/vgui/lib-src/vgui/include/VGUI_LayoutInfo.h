#ifndef VGUI_LAYOUTINFO_H
#define VGUI_LAYOUTINFO_H

#include "VGUI.h"

namespace vgui
{

class VGUIAPI LayoutInfo
{
private:
	virtual LayoutInfo* getThis() = 0;

public:
	LayoutInfo& operator=( const LayoutInfo& ) = default;
};

}

#endif
