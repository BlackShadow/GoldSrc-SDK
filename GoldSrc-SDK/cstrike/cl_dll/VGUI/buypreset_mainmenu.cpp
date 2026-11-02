#include "buypreset_mainmenu.h"
#include "buypreset_editmenu.h"
#include "WeaponSetLabel.h"
#include "CareerBox.h"
#include "buy_presets.h"
#include "Button.h"
#include "Label.h"
#include "game_controls/iviewport.h"
#include "vgui/ILocalize.h"
#include "vgui/IVGui.h"
#include "vgui_controls/Controls.h"
#include "shared_util.h"
#include "cl_util.h"
#include "net_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

using namespace vgui2;

static CBuyPresetEditMainMenu* pMainMenu;
static bool runUpdate = true;
static bool reopenBuyMenu;

class MouseFocusPanel : public Frame
{
	DECLARE_CLASS_SIMPLE_NAMED( MouseFocusPanel, Frame, "vgui::Frame" );

public:
	MouseFocusPanel();
	void OnTick() override;
	void OnClose() override;
	void Paint() override {}
	void PaintBackground() override {}

private:
	int m_framesLeft;
};

MouseFocusPanel::MouseFocusPanel() : Frame( NULL, "MouseFocusPanel", true, false )
{
	SetMoveable( false );
	SetSizeable( false );
	SetTitleBarVisible( false );
	m_framesLeft = 2;
	ivgui()->AddTickSignal( GetVPanel(), 0 );
}

void MouseFocusPanel::OnTick()
{
	if ( --m_framesLeft <= 0 )
		OnClose();
}

void MouseFocusPanel::OnClose()
{
	Frame::OnClose();
	MarkForDeletion();
}

void MaintainBuyPresetInputFocus()
{
	MouseFocusPanel* panel = new MouseFocusPanel;
	panel->SetParent( gViewPortInterface->GetViewPortPanel() );
	panel->Activate();
}

void ShowBuyPresetMainMenu( bool update, int reopen )
{
	net_status_t status;
	gEngfuncs.pNetAPI->Status( &status );
	if ( !status.connected || !gViewPortInterface )
		return;
	runUpdate = update;
	if ( reopen == REOPEN_YES )
		reopenBuyMenu = true;
	else if ( reopen == REOPEN_NO )
		reopenBuyMenu = false;
	MaintainBuyPresetInputFocus();
	gViewPortInterface->HideVGUIMenu( 39 );
	gViewPortInterface->ShowVGUIMenu( 38 );
}

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
CBuyPresetEditMainMenu::CBuyPresetEditMainMenu( Panel* parent )
	: CCSCareerBaseMenu( parent, "BuyPresetEditMainMenu", false )
{
	SetTitle( "#Cstrike_BuyPresetEditPresets", true );
	m_pSaveButton = new Button( this, "save", "" );
	m_pTitleLabel = new Label( this, "Title", "" );
	for ( int i = 0; i < 4; ++i )
		m_pWeaponSetLabel[i] = new BuyPresetEditPanel( this, SharedVarArgs( "LabelPreset%dChoice1", i + 1 ),
			"Resource/UI/BuyPresetViewPanel.res", i );
	LoadControlSettings( "Resource/UI/BuyPresetEditMainMenu.res", NULL );
}

CBuyPresetEditMainMenu::~CBuyPresetEditMainMenu()
{
	if ( this == pMainMenu )
		pMainMenu = NULL;
}

void CBuyPresetEditMainMenu::UpdateButtons()
{
	m_dirty = false;
	if ( TheBuyPresets->GetNumEditPresets() != TheBuyPresets->GetNumPresets() )
		m_dirty = true;
	else
	{
		for ( int i = 0; i < TheBuyPresets->GetNumEditPresets(); ++i )
		{
			const BuyPreset* original = TheBuyPresets->GetPreset( i );
			BuyPreset* edited = TheBuyPresets->GetEditPreset( i );
			if ( wcscmp( original->GetName(), edited->GetName() ) || original->GetNumSets() != edited->GetNumSets() )
			{
				m_dirty = true;
				break;
			}
			for ( int j = 0; j < original->GetNumSets(); ++j )
			{
				const WeaponSet* first = original->GetSet( j );
				const WeaponSet* second = edited->GetSet( j );
				if ( first->m_armor != second->m_armor || first->m_helmet != second->m_helmet
					|| first->m_smokeGrenade != second->m_smokeGrenade || first->m_HEGrenade != second->m_HEGrenade
					|| first->m_flashbangs != second->m_flashbangs || first->m_defuser != second->m_defuser
					|| first->m_nightvision != second->m_nightvision || first->m_extraCash != second->m_extraCash
					|| first->m_primaryWeapon.GetWeaponID() != second->m_primaryWeapon.GetWeaponID()
					|| first->m_primaryWeapon.GetAmmoAmount() != second->m_primaryWeapon.GetAmmoAmount()
					|| first->m_primaryWeapon.GetAmmoType() != second->m_primaryWeapon.GetAmmoType()
					|| first->m_primaryWeapon.GetFillAmmo() != second->m_primaryWeapon.GetFillAmmo()
					|| first->m_secondaryWeapon.GetWeaponID() != second->m_secondaryWeapon.GetWeaponID()
					|| first->m_secondaryWeapon.GetAmmoAmount() != second->m_secondaryWeapon.GetAmmoAmount()
					|| first->m_secondaryWeapon.GetAmmoType() != second->m_secondaryWeapon.GetAmmoType()
					|| first->m_secondaryWeapon.GetFillAmmo() != second->m_secondaryWeapon.GetFillAmmo() )
				{
					m_dirty = true;
					break;
				}
			}
			if ( m_dirty )
				break;
		}
	}
	SetDirty( m_dirty );
	for ( int i = 0; i < 4; ++i )
	{
		BuyPreset* preset = TheBuyPresets->GetEditPreset( i );
		if ( preset )
		{
			wchar_t buffer[128];
			localize()->ConstructString( buffer, sizeof( buffer ), localize()->Find( "#Cstrike_BuyPresetPresetButtonLabel" ),
				2, preset->GetName(), NumAsWString( preset->FullCost() ) );
			const WeaponSet* set = preset->GetSet( 0 );
			m_pWeaponSetLabel[i]->SetVisible( set != NULL );
			m_pWeaponSetLabel[i]->SetWeaponSet( set, false );
			m_pWeaponSetLabel[i]->SetText( buffer );
		}
		else
		{
			m_pWeaponSetLabel[i]->SetWeaponSet( NULL, false );
			m_pWeaponSetLabel[i]->SetText( localize()->Find( "#Cstrike_BuyPresetBlank" ) );
		}
	}
}

void CBuyPresetEditMainMenu::ApplySchemeSettings( IScheme* scheme )
{
	Frame::ApplySchemeSettings( scheme );
	UpdateButtons();
}

void CBuyPresetEditMainMenu::SetDirty( bool dirty )
{
	m_dirty = dirty;
	m_pTitleLabel->SetText( dirty ? "#Cstrike_BuyPresetEditPresetsDirty" : "#Cstrike_BuyPresetEditPresets" );
	m_pSaveButton->SetEnabled( dirty );
}

void CBuyPresetEditMainMenu::EditPreset( int index )
{
	ShowBuyPresetEditMenu( index );
}

void CBuyPresetEditMainMenu::Update()
{
	m_dirty = false;
	gEngfuncs.pfnClientCmd( "setpause\n" );
	gViewPortInterface->CloseTutorTextWindow();
	UpdateScheme();
	pMainMenu = this;
	if ( runUpdate )
	{
		TheBuyPresets->ResetEditPresets();
		SetDirty( false );
	}
	UpdateButtons();
}

void CBuyPresetEditMainMenu::OnCommand( const char* command )
{
	char commandBuf[32] = {};
	char argBuf[32] = {};
	const char* next = SharedParse( command );
	if ( next )
	{
		_snprintf( commandBuf, sizeof( commandBuf ), "%s", SharedGetToken() );
		commandBuf[sizeof( commandBuf ) - 1] = 0;
		if ( SharedParse( next ) )
		{
			_snprintf( argBuf, sizeof( argBuf ), "%s", SharedGetToken() );
			argBuf[sizeof( argBuf ) - 1] = 0;
		}
	}
	if ( !stricmp( command, "Cancel" ) )
	{
		Frame::OnCommand( command );
		return;
	}
	if ( !stricmp( command, "save_presets" ) )
	{
		TheBuyPresets->SetPresets( TheBuyPresets->GetEditPresets() );
		TheBuyPresets->Save();
		SetDirty( false );
		command = "vguicancel";
	}
	else if ( !stricmp( command, "close_with_save_check" ) )
	{
		if ( m_dirty )
		{
			CCareerQueryBox* box = new CCareerQueryBox( this, "BuyBoxQuitNoSave" );
			box->AddActionSignalTarget( this );
			box->ShowWindow();
			Frame::OnCommand( command );
			return;
		}
		command = "vguicancel";
	}
	else if ( !stricmp( command, "use_defaults" ) )
	{
		CCareerQueryBox* box = new CCareerQueryBox( this, "BuyBoxUseDefaults" );
		box->AddActionSignalTarget( this );
		box->ShowWindow();
		Frame::OnCommand( command );
		return;
	}
	else if ( !stricmp( command, "use_defaults_ok" ) )
	{
		TheBuyPresets->ResetEditToDefaults();
		SetDirty( true );
		UpdateButtons();
		Frame::OnCommand( command );
		return;
	}
	else if ( !stricmp( commandBuf, "edit_preset" ) && argBuf[0] )
	{
		ShowBuyPresetEditMenu( atoi( argBuf ) );
		Frame::OnCommand( command );
		return;
	}
	if ( !stricmp( command, "vguicancel" ) )
	{
		if ( reopenBuyMenu )
		{
			MaintainBuyPresetInputFocus();
			gViewPortInterface->HideBackGround();
			OnClose();
			gViewPortInterface->HideVGUIMenu( 38 );
			gViewPortInterface->HideVGUIMenu( 39 );
			gViewPortInterface->ShowVGUIMenu( 28 );
		}
		else
			OnClose();
	}
	Frame::OnCommand( command );
}
