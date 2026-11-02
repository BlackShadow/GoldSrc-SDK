#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

LINK_ENTITY_TO_CLASS( tf_weapon_autorifle, CTFAutoRifle );

void CTFAutoRifle::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_sniper.mdl" );
	PRECACHE_MODEL( "models/p_sniper.mdl" );
	PRECACHE_MODEL( "models/p_sniper2.mdl" );
	PRECACHE_SOUND( "weapons/sniper.wav" );
	m_usFireAutoRifle = PRECACHE_EVENT( 1, "events/wpn/tf_ar.sc" );
}

void CTFAutoRifle::Spawn()
{
	pev->classname = MAKE_STRING( "tf_weapon_autorifle" );
	Precache();
	m_iId = 6;
	m_iDefaultAmmo = 5;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

int CTFAutoRifle::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "buckshot";
	p->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_shells : 75;
	p->pszAmmo2 = NULL;
	p->iAmmo2 = -1;
	p->iMaxClip = -1;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iId = m_iId = 7;
	p->iFlags = 0;
	p->iWeight = 10;
	return 1;
}

int CTFAutoRifle::AddToPlayer( CBasePlayer* player )
{
	if ( !CBasePlayerWeapon::AddToPlayer( player ) )
		return FALSE;
	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, player->pev );
	WRITE_BYTE( m_iId );
	MESSAGE_END();
	return TRUE;
}

BOOL CTFAutoRifle::Deploy()
{
	return DefaultDeploy( "models/v_tfc_sniper.mdl", "models/p_sniper.mdl", 7, "autosniper", 1 );
}

void CTFAutoRifle::Holster( int skiplocal )
{
	m_fInReload = 0;
	m_pPlayer->m_flNextAttack = 0.5;
	SendWeaponAnim( 8, 1 );
}

void CTFAutoRifle::Reload()
{
}

void CTFAutoRifle::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle < 0 )
	{
		SendWeaponAnim( 5, 1 );
		m_flTimeWeaponIdle = 12.5;
	}
}

void CTFAutoRifle::PrimaryAttack()
{
	if ( m_pPlayer->ammo_shells <= 0 )
	{
		m_flNextPrimaryAttack = GetNextAttackDelay( 5 );
		m_flTimeWeaponIdle = 1;
		SendWeaponAnim( 0, 1 );
		PlayEmptySound();
		return;
	}
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireAutoRifle, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_flTimeWeaponIdle = 0.2;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir = gpGlobals->v_forward;
	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecDir * 8192, dont_ignore_monsters, m_pPlayer->edict(), &tr );
	DB_LogShots( 1 );
	if ( tr.flFraction != 1 )
	{
		CBaseEntity* hit = CBaseEntity::Instance( tr.pHit );
		if ( hit )
		{
			ClearMultiDamage();
			hit->TraceAttack( m_pPlayer->pev, 8, vecDir, &tr, DMG_BULLET );
			ApplyMultiDamage( pev, m_pPlayer->pev );
		}
	}
	m_pPlayer->ammo_shells--;
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.1 );
}
