#ifndef VGUI_DESKTOP_H
#define VGUI_DESKTOP_H

#include "VGUI.h"
#include "VGUI_Dar.h"
#include "VGUI_Panel.h"

namespace vgui
{

class DesktopIcon;
class TaskBar;

class VGUIAPI Desktop : public Panel
{
public:
	Desktop( int x, int y, int wide, int tall );

	virtual void setSize( int wide, int tall ) override;

	virtual void iconActivated( DesktopIcon* icon );
	virtual void addIcon( DesktopIcon* icon );
	virtual void arrangeIcons();

	virtual Panel* getBackground();
	virtual Panel* getForeground();

private:
	Panel*				_background;
	Panel*				_foreground;
	TaskBar*			_taskBar;

	Dar<DesktopIcon*>	_desktopIconDar;
	int					_cascade[2];

public:
	Desktop& operator=( const Desktop& ) = default;
};

}

#endif
