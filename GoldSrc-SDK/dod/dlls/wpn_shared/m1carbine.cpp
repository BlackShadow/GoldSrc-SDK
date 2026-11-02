#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "../../cl_dll/dod_shared.h"

#ifndef CLIENT_DLL
#include "gamerules.h"
#include "dod_gamerules.h"
#endif

class CM1CarbineAmmoClip : public CBasePlayerAmmo
{
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( weapon_m1carbine, CM1Carbine );
LINK_ENTITY_TO_CLASS( ammo_m1carbine, CM1CarbineAmmoClip );

void CM1Carbine::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[20].wmodel );
	m_iId = 20;
	m_iDefaultAmmo = P_WpnInfo[20].ammo_default;
	FallInit();
}

void CM1Carbine::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[20].vmodel );
	PRECACHE_MODEL( P_WpnInfo[20].wmodel );
	PRECACHE_MODEL( P_WpnInfo[39].vmodel );
	PRECACHE_MODEL( P_WpnInfo[39].wmodel );
	PRECACHE_SOUND( "weapons/carbine_shoot.wav" );
	PRECACHE_SOUND( "weapons/tommy_draw_slideback.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipout.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );
	m_usFireM1Carbine = PRECACHE_EVENT( 1, "events/weapons/m1carbine.sc" );
}

int CM1Carbine::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_66mm";
	p->iMaxAmmo1 = P_WpnInfo[20].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[20].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = 20;
	p->iWeight = P_WpnInfo[20].misc_weight;
	return 1;
}

BOOL CM1Carbine::CanHolster()
{
	return TRUE;
}

int CM1Carbine::Classify()
{
	return 16;
}

void CM1Carbine::SecondaryAttack()
{
}

void CM1Carbine::Reload()
{
	DefaultReload( P_WpnInfo[20].ammo_maxclip, 1, P_WpnInfo[20].anim_reloadtime );
}

BOOL CM1Carbine::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();

	bool bAlliesPara = false;

#ifndef CLIENT_DLL
	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;
	bAlliesPara = ( pRules && pRules->IsAlliesPara() );
#endif

	if ( bAlliesPara )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
		return DefaultDeploy( P_WpnInfo[39].vmodel, P_WpnInfo[39].pmodel, 4,
			P_WpnInfo[39].szAnimExt, P_WpnInfo[39].szAnimReloadExt, 0 );
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
	return DefaultDeploy( P_WpnInfo[20].vmodel, P_WpnInfo[20].pmodel, 2,
		P_WpnInfo[20].szAnimExt, P_WpnInfo[20].szAnimReloadExt, 0 );
}

BOOL CM1Carbine::SpawnDeploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();

	bool bAlliesPara = false;

#ifndef CLIENT_DLL
	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;
	bAlliesPara = ( pRules && pRules->IsAlliesPara() );
#endif

	if ( bAlliesPara )
	{
		return TimedDeploy( P_WpnInfo[39].vmodel, P_WpnInfo[39].pmodel, 2,
			P_WpnInfo[39].szAnimExt, P_WpnInfo[39].szAnimReloadExt, 1.66f, 1.66f, UseDecrement() != FALSE );
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
	return DefaultDeploy( P_WpnInfo[20].vmodel, P_WpnInfo[20].pmodel, 2,
		P_WpnInfo[20].szAnimExt, P_WpnInfo[20].szAnimReloadExt, 0 );
}

int CM1Carbine::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		bool bAlliesPara = false;

#ifndef CLIENT_DLL
		CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;
		bAlliesPara = ( pRules && pRules->IsAlliesPara() );
#endif

		if ( bAlliesPara )
			SET_MODEL( ENT( pev ), P_WpnInfo[39].wmodel );
		else
			SET_MODEL( ENT( pev ), P_WpnInfo[20].wmodel );

		return TRUE;
	}

	return FALSE;
}

void CM1Carbine::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( 0, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

void CM1Carbine::PrimaryAttack()
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
		BULLET_PLAYER_M1CARBINE, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireM1Carbine, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, vecNewShot.x, vecNewShot.y,
		(int)m_pPlayer->pev->punchangle.x, 0, m_iClip, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[20].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.05;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	m_fInAttack = TRUE;
	RemoveStamina( 2, m_pPlayer );
}

void CM1CarbineAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CM1CarbineAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[20].ammo_maxclip, "ammo_66mm", P_WpnInfo[20].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
