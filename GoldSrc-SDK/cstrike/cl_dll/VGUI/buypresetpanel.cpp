#include "WeaponSetLabel.h"
#include "buy_presets.h"
#include "Button.h"
#include "Label.h"
#include "vgui/ILocalize.h"
#include "vgui/IScheme.h"
#include "vgui/KeyValues.h"
#include "vgui_controls/Controls.h"
#include "shared_util.h"
#include <string.h>

#ifdef GetClassName
#undef GetClassName
#endif

using namespace vgui2;

BuyPresetEditPanel::BuyPresetEditPanel( Panel* parent, const char* name, const char* resourceFilename,
	int fallbackIndex ) : EditablePanel( parent, name )
{
	SetProportional( parent->IsProportional() );
	m_baseWide = m_baseTall = IsProportional() ? scheme()->GetProportionalScaledValue( 100 ) : 100;
	SetSize( m_baseWide, m_baseTall );
	m_fallbackIndex = fallbackIndex;
	m_pTitleLabel = new Label( this, "title", "" );
	m_pCostLabel = new Label( this, "cost", "" );
	m_pPrimaryWeapon = new WeaponLabel( this, "primary" );
	m_pSecondaryWeapon = new WeaponLabel( this, "secondary" );
	m_pHEGrenade = new EquipmentLabel( this, "hegrenade", NULL );
	m_pSmokeGrenade = new EquipmentLabel( this, "smokegrenade", NULL );
	m_pFlashbangs = new EquipmentLabel( this, "flashbang", NULL );
	m_pDefuser = new EquipmentLabel( this, "defuser", NULL );
	m_pNightvision = new EquipmentLabel( this, "nightvision", NULL );
	m_pArmor = new EquipmentLabel( this, "armor", NULL );
	if ( resourceFilename )
		LoadControlSettings( resourceFilename, NULL );
	m_pBgPanel = FindChildByName( "mainBackground", false );
	if ( m_pBgPanel )
	{
		int x, y, wide, tall;
		m_pBgPanel->GetBounds( x, y, wide, tall );
		m_baseWide = x + wide;
		m_baseTall = y + tall;
	}
	SetSize( m_baseWide, m_baseTall );
	Reset();
}

BuyPresetEditPanel::~BuyPresetEditPanel()
{
}

void BuyPresetEditPanel::SetText( const wchar_t* text )
{
	m_pTitleLabel->SetText( text ? text : L"" );
	InvalidateLayout( false, false );
}

void BuyPresetEditPanel::Reset()
{
	m_pTitleLabel->SetText( "#Cstrike_BuyPresetNewChoice" );
	m_pCostLabel->SetText( "" );
	BuyPresetWeapon weapon;
	m_pPrimaryWeapon->SetWeapon( &weapon, true, false );
	m_pSecondaryWeapon->SetWeapon( &weapon, false, false );
	m_pHEGrenade->SetItem( "gfx/vgui/buy_presets/hegrenade_none", 1 );
	m_pSmokeGrenade->SetItem( "gfx/vgui/buy_presets/smokegrenade_none", 1 );
	m_pFlashbangs->SetItem( "gfx/vgui/buy_presets/flashbang_none", 1 );
	m_pDefuser->SetItem( "gfx/vgui/buy_presets/defuser_none", 1 );
	m_pNightvision->SetItem( "gfx/vgui/buy_presets/nightvision_none", 1 );
	m_pArmor->SetItem( "gfx/vgui/buy_presets/kevlar_none", 1 );
}

void BuyPresetEditPanel::SetWeaponSet( const WeaponSet* set, bool )
{
	Reset();
	if ( !set )
		return;
	m_pTitleLabel->SetText( SharedVarArgs( "#Cstrike_BuyPresetChoice%d", m_fallbackIndex ) );
	wchar_t buffer[256];
	localize()->ConstructString( buffer, sizeof( buffer ), SharedFindString( "#Cstrike_BuyPresetPlainCost" ),
		1, NumAsWString( set->FullCost() ) );
	m_pCostLabel->SetText( buffer );
	m_pPrimaryWeapon->SetWeapon( &set->m_primaryWeapon, true, true );
	m_pSecondaryWeapon->SetWeapon( &set->m_secondaryWeapon, false, true );
	if ( set->m_HEGrenade )
		m_pHEGrenade->SetItem( "gfx/vgui/buy_presets/hegrenade", 1 );
	if ( set->m_smokeGrenade )
		m_pSmokeGrenade->SetItem( "gfx/vgui/buy_presets/smokegrenade", 1 );
	if ( set->m_flashbangs )
		m_pFlashbangs->SetItem( "gfx/vgui/buy_presets/flashbang", set->m_flashbangs );
	if ( set->m_defuser )
		m_pDefuser->SetItem( "gfx/vgui/buy_presets/defuser", 1 );
	if ( set->m_nightvision )
		m_pNightvision->SetItem( "gfx/vgui/buy_presets/nightvision", 1 );
	if ( set->m_armor )
		m_pArmor->SetItem( set->m_helmet ? "gfx/vgui/buy_presets/kevlar_helmet" : "gfx/vgui/buy_presets/kevlar", 1 );
}

void BuyPresetEditPanel::SetPanelBgColor( Color color )
{
	if ( m_pBgPanel )
		m_pBgPanel->SetBgColor( color );
}

void BuyPresetEditPanel::OnCommand( const char* command )
{
	if ( stricmp( command, "close" ) )
		PostActionSignal( new KeyValues( "Command", "command", SharedVarArgs( "%s %d", command, m_fallbackIndex ) ) );
	Panel::OnCommand( command );
}

void BuyPresetEditPanel::ApplySchemeSettings( IScheme* scheme )
{
	Panel::ApplySchemeSettings( scheme );
	for ( int i = 0; i < GetChildCount(); ++i )
	{
		Panel* child = GetChild( i );
		if ( child && !stricmp( "button", child->GetClassName() ) )
		{
			Button* button = dynamic_cast<Button*>( child );
			if ( button )
			{
				button->SetDefaultBorder( NULL );
				button->SetDepressedBorder( NULL );
				button->SetKeyFocusBorder( NULL );
			}
		}
	}
	IBorder* border = scheme->GetBorder( "BuyPresetButtonBorder" );
	const char* buttonNames[] = { "editPrimary", "editSecondary", "editGrenades", "editEquipment" };
	for ( int i = 0; i < 4; ++i )
	{
		Panel* child = FindChildByName( buttonNames[i], false );
		if ( !child )
			continue;
		child->SetBorder( border );
		if ( !stricmp( "button", child->GetClassName() ) )
		{
			Button* button = dynamic_cast<Button*>( child );
			if ( button )
			{
				button->SetDefaultBorder( border );
				button->SetDepressedBorder( border );
				button->SetKeyFocusBorder( border );
				Color foreground = GetSchemeColor( "LabelDimText", GetFgColor(), scheme );
				Color background = GetSchemeColor( "ButtonBgColor", GetFgColor(), scheme );
				button->SetDefaultColor( foreground, background );
			}
		}
	}
}

void BuyPresetEditPanel::OnSizeChanged( int wide, int tall )
{
	if ( !m_baseWide )
		m_baseWide = 1;
	if ( !m_baseTall )
		m_baseTall = 1;
	Panel::OnSizeChanged( wide, tall );
	InvalidateLayout( false, false );
	if ( wide == m_baseWide && tall == m_baseTall )
	{
		Repaint();
		return;
	}
	float xScale = (float)wide / m_baseWide;
	float yScale = (float)tall / m_baseTall;
	for ( int i = 0; i < GetChildCount(); ++i )
	{
		Panel* child = GetChild( i );
		if ( !child )
			continue;
		int x, y, width, height;
		child->GetBounds( x, y, width, height );
		float scaledX = x * xScale;
		int left = (int)scaledX;
		float scaledWidth = xScale * ( width + x ) - left;
		int childWide = (int)scaledWidth;
		if ( wide < left + childWide )
			continue;
		float scaledY = y * yScale;
		int top = (int)scaledY;
		float scaledHeight = yScale * height;
		if ( tall >= (int)scaledHeight + top )
		{
			child->SetBounds( left, top, childWide, (int)scaledHeight );
			child->InvalidateLayout( false, false );
		}
	}
	Repaint();
	m_baseWide = wide;
	m_baseTall = tall;
}
