#ifndef VGUI_DESKTOPICON_H
#define VGUI_DESKTOPICON_H

#include "VGUI_Panel.h"

namespace vgui
{

class Desktop;
class Image;
class MiniApp;

class VGUIAPI DesktopIcon : public Panel
{
public:
	DesktopIcon( MiniApp* miniApp, Image* image );

	virtual void doActivate();
	virtual void setImage( Image* image );
	virtual void setDesktop( Desktop* desktop );
	virtual MiniApp* getMiniApp();

protected:
	virtual void paintBackground() override;

	Desktop*	_desktop;
	MiniApp*	_miniApp;
	Image*		_image;

public:
	DesktopIcon& operator=( const DesktopIcon& ) = default;
};

}

#endif
