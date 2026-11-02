//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Client DLL VGUI2 Msgs for viewport, ENGINE DEPENDANT!!!!
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "vgui_TeamFortressViewport.h"
#include "ClientScoreBoardDialog.h"
#include "SpectatorGUI.h"
#include "classmenu.h"
#include "teammenu.h"
#include "hud.h"
#include "parsemsg.h"
#include "../../../engine/controls.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct viewport_extra_player_info_t
{
	short	frags;
	short	deaths;
	short	team_id;
	int		has_c4;
	int		vip;
	vec3_t	origin;
	float	radarflash;
	int		radarflashon;
	int		radarflashes;
	short	playerclass;
	short	teamnumber;
	char	teamname[16];
	bool	dead;
	float	showhealth;
	int		health;
	char	location[32];
	int		sb_health;
	int		sb_account;
	int		has_defuse_kit;
};

void DuckMessage( const char* text )
{
	static char buffer[128];
	static client_textmessage_t message;

	strncpy( buffer, text, sizeof( buffer ) );
	message.pName = "Spec_Duck";
	message.effect = 2;
	message.r1 = 40;
	message.g1 = 255;
	message.b1 = 40;
	message.a1 = 200;
	message.r2 = 0;
	message.g2 = 255;
	message.b2 = 0;
	message.a2 = 200;
	message.x = -1.0f;
	message.y = 0.7f;
	message.fadein = 0.01f;
	message.fadeout = 0.7f;
	message.fxtime = 0.07f;
	message.holdtime = 6.0f;
	message.pMessage = buffer;
	gViewPortInterface->GetClientDllInterface()->MessageAdd( &message );
}

//================================================================
// Message Handlers
int TeamFortressViewport::MsgFunc_ValClass( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	for ( int i = 0; i < 5; i++ )
		m_iValidClasses[i] = READ_SHORT();

	m_pClassMenu->Update( m_iValidClasses, 5 );
	return 1;
}

int TeamFortressViewport::MsgFunc_TeamNames( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int receivedTeams = READ_BYTE();
	if ( receivedTeams < 0 )
		return 1;
	int maximumTeams = sizeof( m_sTeamNames ) / sizeof( m_sTeamNames[0] ) - 2;
	m_iNumTeams = receivedTeams > maximumTeams ? maximumTeams : receivedTeams;
	for ( int i = 0; i < receivedTeams; i++ )
	{
		const char* name = READ_STRING();
		if ( i >= m_iNumTeams )
			continue;
		if ( m_sTeamNames[i] && strlen( m_sTeamNames[i] ) < strlen( name ) + 1 )
		{
			free( m_sTeamNames[i] );
			m_sTeamNames[i] = NULL;
		}
		if ( !m_sTeamNames[i] )
		{
			m_sTeamNames[i] = (char*)malloc( strlen( name ) + 1 );
			if ( !m_sTeamNames[i] )
				return 1;
			memset( m_sTeamNames[i], 0, strlen( name ) + 1 );
		}
		strcpy( m_sTeamNames[i], name );
		m_pClientScoreBoard->SetTeamName( i + 1, m_sTeamNames[i] );
	}

	if ( m_sTeamNames[m_iNumTeams] && strlen( m_sTeamNames[m_iNumTeams] ) + 1 < sizeof( "#Team_AutoAssign" ) )
	{
		free( m_sTeamNames[m_iNumTeams] );
		m_sTeamNames[m_iNumTeams] = NULL;
	}
	if ( !m_sTeamNames[m_iNumTeams] )
		m_sTeamNames[m_iNumTeams] = (char*)malloc( 17 );
	if ( !m_sTeamNames[m_iNumTeams] )
		return 1;
	strcpy( m_sTeamNames[m_iNumTeams], "#Team_AutoAssign" );

	if ( m_sTeamNames[m_iNumTeams + 1] && strlen( m_sTeamNames[m_iNumTeams + 1] ) + 1 < sizeof( "#Menu_Spectate" ) )
	{
		free( m_sTeamNames[m_iNumTeams + 1] );
		m_sTeamNames[m_iNumTeams + 1] = NULL;
	}
	if ( !m_sTeamNames[m_iNumTeams + 1] )
		m_sTeamNames[m_iNumTeams + 1] = (char*)malloc( 15 );
	if ( !m_sTeamNames[m_iNumTeams + 1] )
		return 1;
	strcpy( m_sTeamNames[m_iNumTeams + 1], "#Menu_Spectate" );

	if ( m_pTeamMenu )
		m_pTeamMenu->Update( m_sMapName, m_iAllowSpectators != 0,
			(const char**)m_sTeamNames, m_iNumTeams );
	return 1;
}

int TeamFortressViewport::MsgFunc_Feign( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	m_bIsFeigning = READ_BYTE() != 0;
	return 1;
}

int TeamFortressViewport::MsgFunc_Detpack( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	m_iIsSettingDetpack = READ_BYTE();
	return 1;
}

int TeamFortressViewport::MsgFunc_VGUIMenu( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int menu = READ_BYTE();
	if ( menu == MENU_MAPBRIEFING )
	{
		strncpy( m_sMapName, READ_STRING(), sizeof( m_sMapName ) );
		m_sMapName[sizeof( m_sMapName ) - 1] = 0;
	}
	ShowVGUIMenu( menu );
	return 1;
}

int TeamFortressViewport::MsgFunc_TutorText( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	const char* token = READ_STRING();
	const wchar_t* localized = vgui2::localize()->Find( token );
	wchar_t text[1024];
	wchar_t expanded[1024];
	wchar_t placeholder[39];
	if ( localized )
		wcsncpy( text, localized, 1023 );
	else
		swprintf( text, 1024, L"%hs", token );
	text[1023] = 0;

	int arguments = READ_BYTE();
	for ( int i = 1; i <= arguments; i++ )
	{
		swprintf( placeholder, 32, L"%%s%d", i );
		wchar_t* match = wcsstr( text, placeholder );
		if ( !match )
			break;

		size_t prefix = match - text;
		wcsncpy( expanded, text, prefix );
		wchar_t* dest = expanded + prefix;
		int value = READ_BYTE();
		while ( value > 0 )
		{
			if ( dest < expanded + 1023 )
				*dest++ = (wchar_t)value;
			value = READ_BYTE();
		}
		*dest = 0;
		wcsncpy( dest, match + wcslen( placeholder ), expanded + 1023 - dest );
		expanded[1023] = 0;
		wcscpy( text, expanded );
	}

	int id = READ_SHORT();
	int spectator = READ_SHORT();
	int messageClass = READ_SHORT();
	ShowTutorTextWindow( text, id, messageClass, spectator );
	return 1;
}

int TeamFortressViewport::MsgFunc_TutorLine( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int entity = READ_SHORT();
	int id = READ_SHORT();
	ShowTutorLine( entity, id );
	return 1;
}

int TeamFortressViewport::MsgFunc_TutorState( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	const char* token = READ_STRING();
	const wchar_t* localized = vgui2::localize()->Find( token );
	if ( localized )
		ShowTutorState( localized );

	wchar_t text[1024];
	swprintf( text, 1024, L"%hs", token );
	ShowTutorState( text );
	return 1;
}

int TeamFortressViewport::MsgFunc_TutorClose( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	CloseTutorTextWindow();
	return 1;
}

int TeamFortressViewport::MsgFunc_MOTD( const char*, int size, void* buffer )
{
	if ( m_iGotAllMOTD )
		m_szMOTD[0] = 0;

	BEGIN_READ( buffer, size );
	m_iGotAllMOTD = READ_BYTE();
	int remaining = sizeof( m_szMOTD ) - strlen( m_szMOTD ) - 1;
	if ( remaining < 0 )
		remaining = 0;
	strncat( m_szMOTD, READ_STRING(), remaining );
	m_szMOTD[sizeof( m_szMOTD ) - 1] = 0;
	if ( m_iGotAllMOTD && !gEngfuncs.IsSpectateOnly() )
		ShowVGUIMenu( MENU_INTRO );
	return 1;
}

int TeamFortressViewport::MsgFunc_BuildSt( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	m_iBuildState = READ_SHORT();
	return 1;
}

int TeamFortressViewport::MsgFunc_RandomPC( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	m_iRandomPC = READ_BYTE();
	return 1;
}

int TeamFortressViewport::MsgFunc_ServerName( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	strncpy( m_szServerName, READ_STRING(), sizeof( m_szServerName ) );
	m_szServerName[sizeof( m_szServerName ) - 1] = 0;
	return 1;
}

int TeamFortressViewport::MsgFunc_ScoreInfo( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int player = (short)READ_BYTE();
	short frags = (short)READ_SHORT();
	short deaths = (short)READ_SHORT();
	short playerclass = (short)READ_SHORT();
	short team = (short)READ_SHORT();
	if ( (unsigned short)( player - 1 ) > 63 )
		return 1;

	viewport_extra_player_info_t* players = (viewport_extra_player_info_t*)g_PlayerExtraInfo;
	players[player].frags = frags;
	players[player].deaths = deaths;
	players[player].playerclass = playerclass;
	players[player].teamnumber = team;
	if ( team < 0 )
		players[player].teamnumber = 0;

	if ( m_pTeamMenu )
		m_pTeamMenu->Update( m_sMapName, m_iAllowSpectators != 0,
			(const char**)m_sTeamNames, m_iNumTeams );
	GetAllPlayersInfo();
	bool spectator = m_pSpectatorGUI->IsVisible();
	bool teamplay = m_pClientDllInterface->TeamPlay() != 0;
	m_pClientScoreBoard->Update( m_szServerName, teamplay, spectator );
	m_pSpectatorGUI->UpdateSpectatorPlayerList();
	return 1;
}

int TeamFortressViewport::MsgFunc_TeamScore( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	const char* name = READ_STRING();
	int team;
	for ( team = 1; team <= m_iNumTeams; team++ )
	{
		size_t length = strlen( name );
		const char* teamName = m_pClientScoreBoard->GetTeamName( team );
		if ( !_strnicmp( name, teamName, length ) )
			break;
	}
	if ( team > m_iNumTeams && team < (int)( sizeof( m_sTeamNames ) / sizeof( m_sTeamNames[0] ) ) - 1 )
	{
		m_iNumTeams++;
		if ( m_sTeamNames[team] && strlen( m_sTeamNames[team] ) < strlen( name ) + 1 )
		{
			free( m_sTeamNames[team] );
			m_sTeamNames[team] = NULL;
		}
		if ( !m_sTeamNames[team] )
			m_sTeamNames[team] = (char*)malloc( strlen( name ) + 1 );
		if ( !m_sTeamNames[team] )
			return 1;
		strcpy( m_sTeamNames[team], name );
		m_pClientScoreBoard->SetTeamName( team, name );
	}

	int firstScore = READ_SHORT();
	int secondScore = READ_SHORT();
	m_pClientScoreBoard->SetTeamDetails( team, secondScore, firstScore );
	return 1;
}

int TeamFortressViewport::MsgFunc_TeamInfo( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	short player = (short)READ_BYTE();
	if ( (unsigned short)( player - 1 ) <= 63 )
	{
		viewport_extra_player_info_t* players = (viewport_extra_player_info_t*)g_PlayerExtraInfo;
		strncpy( players[player].teamname, READ_STRING(), sizeof( players[player].teamname ) );
	}
	GetAllPlayersInfo();
	bool spectator = m_pSpectatorGUI->IsVisible();
	bool teamplay = m_pClientDllInterface->TeamPlay() != 0;
	m_pClientScoreBoard->RebuildTeams( m_szServerName, teamplay, spectator );
	return 1;
}

int TeamFortressViewport::MsgFunc_Spectator( const char*, int, void* )
{
/*	BEGIN_READ( pbuf, iSize );

	short cl = (short)READ_BYTE();
	if ( cl > 0 && cl <= MAX_PLAYERS )
	{
		g_IsSpectator[cl] = READ_BYTE();
	}
*/
	return 1;
}

int TeamFortressViewport::MsgFunc_AllowSpec( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	m_iAllowSpectators = READ_BYTE();
	if ( m_pTeamMenu )
		m_pTeamMenu->Update( m_sMapName, m_iAllowSpectators != 0,
			(const char**)m_sTeamNames, m_iNumTeams );
	return 1;
}

int TeamFortressViewport::MsgFunc_ResetFade( const char*, int, void* )
{
	return 1;
}

// used to fade a player's screen out/in when they're spectating someone who is teleported
int TeamFortressViewport::MsgFunc_SpecFade( const char*, int, void* )
{
	return 1;
}
