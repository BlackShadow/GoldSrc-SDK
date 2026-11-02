#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_scopedkar, CScopedKar );

void CScopedKar::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_SCOPEDKAR].wmodel );
	m_iId = WEAPON_SCOPEDKAR;
	m_iDefaultAmmo = P_WpnInfo[WEAPON_SCOPEDKAR].ammo_default;
	FallInit();
}

void CScopedKar::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_SCOPEDKAR].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_SCOPEDKAR].wmodel );
	PRECACHE_SOUND( "weapons/kar_shoot.wav" );
	PRECACHE_SOUND( "weapons/boltforward.wav" );
	PRECACHE_SOUND( "weapons/boltback.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );
	m_usFireScopedKar = PRECACHE_EVENT( 1, "events/weapons/scopedkar.sc" );
}

int CScopedKar::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_16mm";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_SCOPEDKAR].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_SCOPEDKAR].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = WEAPON_SCOPEDKAR;
	p->iWeight = P_WpnInfo[WEAPON_SCOPEDKAR].misc_weight;
	return TRUE;
}

int CScopedKar::Classify()
{
	return 32;
}

BOOL CScopedKar::CanHolster()
{
	return TRUE;
}

int CScopedKar::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

BOOL CScopedKar::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_SCOPEDKAR].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[WEAPON_SCOPEDKAR].vmodel, P_WpnInfo[WEAPON_SCOPEDKAR].pmodel, 3,
		P_WpnInfo[WEAPON_SCOPEDKAR].szAnimExt, P_WpnInfo[WEAPON_SCOPEDKAR].szAnimReloadExt, 0 );
}

void CScopedKar::Holster( int skiplocal )
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	SetThink( NULL );
	CBasePlayerWeapon::Holster( skiplocal );
}

void CScopedKar::Reload()
{
	if ( GetFOV() > 0 )
	{
		m_pPlayer->m_iFOV = ZoomOut();
		UpdateZoomSpeed();
	}
	SetThink( NULL );
	DefaultReload( P_WpnInfo[WEAPON_SCOPEDKAR].ammo_maxclip, 2, P_WpnInfo[WEAPON_SCOPEDKAR].anim_reloadtime );
}

void CScopedKar::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( 0, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

void CScopedKar::SecondaryAttack()
{
	m_pPlayer->m_iFOV = ChangeFOV( 20 );
	UpdateZoomSpeed();
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.2;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2;
}

void CScopedKar::ThinkZoomOutIn()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	SetThink( &CBasePlayerWeapon::ThinkZoomIn );
	pev->nextthink = gpGlobals->time + 1.1f;
}

void CScopedKar::PrimaryAttack()
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
		BULLET_PLAYER_SCOPEDKAR, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireScopedKar, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, GetFOV() > 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_SCOPEDKAR].anim_firedelay;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_SCOPEDKAR].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	m_fInAttack = TRUE;
#ifdef CLIENT_DLL
	flBoltHideXHair = P_WpnInfo[WEAPON_KAR].anim_firedelay;
#endif
	if ( GetFOV() > 0 )
	{
		if ( m_iClip <= 0 )
		{
			SetThink( &CBasePlayerWeapon::ThinkZoomOut );
			pev->nextthink = gpGlobals->time + 0.5f;
		}
		else
		{
			SetThink( &CScopedKar::ThinkZoomOutIn );
			pev->nextthink = gpGlobals->time + 0.5f;
		}
	}
	RemoveStamina( 2, m_pPlayer );
}

class CScopedKarAmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( ammo_scopedkar, CScopedKarAmmoClip );

void CScopedKarAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CScopedKarAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_SCOPEDKAR].ammo_maxclip, "ammo_22mm", P_WpnInfo[WEAPON_SCOPEDKAR].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
