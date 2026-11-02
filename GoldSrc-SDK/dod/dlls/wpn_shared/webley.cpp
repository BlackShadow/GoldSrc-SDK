#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "../../cl_dll/dod_shared.h"

class CWebleyAmmoClip : public CBasePlayerAmmo
{
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( weapon_webley, CWEBLEY );
LINK_ENTITY_TO_CLASS( ammo_webley, CWebleyAmmoClip );

void CWEBLEY::Spawn()
{
	Precache();
	m_iId = 28;
	SET_MODEL( ENT( pev ), P_WpnInfo[28].wmodel );
	m_iDefaultAmmo = P_WpnInfo[28].ammo_default;
	FallInit();
}

void CWEBLEY::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[28].vmodel );
	PRECACHE_MODEL( P_WpnInfo[28].wmodel );
	PRECACHE_SOUND( "weapons/webley_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/colt_reload_clipin.wav" );
	PRECACHE_SOUND( "weapons/colt_reload_clipout.wav" );
	PRECACHE_SOUND( "weapons/colt_draw_pullslide.wav" );
	m_usFireWebley = PRECACHE_EVENT( 1, "events/weapons/webley.sc" );
}

int CWEBLEY::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_12mm";
	p->iMaxAmmo1 = P_WpnInfo[28].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[28].ammo_maxclip;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 64;
	p->iId = m_iId = 28;
	p->iWeight = P_WpnInfo[28].misc_weight;
	return 1;
}

BOOL CWEBLEY::CanHolster()
{
	return TRUE;
}

BOOL CWEBLEY::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_pPlayer->CheckPlayerSpeed();
	return TimedDeploy( P_WpnInfo[28].vmodel, P_WpnInfo[28].pmodel, 3,
		P_WpnInfo[28].szAnimExt, P_WpnInfo[28].szAnimReloadExt,
		P_WpnInfo[28].anim_drawtime, 1, UseDecrement() != 0 );
}

int CWEBLEY::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

void CWEBLEY::SecondaryAttack()
{
}

void CWEBLEY::Reload()
{
	int iAnim = 2;

	DefaultReload( P_WpnInfo[28].ammo_maxclip, iAnim, P_WpnInfo[28].anim_reloadtime );
}

void CWEBLEY::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		int iAnim = 0;

		SendWeaponAnim( iAnim, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
	}
}

void CWEBLEY::PrimaryAttack()
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
	float flSpread = P_WpnInfo[m_iId].base_accuracy;
	if ( m_pPlayer->pev->velocity.Length() > 45 )
		flSpread += P_WpnInfo[m_iId].accuracy_penalty;
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecNewShot = m_pPlayer->FireBulletsNC( vecSrc, vecAiming, flSpread, 8192,
		BULLET_PLAYER_WEBLEY, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireWebley, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[28].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	m_fInAttack = TRUE;
	RemoveStamina( 1, m_pPlayer );
}

void CWebleyAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CWebleyAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[28].ammo_maxclip, "ammo_12mm", P_WpnInfo[28].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
