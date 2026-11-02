#ifndef VGUI_MESSAGEBOX_H
#define VGUI_MESSAGEBOX_H

#include "VGUI.h"
#include "VGUI_Dar.h"
#include "VGUI_Frame.h"

namespace vgui
{

class ActionSignal;
class Button;
class Label;

class VGUIAPI MessageBox : public Frame
{
public:
	MessageBox( const char* title, const char* text, int x, int y );

	virtual void addActionSignal( ActionSignal* s );
	virtual void fireActionSignal();

protected:
	virtual void performLayout() override;

private:
	Label*				_messageLabel;
	Button*				_okButton;

	Dar<ActionSignal*>	_actionSignalDar;

public:
	MessageBox& operator=( const MessageBox& ) = default;
};

}

#endif
