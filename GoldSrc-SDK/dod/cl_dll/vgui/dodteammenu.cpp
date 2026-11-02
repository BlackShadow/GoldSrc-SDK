#include "hud.h"
#include "dodteammenu.h"
#include "dodbutton.h"
#include "idodviewport.h"
#include "Label.h"
#include "RichText.h"
#include "controls.h"
#include "../../../engine/controls.h"
#include "vgui/IScheme.h"
#include "game_controls/iviewport.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>

using namespace vgui2;

MessageMapItem_t CDoDTeamMenu::m_MessageMap[1] =
{
	{ "MouseOver", (MessageFunc_t)(void (Panel::*)(int))&CDoDTeamMenu::OnMouseOver, 1, DATATYPE_INT, "code", DATATYPE_VOID, NULL, 0, 0, 0 }
};

PanelMap_t CDoDTeamMenu::m_PanelMap =
{
	m_MessageMap, 1, "CDoDTeamMenu", &Panel::m_PanelMap, 0
};

PanelMap_t* CDoDTeamMenu::GetPanelMap()
{
	return &m_PanelMap;
}

CDoDTeamMenu::CDoDTeamMenu( Panel* parent ) : BaseClass( parent )
{
	LoadControlSettings( "Resource/UI/TeamMenu.res", NULL );
	InvalidateLayout( false, false );
	m_iActiveTeam = 0;
	m_pNumOnTeamLabel = dynamic_cast<Label*>( FindChildByName( "numberOnTeam", false ) );
}

void CDoDTeamMenu::ApplySchemeSettings( IScheme* scheme )
{
	DoDButton* button = dynamic_cast<DoDButton*>( FindChildByName( "britbutton", false ) );
	if ( button )
		button->SetMouseOverCode( 1 );
	button = dynamic_cast<DoDButton*>( FindChildByName( "amerbutton", false ) );
	if ( button )
		button->SetMouseOverCode( 1 );
	button = dynamic_cast<DoDButton*>( FindChildByName( "axisbutton", false ) );
	if ( button )
		button->SetMouseOverCode( 2 );
	button = dynamic_cast<DoDButton*>( FindChildByName( "specbutton", false ) );
	if ( button )
		button->SetMouseOverCode( 3 );
	BaseClass::ApplySchemeSettings( scheme );
}

void CDoDTeamMenu::Paint()
{
	gDoDViewPortInterface->PaintBackgroundPanel();
	Panel::Paint();
}

void CDoDTeamMenu::OnCommand( const char* command )
{
	if ( stricmp( command, "vguicancel" ) )
		gEngfuncs.pfnClientCmd( (char*)command );
	if ( !stricmp( command, "jointeam 3" ) )
		gViewPortInterface->ShowSpectatorGUI();
	gViewPortInterface->HideBackGround();
	Close();
	Frame::OnCommand( command );
}

Panel* CDoDTeamMenu::CreateControlByName( const char* controlName )
{
	if ( stricmp( "DoDButton", controlName ) )
		return EditablePanel::CreateControlByName( controlName );
	DoDButton* button = new DoDButton( this, NULL, "" );
	button->MakeReadyForUse();
	button->SetDefaultColor( Color( 255, 255, 255, 255 ), Color( 55, 43, 29, 255 ) );
	return button;
}

void CDoDTeamMenu::SetVisibleButton( const char* name, bool visible )
{
	Button* button = dynamic_cast<Button*>( FindChildByName( name, false ) );
	if ( button )
		button->SetVisible( visible );
}

void CDoDTeamMenu::Update()
{
	SetVisibleButton( "amerbutton", !gHUD.m_bBritish );
	SetVisibleButton( "britbutton", gHUD.m_bBritish );
}

void CDoDTeamMenu::Update( const char* mapName, bool allowSpectators, const char**, int )
{
	RichText* mapInfo = dynamic_cast<RichText*>( FindChildByName( "MapInfo", false ) );
	if ( mapInfo )
	{
		IScheme* currentScheme = scheme()->GetIScheme( gViewPortInterface->GetViewPortScheme() );
		mapInfo->SetFgColor( currentScheme->GetColor( "BrightControlText", Color( 255, 255, 255, 255 ) ) );
		mapInfo->SetText( "" );
	}
	BaseClass::Update( mapName, allowSpectators, NULL, 0 );
	Update();
	SetVisibleButton( "specbutton", true );
	SetVisibleButton( "CancelButton", g_iTeamNumber != 0 );
}

void CDoDTeamMenu::OnMouseOver( int button )
{
	m_iActiveTeam = button;
	UpdateNumOnTeamLabel();
}

void CDoDTeamMenu::OnTick()
{
	UpdateNumOnTeamLabel();
}

void CDoDTeamMenu::UpdateNumOnTeamLabel()
{
	if ( !m_pNumOnTeamLabel )
		return;
	if ( m_iActiveTeam < 1 || m_iActiveTeam > 3 )
	{
		m_pNumOnTeamLabel->SetVisible( false );
		return;
	}
	if ( !m_pNumOnTeamLabel->IsVisible() )
		m_pNumOnTeamLabel->SetVisible( true );
	char playerList[512];
	playerList[0] = 0;
	int numPlayers = 0;
	bool first = true;
	for ( int player = 1; player < 64; ++player )
	{
		gEngfuncs.pfnGetPlayerInfo( player, &g_PlayerInfoList[player] );
		if ( !g_PlayerInfoList[player].name || g_PlayerExtraInfo[player].teamnumber != m_iActiveTeam )
			continue;
		++numPlayers;
		if ( first )
		{
			snprintf( playerList, sizeof( playerList ), "%s", g_PlayerInfoList[player].name );
			first = false;
		}
		else
		{
			strncat( playerList, ", ", sizeof( playerList ) - strlen( playerList ) );
			strncat( playerList, g_PlayerInfoList[player].name, sizeof( playerList ) - strlen( playerList ) );
		}
	}
	char number[6];
	wchar_t wideNumber[6];
	wchar_t widePlayers[512];
	wchar_t label[256];
	wchar_t playerFormat[64];
	wchar_t teamName[64];
	snprintf( number, sizeof( number ), "%i", numPlayers );
	localize()->ConvertANSIToUnicode( number, wideNumber, sizeof( wideNumber ) );
	localize()->ConvertANSIToUnicode( playerList, widePlayers, sizeof( widePlayers ) );
	IScheme* currentScheme = scheme()->GetIScheme( gViewPortInterface->GetViewPortScheme() );
	if ( numPlayers == 0 )
	{
		swprintf( label, sizeof( label ), localize()->Find( "Dod_no_players_on_this_team" ) );
		m_pNumOnTeamLabel->SetText( label );
		m_pNumOnTeamLabel->SetFgColor( Color( 255, 255, 255, 255 ) );
		m_pNumOnTeamLabel->SetVisible( true );
		return;
	}
	if ( numPlayers == 1 )
		swprintf( playerFormat, sizeof( playerFormat ), localize()->Find( "Dod_Player" ) );
	else
		localize()->ConstructString( playerFormat, sizeof( playerFormat ), localize()->Find( "Dod_Players" ), 1, wideNumber );
	Color teamColor;
	if ( m_iActiveTeam == 1 )
	{
		swprintf( teamName, sizeof( teamName ), localize()->Find( "Teamname_Allies" ) );
		teamColor = currentScheme->GetColor( "team1", Color( 255, 255, 255, 255 ) );
	}
	else if ( m_iActiveTeam == 2 )
	{
		swprintf( teamName, sizeof( teamName ), localize()->Find( "Teamname_Axis" ) );
		teamColor = currentScheme->GetColor( "team2", Color( 255, 255, 255, 255 ) );
	}
	else
	{
		swprintf( teamName, sizeof( teamName ), localize()->Find( "Teamname_Spectators" ) );
		teamColor = currentScheme->GetColor( "team3", Color( 255, 255, 255, 255 ) );
	}
	if ( m_iActiveTeam == 3 )
		localize()->ConstructString( label, sizeof( label ), localize()->Find( "Dod_players_on_team_spec" ),
			3, teamName, playerFormat, widePlayers );
	else
	{
		char score[6];
		wchar_t wideScore[6];
		snprintf( score, sizeof( score ), "%i", gDoDViewPortInterface->GetTeamScore( m_iActiveTeam ) );
		localize()->ConvertANSIToUnicode( score, wideScore, sizeof( wideScore ) );
		localize()->ConstructString( label, sizeof( label ), localize()->Find( "Dod_players_on_team" ),
			4, teamName, playerFormat, wideScore, widePlayers );
	}
	m_pNumOnTeamLabel->SetVisible( true );
	m_pNumOnTeamLabel->SetText( label );
	m_pNumOnTeamLabel->SetFgColor( teamColor );
}
