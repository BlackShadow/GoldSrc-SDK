#ifndef WEAPONSETLABEL_H
#define WEAPONSETLABEL_H

#include "Panel.h"
#include "EditablePanel.h"

class BuyPresetWeapon;
class WeaponSet;

namespace vgui2
{
class IImage;
class IScheme;
class Label;
class TextImage;
}

struct ImageInfo
{
	vgui2::IImage*	image;
	int				w;
	int				h;
	int				x;
	int				y;
	int				fullW;
	int				fullH;

	void FitInBounds( int left, int top, int wide, int tall, bool centered, int scaleAt1024, bool halfHeight );
	void Paint();
};

class WeaponImageInfo
{
public:
	WeaponImageInfo();
	~WeaponImageInfo();
	void SetBounds( int left, int top, int wide, int tall );
	void SetCentered( bool centered );
	void SetScaleAt1024( int weaponScale, int ammoScale );
	void SetWeapon( const BuyPresetWeapon* weapon, bool isPrimary, bool useCurrentAmmoType );
	void ApplyTextSettings( vgui2::IScheme* scheme, bool proportional );
	void Paint();
	void PaintText();
	void PerformLayout();

private:
	int					m_left;
	int					m_top;
	int					m_wide;
	int					m_tall;
	bool				m_isPrimary;
	int					m_weaponScale;
	int					m_ammoScale;
	bool				m_needLayout;
	bool				m_isCentered;
	ImageInfo			m_weapon;
	ImageInfo			m_ammo;
	vgui2::TextImage*	m_pAmmoText;
};

class ItemImageInfo
{
public:
	ItemImageInfo();
	~ItemImageInfo();
	void SetBounds( int left, int top, int wide, int tall );
	void SetItem( const char* name, int count );
	void ApplyTextSettings( vgui2::IScheme* scheme, bool proportional );
	void Paint();
	void PaintText();
	void PerformLayout();

private:
	int					m_left;
	int					m_top;
	int					m_wide;
	int					m_tall;
	int					m_count;
	bool				m_needLayout;
	ImageInfo			m_image;
	vgui2::TextImage*	m_pText;
};

class WeaponLabel : public vgui2::Panel
{
public:
	WeaponLabel( vgui2::Panel* parent, const char* name );
	~WeaponLabel() override;
	void SetWeapon( const BuyPresetWeapon* weapon, bool isPrimary, bool showAmmo );
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void PerformLayout() override;
	void Paint() override;

protected:
	WeaponImageInfo m_weapon;
};

class EquipmentLabel : public vgui2::Panel
{
public:
	EquipmentLabel( vgui2::Panel* parent, const char* name, const char* item );
	~EquipmentLabel() override;
	void SetItem( const char* item, int count );
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void PerformLayout() override;
	void Paint() override;

protected:
	ItemImageInfo m_item;
};

class BuyPresetEditPanel : public vgui2::EditablePanel
{
public:
	BuyPresetEditPanel( vgui2::Panel* parent, const char* name, const char* title, int fallbackIndex );
	~BuyPresetEditPanel() override;
	void SetWeaponSet( const WeaponSet* set, bool isCurrent );
	virtual void SetText( const wchar_t* text );
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void OnCommand( const char* command ) override;
	void OnSizeChanged( int wide, int tall ) override;
	void SetPanelBgColor( Color color );
	void Reset();

protected:
	vgui2::Panel*	m_pBgPanel;
	vgui2::Label*	m_pTitleLabel;
	vgui2::Label*	m_pCostLabel;
	WeaponLabel*	m_pPrimaryWeapon;
	WeaponLabel*	m_pSecondaryWeapon;
	EquipmentLabel*	m_pHEGrenade;
	EquipmentLabel*	m_pSmokeGrenade;
	EquipmentLabel*	m_pFlashbangs;
	EquipmentLabel*	m_pDefuser;
	EquipmentLabel*	m_pNightvision;
	EquipmentLabel*	m_pArmor;
	int				m_baseWide;
	int				m_baseTall;
	int				m_fallbackIndex;
};

#endif
