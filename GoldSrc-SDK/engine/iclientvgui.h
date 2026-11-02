#pragma once

#include "interface.h"

namespace vgui2
{
typedef unsigned int VPANEL;
}

class IClientVGUI : public IBaseInterface
{
public:
	virtual void Initialize( CreateInterfaceFn* factories, int count );
	virtual void Start();
	virtual void SetParent( vgui2::VPANEL parent );
	virtual bool UseVGUI1();

	virtual void HideScoreBoard();
	virtual void HideAllVGUIMenu();
	virtual void ActivateClientUI();
	virtual void HideClientUI();

	virtual void Shutdown();
};
