#include "hud.h"
#include "dod_shared.h"
#include "dodclassmenu.h"
#include "dodbutton.h"
#include "dodprogressbar.h"
#include "idodviewport.h"
#include "game_controls/iviewport.h"
#include "game_controls/mouseoverpanelbutton.h"
#include "RichText.h"
#include "Label.h"
#include "controls.h"
#include "../../../engine/controls.h"
#include "../../../public/FileSystem.h"
#include "vgui/IScheme.h"
#include "vgui/KeyValues.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>

using namespace vgui2;

static const char buttons[12][16] =
{
	"cls_button1", "cls_button2", "cls_button3", "cls_button4",
	"cls_button5", "cls_button6", "cls_button7", "cls_button8",
	"cls_button9", "cls_button10", "cls_button11", "cls_button12"
};

MessageMapItem_t CDoDClassMenu::m_MessageMap[2] =
{
	{ "ClassPanelActivate", (MessageFunc_t)(void (Panel::*)(int))&CDoDClassMenu::OnClassPanelActivate, 1,
		DATATYPE_INT, "activePanel", DATATYPE_VOID, NULL, 0, 0, 0 },
	{ "UpdateClassNumLabel", (MessageFunc_t)(void (Panel::*)(int))&CDoDClassMenu::UpdateNumClassLabel, 1,
		DATATYPE_INT, "ignored", DATATYPE_VOID, NULL, 0, 0, 0 }
};

PanelMap_t CDoDClassMenu::m_PanelMap =
{
	m_MessageMap, 2, "CDoDClassMenu", &Panel::m_PanelMap, 0
};

PanelMap_t* CDoDClassMenu::GetPanelMap()
{
	return &m_PanelMap;
}

const char* GetClassResourceFile( int menu )
{
	switch ( menu )
	{
	case 10: return "Resource/UI/ClassMenu_Allies.res";
	case 11: return "Resource/UI/ClassMenu_Allies_Para.res";
	case 12: return "Resource/UI/ClassMenu_Allies_Brit.res";
	case 13: return "Resource/UI/ClassMenu_Axis.res";
	case 14: return "Resource/UI/ClassMenu_Axis_Para.res";
	default: return NULL;
	}
}

CDoDClassMenu::CDoDClassMenu( Panel* parent ) : BaseClass( parent, true )
{
	m_iFirst = 1;
	m_iActivePanel = -1;
	m_iClassMask = 0xFFFF;
	for ( int classNumber = 1; classNumber < 27; ++classNumber )
		pcvar_classlimitcvars[classNumber] = gEngfuncs.pfnGetCvarPointer( classlimitcvarNames[classNumber] );
}

void CDoDClassMenu::Paint()
{
	gDoDViewPortInterface->PaintBackgroundPanel();
	Panel::Paint();
}

void CDoDClassMenu::SetVisibleButton( const char* name, bool visible )
{
	Button* button = dynamic_cast<Button*>( FindChildByName( name, false ) );
	if ( button )
		button->SetVisible( visible );
}

void CDoDClassMenu::SetButtonGreyout( const char* name, bool greyed )
{
	Button* button = dynamic_cast<Button*>( FindChildByName( name, false ) );
	if ( !button )
		return;
	if ( greyed )
	{
		button->SetMouseClickEnabled( MOUSE_LEFT, false );
		button->SetMouseClickEnabled( MOUSE_RIGHT, false );
		button->SetMouseClickEnabled( MOUSE_MIDDLE, false );
		button->SetDefaultColor( Color( 255, 255, 255, 80 ), Color( 55, 43, 29, 80 ) );
		button->SetArmedColor( Color( 255, 0, 0, 200 ), Color( 55, 43, 29, 80 ) );
	}
	else
		button->SetDefaultColor( Color( 255, 255, 255, 255 ), Color( 55, 43, 29, 255 ) );
}

void CDoDClassMenu::Update( int*, int menu )
{
	LoadControlSettings( GetClassResourceFile( menu ), NULL );
	for ( int button = 0; button < 12; ++button )
	{
		const bool enabled = ( ( m_iClassMask >> button ) & 1 ) != 0;
		SetVisibleButton( buttons[button], enabled );
		if ( enabled )
			SetButtonGreyout( buttons[button], ( ( m_iClassMask >> ( button + 16 ) ) & 1 ) != 0 );
	}
	SetVisibleButton( "CancelButton", g_iPlayerClass != 0 );
}

bool CDoDClassMenu::IsClassFull( int classNumber )
{
	int bit = classNumber - 10;
	if ( classNumber < 10 || classNumber > 20 )
	{
		bit = classNumber - 1;
		if ( classNumber >= 21 && classNumber <= 26 )
			bit = classNumber - 21;
	}
	return ( ( m_iClassMask >> ( bit + 16 ) ) & 1 ) != 0;
}

void CDoDClassMenu::OnClassPanelActivate( int activePanel )
{
	m_iActivePanel = activePanel;
	UpdateNumClassLabel( 0 );
}

void CDoDClassMenu::OnTick()
{
	if ( !g_iPlayerClass )
		SetVisibleButton( "CancelButton", false );
	UpdateNumClassLabel( 0 );
	Panel::OnTick();
}

void CDoDClassMenu::UpdateNumClassLabel( int )
{
	if ( m_iActivePanel <= 0 )
		return;
	Label* label = dynamic_cast<Label*>( FindChildByName( "numberOnTeam", false ) );
	Label* full = dynamic_cast<Label*>( FindChildByName( "classFull", false ) );
	if ( !label || !full )
		return;
	if ( m_iActivePanel == 99 )
	{
		full->SetVisible( false );
		label->SetVisible( false );
		return;
	}
	int numPlayers = 0;
	for ( int player = 1; player < 64; ++player )
	{
		gEngfuncs.pfnGetPlayerInfo( player, &g_PlayerInfoList[player] );
		if ( g_PlayerInfoList[player].name && g_PlayerExtraInfo[player].teamnumber == g_iTeamNumber &&
			g_PlayerExtraInfo[player].playerclass == m_iActivePanel )
			++numPlayers;
	}
	scheme()->GetIScheme( gViewPortInterface->GetViewPortScheme() );
	label->GetFgColor();
	label->SetVisible( false );
	label->SetVisible( true );
	char limit[3];
	wchar_t wideLimit[3];
	wchar_t text[256];
	snprintf( limit, sizeof( limit ), "%i", numPlayers );
	localize()->ConvertANSIToUnicode( limit, wideLimit, sizeof( wideLimit ) );
	if ( IsClassFull( m_iActivePanel ) )
	{
		localize()->ConstructString( text, sizeof( text ), localize()->Find( "#Dod_class_full" ), 1, wideLimit );
		full->SetText( text );
		full->SetVisible( true );
		label->SetVisible( false );
		full->SetFgColor( Color( 255, 0, 0, 255 ) );
		return;
	}
	if ( numPlayers == 1 )
		swprintf( text, sizeof( text ), localize()->Find( "#Dod_Currently_on_your_team_one" ) );
	else
	{
		char number[6];
		wchar_t wideNumber[6];
		snprintf( number, sizeof( number ), "%i", numPlayers );
		localize()->ConvertANSIToUnicode( number, wideNumber, sizeof( wideNumber ) );
		localize()->ConstructString( text, sizeof( text ), localize()->Find( "#Dod_Currently_on_your_team" ), 1, wideNumber );
	}
	label->SetText( text );
}

Panel* CDoDClassMenu::CreateControlByName( const char* controlName )
{
	if ( !stricmp( "MouseOverPanelButton", controlName ) )
	{
		ClassHelperPanel* panel = new ClassHelperPanel( this, NULL );
		int x, y, wide, tall;
		m_pPanel->GetBounds( x, y, wide, tall );
		panel->SetBounds( x, y, wide, tall );
		MouseOverPanelButton* button = new MouseOverPanelButton( this, NULL, panel );
		panel->SetAssociatedButton( button );
		panel->SetVisible( false );
		if ( m_iFirst )
		{
			m_iFirst = 0;
			m_pFirstButton = button;
		}
		button->MakeReadyForUse();
		button->SetDefaultColor( Color( 255, 255, 255, 255 ), Color( 55, 43, 29, 255 ) );
		return button;
	}
	if ( !stricmp( "DoDButton", controlName ) )
	{
		DoDButton* button = new DoDButton( this, NULL, "" );
		button->MakeReadyForUse();
		button->SetDefaultColor( Color( 255, 255, 255, 255 ), Color( 55, 43, 29, 255 ) );
		button->SetKeyFocusBorder( NULL );
		return button;
	}
	return BaseClass::CreateControlByName( controlName );
}

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
CDoDClassMenu::ClassHelperPanel::ClassHelperPanel( Panel* parent, const char* panelName ) :
	EditablePanel( parent, panelName )
{
	m_iClassId = -1;
	IScheme* currentScheme = scheme()->GetIScheme( scheme()->GetDefaultScheme() );
	m_pClassInfoText = new RichText( this, "weaponDescripLabel", true );
	m_pClassInfoText->SetFont( currentScheme->GetFont( "ClassMenuBodyFont", IsProportional() ) );
	m_pClassInfoText->SetVerticalScrollbar( true );
}

void CDoDClassMenu::ClassHelperPanel::SetAssociatedButton( Panel* button )
{
	m_pAssociatedButton = button;
}

Panel* CDoDClassMenu::ClassHelperPanel::GetAssociatedButton()
{
	return m_pAssociatedButton;
}

void CDoDClassMenu::ClassHelperPanel::SetVisible( bool visible )
{
	if ( visible )
		PostActionSignal( new KeyValues( "ClassPanelActivate", "activePanel", m_iClassId ) );
	Panel::SetVisible( visible );
}

const char* CDoDClassMenu::ClassHelperPanel::GetClassPage( const char* className )
{
	static char classPanel[4096];
	char classBuffer[156];
	const char* pageName = className;
	const char* paraClass = strstr( className, "_p_" );
	if ( paraClass )
	{
		sprintf( classBuffer, "cls%s", paraClass + 2 );
		pageName = classBuffer;
	}
	snprintf( classPanel, sizeof( classPanel ), "resource/classes/%s.res", pageName );
	if ( !filesystem()->FileExists( classPanel ) )
	{
		if ( !filesystem()->FileExists( "resource/classes/default.res" ) )
			return NULL;
		snprintf( classPanel, sizeof( classPanel ), "resource/classes/default.res" );
	}
	if ( m_iClassId <= 0 )
	{
		if ( !strcmp( "autoselect", pageName ) )
			m_iClassId = 99;
		else
		{
			for ( int classNumber = 0; classNumber < 28; ++classNumber )
			{
				if ( !strcmp( g_ClassInfo[classNumber].selectcmd, pageName ) )
				{
					m_iClassId = classNumber;
					break;
				}
			}
		}
	}
	if ( IsVisible() )
		PostActionSignal( new KeyValues( "ClassPanelActivate", "activePanel", m_iClassId ) );
	return classPanel;
}

Panel* CDoDClassMenu::ClassHelperPanel::CreateControlByName( const char* controlName )
{
	if ( !stricmp( "ProgressBar", controlName ) )
		return new DoDProgressBar( this, "" );
	return EditablePanel::CreateControlByName( controlName );
}

void CDoDClassMenu::ClassHelperPanel::ApplySchemeSettings( IScheme* scheme )
{
	Panel::ApplySchemeSettings( scheme );
	RichText* description = dynamic_cast<RichText*>( FindChildByName( "weaponDescripLabel", false ) );
	if ( description )
	{
		description->SetFgColor( scheme->GetColor( "BrightControlText", Color( 255, 0, 255, 255 ) ) );
		description->SetText( "" );
		description->SetBorder( NULL );
	}
	KeyValues* command = new KeyValues( "GetCommand" );
	GetAssociatedButton()->RequestInfo( command );
	LoadControlSettings( GetClassPage( command->GetString( "command", "" ) ), NULL );
	Color light = scheme->GetColor( "ClassMenuLight", Color( 255, 0, 0, 255 ) );
	Color dark = scheme->GetColor( "ClassMenuDark", Color( 0, 0, 0, 255 ) );
	for ( int index = 1; index <= 5; ++index )
	{
		char name[16];
		sprintf( name, "statBar%i", index );
		DoDProgressBar* bar = dynamic_cast<DoDProgressBar*>( FindChildByName( name, false ) );
		if ( bar )
		{
			bar->SetBorder( NULL );
			bar->SetBgColor( dark );
			bar->SetFgColor( light );
		}
	}
	PostActionSignal( new KeyValues( "UpdateClassNumLabel", "ignored", 0 ) );
}
