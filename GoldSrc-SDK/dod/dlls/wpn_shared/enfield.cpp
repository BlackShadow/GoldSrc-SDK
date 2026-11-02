#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "../../cl_dll/dod_shared.h"

class CEnfieldAmmoClip : public CBasePlayerAmmo
{
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( weapon_enfield, CENFIELD );
LINK_ENTITY_TO_CLASS( weapon_scopedenfield, CENFIELD );
LINK_ENTITY_TO_CLASS( ammo_enfield, CEnfieldAmmoClip );

void CENFIELD::Spawn()
{
	Precache();

	if ( m_iWeaponState & 1 )
		SET_MODEL( ENT( pev ), P_WpnInfo[38].wmodel );
	else
		SET_MODEL( ENT( pev ), P_WpnInfo[25].wmodel );

	m_iId = 25;
	m_iDefaultAmmo = P_WpnInfo[25].ammo_default;
	FallInit();
}

void CENFIELD::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[25].vmodel );
	PRECACHE_MODEL( P_WpnInfo[25].wmodel );
	m_usFireEnfield = PRECACHE_EVENT( 1, "events/weapons/enfield.sc" );
	PRECACHE_SOUND( "weapons/enfield_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/boltforward.wav" );
	PRECACHE_SOUND( "weapons/boltback.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );
	PRECACHE_MODEL( P_WpnInfo[38].vmodel );
	PRECACHE_MODEL( P_WpnInfo[38].wmodel );
	PRECACHE_SOUND( "weapons/enfieldsniper_shoot.wav" );
	m_usFireScopedEnfield = PRECACHE_EVENT( 1, "events/weapons/scopedenfield.sc" );
}

int CENFIELD::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_16mm";
	p->iMaxAmmo1 = P_WpnInfo[25].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[25].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = 25;
	p->iWeight = P_WpnInfo[25].misc_weight;
	return 1;
}

BOOL CENFIELD::CanHolster()
{
	return TRUE;
}

int CENFIELD::Classify()
{
	if ( m_iWeaponState & 1 )
		return 32;
	else
		return 16;
}

void CENFIELD::Holster( int skiplocal )
{
	if ( m_iWeaponState & 1 )
	{
		m_pPlayer->m_iFOV = ZoomOut();
		UpdateZoomSpeed();
	}
	SetThink( NULL );
	CBasePlayerWeapon::Holster( skiplocal );
}

void CENFIELD::SecondaryAttack()
{
	if ( m_iWeaponState & 1 )
	{
		m_pPlayer->m_iFOV = ChangeFOV( 20 );
		UpdateZoomSpeed();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.2;
	}
	else
	{
		RifleMeleeAttack( m_pPlayer, 25 );
		m_pPlayer->SetAnimation( PLAYER_ATTACK2 );
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireEnfield, 0,
			(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 1, 0, m_iClip == 0, 0 );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.1;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.1;
	}
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2;
}

int CENFIELD::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

void CENFIELD::ThinkZoomOutIn()
{
	if ( m_iWeaponState & 1 )
	{
		m_pPlayer->m_iFOV = ZoomOut();
		UpdateZoomSpeed();
		SetThink( &CBasePlayerWeapon::ThinkZoomIn );
		pev->nextthink = gpGlobals->time + 1.1f;
	}
}

void CENFIELD::WeaponIdle()
{
	ResetEmptySound();

	if ( m_iWeaponState & 1 )
	{
		if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
			return;

		SendWeaponAnim( 0, 1 );
	}
	else
	{
		if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
			return;

		SendWeaponAnim( 0, 1 );
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
}

void CENFIELD::Reload()
{
	if ( m_iWeaponState & 1 )
	{
		SetThink( NULL );

		if ( GetFOV() > 0 )
		{
			m_pPlayer->m_iFOV = ZoomOut();
			UpdateZoomSpeed();
		}

		DefaultReload( P_WpnInfo[25].ammo_maxclip, 2, P_WpnInfo[38].anim_reloadtime );
	}
	else
	{
		DefaultReload( P_WpnInfo[25].ammo_maxclip, 2, P_WpnInfo[25].anim_reloadtime );
	}
}

BOOL CENFIELD::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();

	if ( m_iWeaponState & 1 )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[38].anim_drawtime;
		return DefaultDeploy( P_WpnInfo[38].vmodel, P_WpnInfo[38].pmodel, 3, P_WpnInfo[38].szAnimExt, P_WpnInfo[38].szAnimReloadExt, 0 );
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[25].anim_drawtime;
		return DefaultDeploy( P_WpnInfo[25].vmodel, P_WpnInfo[25].pmodel, 3, P_WpnInfo[25].szAnimExt, P_WpnInfo[25].szAnimReloadExt, 0 );
	}
}

void CENFIELD::PrimaryAttack()
{
	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;
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
	float flSpread = P_WpnInfo[25].base_accuracy;
	if ( m_iWeaponState & 1 )
	{
		if ( GetFOV() > 0 )
			flSpread = P_WpnInfo[38].base_accuracy2;
		else
			flSpread = P_WpnInfo[38].base_accuracy;
	}
	if ( m_pPlayer->pev->velocity.Length() > 45 )
		flSpread += P_WpnInfo[25].accuracy_penalty;
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecNewShot = m_pPlayer->FireBulletsNC( vecSrc, vecAiming, flSpread, 8192,
		BULLET_PLAYER_ENFIELD, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	if ( m_iWeaponState & 1 )
	{
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireScopedEnfield, 0,
			(float*)&g_vecZero, (float*)&g_vecZero, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, GetFOV() > 0 );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[38].anim_firedelay;
	}
	else
	{
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireEnfield, 0,
			(float*)&g_vecZero, (float*)&g_vecZero, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, 0 );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[25].anim_firedelay;
	}
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[25].anim_firedelay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	m_fInAttack = TRUE;
#ifdef CLIENT_DLL
	flBoltHideXHair = P_WpnInfo[10].anim_firedelay;
#endif
	if ( ( m_iWeaponState & 1 ) && GetFOV() > 0 )
	{
		if ( m_iClip <= 0 )
		{
			SetThink( &CBasePlayerWeapon::ThinkZoomOut );
			pev->nextthink = gpGlobals->time + 0.5f;
		}
		else
		{
			SetThink( &CENFIELD::ThinkZoomOutIn );
			pev->nextthink = gpGlobals->time + 0.5f;
		}
	}
	RemoveStamina( 2, m_pPlayer );
}

void CEnfieldAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CEnfieldAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[25].ammo_maxclip, "ammo_16mm", P_WpnInfo[25].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
