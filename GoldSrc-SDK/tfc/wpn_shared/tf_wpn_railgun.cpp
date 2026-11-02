#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( tf_weapon_railgun, CTFRailgun );

void CTFRailgun::Spawn()
{
	pev->classname = MAKE_STRING( "tf_weapon_railgun" );
	Precache();
	m_iId = 2;
	SET_MODEL( ENT( pev ), "models/w_gauss.mdl" );
	m_iDefaultAmmo = 17;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

void CTFRailgun::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_railgun.mdl" );
	PRECACHE_MODEL( "models/w_gauss.mdl" );
	PRECACHE_MODEL( "models/p_9mmhandgun.mdl" );
	PRECACHE_MODEL( "models/p_9mmhandgun2.mdl" );
	PRECACHE_SOUND( "weapons/railgun.wav" );
	m_usFireRail = PRECACHE_EVENT( 1, "events/wpn/tf_rail.sc" );
}

int CTFRailgun::GetItemInfo( ItemInfo* info )
{
	info->pszName = STRING( pev->classname );
	info->pszAmmo1 = "9mm";
	info->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_nails : 50;
	info->pszAmmo2 = NULL;
	info->iAmmo2 = -1;
	info->iMaxClip = -1;
	info->iSlot = 1;
	info->iPosition = 2;
	info->iFlags = 0;
	info->iId = m_iId = 21;
	info->iWeight = 10;
	return 1;
}

BOOL CTFRailgun::Deploy()
{
	return DefaultDeploy( "models/v_tfc_railgun.mdl", "models/p_9mmhandgun.mdl", 2, "onehanded", 1 );
}

void CTFRailgun::PrimaryAttack()
{
	if ( m_pPlayer->ammo_nails <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.2f );
		return;
	}
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireRail, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector origin = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs +
		gpGlobals->v_right * 2 + gpGlobals->v_up * -4;
	CTFNailgunNail::CreateRailgunNail( origin, m_pPlayer->pev->v_angle, m_pPlayer, this );
	DB_LogShots( 1 );
	m_pPlayer->ammo_nails--;
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.4f );
	m_flTimeWeaponIdle = 12.5f;
}

void CTFRailgun::Reload()
{
}

void CTFRailgun::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= 0 )
	{
		m_flTimeWeaponIdle = 12.5f;
		SendWeaponAnim( 0, 1 );
	}
}
