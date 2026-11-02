#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

void CPistol::Spawn( int weapon_id )
{
	Precache();
	m_iId = weapon_id;
	SET_MODEL( ENT( pev ), P_WpnInfo[m_iId].wmodel );
	m_iDefaultAmmo = P_WpnInfo[m_iId].ammo_default;
	FallInit();
}

BOOL CPistol::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_pPlayer->CheckPlayerSpeed();

	return TimedDeploy( P_WpnInfo[m_iId].vmodel, P_WpnInfo[m_iId].pmodel, GetDrawAnim(), P_WpnInfo[m_iId].szAnimExt,
		P_WpnInfo[m_iId].szAnimReloadExt, P_WpnInfo[m_iId].anim_drawtime, P_WpnInfo[m_iId].anim_drawtime, UseDecrement() != FALSE );
}

BOOL CPistol::CanHolster()
{
	return TRUE;
}

void CPistol::Reload()
{
	DefaultReload( P_WpnInfo[m_iId].ammo_maxclip, GetReloadAnim(), P_WpnInfo[m_iId].anim_reloadtime );
}

void CPistol::PrimaryAttack()
{
	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if ( m_fInAttack )
		return;

	if ( m_iClip <= 0 )
	{
		if ( !m_fInAttack )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
			m_fInAttack = TRUE;
		}
		return;
	}

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_iClip--;

	ItemInfo pistol;
	GetItemInfo( &pistol );

	float flSpread = P_WpnInfo[m_iId].base_accuracy;

	if ( m_pPlayer->pev->velocity.Length() > 45 )
		flSpread += P_WpnInfo[m_iId].accuracy_penalty;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecNewShot = m_pPlayer->FireBulletsNC( vecSrc, vecAiming, flSpread, 8192, pistol.iBulletId, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_iFireEvent, 0, (float *)&g_vecZero.x, (float *)&g_vecZero.x, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_firedelay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	m_fInAttack = TRUE;

	RemoveStamina( 1, m_pPlayer );
}

void CPistol::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( GetIdleAnim(), 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6;
	}
}
