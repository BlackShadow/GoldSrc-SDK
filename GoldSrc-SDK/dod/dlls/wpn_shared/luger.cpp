#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_luger, CLUGER );

void CLUGER::Spawn()
{
	Precache();
	m_iId = WEAPON_LUGER;
	SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_LUGER].wmodel );
	m_iDefaultAmmo = P_WpnInfo[WEAPON_LUGER].ammo_default;
	FallInit();
}

void CLUGER::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_LUGER].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_LUGER].wmodel );
	PRECACHE_SOUND( "weapons/luger_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	m_usFireLuger = PRECACHE_EVENT( 1, "events/weapons/luger.sc" );
}

int CLUGER::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_12mm";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_LUGER].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_LUGER].ammo_maxclip;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 64;
	p->iId = m_iId = WEAPON_LUGER;
	p->iWeight = P_WpnInfo[WEAPON_LUGER].misc_weight;
	return TRUE;
}

BOOL CLUGER::CanHolster()
{
	return TRUE;
}

void CLUGER::SecondaryAttack()
{
}

BOOL CLUGER::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_LUGER].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[WEAPON_LUGER].vmodel, P_WpnInfo[WEAPON_LUGER].pmodel, 7,
		P_WpnInfo[WEAPON_LUGER].szAnimExt, P_WpnInfo[WEAPON_LUGER].szAnimReloadExt, 0 );
}

void CLUGER::Reload()
{
	int iAnim;

	if ( m_iClip == 0 )
		iAnim = 5;
	else
		iAnim = 6;

	DefaultReload( P_WpnInfo[WEAPON_LUGER].ammo_maxclip, iAnim, P_WpnInfo[WEAPON_LUGER].anim_reloadtime );
}

void CLUGER::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( m_iClip <= 0 ? 8 : 0, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
	}
}

void CLUGER::PrimaryAttack()
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
		BULLET_PLAYER_LUGER, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireLuger, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_LUGER].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	m_fInAttack = TRUE;
	RemoveStamina( 1, m_pPlayer );
}

class CLugerAmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( ammo_luger, CLugerAmmoClip );

void CLugerAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CLugerAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_LUGER].ammo_maxclip, "ammo_12mm", P_WpnInfo[WEAPON_LUGER].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
