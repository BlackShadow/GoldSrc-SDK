#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "items.h"
#include "tf_defs.h"
#include "tf_gamerules.h"

extern int gmsgTeamInfo;
extern int gmsgViewMode;
extern BOOL g_fGameOver;
extern cvar_t mp_chattime;

class TFGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	bool CanPlayerHearPlayer( CBasePlayer* pPlayer1, CBasePlayer* pPlayer2 );
};

TFGameMgrHelper g_GameMgrHelper;

bool TFGameMgrHelper::CanPlayerHearPlayer( CBasePlayer* pPlayer1, CBasePlayer* pPlayer2 )
{
	if ( pPlayer1->team_no == pPlayer2->team_no )
		return true;
	return ( teamallies[pPlayer1->team_no] >> ( pPlayer2->team_no - 1 ) ) & 1;
}

CTeamFortress::CTeamFortress()
{
	m_VoiceGameMgr.Init( &g_GameMgrHelper, gpGlobals->maxClients );
}

BOOL CTeamFortress::IsTeamplay()
{
	return gpGlobals->teamplay > 0;
}

void CTeamFortress::PlayerSpawn( CBasePlayer* pPlayer )
{
	pPlayer->pev->weapons |= 0x80000000;
}

int CTeamFortress::DeadPlayerWeapons( CBasePlayer* pPlayer )
{
	return GR_PLR_DROP_GUN_NO;
}

int CTeamFortress::DeadPlayerAmmo( CBasePlayer* pPlayer )
{
	return GR_PLR_DROP_AMMO_ALL;
}

float CTeamFortress::FlItemRespawnTime( CItem* pItem )
{
	return gpGlobals->time + pItem->m_flRespawnTime;
}

void CTeamFortress::ClientUserInfoChanged( CBasePlayer* pPlayer, char* infobuffer )
{
	pPlayer->TeamFortress_SetSkin();
	pPlayer->exec_scripts = 0;
	char* pszValue = g_engfuncs.pfnInfoKeyValue( infobuffer, "ec" );
	if ( !pszValue || !*pszValue )
		pszValue = g_engfuncs.pfnInfoKeyValue( infobuffer, "exec_class" );
	if ( pszValue && *pszValue && ( !strcmp( pszValue, "1" ) || !strcmp( pszValue, "on" ) ) )
		pPlayer->exec_scripts = 1;

	pPlayer->exec_map_scripts = 0;
	pszValue = g_engfuncs.pfnInfoKeyValue( infobuffer, "em" );
	if ( !pszValue || !*pszValue )
		g_engfuncs.pfnInfoKeyValue( infobuffer, "exec_map" );
	if ( pszValue && *pszValue && ( !strcmp( pszValue, "1" ) || !strcmp( pszValue, "on" ) ) )
		pPlayer->exec_map_scripts = 1;

	pPlayer->display_class_briefing = 0;
	pPlayer->take_screenshots = 0;
	pszValue = g_engfuncs.pfnInfoKeyValue( infobuffer, "take_sshot" );
	if ( !pszValue || !*pszValue )
		pszValue = g_engfuncs.pfnInfoKeyValue( infobuffer, "ts" );
	if ( pszValue && *pszValue && ( !strcmp( pszValue, "1" ) || !strcmp( pszValue, "on" ) ) )
		pPlayer->take_screenshots = 1;

	pPlayer->local_blood = 1;
	pszValue = g_engfuncs.pfnInfoKeyValue( infobuffer, "cl_lb" );
	if ( pszValue && *pszValue )
		pPlayer->local_blood = atoi( pszValue ) != 0;
}

const char* CTeamFortress::GetTeamID( CBaseEntity* pEntity )
{
	return GetTeamName( pEntity->team_no );
}

BOOL CTeamFortress::CanHaveItem( CBasePlayer* pPlayer, CItem* pItem )
{
	if ( pPlayer->is_feigning )
		return FALSE;
	if ( pPlayer->tfstate & ( TFSTATE_AIMING | TFSTATE_CANT_MOVE ) )
		return FALSE;
	if ( cb_prematch_time > gpGlobals->time )
		return FALSE;
	return ActivationSucceeded( pItem, pPlayer, NULL );
}

void CTeamFortress::GoToIntermission()
{
	if ( g_fGameOver )
		return;

	MESSAGE_BEGIN( MSG_ALL, SVC_INTERMISSION );
	MESSAGE_END();
	DumpClanScores();
	clan_scores_dumped = 1;
	int iChatTime = (int)CVAR_GET_FLOAT( "mp_chattime" );
	if ( iChatTime <= 0 )
		CVAR_SET_STRING( "mp_chattime", "1" );
	else if ( iChatTime > 120 )
		CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( 120 ) );
	m_flIntermissionEndTime = (int)mp_chattime.value + gpGlobals->time;
	m_iEndIntermissionButtonHit = FALSE;
	g_fGameOver = TRUE;
	m_flIntermissionStartTime = gpGlobals->time;
}

void CTeamFortress::Think()
{
	m_VoiceGameMgr.Update( gpGlobals->frametime );
	CHalfLifeMultiplay::Think();
}

BOOL CTeamFortress::ClientConnected( edict_t* pEntity, const char* pszName, const char* pszAddress,
	char szRejectReason[128] )
{
	m_VoiceGameMgr.ClientConnected( pEntity );
	if ( !g_bFirstClient )
	{
		g_bFirstClient = TRUE;
		ParseTFServerSettings();
	}
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( ENTINDEX( pEntity ) );
		WRITE_STRING( "" );
	MESSAGE_END();
	MESSAGE_BEGIN( MSG_ONE, SVC_STUFFTEXT, NULL, pEntity );
		WRITE_STRING( UTIL_VarArgs( "cl_forwardspeed %d\ncl_backspeed %d\ncl_sidespeed %d\n", 400, 400, 400 ) );
	MESSAGE_END();
	return CHalfLifeMultiplay::ClientConnected( pEntity, pszName, pszAddress, szRejectReason );
}

BOOL CTeamFortress::ClientCommand( CBasePlayer* pPlayer, const char* pcmd )
{
	return m_VoiceGameMgr.ClientCommand( pPlayer, pcmd ) || CHalfLifeTeamplay::ClientCommand( pPlayer, pcmd );
}

void CTeamFortress::InitHUD( CBasePlayer* pl )
{
	CHalfLifeTeamplay::InitHUD( pl );
	UTIL_LogPrintf( "\"%s<%i><%s><>\" entered the game\n", STRING( pl->pev->netname ),
		GETPLAYERUSERID( pl->edict() ), GETPLAYERAUTHID( pl->edict() ) );
	UTIL_LogPrintf( "\"%s<%i><%s><>\" joined team \"SPECTATOR\"\n", STRING( pl->pev->netname ),
		GETPLAYERUSERID( pl->edict() ), GETPLAYERAUTHID( pl->edict() ) );
	for ( int i = 1; number_of_teams >= i; i++ )
		TeamFortress_TeamIncreaseScore( i, 0 );
	pl->TeamFortress_ExecMapScript();
	pl->m_bDisplayedMOTD = FALSE;
	if ( CVAR_GET_FLOAT( "sv_cheats" ) == 0 )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgViewMode, NULL, pl->pev );
		MESSAGE_END();
	}
}
