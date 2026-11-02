#ifndef VGUI_ACTIONSIGNAL_H
#define VGUI_ACTIONSIGNAL_H

#include "VGUI.h"

namespace vgui
{

class Panel;

class VGUIAPI ActionSignal
{
public:
	virtual void actionPerformed( Panel* panel ) = 0;

	ActionSignal& operator=( const ActionSignal& ) = default;
};

}

#endif
