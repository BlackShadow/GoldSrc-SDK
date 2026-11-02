#ifndef VGUI_ETCHEDBORDER_H
#define VGUI_ETCHEDBORDER_H

#include "VGUI_Border.h"

namespace vgui
{

class Panel;

class VGUIAPI EtchedBorder : public Border
{
public:
	EtchedBorder();

protected:
	virtual void paint( Panel* panel ) override;

public:
	EtchedBorder& operator=( const EtchedBorder& ) = default;
};

}

#endif
