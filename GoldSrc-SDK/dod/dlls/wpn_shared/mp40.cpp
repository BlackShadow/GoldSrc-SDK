#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_mp40, CMP40 );

void CMP40::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_MP40].wmodel );
	m_iId = WEAPON_MP40;
	m_iDefaultAmmo = P_WpnInfo[WEAPON_MP40].ammo_default;
	FallInit();
}

void CMP40::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_MP40].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_MP40].wmodel );
	PRECACHE_SOUND( "weapons/weaponempty.wav" );
	PRECACHE_SOUND( "weapons/mp44_shoot.wav" );
	PRECACHE_SOUND( "weapons/mp40_shoot.wav" );
	m_usFireMP40 = PRECACHE_EVENT( 1, "events/weapons/mp40.sc" );
}

int CMP40::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_44mm";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_MP40].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_MP40].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = WEAPON_MP40;
	p->iWeight = P_WpnInfo[WEAPON_MP40].misc_weight;
	return TRUE;
}

int CMP40::Classify()
{
	return 64;
}

BOOL CMP40::CanHolster()
{
	return TRUE;
}

void CMP40::SecondaryAttack()
{
}

int CMP40::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

BOOL CMP40::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_MP40].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[WEAPON_MP40].vmodel, P_WpnInfo[WEAPON_MP40].pmodel, 2,
		P_WpnInfo[WEAPON_MP40].szAnimExt, P_WpnInfo[WEAPON_MP40].szAnimReloadExt, 0 );
}

void CMP40::Reload()
{
	DefaultReload( P_WpnInfo[WEAPON_MP40].ammo_maxclip, 1, P_WpnInfo[WEAPON_MP40].anim_reloadtime );
}

void CMP40::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( 0, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

void CMP40::PrimaryAttack()
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
		BULLET_PLAYER_MP40, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireMP40, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_MP40].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	m_fInAttack = TRUE;
	RemoveStamina( 2, m_pPlayer );
}

class CMP40AmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( ammo_mp40, CMP40AmmoClip );

void CMP40AmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CMP40AmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_MP40].ammo_maxclip, "ammo_44mm", P_WpnInfo[WEAPON_MP40].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
