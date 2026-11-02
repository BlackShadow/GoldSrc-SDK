#pragma once

#include "../../vgui2/controls/EditablePanel.h"

class CEnginePanel : public vgui2::EditablePanel
{
public:
	CEnginePanel( vgui2::Panel* parent, const char* name );
	~CEnginePanel() override;

	void EnableMouseFocus( bool state );

	vgui2::VPANEL IsWithinTraverse( int x, int y, bool traversePopups ) override;

protected:
	bool m_bCanFocus;
};

class CStaticPanel : public vgui2::Panel
{
public:
	CStaticPanel( vgui2::Panel* parent, const char* name );
	~CStaticPanel() override;

	vgui2::VPANEL GetCurrentKeyFocus() override;
	vgui2::VPANEL IsWithinTraverse( int x, int y, bool traversePopups ) override;
};
