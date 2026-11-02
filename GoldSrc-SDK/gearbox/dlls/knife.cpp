#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "knife.h"

#ifdef CLIENT_DLL
extern int giOldWeapons;
#else
extern cvar_t oldweapons;
#endif

void FindHullIntersection( const Vector& source, TraceResult& trace, float* mins, float* maxs, edict_t* entity );

LINK_ENTITY_TO_CLASS( weapon_knife, CKnife );

enum knife_e
{
	KNIFE_IDLE,
	KNIFE_DRAW,
	KNIFE_HOLSTER,
	KNIFE_ATTACK1HIT,
	KNIFE_ATTACK1MISS,
	KNIFE_ATTACK2MISS,
	KNIFE_ATTACK2HIT,
	KNIFE_ATTACK3MISS,
	KNIFE_ATTACK3HIT
};

void CKnife::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_knife" );
	Precache();
	m_iId = WEAPON_KNIFE;
	SET_MODEL( ENT( pev ), "models/w_knife.mdl" );
	m_iClip = WEAPON_NOCLIP;
	FallInit();
}

void CKnife::Precache()
{
	PRECACHE_MODEL( "models/v_knife.mdl" );
	PRECACHE_MODEL( "models/w_knife.mdl" );
	PRECACHE_MODEL( "models/p_knife.mdl" );
	PRECACHE_SOUND( "weapons/knife1.wav" );
	PRECACHE_SOUND( "weapons/knife2.wav" );
	PRECACHE_SOUND( "weapons/knife3.wav" );
	PRECACHE_SOUND( "weapons/knife_hit_flesh1.wav" );
	PRECACHE_SOUND( "weapons/knife_hit_flesh2.wav" );
	PRECACHE_SOUND( "weapons/knife_hit_wall1.wav" );
	PRECACHE_SOUND( "weapons/knife_hit_wall2.wav" );
	m_usKnife = PRECACHE_EVENT( 1, "events/knife.sc" );
}

int CKnife::GetItemInfo( ItemInfo* info )
{
	info->pszName = STRING( pev->classname );
	info->pszAmmo1 = NULL;
	info->iMaxAmmo1 = -1;
	info->pszAmmo2 = NULL;
	info->iMaxAmmo2 = -1;
	info->iMaxClip = WEAPON_NOCLIP;
	info->iSlot = 0;
	info->iPosition = 2;
	info->iId = WEAPON_KNIFE;
	info->iWeight = 0;
	return 1;
}

BOOL CKnife::Deploy()
{
	return DefaultDeploy( "models/v_knife.mdl", "models/p_knife.mdl", KNIFE_DRAW, "crowbar" );
}

void CKnife::Holster( int )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( KNIFE_HOLSTER );
}

BOOL CKnife::UseDecrement()
{
#ifdef CLIENT_DLL
	return giOldWeapons != 1;
#else
	return oldweapons.value != 1;
#endif
}

void CKnife::PrimaryAttack()
{
	if ( !Swing( 1 ) && UseDecrement() )
	{
		SetThink( &CKnife::SwingAgain );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CKnife::SwingAgain()
{
	Swing( 0 );
}

void CKnife::Smack()
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

int CKnife::Swing( int first )
{
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector source = m_pPlayer->GetGunPosition();
	Vector end = source + gpGlobals->v_forward * 32;
	TraceResult trace;
	UTIL_TraceLine( source, end, dont_ignore_monsters, m_pPlayer->edict(), &trace );
#ifndef CLIENT_DLL
	if ( trace.flFraction >= 1 )
	{
		UTIL_TraceHull( source, end, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &trace );
		if ( trace.flFraction < 1 )
		{
			CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
			if ( !hit || hit->IsBSPModel() )
				FindHullIntersection( source, trace, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			end = trace.vecEndPos;
		}
	}
#endif
	PLAYBACK_EVENT_FULL( UseDecrement() ? FEV_NOTHOST : 0, m_pPlayer->edict(), m_usKnife, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	if ( trace.flFraction >= 1 )
	{
		if ( first )
		{
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
		return FALSE;
	}

	CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
	switch ( m_iSwing++ % 2 + 1 )
	{
	case 1: SendWeaponAnim( KNIFE_ATTACK2HIT ); break;
	case 2: SendWeaponAnim( KNIFE_ATTACK3HIT ); break;
	default: SendWeaponAnim( KNIFE_ATTACK1HIT ); break;
	}
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
#ifndef CLIENT_DLL
	float damage = gSkillData.plrDmgKnife;
	int damageType = DMG_CLUB;
	if ( g_pGameRules->IsMultiplayer() )
	{
		UTIL_MakeVectors( hit->pev->v_angle );
		Vector right = gpGlobals->v_right;
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		if ( right.x * gpGlobals->v_forward.y - right.y * gpGlobals->v_forward.x > 0 )
		{
			damage = 100 * gSkillData.plrDmgKnife;
			damageType = DMG_CLUB | DMG_NEVERGIB;
		}
	}
	ClearMultiDamage();
	hit->TraceAttack( m_pPlayer->pev, damage, gpGlobals->v_forward, &trace, damageType );
	ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
	float volume = 1;
	if ( hit && hit->Classify() != CLASS_NONE && hit->Classify() != CLASS_MACHINE )
	{
		EMIT_SOUND( m_pPlayer->edict(), CHAN_ITEM,
			RANDOM_LONG( 0, 1 ) ? "weapons/knife_hit_flesh2.wav" : "weapons/knife_hit_flesh1.wav", 1, ATTN_NORM );
		m_pPlayer->m_iWeaponVolume = 128;
		if ( !hit->IsAlive() )
			return TRUE;
		volume = 0.1f;
	}
	else
	{
		float soundVolume = TEXTURETYPE_PlaySound( &trace, source, source + ( end - source ) * 2, BULLET_PLAYER_CROWBAR );
		if ( g_pGameRules->IsMultiplayer() )
			soundVolume = 1;
		const char* sound = RANDOM_LONG( 0, 1 ) ? "weapons/knife_hit_wall2.wav" : "weapons/knife_hit_wall1.wav";
		EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_ITEM, sound, soundVolume, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
		m_trHit = trace;
	}
	m_pPlayer->m_iWeaponVolume = (int)( volume * 512 );
#else
	(void)hit;
#endif
	SetThink( &CKnife::Smack );
	pev->nextthink = gpGlobals->time + 0.2;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25;
	return TRUE;
}
