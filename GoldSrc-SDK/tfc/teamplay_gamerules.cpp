/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// teamplay_gamerules.cpp
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "tf_defs.h"

extern int gmsgGameMode;
extern int gmsgTeamInfo;
extern int gmsgDeathMsg;
extern void GetWeapon( entvars_t *pevInflictor, entvars_t *pevKiller, char *weapon );

CHalfLifeTeamplay::CHalfLifeTeamplay()
{
	m_DisableDeathMessages = FALSE;
	m_DisableDeathPenalty = FALSE;
}

void CHalfLifeTeamplay::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
}

BOOL CHalfLifeTeamplay::IsTeamplay()
{
	return TRUE;
}

const char *CHalfLifeTeamplay::GetTeamID( CBaseEntity *pEntity )
{
	g_pGameRules->IsTeamplay();
	return "";
}

void CHalfLifeTeamplay::PlayerSpawn( CBasePlayer *pPlayer )
{
	if ( g_pGameRules->IsTeamplay() )
		pPlayer->pev->weapons |= 1 << 31;
	else
		CHalfLifeMultiplay::PlayerSpawn( pPlayer );
}

BOOL CHalfLifeTeamplay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	g_pGameRules->IsTeamplay();
	return CHalfLifeMultiplay::ClientCommand( pPlayer, pcmd );
}

void CHalfLifeTeamplay::UpdateGameMode( CBasePlayer *pPlayer )
{
	if ( !g_pGameRules->IsTeamplay() )
	{
		CHalfLifeMultiplay::UpdateGameMode( pPlayer );
		return;
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 1 );  // game mode teamplay
	MESSAGE_END();
}

void CHalfLifeTeamplay::InitHUD( CBasePlayer *pPlayer )
{
	if ( !g_pGameRules->IsTeamplay() )
	{
		CHalfLifeMultiplay::InitHUD( pPlayer );
		return;
	}

	CHalfLifeMultiplay::InitHUD( pPlayer );
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		edict_t *pEdict = INDEXENT( i );
		if ( !pEdict )
			continue;
		CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE( pEdict );
		if ( !pOther )
			continue;
		MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict() );
			WRITE_BYTE( pOther->entindex() );
			WRITE_STRING( g_pGameRules->GetTeamID( pOther ) );
		MESSAGE_END();
	}
}

void CHalfLifeTeamplay::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	if ( !g_pGameRules->IsTeamplay() )
	{
		CHalfLifeMultiplay::DeathNotice( pVictim, pKiller, pevInflictor );
		return;
	}
	if ( m_DisableDeathMessages )
		return;

	CBasePlayer *pAttacker = NULL;
	if ( pVictim && pKiller && ( pKiller->flags & FL_CLIENT ) )
		pAttacker = (CBasePlayer *)CBaseEntity::Instance( pKiller );
	if ( !pAttacker || pAttacker == pVictim || PlayerRelationship( pVictim, pAttacker ) != GR_TEAMMATE )
	{
		CHalfLifeMultiplay::DeathNotice( pVictim, pKiller, pevInflictor );
		return;
	}

	MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
		WRITE_BYTE( ENTINDEX( ENT( pKiller ) ) );		// the killer
		WRITE_BYTE( pVictim->entindex() );
		WRITE_STRING( "teammate" );		// flag this as a teammate kill
	MESSAGE_END();

	char weapon[30];
	GetWeapon( pevInflictor, pKiller, weapon );
	CBasePlayer *pKillerPlayer = (CBasePlayer *)CBaseEntity::Instance( pKiller );
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
		STRING( pKiller->netname ), GETPLAYERUSERID( ENT( pKiller ) ), GETPLAYERAUTHID( ENT( pKiller ) ),
		pKillerPlayer->team_no ? GetTeamName( pKillerPlayer->team_no ) : "SPECTATOR",
		STRING( pVictim->pev->netname ), GETPLAYERUSERID( pVictim->edict() ), GETPLAYERAUTHID( pVictim->edict() ),
		pVictim->team_no ? GetTeamName( pVictim->team_no ) : "SPECTATOR", weapon );
	if ( !pVictim->undercover_team )
	{
		pAttacker->teamkills++;
		pAttacker->CheckAutoKick();
	}
}

void CHalfLifeTeamplay::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	if ( !g_pGameRules->IsTeamplay() || !m_DisableDeathPenalty )
		CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );
}

int CHalfLifeTeamplay::PlayerRelationship( CBasePlayer *pPlayer, CBaseEntity *pTarget )
{
	if ( !g_pGameRules->IsTeamplay() )
		return CHalfLifeMultiplay::PlayerRelationship( pPlayer, pTarget );
	if ( !pPlayer || !pTarget || !( pTarget->pev->flags & FL_CLIENT ) )
		return GR_NOTTEAMMATE;
	if ( *GetTeamID( pPlayer ) && *GetTeamID( pTarget ) &&
		!stricmp( GetTeamID( pPlayer ), GetTeamID( pTarget ) ) )
		return GR_TEAMMATE;
	return pPlayer->IsAlly( pTarget );
}

BOOL CHalfLifeTeamplay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if ( g_pGameRules->IsTeamplay() && pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE &&
		CVAR_GET_FLOAT( "mp_friendlyfire" ) == 0 && pAttacker != pPlayer )
		return FALSE;
	return CHalfLifeMultiplay::FPlayerCanTakeDamage( pPlayer, pAttacker );
}

BOOL CHalfLifeTeamplay::ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target )
{
	if ( !g_pGameRules->IsTeamplay() )
		return CHalfLifeMultiplay::ShouldAutoAim( pPlayer, target );
	CBaseEntity *pTarget = CBaseEntity::Instance( target );
	if ( pTarget && pTarget->Classify() == CLASS_PLAYER && PlayerRelationship( pPlayer, pTarget ) == GR_TEAMMATE )
		return FALSE;
	return CHalfLifeMultiplay::ShouldAutoAim( pPlayer, target );
}

//=========================================================
//=========================================================
int CHalfLifeTeamplay::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if ( !g_pGameRules->IsTeamplay() )
		return CHalfLifeMultiplay::IPointsForKill( pAttacker, pKilled );
	if ( !pKilled )
		return 0;
	if ( pAttacker && pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return -1;
	return 1;
}
