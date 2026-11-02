#ifndef VGUI_PROGRESSBAR_H
#define VGUI_PROGRESSBAR_H

#include "VGUI.h"
#include "VGUI_Panel.h"

namespace vgui
{

class VGUIAPI ProgressBar : public Panel
{
public:
	ProgressBar( int segmentCount );

	virtual void setProgress( float progress );
	virtual int getSegmentCount();

protected:
	virtual void paintBackground() override;

private:
	int		_segmentCount;
	float	_progress;

public:
	ProgressBar& operator=( const ProgressBar& ) = default;
};

}

#endif
