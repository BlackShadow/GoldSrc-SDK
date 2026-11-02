#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

short sModelIndexFlameThrowerFlame;

LINK_ENTITY_TO_CLASS( tf_weapon_flamethrower, CTFFlamethrower );

void CTFFlamethrower::Spawn()
{
	current_ammo = NULL;
	Precache();
	m_iId = 10;
	SET_MODEL( ENT( pev ), "models/w_egon.mdl" );
	m_iDefaultAmmo = 50;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

void CTFFlamethrower::Precache()
{
	PRECACHE_MODEL( "models/w_egon.mdl" );
	PRECACHE_MODEL( "models/v_flame.mdl" );
	PRECACHE_MODEL( "models/p_egon.mdl" );
	PRECACHE_MODEL( "models/p_egon2.mdl" );
	PRECACHE_MODEL( "models/w_9mmclip.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "weapons/flmfire2.wav" );
	UTIL_PrecacheOther( "tf_flamethrower_burst" );
	m_usFireFlame = PRECACHE_EVENT( 1, "events/wpn/tf_flame.sc" );
}

int CTFFlamethrower::GetItemInfo( ItemInfo* info )
{
	info->pszName = STRING( pev->classname );
	info->pszAmmo1 = "uranium";
	info->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_cells : 200;
	info->pszAmmo2 = NULL;
	info->iAmmo2 = -1;
	info->iMaxClip = -1;
	info->iSlot = 3;
	info->iPosition = 2;
	info->iId = m_iId = 13;
	info->iFlags = 0;
	info->iWeight = 20;
	return 1;
}

int CTFFlamethrower::AddToPlayer( CBasePlayer* player )
{
	if ( !CBasePlayerWeapon::AddToPlayer( player ) )
		return 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, player->pev );
		WRITE_BYTE( m_iId );
	MESSAGE_END();
	current_ammo = &m_pPlayer->ammo_cells;
	return 1;
}

BOOL CTFFlamethrower::Deploy()
{
	return DefaultDeploy( "models/v_flame.mdl", "models/p_egon.mdl", 9, "egon", 1 );
}

void CTFFlamethrower::Holster( int )
{
	m_fInReload = 0;
	m_pPlayer->m_flNextAttack = 0.5f;
	SendWeaponAnim( 10, 1 );
}

void CTFFlamethrower::PrimaryAttack()
{
	if ( m_pPlayer->ammo_cells <= 0 )
	{
		PlayEmptySound();
		return;
	}
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireFlame, 0,
		m_pPlayer->pev->origin, m_pPlayer->pev->angles, 0, 0, 0, 0, m_pPlayer->pev->waterlevel > 2, 0 );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector up = gpGlobals->v_up;
	Vector right = gpGlobals->v_right;
	Vector forward = gpGlobals->v_forward;
	Vector origin = m_pPlayer->GetGunPosition();
	if ( m_pPlayer->pev->waterlevel > 2 )
	{
		m_flNextPrimaryAttack = GetNextAttackDelay( 1 );
		m_flTimeWeaponIdle = 1;
	}
	else
	{
		origin = origin + forward * 16 + right * 8 + up * -8;
		CTFFlamethrowerBurst::CreateBurst( origin, m_pPlayer->pev->v_angle, m_pPlayer, this );
		DB_LogShots( 1 );
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.15f );
		m_flTimeWeaponIdle = 0.15f;
	}
	m_pPlayer->ammo_cells--;
}

void CTFFlamethrower::SecondaryAttack()
{
}

void CTFFlamethrower::Reload()
{
}

void CTFFlamethrower::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= 0 )
	{
		int animation;
		if ( UTIL_SharedRandomLong( m_pPlayer->random_seed, 0, 4 ) )
		{
			m_flTimeWeaponIdle = 12.5f;
			animation = 0;
		}
		else
		{
			m_flTimeWeaponIdle = 3;
			animation = 1;
		}
		SendWeaponAnim( animation, 1 );
	}
}
