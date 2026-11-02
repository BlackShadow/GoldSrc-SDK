#include "VGUI_ImagePanel.h"

#include "VGUI.h"
#include "VGUI_Image.h"
#include "VGUI_Scheme.h"

namespace vgui
{

ImagePanel::ImagePanel() : Panel(), _image( NULL )
{
}

ImagePanel::ImagePanel( Image* image ) : Panel( 0, 0, 64, 64 )
{
	_image = NULL;
	setImage( image );
}

void ImagePanel::setImage( Image* image )
{
	_image = image;

	if ( _image != NULL )
	{
		int wide;
		int tall;
		_image->getSize( wide, tall );
		setSize( wide, tall );
	}

	repaint();
}

void ImagePanel::paintBackground()
{
	if ( _image != NULL )
	{
		drawSetColor( Scheme::sc_white );
		_image->doPaint( this );
	}
}

}
