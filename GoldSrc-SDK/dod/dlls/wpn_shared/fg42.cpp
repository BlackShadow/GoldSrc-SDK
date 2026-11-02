#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

enum
{
	FG42_SCOPED_INFO = 37
};

LINK_ENTITY_TO_CLASS( weapon_fg42, CFG42 );
LINK_ENTITY_TO_CLASS( weapon_scopedfg42, CFG42 );

void CFG42::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_FG42].wmodel );
	m_iId = WEAPON_FG42;
	m_iDefaultAmmo = P_WpnInfo[WEAPON_FG42].ammo_default;
	FallInit();
}

void CFG42::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_FG42].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_FG42].wmodel );
	PRECACHE_MODEL( P_WpnInfo[FG42_SCOPED_INFO].vmodel );
	PRECACHE_MODEL( P_WpnInfo[FG42_SCOPED_INFO].wmodel );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/fg42_shoot.wav" );
	m_usFireFG42 = PRECACHE_EVENT( 1, "events/weapons/fg42.sc" );
}

int CFG42::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_55mm";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_FG42].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_FG42].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = WEAPON_FG42;
	p->iWeight = P_WpnInfo[WEAPON_FG42].misc_weight;
	return TRUE;
}

int CFG42::Classify()
{
	if ( m_iWeaponState & 1 )
		return 32;
	else
		return 256;
}

BOOL CFG42::CanHolster()
{
	if ( IsDeployed() )
		return FALSE;

	return TRUE;
}

BOOL CFG42::IsDeployed()
{
	return m_pPlayer->IsInMGDeploy();
}

int CFG42::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

BOOL CFG42::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.93;

	if ( m_iWeaponState & 1 )
		return DefaultDeploy( P_WpnInfo[FG42_SCOPED_INFO].vmodel, P_WpnInfo[FG42_SCOPED_INFO].pmodel, 2, P_WpnInfo[FG42_SCOPED_INFO].szAnimExt, P_WpnInfo[FG42_SCOPED_INFO].szAnimReloadExt, 0 );
	else
		return DefaultDeploy( P_WpnInfo[WEAPON_FG42].vmodel, P_WpnInfo[WEAPON_FG42].pmodel, 2, P_WpnInfo[WEAPON_FG42].szAnimExt, P_WpnInfo[WEAPON_FG42].szAnimReloadExt, 0 );
}

void CFG42::Holster( int skiplocal )
{
	if ( !IsDeployed() )
	{
		SetThink( NULL );
		CBasePlayerWeapon::Holster( skiplocal );
	}
}

void CFG42::Reload()
{
	if ( GetFOV() > 0 )
	{
		m_pPlayer->m_iFOV = ZoomOut();
		UpdateZoomSpeed();
	}

	if ( IsDeployed() )
		DefaultReload( P_WpnInfo[WEAPON_FG42].ammo_maxclip, 6, P_WpnInfo[WEAPON_FG42].anim_reloadtime );
	else
		DefaultReload( P_WpnInfo[WEAPON_FG42].ammo_maxclip, 1, P_WpnInfo[WEAPON_FG42].anim_reloadtime );
}

void CFG42::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		if ( IsDeployed() )
			SendWeaponAnim( 5, 1 );
		else
			SendWeaponAnim( 0, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

void CFG42::SecondaryAttack()
{
	bool scoped = (bool)( m_iWeaponState & 1 );

	if ( scoped )
	{
		m_pPlayer->m_iFOV = ChangeFOV( 20 );
		UpdateZoomSpeed();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2;
		return;
	}

#ifndef CLIENT_DLL
	if ( m_pPlayer->pev->iuser3 == 0 && m_pPlayer->pev->vuser1.x == 0 )
		return;

	if ( m_pPlayer->pev->vuser1.x == 1 && ( m_pPlayer->pev->button & IN_DUCK ) )
		return;
#endif

	if ( m_pPlayer->pev->iuser3 == 1 || m_pPlayer->pev->vuser1.x == 1 )
	{
#ifndef CLIENT_DLL
		if ( m_pPlayer->pev->vuser1.x == 0 )
			m_pPlayer->pev->iuser3 = 2;
		else
			m_pPlayer->pev->vuser1.x = 2;

		m_pPlayer->SetAnimation( PLAYER_IDLE );
#endif
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.2;
		SendWeaponAnim( 4, 1 );
	}
	else if ( m_pPlayer->pev->iuser3 == 2 || m_pPlayer->pev->vuser1.x == 2 )
	{
#ifndef CLIENT_DLL
		if ( m_pPlayer->pev->vuser1.x == 0 )
			m_pPlayer->pev->iuser3 = 1;
		else
			m_pPlayer->pev->vuser1.x = 1;

		m_pPlayer->SetAnimation( PLAYER_IDLE );
#endif
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.98;
		SendWeaponAnim( 8, 1 );
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.3;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.3;
}

void CFG42::PrimaryAttack()
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
	float flSpread;
	bool scoped = (bool)( m_iWeaponState & 1 );

	if ( scoped )
	{
		if ( GetFOV() > 0 )
			flSpread = 0.008f;
		else
			flSpread = P_WpnInfo[m_iId].base_accuracy;
	}
	else
	{
		if ( IsDeployed() )
			flSpread = P_WpnInfo[m_iId].base_accuracy2;
		else
			flSpread = P_WpnInfo[m_iId].base_accuracy;
	}
	if ( m_pPlayer->pev->velocity.Length() > 45 )
		flSpread += P_WpnInfo[m_iId].accuracy_penalty;
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecNewShot = m_pPlayer->FireBulletsNC( vecSrc, vecAiming, flSpread, 8192,
		BULLET_PLAYER_FG42, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireFG42, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, GetFOV() > 0 );
	if ( GetFOV() > 0 )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[FG42_SCOPED_INFO].anim_firedelay;
	else
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_FG42].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	if ( GetFOV() > 0 && m_iClip <= 0 )
	{
		SetThink( &CBasePlayerWeapon::ThinkZoomOut );
		pev->nextthink = gpGlobals->time + 0.5f;
	}
	RemoveStamina( 2, m_pPlayer );
}

class CFG42AmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( ammo_fg42, CFG42AmmoClip );

void CFG42AmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CFG42AmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_FG42].ammo_maxclip, "ammo_55mm", P_WpnInfo[WEAPON_FG42].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
