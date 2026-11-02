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
//=========================================================
// GameRules.cpp
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "items.h"
#include "skill.h"
#include "game.h"
#include "tf_defs.h"
#include "hltv.h"
#include <ctype.h>

extern edict_t *EntSelectSpawnPoint( edict_t *player );
extern void SendBuildingEventInfo( CBasePlayer *pPlayer );
extern DLL_GLOBAL BOOL g_fGameOver;
extern int gmsgDeathMsg, gmsgScoreInfo, gmsgMOTD, gmsgServerName, gmsgGameMode, gmsgSpectator;

DLL_GLOBAL CGameRules *g_pGameRules = NULL;
char *g_pszDeathType = NULL;
BOOL g_bFirstClient = FALSE;
ip_storage_t g_IpStorage[32] = {};

#define ITEM_RESPAWN_TIME 30
#define WEAPON_RESPAWN_TIME 20
#define AMMO_RESPAWN_TIME 20
#define MAX_INTERMISSION_TIME 120

void CGameRules::RefreshSkillData()
{
	gSkillData.turretHealth = 1000;
	gSkillData.miniturretHealth = 1000;
	gSkillData.sentryHealth = 1000;
	gSkillData.plrDmgCrowbar = 25;
	gSkillData.plrDmg9MM = 12;
	gSkillData.plrDmg357 = 40;
	gSkillData.plrDmgMP5 = 12;
	gSkillData.plrDmgM203Grenade = 100;
	gSkillData.plrDmgBuckshot = 20;
	gSkillData.plrDmgCrossbowClient = 20;
	gSkillData.plrDmgRPG = 120;
	gSkillData.plrDmgEgonWide = 20;
	gSkillData.plrDmgEgonNarrow = 10;
	gSkillData.plrDmgHandGrenade = 100;
	gSkillData.plrDmgSatchel = 120;
	gSkillData.plrDmgTripmine = 150;
	gSkillData.plrDmgHornet = 10;
	gSkillData.monDmg12MM = 8;
	gSkillData.monDmgMP5 = 3;
	gSkillData.monDmg9MM = 5;
	gSkillData.suitchargerCapacity = 75;
	gSkillData.batteryCapacity = 15;
	gSkillData.healthchargerCapacity = 15;
	gSkillData.healthkitCapacity = 25;
	gSkillData.monHead = 3;
	gSkillData.monChest = 1;
	gSkillData.monStomach = 1;
	gSkillData.monLeg = 1;
	gSkillData.monArm = 1;
	gSkillData.plrHead = 3;
	gSkillData.plrChest = 1;
	gSkillData.plrStomach = 1;
	gSkillData.plrLeg = 1;
	gSkillData.plrArm = 1;
}

BOOL CGameRules::CanHaveAmmo( CBasePlayer *pPlayer, const char *pszAmmoName, int iMaxCarry )
{
	if ( pszAmmoName )
	{
		int index = pPlayer->GetAmmoIndex( pszAmmoName );
		if ( index >= 0 && pPlayer->AmmoInventory( index ) < iMaxCarry )
			return TRUE;
	}
	return FALSE;
}

BOOL CGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	ItemInfo info;
	if ( !pWeapon->GetItemInfo( &info ) )
		return TRUE;
	if ( info.pszAmmo1 && CanHaveAmmo( pPlayer, info.pszAmmo1, info.iAmmo1 ) )
		return TRUE;
	return !pPlayer->HasPlayerItem( pWeapon );
}

void CGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *spawn = EntSelectSpawnPoint( pPlayer->edict() );
	entvars_t *spot = VARS( spawn );
	pPlayer->pev->origin = spot->origin + Vector( 0, 0, 1 );
	UTIL_SetOrigin( pPlayer->pev, pPlayer->pev->origin );
	pPlayer->pev->v_angle = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = spot->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = TRUE;
	if ( pPlayer->pev->playerclass )
	{
		CTelefragDeath *telefrag = GetClassPtr( (CTelefragDeath *)NULL );
		UTIL_SetOrigin( telefrag->pev, pPlayer->pev->origin );
		telefrag->pev->owner = pPlayer->edict();
		telefrag->Spawn( NULL );
	}
	EMIT_SOUND( pPlayer->edict(), CHAN_VOICE,
		RANDOM_FLOAT( 0, 1 ) <= 0.5f ? "misc/r_tele4.wav" : "misc/r_tele3.wav", 1, ATTN_NORM );
	CBaseEntity *goal = CBaseEntity::Instance( spawn );
	if ( goal->Classify() == CLASS_TFSPAWN && gpGlobals->time > cb_prematch_time )
	{
		if ( !pPlayer->forced_spawn )
		{
			if ( goal->pev->message )
			{
				pPlayer->delayed_spawn_message = goal->pev->message;
				if ( !(goal->goal_activation & TFSP_MULTIPLEMSGS) )
					goal->pev->message = 0;
			}
			if ( goal->org_message )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, STRING( goal->org_message ) );
				if ( !(goal->goal_activation & TFSP_MULTIPLEMSGS) )
					goal->org_message = 0;
			}
		}
		DoResults( goal, pPlayer, TRUE );
		if ( goal->items && !(goal->goal_activation & TFSP_MULTIPLEITEMS) )
			goal->items = 0;
		if ( goal->goal_effects == TFSP_REMOVESELF )
			goal->goal_state = TFGS_REMOVED;
	}
	pPlayer->forced_spawn = 0;
	if ( cease_fire )
	{
		if ( !no_cease_fire_text )
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Game_ceasefire" );
		UTIL_LogPrintf( "World triggered \"Cease_Fire\"\n" );
		pPlayer->pev->iuser4 = 1;
		pPlayer->tfstate |= TFSTATE_CANT_MOVE;
		pPlayer->immune_to_check = gpGlobals->time + 10;
		pPlayer->TeamFortress_SetSpeed();
	}
	CBasePlayer *observer = NULL;
	while ( (observer = (CBasePlayer *)UTIL_FindEntityByClassname( observer, "player" )) != NULL )
	{
		if ( FNullEnt( observer->edict() ) )
			break;
		if ( observer != pPlayer && observer->pev && observer->pev->iuser2 == pPlayer->entindex() &&
			(CBaseEntity *)observer->m_hObserverTarget == pPlayer )
		{
			observer->pev->origin = pPlayer->pev->origin;
			UTIL_SetOrigin( observer->pev, observer->pev->origin );
		}
	}
}

CHalfLifeRules::CHalfLifeRules()
{
	RefreshSkillData();
}

void CHalfLifeRules::Think()
{
}

BOOL CHalfLifeRules::IsMultiplayer()
{
	return FALSE;
}

BOOL CHalfLifeRules::IsDeathmatch()
{
	return FALSE;
}

BOOL CHalfLifeRules::IsCoOp()
{
	return FALSE;
}

BOOL CHalfLifeRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem * )
{
	return !pPlayer->m_pActiveItem || pPlayer->m_pActiveItem->CanHolster();
}

BOOL CHalfLifeRules::GetNextBestWeapon( CBasePlayer *, CBasePlayerItem * )
{
	return FALSE;
}

BOOL CHalfLifeRules::ClientConnected( edict_t *, const char *, const char *, char * )
{
	return TRUE;
}

void CHalfLifeRules::InitHUD( CBasePlayer * )
{
}

void CHalfLifeRules::ClientDisconnected( edict_t * )
{
}

float CHalfLifeRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * 100 / (PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED);
}

void CHalfLifeRules::PlayerSpawn( CBasePlayer * )
{
}

BOOL CHalfLifeRules::AllowAutoTargetCrosshair()
{
	return g_iSkillLevel == SKILL_EASY;
}

void CHalfLifeRules::PlayerThink( CBasePlayer * )
{
}

BOOL CHalfLifeRules::FPlayerCanRespawn( CBasePlayer * )
{
	return TRUE;
}

float CHalfLifeRules::FlPlayerSpawnTime( CBasePlayer * )
{
	return gpGlobals->time;
}

int CHalfLifeRules::IPointsForKill( CBasePlayer *, CBasePlayer * )
{
	return 1;
}

void CHalfLifeRules::PlayerKilled( CBasePlayer *, entvars_t *, entvars_t * )
{
}

void CHalfLifeRules::DeathNotice( CBasePlayer *, entvars_t *, entvars_t * )
{
}

void CHalfLifeRules::PlayerGotWeapon( CBasePlayer *, CBasePlayerItem * )
{
}

float CHalfLifeRules::FlWeaponRespawnTime( CBasePlayerItem * )
{
	return -1;
}

float CHalfLifeRules::FlWeaponTryRespawn( CBasePlayerItem * )
{
	return 0;
}

Vector CHalfLifeRules::VecWeaponRespawnSpot( CBasePlayerItem *weapon )
{
	return weapon->pev->origin;
}

int CHalfLifeRules::WeaponShouldRespawn( CBasePlayerItem * )
{
	return GR_WEAPON_RESPAWN_NO;
}

BOOL CHalfLifeRules::CanHaveItem( CBasePlayer *, CItem * )
{
	return TRUE;
}

void CHalfLifeRules::PlayerGotItem( CBasePlayer *, CItem * )
{
}

int CHalfLifeRules::ItemShouldRespawn( CItem * )
{
	return GR_ITEM_RESPAWN_NO;
}

float CHalfLifeRules::FlItemRespawnTime( CItem * )
{
	return -1;
}

Vector CHalfLifeRules::VecItemRespawnSpot( CItem *item )
{
	return item->pev->origin;
}

BOOL CHalfLifeRules::IsAllowedToSpawn( CBaseEntity * )
{
	return TRUE;
}

void CHalfLifeRules::PlayerGotAmmo( CBasePlayer *, char *, int )
{
}

int CHalfLifeRules::AmmoShouldRespawn( CBasePlayerAmmo * )
{
	return GR_AMMO_RESPAWN_NO;
}

float CHalfLifeRules::FlAmmoRespawnTime( CBasePlayerAmmo * )
{
	return -1;
}

Vector CHalfLifeRules::VecAmmoRespawnSpot( CBasePlayerAmmo *ammo )
{
	return ammo->pev->origin;
}

float CHalfLifeRules::FlHealthChargerRechargeTime()
{
	return 0;
}

int CHalfLifeRules::DeadPlayerWeapons( CBasePlayer * )
{
	return GR_PLR_DROP_GUN_NO;
}

int CHalfLifeRules::DeadPlayerAmmo( CBasePlayer * )
{
	return GR_PLR_DROP_AMMO_NO;
}

int CHalfLifeRules::PlayerRelationship( CBasePlayer *, CBaseEntity * )
{
	return GR_NOTTEAMMATE;
}

CHalfLifeMultiplay::CHalfLifeMultiplay()
{
	RefreshSkillData();
	m_flIntermissionEndTime = 0;
	m_flIntermissionStartTime = 0;
	m_iEndIntermissionButtonHit = FALSE;
	if ( !IS_DEDICATED_SERVER() )
	{
		const char *config = CVAR_GET_STRING( "lservercfgfile" );
		if ( config && config[0] )
		{
			char command[256];
			ALERT( at_console, "Executing listen server config file\n" );
			sprintf( command, "exec %s\n", config );
			SERVER_COMMAND( command );
		}
	}
	SERVER_EXECUTE();
	UTIL_LogPrintf( "Server name is \"%s\"\n", CVAR_GET_STRING( "hostname" ) );
}

BOOL CHalfLifeMultiplay::IsMultiplayer()
{
	return TRUE;
}

BOOL CHalfLifeMultiplay::IsDeathmatch()
{
	return TRUE;
}

BOOL CHalfLifeMultiplay::IsCoOp()
{
	return FALSE;
}

BOOL CHalfLifeMultiplay::ClientConnected( edict_t *entity, const char *, const char *address, char *reject )
{
	int id = GetBattleID( entity );
	CGhost *ghost = id ? FindGhost( id ) : NULL;
	if ( tfc_clanbattle_locked.value && gpGlobals->time > cb_prematch_time && !ghost )
	{
		strcpy( reject, "#Game_locked" );
		return FALSE;
	}
	for ( int i = 0; i <= gpGlobals->maxClients; ++i )
	{
		if ( g_IpStorage[i].ip )
			continue;
		char *port = strchr( (char *)address, ':' );
		if ( port )
			*port = 0;
		g_IpStorage[i].pEdict = entity;
		g_IpStorage[i].ip = ALLOC_STRING( address );
		if ( port )
			*port = ':';
		break;
	}
	return TRUE;
}

void CHalfLifeMultiplay::ClientDisconnected( edict_t *client )
{
	if ( !client )
		return;
	CBasePlayer *player = (CBasePlayer *)CBaseEntity::Instance( client );
	if ( !player )
		return;
	player->has_disconnected = TRUE;
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" disconnected\n", STRING( player->pev->netname ),
		GETPLAYERUSERID( player->edict() ), GETPLAYERAUTHID( player->edict() ),
		player->team_no ? GetTeamName( player->team_no ) : "SPECTATOR" );
	player->CleanupOnPlayerDisconnection();
	if ( tfc_clanbattle.value && player->tf_id )
		player->CreateGhost();
	player->pev->playerclass = 0;
	player->pev->health = 0;
	player->RemoveAllItems( FALSE );
	MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
		WRITE_BYTE( ENTINDEX( client ) );
		WRITE_BYTE( 0 );
	MESSAGE_END();
}

void CHalfLifeMultiplay::UpdateGameMode( CBasePlayer *player )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, player->edict() );
		WRITE_BYTE( 0 );
	MESSAGE_END();
}

void CHalfLifeMultiplay::InitHUD( CBasePlayer *player )
{
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_playerjoin",
		player->pev->netname && STRING( player->pev->netname )[0] ? STRING( player->pev->netname ) : "unconnected" );
	UpdateGameMode( player );
	player->m_MenuUpdateTime = gpGlobals->time + 0.1;
	MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, player->edict() );
		WRITE_BYTE( player->entindex() );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
	MESSAGE_END();
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		edict_t *entity = INDEXENT( i );
		if ( !entity )
			continue;
		CBasePlayer *other = (CBasePlayer *)CBaseEntity::Instance( entity );
		if ( !other )
			continue;
		MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, player->edict() );
			WRITE_BYTE( i );
			WRITE_SHORT( other->pev->frags );
			WRITE_SHORT( other->m_iDeaths );
			WRITE_SHORT( other->pev->playerclass );
			WRITE_SHORT( other->pev->team );
		MESSAGE_END();
		MESSAGE_BEGIN( MSG_ONE, gmsgSpectator, NULL, player->edict() );
			WRITE_BYTE( i );
			WRITE_BYTE( other->pev->iuser1 != 0 );
		MESSAGE_END();
	}
	SendBuildingEventInfo( player );
	if ( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, player->edict() );
		MESSAGE_END();
	}
}

BOOL CHalfLifeMultiplay::ClientCommand( CBasePlayer *player, const char *command )
{
	if ( strcmp( command, "menuselect" ) )
		return FALSE;
	if ( CMD_ARGC() > 1 )
		player->Menu_Input( (float)atoi( CMD_ARGV( 1 ) ) );
	return TRUE;
}

void CHalfLifeMultiplay::Think()
{
	static int lastFrags, lastTime;
	if ( g_fGameOver )
	{
		int chatTime = (int)CVAR_GET_FLOAT( "mp_chattime" );
		if ( chatTime <= 0 )
			CVAR_SET_STRING( "mp_chattime", "1" );
		else if ( chatTime > MAX_INTERMISSION_TIME )
			CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );
		m_flIntermissionEndTime = m_flIntermissionStartTime + mp_chattime.value;
		if ( gpGlobals->time > m_flIntermissionEndTime &&
			(m_iEndIntermissionButtonHit || gpGlobals->time > m_flIntermissionStartTime + MAX_INTERMISSION_TIME) )
			ChangeLevel();
		return;
	}
	float flTimeLimit = timelimit.value * 60;
	float flFragLimit = fraglimit.value;
	int remainingTime = (int)( flTimeLimit ? ( flTimeLimit - gpGlobals->time ) : 0 );
	if ( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
	{
		GoToIntermission();
		return;
	}
	int remainingFrags = 0;
	if ( flFragLimit )
	{
		remainingFrags = 9999;
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			edict_t *entity = INDEXENT( i );
			if ( !entity )
				continue;
			if ( entity->v.frags >= flFragLimit )
			{
				GoToIntermission();
				return;
			}
			int remain = (int)( flFragLimit - entity->v.frags );
			if ( remain < remainingFrags )
				remainingFrags = remain;
		}
	}
	if ( remainingFrags != lastFrags )
		g_engfuncs.pfnCvar_DirectSet( &fragsleft, UTIL_VarArgs( "%i", remainingFrags ) );
	if ( timeleft.value != lastTime )
		g_engfuncs.pfnCvar_DirectSet( &timeleft, UTIL_VarArgs( "%i", remainingTime ) );
	lastFrags = remainingFrags;
	lastTime = remainingTime;
}

void CHalfLifeMultiplay::GoToIntermission()
{
	if ( !g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ALL, SVC_INTERMISSION );
		MESSAGE_END();
		g_fGameOver = TRUE;
		m_iEndIntermissionButtonHit = FALSE;
		m_flIntermissionEndTime = gpGlobals->time + 6;
	}
}

BOOL CHalfLifeMultiplay::FShouldSwitchWeapon( CBasePlayer *, CBasePlayerItem * )
{
	return FALSE;
}

BOOL CHalfLifeMultiplay::GetNextBestWeapon( CBasePlayer *player, CBasePlayerItem *current )
{
	if ( !current->CanHolster() )
		return FALSE;
	ItemInfo info;
	current->GetItemInfo( &info );
	int currentWeight = info.iWeight;
	int bestWeight = -1;
	CBasePlayerItem *best = NULL;
	for ( int i = 0; i < MAX_ITEM_TYPES; ++i )
	{
		for ( CBasePlayerItem *weapon = player->m_rgpPlayerItems[i]; weapon; weapon = weapon->m_pNext )
		{
			weapon->GetItemInfo( &info );
			if ( info.iWeight >= 0 && info.iWeight == currentWeight && weapon != current )
			{
				if ( weapon->CanDeploy() && player->SwitchWeapon( weapon ) )
					return TRUE;
			}
			else if ( info.iWeight > bestWeight && weapon != current && weapon->CanDeploy() )
			{
				best = weapon;
				bestWeight = info.iWeight;
			}
		}
	}
	if ( !best )
		return FALSE;
	player->SwitchWeapon( best );
	return TRUE;
}

float CHalfLifeMultiplay::FlPlayerFallDamage( CBasePlayer *player )
{
	if ( (int)falldamage.value != 1 )
		return 10;
	player->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return player->m_flFallVelocity * 100 / (PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED);
}

BOOL CHalfLifeMultiplay::FPlayerCanTakeDamage( CBasePlayer *, CBaseEntity * )
{
	return TRUE;
}

void CHalfLifeMultiplay::PlayerThink( CBasePlayer *player )
{
	if ( g_fGameOver )
	{
		if ( player->m_afButtonPressed & (IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP) )
			m_iEndIntermissionButtonHit = TRUE;
		player->m_afButtonPressed = 0;
		player->pev->button = 0;
		player->m_afButtonReleased = 0;
	}
}

void CHalfLifeMultiplay::PlayerSpawn( CBasePlayer *player )
{
	player->pev->weapons |= 1 << WEAPON_SUIT;
	player->GiveNamedItem( "weapon_crowbar" );
	player->GiveNamedItem( "weapon_9mmhandgun" );
	player->GiveAmmo( 68, "9mm", 250 );
}

BOOL CHalfLifeMultiplay::FPlayerCanRespawn( CBasePlayer * )
{
	return TRUE;
}

float CHalfLifeMultiplay::FlPlayerSpawnTime( CBasePlayer * )
{
	return gpGlobals->time;
}

BOOL CHalfLifeMultiplay::AllowAutoTargetCrosshair()
{
	return aimcrosshair.value != 0;
}

int CHalfLifeMultiplay::IPointsForKill( CBasePlayer *, CBasePlayer * )
{
	return 1;
}

void CHalfLifeMultiplay::PlayerKilled( CBasePlayer *victim, entvars_t *killer, entvars_t *inflictor )
{
	DeathNotice( victim, killer, inflictor );
	++victim->m_iDeaths;
	CBaseEntity *entity = CBaseEntity::Instance( killer );
	CBasePlayer *player = entity && entity->Classify() == CLASS_PLAYER ? (CBasePlayer *)entity : NULL;
	if ( killer != victim->pev && (killer->flags & FL_CLIENT) )
		player->TF_AddFrags( IPointsForKill( player, victim ) );
	else
		victim->TF_AddFrags( -1 );
	entity = CBaseEntity::Instance( killer );
	if ( entity && entity->Classify() == CLASS_PLAYER )
		((CBasePlayer *)entity)->m_flNextDecalTime = gpGlobals->time;
}

void CHalfLifeMultiplay::PlayerGotWeapon( CBasePlayer *, CBasePlayerItem * )
{
}

void CHalfLifeMultiplay::DeathNotice( CBasePlayer *victim, entvars_t *killer, entvars_t *inflictor )
{
	CBaseEntity::Instance( killer );
	int killerIndex = 0;
	const char *weaponName = g_pszDeathType;
	if ( !weaponName )
	{
		bool stripPrefix = false;
		weaponName = "world";
		if ( killer->flags & FL_CLIENT )
		{
			killerIndex = ENTINDEX( ENT( killer ) );
			if ( inflictor == killer )
			{
				CBasePlayer *player = (CBasePlayer *)CBaseEntity::Instance( killer );
				if ( player->m_pActiveItem )
				{
					stripPrefix = true;
					if ( player->m_pActiveItem->deathtype )
						weaponName = STRING( player->m_pActiveItem->deathtype );
					else
					{
						ItemInfo info;
						if ( player->m_pActiveItem->GetItemInfo( &info ) )
							weaponName = info.pszName;
					}
				}
			}
			else if ( inflictor )
			{
				stripPrefix = true;
				CBaseEntity *entity = CBaseEntity::Instance( inflictor );
				weaponName = STRING( entity && entity->deathtype ? entity->deathtype : inflictor->classname );
			}
		}
		else
		{
			stripPrefix = true;
			CBaseEntity *entity = CBaseEntity::Instance( inflictor );
			weaponName = STRING( entity && entity->deathtype ? entity->deathtype : inflictor->classname );
		}

		if ( stripPrefix )
		{
			if ( !strncmp( weaponName, "weapon_", 7 ) )
				weaponName += 7;
			else if ( !strncmp( weaponName, "tf_weapon_", 10 ) )
				weaponName += 10;
			else if ( !strncmp( weaponName, "monster_", 8 ) )
				weaponName += 8;
			else if ( !strncmp( weaponName, "func_", 5 ) )
				weaponName += 5;
		}
	}

	MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
		WRITE_BYTE( killerIndex );
		WRITE_BYTE( victim->entindex() );
		WRITE_STRING( weaponName );
	MESSAGE_END();

	const char *victimTeam = victim->team_no ? GetTeamName( victim->team_no ) : "SPECTATOR";
	if ( killer == victim->pev )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n",
			STRING( victim->pev->netname ), GETPLAYERUSERID( victim->edict() ),
			GETPLAYERAUTHID( victim->edict() ), victimTeam, weaponName );
	}
	else if ( killer->flags & FL_CLIENT )
	{
		CBaseEntity *entity = CBaseEntity::Instance( killer );
		const char *killerTeam = entity->team_no ? GetTeamName( entity->team_no ) : "SPECTATOR";
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
			STRING( killer->netname ), GETPLAYERUSERID( ENT( killer ) ), GETPLAYERAUTHID( ENT( killer ) ),
			killerTeam, STRING( victim->pev->netname ), GETPLAYERUSERID( victim->edict() ),
			GETPLAYERAUTHID( victim->edict() ), victimTeam, weaponName );
	}
	else
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n",
			STRING( victim->pev->netname ), GETPLAYERUSERID( victim->edict() ),
			GETPLAYERAUTHID( victim->edict() ), victimTeam, weaponName );
	}
	g_pszDeathType = NULL;

	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE( 9 );
		WRITE_BYTE( DRC_CMD_EVENT );
		WRITE_SHORT( victim->entindex() );
		WRITE_SHORT( ENTINDEX( ENT( inflictor ? inflictor : killer ) ) );
		WRITE_LONG( 7 | DRC_FLAG_DRAMATIC );
	MESSAGE_END();
}

float CHalfLifeMultiplay::FlWeaponRespawnTime( CBasePlayerItem *weapon )
{
	if ( weaponstay.value > 0 )
	{
		ItemInfo info;
		info.iFlags = 0;
		weapon->GetItemInfo( &info );
		if ( !(info.iFlags & ITEM_FLAG_LIMITINWORLD) )
			return gpGlobals->time;
	}
	return gpGlobals->time + WEAPON_RESPAWN_TIME;
}

float CHalfLifeMultiplay::FlWeaponTryRespawn( CBasePlayerItem *weapon )
{
	if ( weapon && weapon->m_iId && (weapon->iFlags() & ITEM_FLAG_LIMITINWORLD) &&
		NUMBER_OF_ENTITIES() >= gpGlobals->maxEntities - 100 )
		return FlWeaponRespawnTime( weapon );
	return 0;
}

Vector CHalfLifeMultiplay::VecWeaponRespawnSpot( CBasePlayerItem *weapon )
{
	return weapon->pev->origin;
}

int CHalfLifeMultiplay::WeaponShouldRespawn( CBasePlayerItem *weapon )
{
	return weapon->pev->spawnflags & SF_NORESPAWN ? GR_WEAPON_RESPAWN_NO : GR_WEAPON_RESPAWN_YES;
}

BOOL CHalfLifeMultiplay::CanHavePlayerItem( CBasePlayer *player, CBasePlayerItem *item )
{
	if ( weaponstay.value > 0 )
	{
		ItemInfo info;
		info.iFlags = 0;
		item->GetItemInfo( &info );
		if ( !(info.iFlags & ITEM_FLAG_LIMITINWORLD) )
		{
			for ( int i = 0; i < MAX_ITEM_TYPES; ++i )
				for ( CBasePlayerItem *owned = player->m_rgpPlayerItems[i]; owned; owned = owned->m_pNext )
					if ( owned->m_iId == info.iId )
						return FALSE;
		}
	}
	return CGameRules::CanHavePlayerItem( player, item );
}

BOOL CHalfLifeMultiplay::CanHaveItem( CBasePlayer *, CItem * )
{
	return TRUE;
}

void CHalfLifeMultiplay::PlayerGotItem( CBasePlayer *, CItem * )
{
}

int CHalfLifeMultiplay::ItemShouldRespawn( CItem *item )
{
	return item->pev->spawnflags & SF_NORESPAWN ? GR_ITEM_RESPAWN_NO : GR_ITEM_RESPAWN_YES;
}

float CHalfLifeMultiplay::FlItemRespawnTime( CItem * )
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

Vector CHalfLifeMultiplay::VecItemRespawnSpot( CItem *item )
{
	return item->pev->origin;
}

void CHalfLifeMultiplay::PlayerGotAmmo( CBasePlayer *, char *, int )
{
}

BOOL CHalfLifeMultiplay::IsAllowedToSpawn( CBaseEntity * )
{
	return TRUE;
}

int CHalfLifeMultiplay::AmmoShouldRespawn( CBasePlayerAmmo *ammo )
{
	return ammo->pev->spawnflags & SF_NORESPAWN ? GR_AMMO_RESPAWN_NO : GR_AMMO_RESPAWN_YES;
}

float CHalfLifeMultiplay::FlAmmoRespawnTime( CBasePlayerAmmo * )
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

Vector CHalfLifeMultiplay::VecAmmoRespawnSpot( CBasePlayerAmmo *ammo )
{
	return ammo->pev->origin;
}

float CHalfLifeMultiplay::FlHealthChargerRechargeTime()
{
	return 60;
}

float CHalfLifeMultiplay::FlHEVChargerRechargeTime()
{
	return 30;
}

int CHalfLifeMultiplay::DeadPlayerWeapons( CBasePlayer * )
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

int CHalfLifeMultiplay::DeadPlayerAmmo( CBasePlayer * )
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

int CHalfLifeMultiplay::PlayerRelationship( CBasePlayer *, CBaseEntity * )
{
	return GR_NOTTEAMMATE;
}

BOOL CHalfLifeMultiplay::PlayFootstepSounds( CBasePlayer *player, float )
{
	return (!g_footsteps || g_footsteps->value != 0) &&
		(player->IsOnLadder() || player->pev->velocity.Length2D() > 220);
}

BOOL CHalfLifeMultiplay::FAllowFlashlight()
{
	return flashlight.value != 0;
}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;

	char mapname[ 32 ];
	int  minplayers, maxplayers;
	char rulebuffer[ MAX_RULE_BUFFER ];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
} mapcycle_t;

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle( mapcycle_t *cycle )
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if ( p )
	{
		start = p;
		p = p->next;
		while ( p != start )
		{
			n = p->next;
			delete p;
			p = n;
		}
		
		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

static char com_token[ 1500 ];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	int             c;
	int             len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;
		data++;
	}
	
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
int COM_TokenWaiting( char *buffer )
{
	char *p;

	p = buffer;
	while ( *p && *p!='\n')
	{
		if ( !isspace( *p ) || isalnum( *p ) )
			return 1;

		p++;
	}

	return 0;
}



/*
==============
ReloadMapCycleFile


Parses mapcycle.txt file into mapcycle_t structure
==============
*/
int ReloadMapCycleFile( char *filename, mapcycle_t *cycle )
{
	char szBuffer[ MAX_RULE_BUFFER ];
	char szMap[ 32 ];
	int length;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( filename, &length );
	int hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;

	if ( pFileList && length )
	{
		// the first map name in the file becomes the default
		while ( 1 )
		{
			hasbuffer = 0;
			memset( szBuffer, 0, MAX_RULE_BUFFER );

			pFileList = COM_Parse( pFileList );
			if ( strlen( com_token ) <= 0 )
				break;

			strncpy( szMap, com_token, sizeof( szMap ) );
			szMap[ sizeof( szMap ) - 1 ] = '\0';

			// Any more tokens on this line?
			if ( COM_TokenWaiting( pFileList ) )
			{
				pFileList = COM_Parse( pFileList );
				if ( strlen( com_token ) > 0 )
				{
					hasbuffer = 1;
					strncpy( szBuffer, com_token, sizeof( szBuffer ) );
					szBuffer[ sizeof( szBuffer ) - 1 ] = '\0';
				}
			}

			// Check map
			if ( IS_MAP_VALID( szMap ) )
			{
				// Create entry
				char *s;

				item = new mapcycle_item_s;

				strcpy( item->mapname, szMap );

				item->minplayers = 0;
				item->maxplayers = 0;

				memset( item->rulebuffer, 0, MAX_RULE_BUFFER );

				if ( hasbuffer )
				{
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "minplayers" );
					if ( s && s[0] )
					{
						item->minplayers = atoi( s );
						item->minplayers = max( item->minplayers, 0 );
						item->minplayers = min( item->minplayers, gpGlobals->maxClients );
					}
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "maxplayers" );
					if ( s && s[0] )
					{
						item->maxplayers = atoi( s );
						item->maxplayers = max( item->maxplayers, 0 );
						item->maxplayers = min( item->maxplayers, gpGlobals->maxClients );
					}

					// Remove keys
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "minplayers" );
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "maxplayers" );

					strcpy( item->rulebuffer, szBuffer );
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT( at_console, "Skipping %s from mapcycle, not a valid map\n", szMap );
			}

		}

		FREE_FILE( aFileList );
	}

	item = cycle->items;

	while ( item )
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	if ( !item )
	{
		return 0;
	}

	while ( item->next )
	{
		item = item->next;
	}
	item->next = cycle->items;
	
	cycle->next_item = item->next;

	return 1;
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
*/
int CountPlayers( void )
{
	int	num = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );

		if ( pEnt )
		{
			num = num + 1;
		}
	}

	return num;
}

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
void ExtractCommandString( char *s, char *szCommand )
{
	char	pkey[512];
	char	value[512];
	char	*o;
	
	if ( *s == '\\' )
		s++;

	while (1)
	{
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat( szCommand, pkey );
		if ( strlen( value ) > 0 )
		{
			strcat( szCommand, " " );
			strcat( szCommand, value );
		}
		strcat( szCommand, "\n" );

		if (!*s)
			return;
		s++;
	}
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void CHalfLifeMultiplay :: ChangeLevel( void )
{
	static char szPreviousMapCycleFile[ 256 ];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[ 1500 ];
	char szRules[ 1500 ];
	int minplayers = 0, maxplayers = 0;
	strcpy( szFirstMapInList, "hldm1" );  // the absolute default level is hldm1

	int	curplayers;
	BOOL do_cycle = TRUE;

	// find the map to change to
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	szCommands[ 0 ] = '\0';
	szRules[ 0 ] = '\0';

	curplayers = CountPlayers();

	// Has the map cycle filename changed?
	szPreviousMapCycleFile[ sizeof( szPreviousMapCycleFile ) - 1 ] = '\0';
	if ( stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		strncpy( szPreviousMapCycleFile, mapcfile, sizeof( szPreviousMapCycleFile ) - 1 );

		DestroyMapCycle( &mapcycle );

		if ( !ReloadMapCycleFile( mapcfile, &mapcycle ) || ( !mapcycle.items ) )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
			do_cycle = FALSE;
		}
	}

	if ( do_cycle && mapcycle.items )
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		// Assume current map
		strcpy( szNextMap, STRING(gpGlobals->mapname) );
		strcpy( szFirstMapInList, STRING(gpGlobals->mapname) );

		// Traverse list
		for ( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = FALSE;

			ASSERT( item != NULL );

			if ( item->minplayers != 0 )
			{
				if ( curplayers >= item->minplayers )
				{
					found = TRUE;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( item->maxplayers != 0 )
			{
				if ( curplayers <= item->maxplayers )
				{
					found = TRUE;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( keeplooking )
				continue;

			found = TRUE;
			break;
		}

		if ( !found )
		{
			item = mapcycle.next_item;
		}			
		
		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		strcpy( szNextMap, item->mapname );

		ExtractCommandString( item->rulebuffer, szCommands );
		strcpy( szRules, item->rulebuffer );
	}

	if ( !IS_MAP_VALID(szNextMap) )
	{
		strcpy( szNextMap, szFirstMapInList );
	}

	g_fGameOver = TRUE;

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	if ( minplayers || maxplayers )
	{
		ALERT( at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers );
	}
	if ( strlen( szRules ) > 0 )
	{
		ALERT( at_console, "RULES:  %s\n", szRules );
	}
	
	CHANGE_LEVEL( szNextMap, NULL );
	if ( strlen( szCommands ) > 0 )
	{
		SERVER_COMMAND( szCommands );
	}
}

#define MAX_MOTD_CHUNK	  60
#define MAX_MOTD_LENGTH   1536 // (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay :: SendMOTDToClient( edict_t *client )
{
	// send the server name
	MESSAGE_BEGIN( MSG_ONE, gmsgServerName, NULL, client );
		WRITE_STRING( CVAR_GET_STRING("hostname") );
	MESSAGE_END();

	// read from the MOTD.txt file
	int length, char_count = 0;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( (char *)CVAR_GET_STRING( "motdfile" ), &length );

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while ( pFileList && *pFileList && char_count < MAX_MOTD_LENGTH )
	{
		char chunk[MAX_MOTD_CHUNK+1];
		
		if ( strlen( pFileList ) < MAX_MOTD_CHUNK )
		{
			strcpy( chunk, pFileList );
		}
		else
		{
			strncpy( chunk, pFileList, MAX_MOTD_CHUNK );
			chunk[MAX_MOTD_CHUNK] = 0;		// strncpy doesn't always append the null terminator
		}

		char_count += strlen( chunk );
		if ( char_count < MAX_MOTD_LENGTH )
			pFileList = aFileList + char_count; 
		else
			*pFileList = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, client );
			WRITE_BYTE( *pFileList ? FALSE : TRUE );	// FALSE means there is still more message to come
			WRITE_STRING( chunk );
		MESSAGE_END();
	}

	FREE_FILE( aFileList );
}
	
