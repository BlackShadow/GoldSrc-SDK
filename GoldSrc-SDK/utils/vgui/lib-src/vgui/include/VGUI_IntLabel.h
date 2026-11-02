#ifndef VGUI_INTLABEL_H
#define VGUI_INTLABEL_H

#include "VGUI_Label.h"
#include "VGUI_IntChangeSignal.h"

namespace vgui
{

class VGUIAPI IntLabel : public Label, public IntChangeSignal
{
public:
	IntLabel( int value, int x, int y, int wide, int tall );

	virtual void setValue( int value );
	virtual void intChanged( int value, Panel* panel ) override;

protected:
	virtual void paintBackground() override;

private:
	int	_value;

public:
	IntLabel& operator=( const IntLabel& ) = default;
};

}

#endif
