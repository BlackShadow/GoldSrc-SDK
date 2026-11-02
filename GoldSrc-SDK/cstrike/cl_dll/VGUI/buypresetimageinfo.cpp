#include "WeaponSetLabel.h"
#include "buy_presets.h"
#include "TextImage.h"
#include "vgui/ILocalize.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"
#include "shared_util.h"
#include <math.h>
#include <string.h>

void ImageInfo::Paint()
{
	if ( image )
	{
		image->SetSize( w, h );
		image->SetPos( x, y );
		image->Paint();
		image->SetSize( 0, 0 );
	}
}

void ImageInfo::FitInBounds( int left, int top, int wide, int tall, bool centered, int scaleAt1024, bool )
{
	if ( !image )
		return;
	image->GetContentSize( fullW, fullH );
	bool fitToBounds = true;
	if ( scaleAt1024 )
	{
		int screenWide, screenTall;
		vgui2::surface()->GetScreenSize( screenWide, screenTall );
		w = scaleAt1024 * ( screenWide * fullW / 1024 ) / 100;
		h = scaleAt1024 * ( screenWide * fullH / 1024 ) / 100;
		if ( scaleAt1024 == 100 && fullH > 64 )
		{
			w = ( w * 64 ) / fullH;
			h = ( h * 64 ) / fullH;
		}
		fitToBounds = h > tall * 1.2;
	}

	if ( fitToBounds )
	{
		w = fullW;
		h = fullH;
		if ( tall != fullH )
		{
			h = tall;
			float fittedWide = (float)( (double)fullW * tall / fullH );
			w = (int)fittedWide;
		}
		if ( wide < w )
		{
			float fittedTall = (float)( (double)tall * wide / w );
			w = wide;
			h = (int)fittedTall;
		}
	}

	x = centered ? left + ( wide - w ) / 2 : left;
	y = top + ( tall - h ) / 2;
}

WeaponImageInfo::WeaponImageInfo()
{
	m_left = 0;
	m_top = 0;
	m_wide = 0;
	m_tall = 0;
	m_isCentered = false;
	m_needLayout = false;
	m_isPrimary = false;
	memset( &m_weapon, 0, sizeof( m_weapon ) );
	memset( &m_ammo, 0, sizeof( m_ammo ) );
	m_ammoScale = 0;
	m_weaponScale = 0;
	m_pAmmoText = new vgui2::TextImage( "" );
}

WeaponImageInfo::~WeaponImageInfo()
{
	delete m_pAmmoText;
}

void WeaponImageInfo::SetBounds( int left, int top, int wide, int tall )
{
	m_needLayout = true;
	m_left = left;
	m_top = top;
	m_wide = wide;
	m_tall = tall;
}

void WeaponImageInfo::SetCentered( bool centered )
{
	m_needLayout = true;
	m_isCentered = centered;
}

void WeaponImageInfo::SetScaleAt1024( int weaponScale, int ammoScale )
{
	m_needLayout = true;
	m_weaponScale = weaponScale;
	m_ammoScale = ammoScale;
}

void WeaponImageInfo::ApplyTextSettings( vgui2::IScheme* scheme, bool proportional )
{
	m_pAmmoText->SetColor( scheme->GetColor( "BaseText", Color() ) );
	m_pAmmoText->SetFont( scheme->GetFont( "Default", proportional ) );
	m_pAmmoText->SetWrap( false );
}

void WeaponImageInfo::SetWeapon( const BuyPresetWeapon* weapon, bool isPrimary, bool useCurrentAmmoType )
{
	m_pAmmoText->SetText( L"" );
	m_weapon.image = NULL;
	m_ammo.image = NULL;
	m_isPrimary = isPrimary;
	if ( !weapon )
		return;

	const wchar_t* multiplier = vgui2::localize()->Find( "#Cstrike_BuyMenuPresetMultiplier" );
	if ( !multiplier )
		multiplier = L"";
	if ( weapon->GetAmmoType() == AMMO_CLIPS )
	{
		WeaponIdType weaponID = weapon->GetWeaponID();
		const WeaponInfoStruct* info = GetWeaponInfo( weaponID );
		int clips = weapon->GetAmmoAmount();
		if ( info )
		{
			int maxClips = info->buyClipSize > 0 ? (int)ceilf( (float)info->maxRounds / info->buyClipSize ) : 0;
			if ( clips > maxClips )
				clips = maxClips;
			m_weapon.image = vgui2::scheme()->GetImage( ImageFnameFromWeaponID( weaponID, m_isPrimary ), true );
		}
		else if ( clips > 0 && useCurrentAmmoType )
		{
			WeaponIdType currentID = ClientWeaponIDToGameWeaponID( GetClientWeaponID( isPrimary ) );
			m_weapon.image = vgui2::scheme()->GetImage( ImageFnameFromWeaponID( currentID, m_isPrimary ), true );
			info = GetWeaponInfo( currentID );
			if ( !info )
			{
				m_weapon.image = NULL;
				m_pAmmoText->SetText( L"" );
				m_needLayout = true;
				return;
			}
		}
		else if ( !useCurrentAmmoType )
		{
			m_weapon.image = vgui2::scheme()->GetImage( ImageFnameFromWeaponID( weaponID, m_isPrimary ), true );
		}
		else
		{
			m_needLayout = true;
			return;
		}

		if ( clips )
		{
			if ( info && info->ammoType == AMMO_BUCKSHOT )
				m_ammo.image = vgui2::scheme()->GetImage( "gfx/vgui/shell", true );
			else
				m_ammo.image = vgui2::scheme()->GetImage( isPrimary ? "gfx/vgui/bullet" : "gfx/vgui/cartridge", true );
			if ( clips > 1 )
			{
				wchar_t text[32];
				vgui2::localize()->ConstructString( text, sizeof( text ), (wchar_t*)multiplier, 1, NumAsWString( clips ) );
				m_pAmmoText->SetText( text );
			}
			else
			{
				m_pAmmoText->SetText( L"" );
			}
		}
		else
		{
			m_ammo.image = NULL;
			m_pAmmoText->SetText( L"" );
		}
	}

	m_needLayout = true;
}

void WeaponImageInfo::PerformLayout()
{
	m_needLayout = false;
	int weaponWide = (int)( m_wide * 0.8 );
	m_weapon.FitInBounds( m_left, m_top, weaponWide, m_tall, m_isCentered, m_weaponScale, false );
	weaponWide = m_weapon.w;
	int maxWide = 5 * m_wide / 6;
	int ammoWide = 9 * m_tall / 16;
	if ( !m_isPrimary )
	{
		ammoWide = 25 * ammoWide / 40;
		weaponWide = 3 * weaponWide / 4;
	}
	if ( weaponWide > maxWide )
		weaponWide = maxWide;
	if ( weaponWide + ammoWide > m_wide )
		weaponWide = m_wide - ammoWide;

	m_ammo.FitInBounds( m_left + weaponWide, m_top + m_tall - ammoWide,
		ammoWide, ammoWide, false, m_ammoScale, false );
	m_pAmmoText->ResizeImageToContent();
	int textWide, textTall;
	m_pAmmoText->GetSize( textWide, textTall );
	if ( m_isPrimary )
	{
		int textX;
		if ( m_ammoScale > 74 )
			textX = m_left + weaponWide + ammoWide - textWide;
		else
		{
			float fittedX = (float)( m_left + weaponWide + ammoWide * 1.25 - textWide );
			textX = (int)fittedX;
		}
		m_pAmmoText->SetPos( textX, m_top + m_tall - textTall );
	}
	else
	{
		m_pAmmoText->SetPos( m_left + weaponWide + ammoWide, m_top + m_tall - textTall );
	}
}

void WeaponImageInfo::Paint()
{
	if ( m_needLayout )
		PerformLayout();
	m_weapon.Paint();
	m_ammo.Paint();
}

void WeaponImageInfo::PaintText()
{
	if ( m_needLayout )
		PerformLayout();
	m_pAmmoText->Paint();
}

ItemImageInfo::ItemImageInfo()
{
	m_needLayout = false;
	m_tall = 0;
	m_wide = 0;
	m_top = 0;
	m_left = 0;
	m_count = 0;
	memset( &m_image, 0, sizeof( m_image ) );
	m_pText = new vgui2::TextImage( "" );
	SetBounds( 0, 0, 100, 100 );
}

ItemImageInfo::~ItemImageInfo()
{
	delete m_pText;
}

void ItemImageInfo::SetBounds( int left, int top, int wide, int tall )
{
	m_needLayout = true;
	m_left = left;
	m_top = top;
	m_wide = wide;
	m_tall = tall;
}

void ItemImageInfo::SetItem( const char* name, int count )
{
	m_pText->SetText( L"" );
	m_count = count;
	if ( name )
		m_image.image = vgui2::scheme()->GetImage( name, true );
	if ( count > 1 )
	{
		const wchar_t* multiplier = vgui2::localize()->Find( "#Cstrike_BuyMenuPresetMultiplier" );
		if ( !multiplier )
			multiplier = L"";
		wchar_t text[32];
		vgui2::localize()->ConstructString( text, sizeof( text ), (wchar_t*)multiplier, 1, NumAsWString( count ) );
		m_pText->SetText( text );
	}
	m_needLayout = true;
}

void ItemImageInfo::ApplyTextSettings( vgui2::IScheme* scheme, bool proportional )
{
	m_pText->SetColor( scheme->GetColor( "BaseText", Color() ) );
	m_pText->SetFont( scheme->GetFont( "Default", proportional ) );
	m_pText->SetWrap( false );
}

void ItemImageInfo::PerformLayout()
{
	m_needLayout = false;
	m_image.FitInBounds( m_left, m_top, m_wide, m_tall, false, 0, false );
	m_pText->ResizeImageToContent();
	int textWide, textTall;
	m_pText->GetSize( textWide, textTall );
	m_pText->SetPos( m_left + m_image.w - textWide, m_top + m_tall - textTall );
}

void ItemImageInfo::Paint()
{
	if ( m_needLayout )
		PerformLayout();
	if ( m_count )
		m_image.Paint();
}

void ItemImageInfo::PaintText()
{
	if ( m_needLayout )
		PerformLayout();
	m_pText->Paint();
}

WeaponLabel::WeaponLabel( vgui2::Panel* parent, const char* name ) : vgui2::Panel( parent, name )
{
	SetSize( 10, 10 );
	SetMouseInputEnabled( false );
}

WeaponLabel::~WeaponLabel()
{
}

void WeaponLabel::SetWeapon( const BuyPresetWeapon* presetWeapon, bool isPrimary, bool showAmmo )
{
	BuyPresetWeapon weapon( WEAPON_NONE );
	if ( presetWeapon )
		weapon = *presetWeapon;
	if ( !showAmmo )
		weapon.SetAmmoAmount( 0 );
	m_weapon.SetWeapon( &weapon, isPrimary, false );
}

void WeaponLabel::ApplySchemeSettings( vgui2::IScheme* scheme )
{
	vgui2::Panel::ApplySchemeSettings( scheme );
	m_weapon.ApplyTextSettings( scheme, IsProportional() );
}

void WeaponLabel::PerformLayout()
{
	vgui2::Panel::PerformLayout();
	int wide, tall;
	GetSize( wide, tall );
	m_weapon.SetBounds( 0, 0, wide, tall );
}

void WeaponLabel::Paint()
{
	vgui2::Panel::Paint();
	m_weapon.Paint();
	m_weapon.PaintText();
}

EquipmentLabel::EquipmentLabel( vgui2::Panel* parent, const char* name, const char* item )
	: vgui2::Panel( parent, name )
{
	SetSize( 10, 10 );
	m_item.SetItem( item, 0 );
	SetMouseInputEnabled( false );
}

EquipmentLabel::~EquipmentLabel()
{
}

void EquipmentLabel::SetItem( const char* item, int count )
{
	m_item.SetItem( item, count );
}

void EquipmentLabel::ApplySchemeSettings( vgui2::IScheme* scheme )
{
	vgui2::Panel::ApplySchemeSettings( scheme );
	m_item.ApplyTextSettings( scheme, IsProportional() );
}

void EquipmentLabel::PerformLayout()
{
	vgui2::Panel::PerformLayout();
	int wide, tall;
	GetSize( wide, tall );
	m_item.SetBounds( 0, 0, wide, tall );
}

void EquipmentLabel::Paint()
{
	vgui2::Panel::Paint();
	m_item.Paint();
	m_item.PaintText();
}

void DrawDashedLine( int x0, int y0, int x1, int y1, int dashLength, int gapLength )
{
	if ( x1 - x0 > y1 - y0 )
	{
		int x = x0;
		do
		{
			int end = x + dashLength;
			if ( x1 < end )
				end = x1;
			vgui2::surface()->DrawFilledRect( x, y0, end, y1 + 1 );
			x += dashLength + gapLength;
		}
		while ( x <= x1 );
	}
	else
	{
		int y = y0;
		do
		{
			int end = y + dashLength;
			if ( y1 < end )
				end = y1;
			vgui2::surface()->DrawFilledRect( x0, y, x1 + 1, end );
			y += dashLength + gapLength;
		}
		while ( y <= y1 );
	}
}
