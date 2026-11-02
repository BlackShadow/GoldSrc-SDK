#pragma once

#include "basecareermenu.h"

class BuyPresetEditPanel;
namespace vgui2 { class Label; class Button; }

class CBuyPresetEditMainMenu : public CCSCareerBaseMenu
{
public:
	CBuyPresetEditMainMenu( vgui2::Panel* parent );
	~CBuyPresetEditMainMenu() override;
	void Update();
	void EditPreset( int index );

private:
	void UpdateButtons();
	void OnCommand( const char* command ) override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void SetDirty( bool dirty );

	vgui2::Label* m_pTitleLabel;
	vgui2::Button* m_pSaveButton;
	BuyPresetEditPanel* m_pWeaponSetLabel[4];
	bool m_dirty;
};

void MaintainBuyPresetInputFocus();
void ShowBuyPresetMainMenu( bool runUpdate, int reopenBuyMenu );
