#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "tf_defs.h"

#define AC_WINDUP_DELAY			0.5f
#define AC_FIRE_INTERVAL		0.1f
#define AC_WINDUP_IDLE_TIME		0.6
#define AC_SPIN_DOWN_TIME		2
#define AC_IDLE_TIME			12.5

enum
{
	AC_IDLE = 0,
	AC_WINDUP,
	AC_FIRING,
	AC_SPINNING,
};

enum
{
	AC_DRAW = 5,
	AC_HOLSTER
};

LINK_ENTITY_TO_CLASS( tf_weapon_ac, CTFAssaultC );

void CTFAssaultC::Spawn()
{
	pev->classname = MAKE_STRING( "tf_weapon_ac" );
	Precache();

	m_iId = 4;
	m_iDefaultAmmo = 25;
	m_iWeaponState = AC_IDLE;

	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

void CTFAssaultC::Precache()
{
	PRECACHE_MODEL( "models/v_tfac.mdl" );
	PRECACHE_MODEL( "models/p_mini.mdl" );
	PRECACHE_MODEL( "models/p_mini2.mdl" );
	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );

	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/asscan1.wav" );
	PRECACHE_SOUND( "weapons/asscan2.wav" );
	PRECACHE_SOUND( "weapons/asscan3.wav" );
	PRECACHE_SOUND( "weapons/asscan4.wav" );

	m_usWindUp = PRECACHE_EVENT( 1, "events/wpn/tf_acwu.sc" );
	m_usWindDown = PRECACHE_EVENT( 1, "events/wpn/tf_acwd.sc" );
	m_usFire = PRECACHE_EVENT( 1, "events/wpn/tf_acfire.sc" );
	m_usStartSpin = PRECACHE_EVENT( 1, "events/wpn/tf_acsspin.sc" );
	m_usSpin = PRECACHE_EVENT( 1, "events/wpn/tf_acspin.sc" );
	m_usACStart = PRECACHE_EVENT( 1, "events/wpn/tf_acstart.sc" );
}

int CTFAssaultC::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );

	p->pszAmmo1 = "buckshot";
	p->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_shells : 200;
	p->pszAmmo2 = NULL;
	p->iAmmo2 = -1;
	p->iMaxClip = -1;

	p->iSlot = 4;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_ASSAULT_CANNON;
	p->iWeight = 15;

	return 1;
}

int CTFAssaultC::AddToPlayer( CBasePlayer* player )
{
	if ( !CBasePlayerWeapon::AddToPlayer( player ) )
		return FALSE;

	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, player->pev );
		WRITE_BYTE( m_iId );
	MESSAGE_END();

	return TRUE;
}

BOOL CTFAssaultC::Deploy()
{
	return DefaultDeploy( "models/v_tfac.mdl", "models/p_mini.mdl", AC_DRAW, "ac", 1 );
}

void CTFAssaultC::Holster( int skiplocal )
{
	WindDown( true );
	m_fInReload = 0;
	m_pPlayer->m_flNextAttack = 0.1;
	SendWeaponAnim( AC_HOLSTER, 1 );
}

void CTFAssaultC::Reload()
{
}

void CTFAssaultC::WindUp()
{
	if ( !( m_pPlayer->pev->button & IN_ATTACK ) )
		return;

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usWindUp, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );

	m_iWeaponState = AC_WINDUP;
	m_pPlayer->tfstate |= TFSTATE_AIMING;
	m_pPlayer->TeamFortress_SetSpeed();
}

void CTFAssaultC::WindDown( bool bFromHolster )
{
	PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usWindDown, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, bFromHolster, 0 );

	m_iWeaponState = AC_IDLE;
	m_pPlayer->tfstate &= ~TFSTATE_AIMING;
	m_pPlayer->TeamFortress_SetSpeed();
	m_flTimeWeaponIdle = AC_SPIN_DOWN_TIME;
}

void CTFAssaultC::StartSpin()
{
	PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usStartSpin, 0,
		m_pPlayer->pev->origin, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );

	m_iWeaponState = AC_SPINNING;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->pev->effects &= ~EF_MUZZLEFLASH;
}

void CTFAssaultC::Spin()
{
	PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usSpin, 0,
		m_pPlayer->pev->origin, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
}

void CTFAssaultC::Fire()
{
	if ( m_flNextPrimaryAttack > 0 )
		return;

	PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_UPDATE, m_pPlayer->edict(), m_usFire, 0,
		(float*)&g_vecZero, (float*)&g_vecZero,
		0, 0, 0, 0, m_pPlayer->ammo_shells & 1, 0 );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs
		+ gpGlobals->v_right * 2 + gpGlobals->v_up * -4;
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	m_pPlayer->FireBullets( 5, vecSrc, vecAiming, Vector( 0.1, 0.1, 0 ), 8192,
		BULLET_PLAYER_TF_ASSAULT, 8, 7 );

	DB_LogShots( 1 );
	m_pPlayer->ammo_shells--;
}

void CTFAssaultC::PrimaryAttack()
{
	switch ( m_iWeaponState )
	{
	case AC_FIRING:
		if ( m_pPlayer->ammo_shells <= 0 )
			StartSpin();
		else
			Fire();
		break;

	case AC_SPINNING:
		if ( m_pPlayer->ammo_shells <= 0 )
			Spin();
		else
			m_iWeaponState = AC_WINDUP;
		break;

	case AC_WINDUP:
		if ( m_flNextPrimaryAttack > 0 )
			return;

		PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usACStart, 0,
			m_pPlayer->pev->origin, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );
		m_iWeaponState = AC_FIRING;
		break;

	default:
		WindUp();
		m_flNextPrimaryAttack = GetNextAttackDelay( AC_WINDUP_DELAY );
		m_flTimeWeaponIdle = AC_WINDUP_IDLE_TIME;
		return;
	}

	m_flTimeWeaponIdle = m_flNextPrimaryAttack = GetNextAttackDelay( AC_FIRE_INTERVAL );
}

void CTFAssaultC::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > 0 )
		return;

	if ( m_iWeaponState != AC_IDLE )
	{
		WindDown( false );
	}
	else
	{
		SendWeaponAnim( !( UTIL_SharedRandomLong( m_pPlayer->random_seed, 0, 1 ) & 1 ), 1 );
		m_flTimeWeaponIdle = AC_IDLE_TIME;
	}
}
