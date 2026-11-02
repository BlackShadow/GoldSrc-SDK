#ifndef VGUI_TASKBAR_H
#define VGUI_TASKBAR_H

#include "VGUI.h"
#include "VGUI_Dar.h"
#include "VGUI_Panel.h"

namespace vgui
{

class Frame;
class Button;

class VGUIAPI TaskBar : public Panel
{
public:
	TaskBar( int x, int y, int wide, int tall );

	virtual void addFrame( Frame* frame );

protected:
	virtual void performLayout() override;

	Dar<Frame*>		_frameDar;
	Dar<Button*>	_taskButtonDar;
	Panel*			_tray;

public:
	TaskBar& operator=( const TaskBar& ) = default;
};

}

#endif
