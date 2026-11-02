#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "../../cl_dll/dod_shared.h"

class CSpringAmmoClip : public CBasePlayerAmmo
{
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( weapon_spring, CSPRING );
LINK_ENTITY_TO_CLASS( ammo_spring, CSpringAmmoClip );

void CSPRING::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[9].wmodel );
	m_iId = 9;
	m_iDefaultAmmo = P_WpnInfo[9].ammo_default;
	FallInit();
}

void CSPRING::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[9].vmodel );
	PRECACHE_MODEL( P_WpnInfo[9].wmodel );
	PRECACHE_SOUND( "weapons/reloadgarand.wav" );
	PRECACHE_SOUND( "weapons/spring_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/garand_reload_clipin.wav" );
	m_usFireSpring = PRECACHE_EVENT( 1, "events/weapons/spring.sc" );
}

int CSPRING::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_22mm";
	p->iMaxAmmo1 = P_WpnInfo[9].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[9].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = 9;
	p->iWeight = P_WpnInfo[9].misc_weight;
	return 1;
}

BOOL CSPRING::CanHolster()
{
	return TRUE;
}

int CSPRING::Classify()
{
	return 32;
}

void CSPRING::ThinkZoomOutIn()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	SetThink( &CBasePlayerWeapon::ThinkZoomIn );
	pev->nextthink = gpGlobals->time + 1.4f;
}

void CSPRING::ThinkZoomOut()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
}

void CSPRING::SecondaryAttack()
{
	m_pPlayer->m_iFOV = ChangeFOV( 20 );
	UpdateZoomSpeed();
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.2;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2;
}

void CSPRING::Holster( int skiplocal )
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	SetThink( NULL );
	CBasePlayerWeapon::Holster( skiplocal );
}

void CSPRING::Reload()
{
	if ( GetFOV() > 0 )
	{
		m_pPlayer->m_iFOV = ZoomOut();
		UpdateZoomSpeed();
	}
	SetThink( NULL );
	DefaultReload( P_WpnInfo[9].ammo_maxclip, 3, P_WpnInfo[9].anim_reloadtime );
}

BOOL CSPRING::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[9].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[9].vmodel, P_WpnInfo[9].pmodel, 4,
		P_WpnInfo[9].szAnimExt, P_WpnInfo[9].szAnimReloadExt, 0 );
}

int CSPRING::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

void CSPRING::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( 0, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

void CSPRING::PrimaryAttack()
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
	float flSpread;

	if ( GetFOV() > 0 )
		flSpread = P_WpnInfo[m_iId].base_accuracy2;
	else
		flSpread = P_WpnInfo[m_iId].base_accuracy;
	if ( m_pPlayer->pev->velocity.Length() > 45 )
		flSpread += P_WpnInfo[m_iId].accuracy_penalty;
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecNewShot = m_pPlayer->FireBulletsNC( vecSrc, vecAiming, flSpread, 8192,
		BULLET_PLAYER_SPRING, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireSpring, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[9].anim_firedelay;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[9].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[9].anim_firedelay;
	m_fInAttack = TRUE;
#ifdef CLIENT_DLL
	flBoltHideXHair = P_WpnInfo[10].anim_firedelay;
#endif
	if ( GetFOV() > 0 )
	{
		if ( m_iClip <= 0 )
		{
			SetThink( &CSPRING::ThinkZoomOut );
			pev->nextthink = gpGlobals->time + 0.5f;
		}
		else
		{
			SetThink( &CSPRING::ThinkZoomOutIn );
			pev->nextthink = gpGlobals->time + 0.5f;
		}
	}
	RemoveStamina( 2, m_pPlayer );
}

void CSpringAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CSpringAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[9].ammo_maxclip, "ammo_22mm", P_WpnInfo[9].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
