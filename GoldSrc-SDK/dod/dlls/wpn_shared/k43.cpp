#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_k43, CK43 );

void CK43::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_K43].wmodel );
	m_iId = WEAPON_K43;
	m_iDefaultAmmo = P_WpnInfo[WEAPON_K43].ammo_default;
	FallInit();
}

void CK43::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_K43].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_K43].wmodel );
	PRECACHE_SOUND( "weapons/k43_shoot.wav" );
	m_usFireK43 = PRECACHE_EVENT( 1, "events/weapons/k43.sc" );
}

int CK43::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_66mm";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_K43].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_K43].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = WEAPON_K43;
	p->iWeight = P_WpnInfo[WEAPON_K43].misc_weight;
	return TRUE;
}

int CK43::Classify()
{
	return 16;
}

BOOL CK43::CanHolster()
{
	return TRUE;
}

int CK43::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_K43].wmodel );
		return TRUE;
	}

	return FALSE;
}

BOOL CK43::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	return TimedDeploy( P_WpnInfo[WEAPON_K43].vmodel, P_WpnInfo[WEAPON_K43].pmodel, 4,
		P_WpnInfo[WEAPON_K43].szAnimExt, P_WpnInfo[WEAPON_K43].szAnimReloadExt, 0.79f, 0.79f, UseDecrement() != FALSE );
}

void CK43::Reload()
{
	DefaultReload( P_WpnInfo[WEAPON_K43].ammo_maxclip, 3, P_WpnInfo[WEAPON_K43].anim_reloadtime );
}

void CK43::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( 0, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

void CK43::SecondaryAttack()
{
	RifleMeleeAttack( m_pPlayer, WEAPON_K43 );
	m_pPlayer->SetAnimation( PLAYER_BUT_SWING );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireK43, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 1, 0, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.1;
}

void CK43::PrimaryAttack()
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
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_iClip--;
	float flSpread = P_WpnInfo[m_iId].base_accuracy;
	if ( m_pPlayer->pev->velocity.Length() > 45 )
		flSpread = flSpread + 0.1;
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecNewShot = m_pPlayer->FireBulletsNC( vecSrc, vecAiming, flSpread, 8192,
		BULLET_PLAYER_K43, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireK43, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, vecNewShot.x, vecNewShot.y,
		(int)m_pPlayer->pev->punchangle.x, 0, m_iClip, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_K43].anim_firedelay;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_K43].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	m_fInAttack = TRUE;
	RemoveStamina( 2, m_pPlayer );
}

class CK43AmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( ammo_k43, CK43AmmoClip );

void CK43AmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CK43AmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_K43].ammo_maxclip, "ammo_66mm", P_WpnInfo[WEAPON_K43].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
