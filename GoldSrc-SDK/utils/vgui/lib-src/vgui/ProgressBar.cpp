#include "VGUI_ProgressBar.h"

#include <math.h>

using namespace vgui;

ProgressBar::ProgressBar( int segmentCount ) : Panel( 0, 0, 10, 110 )
{
	_segmentCount = segmentCount;
	_progress = 0.0f;
}

void ProgressBar::setProgress( float progress )
{
	if ( progress != _progress )
	{
		_progress = progress;
		repaint();
	}
}

void ProgressBar::paintBackground()
{
	int wide;
	int tall;
	int x;
	int segmentWide;
	int completeSegments;

	getPaintSize( wide, tall );
	drawSetColor( Scheme::sc_secondary2 );
	drawFilledRect( 0, 0, wide, tall );

	x = 0;
	segmentWide = wide / _segmentCount - 2;
	completeSegments = (int)floor( (double)_progress );

	for ( int i = 0; i < completeSegments; ++i )
	{
		drawSetColor( 0, 0, 100, 0 );
		drawFilledRect( x, 0, x + segmentWide, tall );
		x += segmentWide + 2;
	}

	if ( (double)_segmentCount > _progress )
	{
		drawSetColor( 0, 0, (int)( 255.0 - ( _progress - floor( (double)_progress ) ) * 155.0 ), 0 );
		drawFilledRect( x, 0, x + segmentWide, tall );
	}
}

int ProgressBar::getSegmentCount()
{
	return _segmentCount;
}
