#ifndef VGUI_BORDERPAIR_H
#define VGUI_BORDERPAIR_H

#include "VGUI_Border.h"

namespace vgui
{

class Panel;

class VGUIAPI BorderPair : public Border
{
public:
	BorderPair( Border* border0, Border* border1 );

protected:
	virtual void paint( Panel* panel ) override;

public:
	virtual void doPaint( Panel* panel ) override;

protected:
	Border*	_border[2];

public:
	BorderPair& operator=( const BorderPair& ) = default;
};

}

#endif
