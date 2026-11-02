#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( tf_weapon_ic, CTFIncendiaryC );
LINK_ENTITY_TO_CLASS( tf_ic_rocket, CTFIncendiaryCRocket );

void CTFIncendiaryC::Spawn()
{
	Precache();
	m_iId = 8;
	current_ammo = NULL;
	SET_MODEL( ENT( pev ), "models/w_rpg.mdl" );
	m_iDefaultAmmo = 50;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

void CTFIncendiaryC::Precache()
{
	PRECACHE_MODEL( "models/w_rpg.mdl" );
	PRECACHE_MODEL( "models/v_rpg.mdl" );
	PRECACHE_MODEL( "models/p_rpg.mdl" );
	PRECACHE_MODEL( "models/p_rpg2.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	UTIL_PrecacheOther( "tf_ic_rocket" );
	PRECACHE_SOUND( "weapons/rocketfire1.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" );
	m_usFireIC = PRECACHE_EVENT( 1, "events/wpn/tf_ic.sc" );
}

int CTFIncendiaryC::GetItemInfo( ItemInfo* info )
{
	info->pszName = STRING( pev->classname );
	info->pszAmmo1 = "rockets";
	info->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_rockets : 20;
	info->pszAmmo2 = NULL;
	info->iAmmo2 = -1;
	info->iMaxClip = -1;
	info->iSlot = 4;
	info->iPosition = 2;
	info->iId = m_iId = 15;
	info->iFlags = 0;
	info->iWeight = 20;
	return 1;
}

int CTFIncendiaryC::AddToPlayer( CBasePlayer* player )
{
	if ( !CBasePlayerWeapon::AddToPlayer( player ) )
		return 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, player->pev );
		WRITE_BYTE( m_iId );
	MESSAGE_END();
	current_ammo = &m_pPlayer->ammo_rockets;
	return 1;
}

BOOL CTFIncendiaryC::Deploy()
{
	if ( m_pPlayer->ammo_rockets )
		return DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", 5, "rpg", 1 );
	return DefaultDeploy( "models/v_tfc_rpg.mdl", "models/p_rpg.mdl", 7, "rpg", 1 );
}

void CTFIncendiaryC::Holster( int )
{
	m_fInReload = 0;
	m_pPlayer->m_flNextAttack = 0.5f;
	SendWeaponAnim( 4, 1 );
}

void CTFIncendiaryC::PrimaryAttack()
{
	if ( m_pPlayer->ammo_rockets <= 0 )
	{
		PlayEmptySound();
		return;
	}
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireIC, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector up = gpGlobals->v_up * -8;
	Vector right = gpGlobals->v_right * 8;
	Vector forward = gpGlobals->v_forward * 16;
	Vector origin = m_pPlayer->GetGunPosition();
	origin = forward + origin + right + up;
	CTFIncendiaryCRocket::CreateRpgRocket( origin, m_pPlayer->pev->v_angle, m_pPlayer, this );
	DB_LogShots( 1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	m_pPlayer->ammo_rockets--;
	m_flNextPrimaryAttack = GetNextAttackDelay( 1.2f );
	m_flTimeWeaponIdle = 1.2f;
}

void CTFIncendiaryC::SecondaryAttack()
{
}

void CTFIncendiaryC::Reload()
{
}

void CTFIncendiaryC::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= 0 )
	{
		if ( m_pPlayer->ammo_rockets <= 0 )
			m_flTimeWeaponIdle = 1;
		else
		{
			m_flTimeWeaponIdle = 3;
			SendWeaponAnim( 1, 1 );
		}
	}
}
