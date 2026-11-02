#pragma once

#include "basecareermenu.h"
#include "buy_presets.h"

class BuyPresetEditPanel;
namespace vgui2 { class Button; class Label; class TextEntry; }

class CBuyPresetEditMenu : public CCSCareerBaseMenu
{
public:
	CBuyPresetEditMenu( vgui2::Panel* parent );
	~CBuyPresetEditMenu() override;
	void Update();
	void SetDirty();

private:
	void FinishEditingWeaponSet();
	void UpdateButtons();
	void OnCommand( const char* command ) override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void EditFallback( int index );
	void DeleteWeaponSet();

	vgui2::TextEntry* m_pPresetName;
	BuyPresetEditPanel* m_pChoicePanel[5];
	int m_numValidChoices;
	int m_selectedIndex;
	Color m_selectedFgColor;
	Color m_selectedBgColor;
	Color m_normalFgColor;
	Color m_normalBgColor;
	Color m_alternateFgColor;
	Color m_alternateBgColor;
	WeaponSet m_weaponSet;
	BuyPreset m_originalBuyPreset;
	vgui2::Button* m_pAcceptButton;
	vgui2::Label* m_pTitleLabel;
	bool m_isDirty;
};

void ShowBuyPresetEditMenu( int presetIndex );
