#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "items.h"
#include "gamerules.h"
#include "game.h"
#include "coopplay_gamerules.h"

#define ENTITY_INTOLERANCE	100

extern int gmsgGameMode;
extern int gmsgScoreInfo;
extern BOOL g_fGameOver;

CHalfLifeCoopplay::CHalfLifeCoopplay()
{
	m_DisableDeathMessages = FALSE;
	m_DisableDeathPenalty = FALSE;
	CVAR_SET_FLOAT( "mp_allowmonsters", 1 );
}

BOOL CHalfLifeCoopplay::IsCoOp()
{
	return TRUE;
}

void CHalfLifeCoopplay::UpdateGameMode( CBasePlayer* pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 1 );
	MESSAGE_END();
}

void CHalfLifeCoopplay::DeathNotice( CBasePlayer*, entvars_t*, entvars_t* )
{
}

void CHalfLifeCoopplay::MonsterKilled( CBaseMonster* pVictim, entvars_t* pKiller, entvars_t* )
{
	CBaseEntity* pEntity = CBaseEntity::Instance( pKiller );
	if ( !pEntity || !pEntity->IsPlayer() || pVictim->IsPlayer() )
		return;

	CBasePlayer* pPlayer = (CBasePlayer*)pEntity;
	float flFrags = pKiller->frags;
	int points = IPointsForMonsterKill( pPlayer, pVictim );
	pKiller->frags = flFrags + points;

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
		WRITE_SHORT( (int)pPlayer->pev->frags );
		WRITE_SHORT( pPlayer->m_iDeaths );
	MESSAGE_END();
}

BOOL CHalfLifeCoopplay::IsTeamplay()
{
	return TRUE;
}

int CHalfLifeCoopplay::PlayerRelationship( CBaseEntity*, CBaseEntity* )
{
	return GR_TEAMMATE;
}

BOOL CHalfLifeCoopplay::ShouldAutoAim( CBasePlayer* pPlayer, edict_t* target )
{
	CBaseEntity* pEntity = CBaseEntity::Instance( target );
	if ( pEntity && pEntity->IsPlayer() )
		return PlayerRelationship( pPlayer, pEntity ) != GR_TEAMMATE;
	return TRUE;
}

int CHalfLifeCoopplay::IPointsForKill( CBasePlayer* pAttacker, CBasePlayer* pKilled )
{
	if ( !pKilled )
		return 0;
	if ( pAttacker && pAttacker != pKilled )
		return PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE ? -1 : 1;
	return 1;
}

int CHalfLifeCoopplay::IPointsForMonsterKill( CBasePlayer*, CBaseMonster* pKilled )
{
	return pKilled != NULL;
}

float CHalfLifeCoopplay::FlPlayerFallDamage( CBasePlayer* pPlayer )
{
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return (float)(pPlayer->m_flFallVelocity * 100.0 / (PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED));
}

BOOL CHalfLifeCoopplay::FPlayerCanRespawn( CBasePlayer* )
{
	return TRUE;
}

float CHalfLifeCoopplay::FlPlayerSpawnTime( CBasePlayer* )
{
	return gpGlobals->time;
}

float CHalfLifeCoopplay::FlHealthChargerRechargeTime()
{
	if ( coopweprespawn.value <= 0 )
		return 0;
	return 60;
}

int CHalfLifeCoopplay::DeadPlayerWeapons( CBasePlayer* )
{
	return GR_PLR_DROP_GUN_NO;
}

BOOL CHalfLifeCoopplay::FAllowMonsters()
{
	return TRUE;
}

void CHalfLifeCoopplay::PlayerKilled( CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor )
{
	if ( !m_DisableDeathPenalty )
		CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );
}

void CHalfLifeCoopplay::Think()
{
	if ( g_fGameOver )
		CHalfLifeMultiplay::Think();
}

int CHalfLifeCoopplay::WeaponShouldRespawn( CBasePlayerItem* pWeapon )
{
	if ( coopweprespawn.value > 0 && !(pWeapon->pev->spawnflags & SF_NORESPAWN) )
		return GR_WEAPON_RESPAWN_YES;
	return GR_WEAPON_RESPAWN_NO;
}

int CHalfLifeCoopplay::ItemShouldRespawn( CItem* pItem )
{
	if ( coopweprespawn.value <= 0 || (pItem->pev->spawnflags & SF_NORESPAWN) )
		return GR_ITEM_RESPAWN_NO;
	return GR_ITEM_RESPAWN_YES;
}

float CHalfLifeCoopplay::FlItemRespawnTime( CItem* )
{
	if ( coopweprespawn.value <= 0 )
		return -1;
	return gpGlobals->time + 30;
}

int CHalfLifeCoopplay::AmmoShouldRespawn( CBasePlayerAmmo* pAmmo )
{
	if ( coopweprespawn.value <= 0 || (pAmmo->pev->spawnflags & SF_NORESPAWN) )
		return GR_AMMO_RESPAWN_NO;
	return GR_AMMO_RESPAWN_YES;
}

float CHalfLifeCoopplay::FlAmmoRespawnTime( CBasePlayerAmmo* )
{
	if ( coopweprespawn.value <= 0 )
		return -1;
	return gpGlobals->time + 20;
}

BOOL CHalfLifeCoopplay::FPlayerCanTakeDamage( CBasePlayer* pPlayer, CBaseEntity* pAttacker )
{
	return (friendlyfire.value != 0 || !pAttacker->IsPlayer() || pAttacker == pPlayer)
		&& CHalfLifeMultiplay::FPlayerCanTakeDamage( pPlayer, pAttacker );
}

BOOL CHalfLifeCoopplay::ClientCommand( CBasePlayer*, const char* pcmd )
{
	if ( strcmp( pcmd, "menuselect" ) )
		return FALSE;
	if ( CMD_ARGC() > 1 )
		(void)atoi( CMD_ARGV( 1 ) );
	return TRUE;
}

float CHalfLifeCoopplay::FlWeaponTryRespawn( CBasePlayerItem* pWeapon )
{
	if ( coopweprespawn.value > 0 && pWeapon && pWeapon->m_iId
		&& (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD)
		&& NUMBER_OF_ENTITIES() >= gpGlobals->maxEntities - ENTITY_INTOLERANCE )
		return FlWeaponRespawnTime( pWeapon );
	return 0;
}

float CHalfLifeCoopplay::FlWeaponRespawnTime( CBasePlayerItem* pWeapon )
{
	if ( coopweprespawn.value <= 0 )
		return -1;
	if ( weaponstay.value <= 0 || (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) )
		return gpGlobals->time + 20;
	return gpGlobals->time + 0;
}

const char* CHalfLifeCoopplay::GetGameDescription()
{
	return "HL Coopplay";
}
