#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_mp44, CMP44 );

void CMP44::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_MP44].wmodel );
	m_iId = WEAPON_MP44;
	m_iDefaultAmmo = P_WpnInfo[WEAPON_MP44].ammo_default;
	FallInit();
}

void CMP44::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_MP44].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_MP44].wmodel );
	PRECACHE_SOUND( "weapons/weaponempty.wav" );
	PRECACHE_SOUND( "weapons/mp44_shoot.wav" );
	m_usFireMP44 = PRECACHE_EVENT( 1, "events/weapons/mp44.sc" );
}

int CMP44::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_55mm";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_MP44].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_MP44].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = WEAPON_MP44;
	p->iWeight = P_WpnInfo[WEAPON_MP44].misc_weight;
	return TRUE;
}

int CMP44::Classify()
{
	return 64;
}

BOOL CMP44::CanHolster()
{
	return TRUE;
}

void CMP44::SecondaryAttack()
{
}

int CMP44::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

BOOL CMP44::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_MP44].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[WEAPON_MP44].vmodel, P_WpnInfo[WEAPON_MP44].pmodel, 2,
		P_WpnInfo[WEAPON_MP44].szAnimExt, P_WpnInfo[WEAPON_MP44].szAnimReloadExt, 0 );
}

void CMP44::Reload()
{
	DefaultReload( P_WpnInfo[WEAPON_MP44].ammo_maxclip, 1, P_WpnInfo[WEAPON_MP44].anim_reloadtime );
}

void CMP44::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( 0, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

void CMP44::PrimaryAttack()
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
		BULLET_PLAYER_MP44, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireMP44, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_MP44].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	RemoveStamina( 2, m_pPlayer );
}

class CMP44AmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( ammo_mp44, CMP44AmmoClip );

void CMP44AmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CMP44AmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_MP44].ammo_maxclip, "ammo_55mm", P_WpnInfo[WEAPON_MP44].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
