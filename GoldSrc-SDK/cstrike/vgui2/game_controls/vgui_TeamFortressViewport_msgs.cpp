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
#include "vgui_controls/Controls.h"
#include "vgui/ILocalize.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "buildslave_assert.h"
#define ASSERT_FILE BUILDSLAVE_PATH( L"vgui2\\game_controls\\vgui_TeamFortressViewport_msgs.cpp" )

void DuckMessage( const char* text )
{
	static char tempString[128];
	static client_textmessage_t tst;

	strncpy( tempString, text, sizeof( tempString ) );
	tempString[sizeof( tempString ) - 1] = 0;
	tst.pName = "Spec_Duck";
	tst.effect = 2;
	tst.r1 = 40;
	tst.g1 = 255;
	tst.b1 = 40;
	tst.a1 = 200;
	tst.r2 = 0;
	tst.g2 = 255;
	tst.b2 = 0;
	tst.a2 = 200;
	tst.x = -1.0f;
	tst.y = 0.7f;
	tst.fadein = 0.01f;
	tst.fadeout = 0.7f;
	tst.fxtime = 0.07f;
	tst.holdtime = 6.0f;
	tst.pMessage = tempString;
	gViewPortInterface->GetClientDllInterface()->MessageAdd( &tst );
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
	int teams = READ_BYTE();
	if ( teams > (int)ARRAYSIZE( m_sTeamNames ) - 2 )
		return 1;
	m_iNumTeams = teams;

	for ( int teamNum = 0; teamNum < m_iNumTeams; teamNum++ )
	{
		const char* name = READ_STRING();
		if ( m_sTeamNames[teamNum] && strlen( m_sTeamNames[teamNum] ) < strlen( name ) + 1 )
		{
			free( m_sTeamNames[teamNum] );
			m_sTeamNames[teamNum] = NULL;
		}
		if ( !m_sTeamNames[teamNum] )
		{
			m_sTeamNames[teamNum] = (char*)malloc( strlen( name ) + 1 );
			ASSERT_AT( m_sTeamNames[teamNum], 132 );
			memset( m_sTeamNames[teamNum], 0, strlen( name ) + 1 );
		}
		strcpy( m_sTeamNames[teamNum], name );
		m_pClientScoreBoard->SetTeamName( teamNum + 1, m_sTeamNames[teamNum] );
	}

	if ( m_sTeamNames[m_iNumTeams] && strlen( m_sTeamNames[m_iNumTeams] ) < strlen( "#Team_AutoAssign" ) )
	{
		free( m_sTeamNames[m_iNumTeams] );
		m_sTeamNames[m_iNumTeams] = NULL;
	}

	// now add on the two special teams
	if ( !m_sTeamNames[m_iNumTeams] )
	{
		m_sTeamNames[m_iNumTeams] = (char*)malloc( strlen( "#Team_AutoAssign" ) + 1 );
		ASSERT_AT( m_sTeamNames[m_iNumTeams], 145 );
	}
	strcpy( m_sTeamNames[m_iNumTeams], "#Team_AutoAssign" );

	if ( m_sTeamNames[m_iNumTeams + 1] && strlen( m_sTeamNames[m_iNumTeams + 1] ) < strlen( "#Menu_Spectate" ) )
	{
		free( m_sTeamNames[m_iNumTeams + 1] );
		m_sTeamNames[m_iNumTeams + 1] = NULL;
	}
	if ( !m_sTeamNames[m_iNumTeams + 1] )
	{
		m_sTeamNames[m_iNumTeams + 1] = (char*)malloc( strlen( "#Menu_Spectate" ) + 1 );
		ASSERT_AT( m_sTeamNames[m_iNumTeams + 1], 151 );
	}
	strcpy( m_sTeamNames[m_iNumTeams + 1], "#Menu_Spectate" );

	if ( m_pTeamMenu )
		m_pTeamMenu->Update( m_sMapName, m_iAllowSpectators != 0, (const char**)m_sTeamNames, m_iNumTeams );
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
	wchar_t placeholder[32];
	if ( localized )
		wcsncpy( text, localized, ARRAYSIZE( text ) - 1 );
	else
		_snwprintf( text, ARRAYSIZE( text ), L"%hs", token );
	text[ARRAYSIZE( text ) - 1] = 0;

	int arguments = READ_BYTE();
	for ( int i = 1; i <= arguments; i++ )
	{
		_snwprintf( placeholder, ARRAYSIZE( placeholder ), L"%%s%d", i );
		wchar_t* match = wcsstr( text, placeholder );
		if ( !match )
			break;

		size_t prefix = match - text;
		wcsncpy( expanded, text, prefix );
		size_t written = prefix;
		int character = READ_BYTE();
		while ( character > 0 )
		{
			if ( written < ARRAYSIZE( expanded ) - 1 )
				expanded[written++] = (wchar_t)character;
			character = READ_BYTE();
		}
		if ( character < 0 )
			return 1;
		expanded[written] = 0;
		wcsncat( expanded, match + wcslen( placeholder ), ARRAYSIZE( expanded ) - written - 1 );
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
	_snwprintf( text, ARRAYSIZE( text ), L"%hs", token );
	text[ARRAYSIZE( text ) - 1] = 0;
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

	g_PlayerExtraInfo[player].frags = frags;
	g_PlayerExtraInfo[player].deaths = deaths;
	g_PlayerExtraInfo[player].playerclass = playerclass;
	g_PlayerExtraInfo[player].teamnumber = team;
	if ( team < 0 )
		g_PlayerExtraInfo[player].teamnumber = 0;

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
		if ( teamName && !_strnicmp( name, teamName, length ) )
			break;
	}
	if ( team > m_iNumTeams && team <= (int)ARRAYSIZE( m_sTeamNames ) - 2 )
	{
		if ( m_sTeamNames[team] && strlen( m_sTeamNames[team] ) < strlen( name ) + 1 )
		{
			char* replacement = (char*)malloc( strlen( name ) + 1 );
			if ( !replacement )
				return 1;
			free( m_sTeamNames[team] );
			m_sTeamNames[team] = replacement;
		}
		if ( !m_sTeamNames[team] )
			m_sTeamNames[team] = (char*)malloc( strlen( name ) + 1 );
		if ( !m_sTeamNames[team] )
			return 1;
		strcpy( m_sTeamNames[team], name );
		m_iNumTeams++;
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
		strncpy( g_PlayerExtraInfo[player].teamname, READ_STRING(), sizeof( g_PlayerExtraInfo[player].teamname ) );
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
