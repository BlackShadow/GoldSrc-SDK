#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "pipewrench.h"

#ifdef CLIENT_DLL
extern int giOldWeapons;
#else
extern cvar_t oldweapons;
#endif

void FindHullIntersection( const Vector& source, TraceResult& trace, float* mins, float* maxs, edict_t* entity );

LINK_ENTITY_TO_CLASS( weapon_pipewrench, CPipewrench );

enum pipewrench_e
{
	PIPEWRENCH_IDLE1,
	PIPEWRENCH_IDLE2,
	PIPEWRENCH_IDLE3,
	PIPEWRENCH_DRAW,
	PIPEWRENCH_HOLSTER,
	PIPEWRENCH_ATTACK1HIT,
	PIPEWRENCH_ATTACK1MISS,
	PIPEWRENCH_ATTACK2HIT,
	PIPEWRENCH_ATTACK2MISS,
	PIPEWRENCH_ATTACK3HIT,
	PIPEWRENCH_ATTACK3MISS,
	PIPEWRENCH_BIGWINDUP,
	PIPEWRENCH_BIGHIT,
	PIPEWRENCH_BIGMISS
};

#ifndef CLIENT_DLL
TYPEDESCRIPTION CPipewrench::m_SaveData[] =
{
	DEFINE_FIELD( CPipewrench, m_flBigSwingStart, FIELD_TIME ),
	DEFINE_FIELD( CPipewrench, m_iSwing, FIELD_INTEGER ),
	DEFINE_FIELD( CPipewrench, m_iSwingMode, FIELD_INTEGER ),
	DEFINE_FIELD( CPipewrench, m_iBigSwingHit, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( CPipewrench, CBasePlayerWeapon );
#endif

void CPipewrench::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_pipewrench" );
	Precache();
	m_iId = WEAPON_PIPEWRENCH;
	SET_MODEL( ENT( pev ), "models/w_pipe_wrench.mdl" );
	m_iClip = WEAPON_NOCLIP;
	m_iSwingMode = 0;
	FallInit();
}

void CPipewrench::Precache()
{
	PRECACHE_MODEL( "models/v_pipe_wrench.mdl" );
	PRECACHE_MODEL( "models/w_pipe_wrench.mdl" );
	PRECACHE_MODEL( "models/p_pipe_wrench.mdl" );
	PRECACHE_SOUND( "weapons/pwrench_hit1.wav" );
	PRECACHE_SOUND( "weapons/pwrench_hit2.wav" );
	PRECACHE_SOUND( "weapons/pwrench_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/pwrench_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/pwrench_hitbod3.wav" );
	PRECACHE_SOUND( "weapons/pwrench_miss1.wav" );
	PRECACHE_SOUND( "weapons/pwrench_miss2.wav" );
	PRECACHE_SOUND( "weapons/pwrench_big_hit1.wav" );
	PRECACHE_SOUND( "weapons/pwrench_big_hit2.wav" );
	PRECACHE_SOUND( "weapons/pwrench_big_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/pwrench_big_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/pwrench_big_miss.wav" );
	m_usPipeWrench = PRECACHE_EVENT( 1, "events/pipewrench.sc" );
}

int CPipewrench::GetItemInfo( ItemInfo* info )
{
	info->pszName = STRING( pev->classname );
	info->pszAmmo1 = NULL;
	info->iMaxAmmo1 = -1;
	info->pszAmmo2 = NULL;
	info->iMaxAmmo2 = -1;
	info->iMaxClip = WEAPON_NOCLIP;
	info->iSlot = 0;
	info->iPosition = 1;
	info->iId = m_iId = WEAPON_PIPEWRENCH;
	info->iWeight = 2;
	return 1;
}

BOOL CPipewrench::Deploy()
{
	return DefaultDeploy( "models/v_pipe_wrench.mdl", "models/p_pipe_wrench.mdl", PIPEWRENCH_DRAW, "crowbar" );
}

void CPipewrench::Holster( int )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( PIPEWRENCH_HOLSTER );
}

BOOL CPipewrench::UseDecrement()
{
#ifdef CLIENT_DLL
	return giOldWeapons != 1;
#else
	return oldweapons.value != 1;
#endif
}

void CPipewrench::PrimaryAttack()
{
	if ( m_iSwingMode == 0 && !Swing( 1 ) && UseDecrement() )
	{
		SetThink( &CPipewrench::SwingAgain );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CPipewrench::SecondaryAttack()
{
	if ( m_iSwingMode != 1 )
	{
		SendWeaponAnim( PIPEWRENCH_BIGWINDUP );
		m_flBigSwingStart = gpGlobals->time;
	}
	m_iSwingMode = 1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2f;
}

void CPipewrench::SwingAgain()
{
	Swing( 0 );
}

void CPipewrench::Smack()
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

int CPipewrench::Swing( int first )
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
	PLAYBACK_EVENT_FULL( UseDecrement() ? FEV_NOTHOST : 0, m_pPlayer->edict(), m_usPipeWrench, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, trace.flFraction < 1 );
	const char* miss = UTIL_SharedRandomLong( m_pPlayer->random_seed, 0, 1 ) ?
		"weapons/pwrench_miss2.wav" : "weapons/pwrench_miss1.wav";
	EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, miss, 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 15 ) );
	if ( trace.flFraction >= 1 )
	{
		if ( first )
		{
			switch ( m_iSwing++ % 3 )
			{
			case 0: SendWeaponAnim( PIPEWRENCH_ATTACK1MISS ); break;
			case 1: SendWeaponAnim( PIPEWRENCH_ATTACK2MISS ); break;
			case 2: SendWeaponAnim( PIPEWRENCH_ATTACK3MISS ); break;
			}
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.7;
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.2;
		return FALSE;
	}

	CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
	switch ( m_iSwing++ % 2 + 1 )
	{
	case 1: SendWeaponAnim( PIPEWRENCH_ATTACK2HIT ); break;
	case 2: SendWeaponAnim( PIPEWRENCH_ATTACK3HIT ); break;
	default: SendWeaponAnim( PIPEWRENCH_ATTACK1HIT ); break;
	}
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
#ifndef CLIENT_DLL
	ClearMultiDamage();
	float damage = gSkillData.plrDmgPipewrench;
	float previousAttack = m_flNextPrimaryAttack + 1;
	if ( !( UTIL_WeaponTimeBase() > previousAttack ) && !g_pGameRules->IsMultiplayer() )
		damage *= 0.5;
	hit->TraceAttack( m_pPlayer->pev, damage, gpGlobals->v_forward, &trace, DMG_CLUB );
	ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
	float volume = 1;
	if ( hit && hit->Classify() != CLASS_NONE && hit->Classify() != CLASS_MACHINE )
	{
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0: EMIT_SOUND( m_pPlayer->edict(), CHAN_ITEM, "weapons/pwrench_hitbod1.wav", 1, ATTN_NORM ); break;
		case 1: EMIT_SOUND( m_pPlayer->edict(), CHAN_ITEM, "weapons/pwrench_hitbod2.wav", 1, ATTN_NORM ); break;
		case 2: EMIT_SOUND( m_pPlayer->edict(), CHAN_ITEM, "weapons/pwrench_hitbod3.wav", 1, ATTN_NORM ); break;
		}
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
		const char* sound = RANDOM_LONG( 0, 1 ) ? "weapons/pwrench_hit2.wav" : "weapons/pwrench_hit1.wav";
		EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_ITEM, sound, soundVolume, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
		m_trHit = trace;
	}
	m_pPlayer->m_iWeaponVolume = (int)( volume * 512 );
#else
	(void)hit;
#endif
	SetThink( &CPipewrench::Smack );
	pev->nextthink = gpGlobals->time + 0.275;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.2;
	return TRUE;
}

void CPipewrench::BigSwing()
{
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector source = m_pPlayer->GetGunPosition();
	Vector end = source + gpGlobals->v_forward * 32;
	TraceResult trace;
	UTIL_TraceLine( source, end, dont_ignore_monsters, m_pPlayer->edict(), &trace );
	if ( trace.flFraction >= 1 )
	{
#ifndef CLIENT_DLL
		UTIL_TraceHull( source, end, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &trace );
		if ( trace.flFraction < 1 )
		{
			CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
			if ( !hit || hit->IsBSPModel() )
				FindHullIntersection( source, trace, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			end = trace.vecEndPos;
		}
#endif
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}
	PLAYBACK_EVENT_FULL( UseDecrement() ? FEV_NOTHOST : 0, m_pPlayer->edict(), m_usPipeWrench, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 1, trace.flFraction < 1 );
	EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, "weapons/pwrench_big_miss.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 15 ) );
	if ( trace.flFraction >= 1 )
	{
		SendWeaponAnim( PIPEWRENCH_BIGMISS );
		return;
	}
	m_pPlayer->pev->punchangle.x += 2;
	CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	SendWeaponAnim( PIPEWRENCH_BIGHIT );
#ifndef CLIENT_DLL
	float damage = ( gpGlobals->time - m_flBigSwingStart ) * gSkillData.plrDmgPipewrench + 25;
	ClearMultiDamage();
	if ( damage > 150 )
		damage = 150;
	hit->TraceAttack( m_pPlayer->pev, damage, gpGlobals->v_forward, &trace, DMG_CLUB );
	ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
	if ( hit->Classify() == CLASS_NONE || hit->Classify() == CLASS_MACHINE )
	{
		float volume = TEXTURETYPE_PlaySound( &trace, source, source + ( end - source ) * 2, BULLET_PLAYER_CROWBAR );
		if ( g_pGameRules->IsMultiplayer() )
			volume = 1;
		EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_ITEM, "weapons/pwrench_hit2.wav", volume, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
	}
	else
	{
		EMIT_SOUND( m_pPlayer->edict(), CHAN_ITEM, "weapons/pwrench_hitbod1.wav", 1, ATTN_NORM );
		m_pPlayer->m_iWeaponVolume = 128;
		hit->IsAlive();
	}
#else
	(void)hit;
#endif
}

void CPipewrench::WeaponIdle()
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	if ( m_iSwingMode == 1 )
	{
		if ( gpGlobals->time > m_flBigSwingStart + 1 )
		{
			m_iSwingMode = 2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.2;
			SetThink( &CPipewrench::BigSwing );
			pev->nextthink = gpGlobals->time + 0.1;
		}
		return;
	}
	m_iSwingMode = 0;
	float random = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	int animation;
	if ( random <= 0.5 )
	{
		animation = PIPEWRENCH_IDLE3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	}
	else if ( random <= 0.9 )
	{
		animation = PIPEWRENCH_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
	}
	else
	{
		animation = PIPEWRENCH_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	}
	SendWeaponAnim( animation );
}
