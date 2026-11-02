#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "tf_defs.h"

float g_fNextPrematchAlert = 0;

class CGhost : public CBaseEntity
{
};

LINK_ENTITY_TO_CLASS( ghost, CGhost );

void CBasePlayer::CreateGhost()
{
	CBaseEntity* pGhost = CBaseEntity::Create( "ghost", g_vecZero, g_vecZero, edict() );
	pGhost->tf_id = tf_id;
	pGhost->team_no = team_no;
	pGhost->pev->frags = pev->frags;
	pGhost->real_frags = real_frags;
	pGhost->pev->netname = pev->netname;
	pGhost->pev->playerclass = pev->playerclass;
	if ( tfstate & TFSTATE_RANDOMPC )
		pGhost->tfstate = TFSTATE_RANDOMPC;
}

CGhost* FindGhost( int iGhostID )
{
	CBaseEntity* pGhost = NULL;
	while ( ( pGhost = UTIL_FindEntityByClassname( pGhost, "ghost" ) ) != NULL )
	{
		if ( pGhost->tf_id == iGhostID )
			return (CGhost*)pGhost;
	}
	return NULL;
}

int GetBattleID( edict_t* pEntity )
{
	int id = 0;
	char* buffer = g_engfuncs.pfnGetInfoKeyBuffer( pEntity );
	const char* value = g_engfuncs.pfnInfoKeyValue( buffer, "tf_id" );
	if ( value && *value )
	{
		if ( sscanf( value, "%d", &id ) != 1 )
			id = 0;
	}
	return id;
}

void CBasePlayer::CreateBattleID()
{
	if ( tfc_clanbattle.value == 0 )
		return;
	last_id += RANDOM_FLOAT( 20, 40 );
	tf_id = (int)last_id;
	char* buffer = g_engfuncs.pfnGetInfoKeyBuffer( edict() );
	char* value = UTIL_VarArgs( "%d", tf_id );
	g_engfuncs.pfnSetClientKeyValue( entindex(), buffer, "tf_id", value );
	ClientPrint( pev, HUD_PRINTNOTIFY, "#Game_battleid", UTIL_dtos1( tf_id ) );
}

void CBasePlayer::ConnectDuringBattle()
{
	int id = GetBattleID( edict() );
	CGhost* pGhost = id ? FindGhost( id ) : NULL;
	if ( !pGhost )
	{
		if ( tfc_clanbattle.value != 0 )
			CreateBattleID();
		return;
	}
	ClientPrint( pev, HUD_PRINTCENTER, "#Game_welcomeback" );
	tf_id = id;
	TeamFortress_TeamSet( pGhost->team_no );
	pev->frags = pGhost->pev->frags;
	real_frags = pGhost->real_frags;
	pev->frags = (float)real_frags;
	pev->playerclass = pGhost->pev->playerclass;
	tfstate = pGhost->tfstate;
	ResetMenu();
	UTIL_Remove( pGhost );
}

void Display_Prematch()
{
	if ( g_fNextPrematchAlert > gpGlobals->time )
		return;
	int seconds = (int)ceilf( cb_prematch_time - gpGlobals->time );
	if ( seconds % 60 == 0 && seconds > 119 )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_minsleft", UTIL_dtos1( seconds / 60 ) );
		g_fNextPrematchAlert = gpGlobals->time + (float)( seconds - 60 );
	}
	else if ( seconds == 60 )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_oneminleft" );
		g_fNextPrematchAlert = gpGlobals->time + 30;
	}
	else if ( seconds == 30 )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_thirtysecleft" );
		g_fNextPrematchAlert = gpGlobals->time + 20;
	}
	else if ( seconds >= 2 && seconds <= 10 )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_secsleft", UTIL_dtos1( seconds ) );
		g_fNextPrematchAlert = gpGlobals->time + 1;
	}
	else if ( seconds == 1 )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_onesec" );
		g_fNextPrematchAlert = gpGlobals->time + 1;
	}
	else if ( seconds <= 0 )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_matchbegin" );
		UTIL_LogPrintf( "World triggered \"Match_Begins_Now\"\n" );
		if ( tfc_clanbattle_locked.value != 0 )
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_matchlocked" );
		for ( int team = 1; team <= 4; team++ )
		{
			teamscores[team] = 0;
			teamfrags[team] = 0;
		}
		cb_prematch_time = 0;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;
			if ( pPlayer->tf_id == 0 && tfc_clanbattle.value != 0 )
				pPlayer->CreateBattleID();
			pPlayer->TeamFortress_RemoveTimers();
			pPlayer->TakeDamage( pPlayer->pev, pPlayer->pev, 10000, DMG_NEVERGIB | DMG_IGNOREARMOR );
			pPlayer->pev->frags = 0;
			pPlayer->real_frags = 0;
			pPlayer->m_iDeaths = 0;
		}
		CBaseEntity* pGrenade = NULL;
		while ( ( pGrenade = UTIL_FindEntityByClassname( pGrenade, "grenade" ) ) != NULL )
		{
			pGrenade->SetThink( &CBaseEntity::SUB_Remove );
			pGrenade->pev->nextthink = gpGlobals->time;
		}
	}
}

void Check_Ceasefire()
{
	if ( gpGlobals->time > cb_ceasefire_time && initial_cease_fire )
	{
		if ( cease_fire )
		{
			initial_cease_fire = FALSE;
			cease_fire = FALSE;
		}
	}
	else if ( cease_fire )
	{
		last_cease_fire = cease_fire;
		return;
	}
	if ( !no_cease_fire_text )
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_resumefire" );
	UTIL_LogPrintf( "World triggered \"Resume_Fire\"\n" );
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( !pPlayer )
			continue;
		pPlayer->pev->iuser4 = 0;
		pPlayer->tfstate &= ~TFSTATE_CANT_MOVE;
		pPlayer->immune_to_check = gpGlobals->time + 10;
		pPlayer->TeamFortress_SetSpeed();
	}
	last_cease_fire = cease_fire;
}

void LogMatchResults( int*, int* iUnaccountedFrags, int )
{
	for ( int team = 1; team <= number_of_teams; team++ )
	{
		char teamName[32];
		char allies[128];
		char results[500];
		allies[0] = 0;
		const char* unaccounted = UTIL_dtos4( iUnaccountedFrags[team] );
		const char* frags = UTIL_dtos3( teamfrags[team] );
		const char* noun = TeamFortress_TeamGetNoPlayers( team ) == 1 ? "player" : "players";
		const char* players = UTIL_dtos2( TeamFortress_TeamGetNoPlayers( team ) );
		const char* score = UTIL_dtos1( teamscores[team] );
		sprintf( results, "Team \"%s\" scored \"%s\" with \"%s\" %s (kills \"%s\") (kills_unaccounted \"%s\")",
			GetTeamName( team ), score, players, noun, frags, unaccounted );
		BOOL hasAllies = FALSE;
		for ( int ally = 1; ally < number_of_teams; ally++ )
		{
			if ( !( teamallies[team] & ( 1 << ( ally - 1 ) ) ) )
				continue;
			sprintf( teamName, "<%s>", GetTeamName( ally ) );
			strcat( allies, teamName );
			hasAllies = TRUE;
		}
		if ( hasAllies )
		{
			strcat( results, " (Allies \"" );
			strcat( results, allies );
			strcat( results, "\")" );
		}
		strcat( results, "\n" );
		UTIL_LogPrintf( results );
	}
}

void DumpClanScores()
{
	BOOL draw = TeamFortress_SortTeams();
	int players[5];
	int unaccounted[5];
	for ( int team = 0; team < 5; team++ )
	{
		players[team] = TeamFortress_TeamGetNoPlayers( team );
		unaccounted[team] = 0;
	}
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer )
			unaccounted[pPlayer->team_no] += pPlayer->real_frags;
	}
	for ( int team = 0; team < 5; team++ )
		unaccounted[team] = teamfrags[team] - unaccounted[team];
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Match_results" );
	LogMatchResults( players, unaccounted, draw );
	if ( draw )
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Match_draw" );
	else
	{
		const char* second = GetTeamName( g_iOrderedTeams[2] );
		const char* first = GetTeamName( g_iOrderedTeams[1] );
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Match_winner", first, second );
		if ( number_of_teams > 3 )
		{
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Match_losers4", GetTeamName( g_iOrderedTeams[3] ) );
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Match_losers3", GetTeamName( g_iOrderedTeams[4] ) );
		}
		else if ( !( number_of_teams <= 2 ) )
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Match_losers3", GetTeamName( g_iOrderedTeams[3] ) );
	}
	for ( int team = 1; team <= number_of_teams; team++ )
	{
		const char* count = UTIL_dtos2( TeamFortress_TeamGetNoPlayers( team ) );
		const char* number = UTIL_dtos1( team );
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Match_team", number, count );
		const char* score = UTIL_dtos3( teamscores[team] );
		const char* missing = UTIL_dtos2( unaccounted[team] );
		const char* frags = UTIL_dtos1( teamfrags[team] );
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Match_teamdump", frags, missing, score );
	}
}
