#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"

LINK_ENTITY_TO_CLASS( tf_weapon_rpg, CTFRpg );
LINK_ENTITY_TO_CLASS( tf_rpg_rocket, CTFRpgRocket );

void CTFRpg::Spawn()
{
	current_ammo = NULL;
	Precache();
	m_iId = 8;
	SET_MODEL( ENT( pev ), "models/w_rpg.mdl" );
	m_iDefaultAmmo = 50;
	m_fReloadTime = 1.25f;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

void CTFRpg::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_rpg.mdl" );
	PRECACHE_MODEL( "models/p_srpg.mdl" );
	PRECACHE_MODEL( "models/p_rpg2.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	UTIL_PrecacheOther( "tf_rpg_rocket" );
	PRECACHE_SOUND( "weapons/rocketfire1.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" );
	m_usFireRPG = PRECACHE_EVENT( 1, "events/wpn/tf_rpg.sc" );
}

int CTFRpg::GetItemInfo( ItemInfo* info )
{
	info->pszName = STRING( pev->classname );
	info->pszAmmo1 = "rockets";
	info->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_rockets : 50;
	info->pszAmmo2 = NULL;
	info->iAmmo2 = -1;
	info->iMaxClip = 4;
	info->iSlot = 4;
	info->iPosition = 1;
	info->iId = m_iId = 14;
	info->iFlags = 0;
	info->iWeight = 20;
	return 1;
}

int CTFRpg::AddToPlayer( CBasePlayer* player )
{
	if ( !CBasePlayerWeapon::AddToPlayer( player ) )
		return 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, player->pev );
		WRITE_BYTE( m_iId );
	MESSAGE_END();
	current_ammo = &m_pPlayer->ammo_rockets;
	return 1;
}

BOOL CTFRpg::Deploy()
{
	return DefaultDeploy( "models/v_tfc_rpg.mdl", "models/p_srpg.mdl", m_iClip ? 4 : 6, "rpg", 1 );
}

void CTFRpg::Holster( int )
{
	m_pPlayer->tfstate &= ~TFSTATE_RELOADING;
	m_fInSpecialReload = 0;
	m_pPlayer->m_flNextAttack = 0.5f;
	SendWeaponAnim( 3, 1 );
}

void CTFRpg::PrimaryAttack()
{
	if ( m_iClip == 0 )
	{
		PlayEmptySound();
		Reload();
		return;
	}
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireRPG, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector up = gpGlobals->v_up * -8;
	Vector right = gpGlobals->v_right * 8;
	Vector forward = gpGlobals->v_forward * 16;
	Vector origin = m_pPlayer->GetGunPosition();
	origin = forward + origin + right + up;
	CTFRpgRocket::CreateRpgRocket( origin, m_pPlayer->pev->v_angle, m_pPlayer, this );
	DB_LogShots( 1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	m_fInSpecialReload = 0;
	m_pPlayer->tfstate &= ~TFSTATE_RELOADING;
	m_iClip--;
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.8f );
	m_flTimeWeaponIdle = 0.8f;
	ResetEmptySound();
}

void CTFRpg::SecondaryAttack()
{
}

void CTFRpg::Reload()
{
	if ( !m_pPlayer->ammo_rockets || m_iClip == 4 || !( m_flNextReload <= 0 ) ||
		!( m_flNextPrimaryAttack <= 0 ) )
		return;
	if ( !m_fInSpecialReload )
	{
		SendWeaponAnim( 7, 1 );
		m_pPlayer->tfstate |= TFSTATE_RELOADING;
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = 0.1f;
		m_flTimeWeaponIdle = 0.1f;
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.1f );
		m_flNextSecondaryAttack = 0.1f;
	}
	else if ( m_fInSpecialReload == 1 )
	{
		if ( m_flTimeWeaponIdle <= gpGlobals->time )
		{
			m_fInSpecialReload = 2;
			SendWeaponAnim( 8, 1 );
			m_flNextReload = m_fReloadTime;
			m_flTimeWeaponIdle = m_fReloadTime;
		}
	}
	else
	{
		m_iClip++;
		m_pPlayer->ammo_rockets--;
		m_fInSpecialReload = 1;
		m_pPlayer->tfstate &= ~TFSTATE_RELOADING;
	}
}

void CTFRpg::WeaponIdle()
{
	if ( !( m_flTimeWeaponIdle <= 0 ) )
		return;
	if ( m_fInSpecialReload )
	{
		if ( m_iClip == 4 || m_pPlayer->ammo_rockets <= 0 )
		{
			SendWeaponAnim( 9, 1 );
			m_fInSpecialReload = 0;
			m_flTimeWeaponIdle = 1.5f;
		}
		else
			Reload();
	}
	else if ( !m_iClip && m_pPlayer->ammo_rockets > 0 )
		Reload();
	else if ( m_pPlayer->current_ammo )
	{
		m_flTimeWeaponIdle = 3;
		ResetEmptySound();
		SendWeaponAnim( m_iClip ? 1 : 11, 1 );
	}
	else
		m_flTimeWeaponIdle = 1;
}
