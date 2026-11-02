#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "tf_defs.h"

LINK_ENTITY_TO_CLASS( tf_flame, CTFFlame );

void CTFFlame::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	pev->classname = MAKE_STRING( "tf_fire" );
	deathtype = MAKE_STRING( "flames" );
	pev->health = gpGlobals->time + 5.0;
	pev->effects |= EF_DIMLIGHT | EF_NODRAW;
	pev->nextthink = gpGlobals->time + 1;
	SetThink( &CTFFlame::FlameThink );
	m_flNextDamageTime = pev->nextthink;
}

void CTFFlame::Precache()
{
	PRECACHE_SOUND( "ambience/fire1.wav" );
}

void CTFFlame::FlameThink()
{
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	CBaseEntity* pTarget = CBaseEntity::Instance( pev->enemy );
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pTarget->pev->origin );
	WRITE_BYTE( TE_PLAYERATTACHMENT );
	WRITE_SHORT( pTarget->entindex() );
	WRITE_SHORT( g_sModelIndexPlayerFlame );
	WRITE_BYTE( 5 );
	WRITE_BYTE( 50 );
	MESSAGE_END();
	if ( pTarget->numflames == 0 )
	{
		FlameDestroy();
		return;
	}
	if ( pTarget->pev->health < 1 )
	{
		::RadiusDamage( pev->origin, pev, pOwner ? pOwner->pev : pev, 10, 10, CLASS_NONE,
			DMG_BURN | DMG_RADIUS_QUAKE );
		pTarget->numflames = 0;
		FlameDestroy();
		return;
	}
	if ( ( pTarget->armorclass & AT_SAVEFIRE ) && pTarget->pev->armorvalue > 0 )
	{
		pTarget->numflames--;
		FlameDestroy();
		return;
	}
	if ( pTarget->tfstate & TFSTATE_RESET_FLAMETIME )
	{
		pev->health = gpGlobals->time + 5.0;
		pTarget->tfstate &= ~TFSTATE_RESET_FLAMETIME;
	}
	if ( gpGlobals->time < m_flNextDamageTime )
	{
		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}
	if ( !( gpGlobals->time <= pev->health ) )
	{
		pTarget->numflames = 0;
		FlameDestroy();
		return;
	}
	if ( pTarget->IsAlive() )
	{
		pTarget->TakeDamage( pev, VARS( pev->owner ), pTarget->numflames * 2, DMG_BURN );
		if ( !pTarget->IsAlive() )
		{
			char* targetTeam = pTarget->team_no ? GetTeamName( pTarget->team_no ) : "SPECTATOR";
			const char* targetAuth = GETPLAYERAUTHID( pTarget->edict() );
			int targetUser = GETPLAYERUSERID( pTarget->edict() );
			const char* targetName = STRING( pTarget->pev->netname );
			char* ownerTeam = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
			const char* ownerAuth = GETPLAYERAUTHID( pOwner->edict() );
			int ownerUser = GETPLAYERUSERID( pOwner->edict() );
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
				STRING( pOwner->pev->netname ), ownerUser, ownerAuth, ownerTeam,
				targetName, targetUser, targetAuth, targetTeam, "flames" );
		}
	}
	m_flNextDamageTime = gpGlobals->time + 1;
	pev->nextthink = gpGlobals->time + 0.2;
}

CTFFlame* CTFFlame::FlameSpawn( CBaseEntity* pOwner, CBaseEntity* pTarget )
{
	CTFFlame* pFlame = GetClassPtr( (CTFFlame*)NULL );
	UTIL_SetOrigin( pFlame->pev, pTarget->pev->origin );
	pFlame->Spawn();
	pFlame->pev->enemy = pTarget->edict();
	pFlame->pev->owner = pOwner->edict();
	EMIT_SOUND_DYN( pFlame->edict(), CHAN_VOICE, "ambience/fire1.wav", 1.0, 0.5, 0, PITCH_NORM );
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pTarget->pev->origin );
	WRITE_BYTE( TE_PLAYERATTACHMENT );
	WRITE_SHORT( pTarget->entindex() );
	WRITE_SHORT( g_sModelIndexPlayerFlame );
	WRITE_BYTE( 5 );
	WRITE_BYTE( 25 );
	MESSAGE_END();
	pFlame->pev->movetype = MOVETYPE_FOLLOW;
	pFlame->pev->aiment = pTarget->edict();
	return pFlame;
}

void CTFFlame::FlameDestroy()
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "ambience/fire1.wav", 0, 0, SND_STOP, PITCH_NORM );
	num_world_flames--;
	SUB_Remove();
}
