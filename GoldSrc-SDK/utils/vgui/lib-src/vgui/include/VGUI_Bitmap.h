#ifndef VGUI_BITMAP_H
#define VGUI_BITMAP_H

#include "VGUI_Image.h"

namespace vgui
{

class VGUIAPI Bitmap : public Image
{
public:
	Bitmap();
	virtual void paint( Panel* panel ) override;

protected:
	virtual void setSize( int wide, int tall ) override;
	virtual void setRGBA( int x, int y, uchar r, uchar g, uchar b, uchar a );

private:
	int		_id;
	bool	_uploaded;

protected:
	uchar*	_rgba;

public:
	Bitmap& operator=( const Bitmap& ) = default;
};

}

#endif
