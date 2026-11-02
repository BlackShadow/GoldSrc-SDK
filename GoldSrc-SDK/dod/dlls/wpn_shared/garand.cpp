#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "../../cl_dll/dod_shared.h"

enum
{
	GARAND_IDLE = 0,
	GARAND_RELOAD = 5,
	GARAND_DRAW,
	GARAND_IDLE_EMPTY
};

class CGarandAmmoClip : public CBasePlayerAmmo
{
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( weapon_garand, CGarand );
LINK_ENTITY_TO_CLASS( ammo_garand, CGarandAmmoClip );

void CGarand::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_GARAND].wmodel );
	m_iId = WEAPON_GARAND;
	m_iDefaultAmmo = P_WpnInfo[WEAPON_GARAND].ammo_default;

	FallInit();
}

void CGarand::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_GARAND].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_GARAND].wmodel );

	PRECACHE_SOUND( "weapons/reloadgarand.wav" );
	PRECACHE_SOUND( "weapons/garand_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/garand_reload_clipding.wav" );
	PRECACHE_SOUND( "weapons/garand_reload_clipin.wav" );
	PRECACHE_SOUND( "weapons/butt_hit.wav" );

	m_usFireGarand = PRECACHE_EVENT( 1, "events/weapons/garand.sc" );
}

int CGarand::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_16mm";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_GARAND].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_GARAND].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = WEAPON_GARAND;
	p->iWeight = P_WpnInfo[WEAPON_GARAND].misc_weight;

	return 1;
}

BOOL CGarand::CanHolster()
{
	return TRUE;
}

int CGarand::Classify()
{
	return 16;
}

BOOL CGarand::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_GARAND].anim_drawtime;

	return DefaultDeploy( P_WpnInfo[WEAPON_GARAND].vmodel, P_WpnInfo[WEAPON_GARAND].pmodel, GARAND_DRAW,
		P_WpnInfo[WEAPON_GARAND].szAnimExt, P_WpnInfo[WEAPON_GARAND].szAnimReloadExt, 0 );
}

int CGarand::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

void CGarand::SecondaryAttack()
{
	RifleMeleeAttack( m_pPlayer, WEAPON_GARAND );
	m_pPlayer->SetAnimation( PLAYER_BUT_SWING );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireGarand, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 1, 0, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.1;
}

void CGarand::PrimaryAttack()
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
		flSpread += P_WpnInfo[m_iId].accuracy_penalty;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecNewShot = m_pPlayer->FireBulletsNC( vecSrc, vecAiming, flSpread, 8192,
		BULLET_PLAYER_GARAND, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireGarand, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, vecNewShot.x, vecNewShot.y,
		(int)m_pPlayer->pev->punchangle.x, 0, m_iClip, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_GARAND].anim_firedelay;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_GARAND].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	m_fInAttack = 1;
	RemoveStamina( 2, m_pPlayer );
}

void CGarand::Reload()
{
	if ( m_iClip <= 0 )
		DefaultReload( P_WpnInfo[WEAPON_GARAND].ammo_maxclip, GARAND_RELOAD,
			P_WpnInfo[WEAPON_GARAND].anim_reloadtime );
}

void CGarand::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( m_iClip <= 0 ? GARAND_IDLE_EMPTY : GARAND_IDLE, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

void CGarandAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CGarandAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_GARAND].ammo_maxclip, "ammo_16mm", P_WpnInfo[WEAPON_GARAND].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
