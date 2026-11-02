#ifndef VGUI_TICKSIGNAL_H
#define VGUI_TICKSIGNAL_H

#include "VGUI.h"

namespace vgui
{

class VGUIAPI TickSignal
{
public:
	virtual void ticked() = 0;

	TickSignal& operator=( const TickSignal& ) = default;
};

}

#endif
