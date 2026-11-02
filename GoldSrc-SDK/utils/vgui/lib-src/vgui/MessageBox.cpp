#include "VGUI_MessageBox.h"
#include "VGUI_Dar.h"
#include "VGUI_ActionSignal.h"
#include "VGUI_Frame.h"
#include "VGUI.h"
#include "VGUI_Label.h"
#include "VGUI_Button.h"
#include "VGUI_Panel.h"

using namespace vgui;

namespace
{

class FooButtonHandler : public ActionSignal
{
private:
	MessageBox*	_messageBox;

public:
	FooButtonHandler( MessageBox* messageBox )
	{
		_messageBox = messageBox;
	}

	virtual void actionPerformed( Panel* panel ) override;
};

}

void FooButtonHandler::actionPerformed( Panel* )
{
	_messageBox->fireActionSignal();
}

void MessageBox::performLayout()
{
	int wide;
	int tall;

	getClient()->getSize( wide, tall );

	_messageLabel->setPos( wide / 2 - _messageLabel->getWide() / 2, tall / 2 - _messageLabel->getTall() / 2 - 20 );
	_okButton->setPos( wide / 2 - _okButton->getWide() / 2, tall - _okButton->getTall() - 10 );
}

void MessageBox::addActionSignal( ActionSignal* s )
{
	_actionSignalDar.putElement( s );
}

void MessageBox::fireActionSignal()
{
	for ( int i = 0; i < _actionSignalDar.getCount(); i++ )
	{
		_actionSignalDar[i]->actionPerformed( this );
	}
}

MessageBox::MessageBox( const char* title, const char* text, int x, int y ) : Frame( x, y, 64, 64 )
{
	int wide;
	int tall;

	setTitle( title );
	setMenuButtonVisible( false );
	setTrayButtonVisible( false );
	setMinimizeButtonVisible( false );
	setMaximizeButtonVisible( false );
	setCloseButtonVisible( false );
	setSizeable( false );

	_messageLabel = new Label( text );
	_messageLabel->setParent( getClient() );

	_okButton = new Button( "Ok", 10, 10 );
	_okButton->setParent( getClient() );
	_okButton->addActionSignal( new FooButtonHandler( this ) );

	_messageLabel->getSize( wide, tall );
	setSize( wide + 100, tall + 100 );
}
