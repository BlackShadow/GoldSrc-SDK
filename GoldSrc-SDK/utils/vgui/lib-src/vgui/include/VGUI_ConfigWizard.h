#ifndef VGUI_CONFIGWIZARD_H
#define VGUI_CONFIGWIZARD_H

#include "VGUI.h"
#include "VGUI_Panel.h"

namespace vgui
{

class Button;
class TreeFolder;

class VGUIAPI ConfigWizard : public Panel
{
public:
	ConfigWizard( int x, int y, int wide, int tall );

	virtual void setSize( int wide, int tall ) override;
	virtual Panel* getClient();
	virtual TreeFolder* getFolder();

private:
	TreeFolder*	_treeFolder;
	Panel*		_client;

	Button*		_okButton;
	Button*		_cancelButton;
	Button*		_applyButton;
	Button*		_helpButton;

public:
	ConfigWizard& operator=( const ConfigWizard& ) = default;
};

}

#endif
