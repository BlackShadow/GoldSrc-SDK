#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "../../cl_dll/dod_shared.h"

class CSTENAmmoClip : public CBasePlayerAmmo
{
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( weapon_sten, CSTEN );
LINK_ENTITY_TO_CLASS( ammo_sten, CSTENAmmoClip );

void CSTEN::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[26].wmodel );
	m_iId = 26;
	m_iDefaultAmmo = P_WpnInfo[26].ammo_default;
	FallInit();
}

void CSTEN::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[26].vmodel );
	PRECACHE_MODEL( P_WpnInfo[26].wmodel );
	PRECACHE_SOUND( "weapons/reloadgarand.wav" );
	PRECACHE_SOUND( "weapons/sten_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/tommy_draw_slideback.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipout.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );
	m_usFireSten = PRECACHE_EVENT( 1, "events/weapons/sten.sc" );
}

int CSTEN::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_44mm";
	p->iMaxAmmo1 = P_WpnInfo[26].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[26].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = 26;
	p->iWeight = P_WpnInfo[26].misc_weight;
	return 1;
}

BOOL CSTEN::CanHolster()
{
	return TRUE;
}

int CSTEN::Classify()
{
	return 64;
}

BOOL CSTEN::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
	return DefaultDeploy( P_WpnInfo[26].vmodel, P_WpnInfo[26].pmodel, 2,
		P_WpnInfo[26].szAnimExt, P_WpnInfo[26].szAnimReloadExt, 0 );
}

int CSTEN::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

void CSTEN::SecondaryAttack()
{
}

void CSTEN::Reload()
{
	DefaultReload( P_WpnInfo[26].ammo_maxclip, 1, P_WpnInfo[26].anim_reloadtime );
}

void CSTEN::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		int iAnim = 0;

		SendWeaponAnim( iAnim, UseDecrement() != FALSE );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

void CSTEN::PrimaryAttack()
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
		BULLET_PLAYER_STEN, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireSten, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[26].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	RemoveStamina( 2, m_pPlayer );
}

void CSTENAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CSTENAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[26].ammo_maxclip, "ammo_44mm", P_WpnInfo[26].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
