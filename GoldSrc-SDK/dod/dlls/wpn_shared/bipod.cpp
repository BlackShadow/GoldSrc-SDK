#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

#define BIPOD_MOVEMENT_SPEED		45
#define BIPOD_HEAT_PER_SHOT			3
#define BIPOD_HEAT_WARNING			95
#define BIPOD_HEAT_LIMIT			99
#define BIPOD_COOLING_STEP			0.6
#define BIPOD_COOLING_INTERVAL		0.1

void CBipodWeapon::Spawn( int weapon_id )
{
	Precache();

	m_iId = weapon_id;
	SET_MODEL( ENT( pev ), P_WpnInfo[m_iId].wmodel );
	m_iDefaultAmmo = P_WpnInfo[m_iId].ammo_default;

	FallInit();
}

void CBipodWeapon::Precache()
{
	m_iOverheatEvent = PRECACHE_EVENT( 1, "events/effects/overheat.sc" );
}

BOOL CBipodWeapon::CanHolster()
{
	if ( IsDeployed() )
		return FALSE;

	return TRUE;
}

void CBipodWeapon::Holster( int skiplocal )
{
	CBasePlayerWeapon::Holster( skiplocal );
}

BOOL CBipodWeapon::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;

	return DefaultDeploy( P_WpnInfo[m_iId].vmodel, P_WpnInfo[m_iId].pmodel, GetDrawAnim(),
		P_WpnInfo[m_iId].szAnimExt, P_WpnInfo[m_iId].szAnimReloadExt, 0 );
}

int CBipodWeapon::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

void CBipodWeapon::Reload()
{
	float flReloadTime = 0;

	if ( m_iId == WEAPON_BREN && IsDeployed() )
		flReloadTime = P_WpnInfo[m_iId].anim_reloadtime2;
	else
		flReloadTime = P_WpnInfo[m_iId].anim_reloadtime;

	DefaultReload( P_WpnInfo[m_iId].ammo_maxclip, GetReloadAnim(), flReloadTime );
}

void CBipodWeapon::CoolThink()
{
	m_flWeaponHeat = m_flWeaponHeat - BIPOD_COOLING_STEP;
	if ( m_flWeaponHeat <= 0 )
	{
		m_flWeaponHeat = 0;
		SetThink( NULL );
	}
	else
		pev->nextthink = gpGlobals->time + BIPOD_COOLING_INTERVAL;
}

void CBipodWeapon::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() &&
		m_flNextPrimaryAttack <= UTIL_WeaponTimeBase() &&
		m_flNextSecondaryAttack <= UTIL_WeaponTimeBase() )
	{
		int i_Anim = GetIdleAnim();
		if ( i_Anim > -1 )
			SendWeaponAnim( i_Anim, 1 );
	}
}

void CBipodWeapon::SecondaryAttack()
{
	int debug;
	debug = 1;

	if ( m_pPlayer->pev->waterlevel > 0 && m_pPlayer->pev->iuser3 != 2 )
		return;
#ifndef CLIENT_DLL
	if ( m_pPlayer->pev->iuser3 == 0 && m_pPlayer->pev->vuser1.x == 0 )
		return;

	if ( m_pPlayer->pev->vuser1.x == 1 && ( m_pPlayer->pev->button & IN_DUCK ) )
		return;
#endif

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_down2uptime;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_down2uptime;

	if ( m_pPlayer->pev->iuser3 == 1 || m_pPlayer->pev->vuser1.x == 1 )
	{
#ifndef CLIENT_DLL
		if ( m_pPlayer->pev->vuser1.x == 0 )
			m_pPlayer->pev->iuser3 = 2;
		else
			m_pPlayer->pev->vuser1.x = 2;
		m_pPlayer->SetAnimation( PLAYER_IDLE );
#endif
		SendWeaponAnim( GetUpToDownAnim(), 1 );
	}
	else if ( m_pPlayer->pev->iuser3 == 2 || m_pPlayer->pev->vuser1.x == 2 )
	{
#ifndef CLIENT_DLL
		if ( m_pPlayer->pev->vuser1.x == 0 )
			m_pPlayer->pev->iuser3 = 1;
		else
			m_pPlayer->pev->vuser1.x = 1;
		m_pPlayer->SetAnimation( PLAYER_IDLE );
		UTIL_SendDirectorMessage( m_pPlayer, NULL, 130 );
#endif
		SendWeaponAnim( GetDownToUpAnim(), 1 );
	}
#ifndef CLIENT_DLL
	m_pPlayer->CheckPlayerSpeed();
#endif
}

void CBipodWeapon::PrimaryAttack()
{
	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
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

	ItemInfo bipod;
	GetItemInfo( &bipod );

	if ( bipod.iFlags & WEAPON_FLAG_OVERHEAT )
	{
		if ( m_fInAttack )
			return;

		if ( m_flWeaponHeat >= BIPOD_HEAT_LIMIT )
		{
#ifndef CLIENT_DLL
			if ( !( m_pPlayer->m_flDisplayHistory & 0x40000 ) )
			{
				m_pPlayer->m_flDisplayHistory |= 0x40000;
				m_pPlayer->HintMessage( "#Hint_mg_overheat", FALSE );
			}
#endif
			m_fInAttack = TRUE;
			return;
		}

		if ( m_flWeaponHeat >= BIPOD_HEAT_WARNING )
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), (unsigned short)m_iOverheatEvent, 0,
				(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	}

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	float flSpread = GetBipodSpread();
	if ( !IsDeployed() && m_pPlayer->pev->velocity.Length() > BIPOD_MOVEMENT_SPEED )
		flSpread += P_WpnInfo[m_iId].accuracy_penalty;
#ifndef CLIENT_DLL
	if ( m_pPlayer->pev->iuser3 != 2 && m_pPlayer->pev->vuser1.x != 2 &&
		m_iId != WEAPON_BAR && m_iId != WEAPON_BREN && !( m_pPlayer->m_flDisplayHistory & 0x200 ) )
	{
		m_pPlayer->m_flDisplayHistory |= 0x200;
		m_pPlayer->HintMessage( "#Hint_mgs_fire_better_deployed", FALSE );
	}
#endif

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecNewShot = m_pPlayer->FireBulletsNC( vecSrc, vecAiming, flSpread, 8192, bipod.iBulletId, 3, 0,
		m_pPlayer->pev, m_pPlayer->random_seed );
	m_iClip--;

	if ( m_iId == WEAPON_MG42 )
	{
		m_iClip--;
		Vector vecNewShot2 = m_pPlayer->FireBulletsNC( vecSrc, vecAiming, flSpread, 8192, bipod.iBulletId, 3, 0,
			m_pPlayer->pev, m_pPlayer->random_seed + 5 );
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), (unsigned short)m_iFireEvent, 0,
			(float*)&g_vecZero.x, (float*)&g_vecZero.x, vecNewShot2.x, vecNewShot2.y, m_iClip, 0, 1, 0 );
	}

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), (unsigned short)m_iFireEvent, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, vecNewShot.x, vecNewShot.y, m_iClip, 0, 0, 0 );

	if ( bipod.iFlags & WEAPON_FLAG_OVERHEAT )
		m_flWeaponHeat += BIPOD_HEAT_PER_SHOT;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_firedelay;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + P_WpnInfo[m_iId].anim_firedelay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );

	if ( !IsDeployed() )
		RemoveStamina( bipod.flRecoil, m_pPlayer );
#ifndef CLIENT_DLL
	if ( bipod.iFlags & WEAPON_FLAG_OVERHEAT )
	{
		SetThink( &CBipodWeapon::CoolThink );
		pev->nextthink = gpGlobals->time + BIPOD_COOLING_INTERVAL;
	}
#endif
}

void CBipodWeapon::ForceUndeploy()
{
	SendWeaponAnim( GetDownToUpAnim(), 0 );
}

bool CBipodWeapon::IsDeployed()
{
	return m_pPlayer->IsInMGDeploy();
}

float CBipodWeapon::GetBipodSpread()
{
	if ( IsDeployed() )
		return P_WpnInfo[m_iId].base_accuracy2;
	else
		return P_WpnInfo[m_iId].base_accuracy;
}
