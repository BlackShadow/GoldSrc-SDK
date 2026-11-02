#ifndef VGUI_IMAGEPANEL_H
#define VGUI_IMAGEPANEL_H

#include "VGUI_Panel.h"

namespace vgui
{

class Image;

class VGUIAPI ImagePanel : public Panel
{
public:
	ImagePanel();
	ImagePanel( Image* image );

	virtual void setImage( Image* image );

protected:
	virtual void paintBackground() override;

	Image*	_image;

public:
	ImagePanel& operator=( const ImagePanel& ) = default;
};

}

#endif
