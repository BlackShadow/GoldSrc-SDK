#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "../../cl_dll/dod_shared.h"

class CGreaseGunAmmoClip : public CBasePlayerAmmo
{
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( weapon_greasegun, CGreaseGun );
LINK_ENTITY_TO_CLASS( ammo_greasegun, CGreaseGunAmmoClip );

void CGreaseGun::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[22].wmodel );
	m_iId = 22;
	m_iDefaultAmmo = P_WpnInfo[22].ammo_default;
	FallInit();
}

void CGreaseGun::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[22].vmodel );
	PRECACHE_MODEL( P_WpnInfo[22].wmodel );
	PRECACHE_SOUND( "weapons/greasegun_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	m_usFireGreaseGun = PRECACHE_EVENT( 1, "events/weapons/greasegun.sc" );
}

int CGreaseGun::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_44mm";
	p->iMaxAmmo1 = P_WpnInfo[22].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[22].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = 22;
	p->iWeight = P_WpnInfo[22].misc_weight;
	return 1;
}

BOOL CGreaseGun::CanHolster()
{
	return TRUE;
}

int CGreaseGun::Classify()
{
	return 64;
}

BOOL CGreaseGun::SpawnDeploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	return TimedDeploy( P_WpnInfo[22].vmodel, P_WpnInfo[22].pmodel, 2,
		P_WpnInfo[22].szAnimExt, P_WpnInfo[22].szAnimReloadExt, 1.6f, 1.6f, UseDecrement() != 0 );
}

BOOL CGreaseGun::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
	return DefaultDeploy( P_WpnInfo[22].vmodel, P_WpnInfo[22].pmodel, 6,
		P_WpnInfo[22].szAnimExt, P_WpnInfo[22].szAnimReloadExt, 0 );
}

int CGreaseGun::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

void CGreaseGun::Reload()
{
	DefaultReload( P_WpnInfo[22].ammo_maxclip, 1, P_WpnInfo[22].anim_reloadtime );
}

void CGreaseGun::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( m_iClip <= 0 ? 5 : 0, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

void CGreaseGun::PrimaryAttack()
{
	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}
	if ( m_iClip <= 0 )
	{
		if ( !m_fInAttack )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
			m_fInAttack = 1;
		}
		return;
	}
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_iClip--;
	float flSpread = P_WpnInfo[m_iId].base_accuracy;
	if ( m_pPlayer->pev->velocity.Length() > 45 )
		flSpread += P_WpnInfo[m_iId].accuracy_penalty;
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecNewShot = m_pPlayer->FireBulletsNC( vecSrc, vecAiming, flSpread, 8192,
		BULLET_PLAYER_GREASEGUN, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireGreaseGun, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[22].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	RemoveStamina( 2, m_pPlayer );
}

void CGreaseGunAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CGreaseGunAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[22].ammo_maxclip, "ammo_44mm", P_WpnInfo[22].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
