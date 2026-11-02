#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "tf_defs.h"

#define SHOTGUN_MAX_CLIP		8
#define SUPER_SHOTGUN_MAX_CLIP	16
#define SHOTGUN_PUMP_IDLE		1000

LINK_ENTITY_TO_CLASS( tf_weapon_shotgun, CTFShotgun );
LINK_ENTITY_TO_CLASS( tf_weapon_supershotgun, CTFSuperShotgun );

void CTFShotgun::Spawn()
{
	Precache();

	m_iId = WEAPON_TF_SHOTGUN;
	m_iMaxClipSize = SHOTGUN_MAX_CLIP;
	m_iShellsReloaded = 1;
	current_ammo = NULL;
	m_iDefaultAmmo = SHOTGUN_MAX_CLIP;
	m_fReloadTime = 0.25;
	m_flPumpTime = SHOTGUN_PUMP_IDLE;

	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

void CTFSuperShotgun::Spawn()
{
	Precache();

	m_iId = WEAPON_TF_SHOTGUN;
	current_ammo = NULL;
	m_flPumpTime = SHOTGUN_PUMP_IDLE;
	pev->solid = SOLID_TRIGGER;
	m_iDefaultAmmo = SUPER_SHOTGUN_MAX_CLIP;
	m_iMaxClipSize = SUPER_SHOTGUN_MAX_CLIP;
	SetTouch( &CBasePlayerItem::DefaultTouch );
	m_fReloadTime = 0.375;
	m_iShellsReloaded = 2;
}

void CTFShotgun::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_12gauge.mdl" );
	PRECACHE_MODEL( "models/p_smallshotgun.mdl" );
	PRECACHE_MODEL( "models/p_shotgun2.mdl" );
	m_iShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );

	PRECACHE_SOUND( "weapons/dbarrel1.wav" );
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );
	PRECACHE_SOUND( "weapons/reload1.wav" );
	PRECACHE_SOUND( "weapons/reload3.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/scock1.wav" );
	PRECACHE_SOUND( "weapons/shotgn2.wav" );

	m_usFireShotgun = PRECACHE_EVENT( 1, "events/wpn/tf_sg.sc" );
	m_usReloadShotgun = PRECACHE_EVENT( 1, "events/wpn/tf_sgreload.sc" );
	m_usPumpShotgun = PRECACHE_EVENT( 1, "events/wpn/tf_sgpump.sc" );
}

void CTFSuperShotgun::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_shotgun.mdl" );
	PRECACHE_MODEL( "models/p_shotgun.mdl" );
	PRECACHE_MODEL( "models/p_shotgun2.mdl" );
	m_iShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );

	PRECACHE_SOUND( "weapons/dbarrel1.wav" );
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );
	PRECACHE_SOUND( "weapons/reload1.wav" );
	PRECACHE_SOUND( "weapons/reload3.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/scock1.wav" );
	PRECACHE_SOUND( "weapons/shotgn2.wav" );

	m_usFireSuperShotgun = PRECACHE_EVENT( 1, "events/wpn/tf_ssg.sc" );
	m_usReloadShotgun = PRECACHE_EVENT( 1, "events/wpn/tf_sgreload.sc" );
	m_usPumpShotgun = PRECACHE_EVENT( 1, "events/wpn/tf_sgpump.sc" );
}

int CTFShotgun::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "buckshot";
	p->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_shells : 200;
	p->pszAmmo2 = NULL;
	p->iAmmo2 = -1;
	p->iMaxClip = m_iMaxClipSize;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_TF_SHOTGUN;
	p->iWeight = 15;
	return 1;
}

int CTFSuperShotgun::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "buckshot";
	p->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_shells : 200;
	p->pszAmmo2 = NULL;
	p->iAmmo2 = -1;
	p->iMaxClip = m_iMaxClipSize;
	p->iSlot = 2;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SUPER_SHOTGUN;
	p->iWeight = 15;
	return 1;
}

int CTFShotgun::AddToPlayer( CBasePlayer* player )
{
	if ( !CBasePlayerWeapon::AddToPlayer( player ) )
		return FALSE;

	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, player->pev );
		WRITE_BYTE( m_iId );
	MESSAGE_END();

	current_ammo = &m_pPlayer->ammo_shells;
	return TRUE;
}

BOOL CTFShotgun::Deploy()
{
	return DefaultDeploy( "models/v_tfc_12gauge.mdl", "models/p_smallshotgun.mdl", 6, "shotgun", 1 );
}

BOOL CTFSuperShotgun::Deploy()
{
	return DefaultDeploy( "models/v_tfc_shotgun.mdl", "models/p_shotgun.mdl", 6, "shotgun", 1 );
}

void CTFShotgun::SecondaryAttack()
{
}

void CTFShotgun::Reload()
{
	if ( m_pPlayer->ammo_shells < m_iShellsReloaded || m_iClip == m_iMaxClipSize
		|| m_flNextReload > 0 || m_flNextPrimaryAttack > 0 )
		return;

	if ( m_fInSpecialReload == 0 )
	{
		SendWeaponAnim( 5, 1 );
		m_pPlayer->tfstate |= TFSTATE_RELOADING;
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = 0.1;
		m_flTimeWeaponIdle = 0.1;
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.1 );
		m_flNextSecondaryAttack = 0.1;
	}
	else if ( m_fInSpecialReload == 1 )
	{
		if ( m_flTimeWeaponIdle > 0 )
			return;

		m_fInSpecialReload = 2;
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usReloadShotgun, 0,
			(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );
		m_flTimeWeaponIdle = m_flNextReload = m_fReloadTime;
	}
	else
	{
		m_iClip += m_iShellsReloaded;
		m_pPlayer->ammo_shells -= m_iShellsReloaded;
		m_fInSpecialReload = 1;
		m_pPlayer->tfstate &= ~TFSTATE_RELOADING;
	}
}

void CTFShotgun::PrimaryAttack()
{
	if ( m_iClip <= 0 )
	{
		Reload();
		if ( m_iClip == 0 )
			PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireShotgun, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	m_pPlayer->FireBullets( 6, vecSrc, vecAiming, Vector( 0.04, 0.04, 0 ), 2048,
		BULLET_PLAYER_TF_BUCKSHOT, 0, 4 );
	DB_LogShots( 1 );

	if ( m_iClip == 0 && m_pPlayer->ammo_shells <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", 0, 0 );

	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_flTimeWeaponIdle = 5;
	m_fInSpecialReload = 0;
	m_pPlayer->tfstate &= ~TFSTATE_RELOADING;
	m_flPumpTime = 0.5;
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
}

void CTFSuperShotgun::PrimaryAttack()
{
	if ( m_iClip <= 0 )
	{
		Reload();
		if ( m_iClip == 0 )
			PlayEmptySound();
		return;
	}

	if ( m_iClip == 1 )
	{
		CTFShotgun::PrimaryAttack();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireSuperShotgun, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	m_pPlayer->FireBullets( 14, vecSrc, vecAiming, Vector( 0.04, 0.04, 0 ), 2048,
		BULLET_PLAYER_TF_BUCKSHOT, 4, 4 );
	DB_LogShots( 1 );

	if ( m_iClip == 0 && m_pPlayer->ammo_shells <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", 0, 0 );

	m_iClip -= 2;
	m_flTimeWeaponIdle = 5;
	m_fInSpecialReload = 0;
	m_flPumpTime = 0.7;
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.7 );
}

void CTFShotgun::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle >= 0 )
		return;

	if ( !m_iClip && !m_fInSpecialReload && m_pPlayer->ammo_shells >= m_iShellsReloaded )
	{
		Reload();
		return;
	}

	if ( m_fInSpecialReload )
	{
		if ( m_iClip == m_iMaxClipSize || m_pPlayer->ammo_shells < m_iShellsReloaded )
		{
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usPumpShotgun, 0,
				(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 1, 0 );
			m_fInSpecialReload = 0;
			m_flTimeWeaponIdle = 1.5;
		}
		else
		{
			Reload();
		}

		return;
	}

	int idle = UTIL_SharedRandomLong( m_pPlayer->random_seed, 0, 20 );
	if ( idle % 5 )
	{
		m_flTimeWeaponIdle = 5;
		SendWeaponAnim( 9, 1 );
	}
	else if ( idle % 20 )
	{
		m_flTimeWeaponIdle = 2.1052632f;
		SendWeaponAnim( 8, 1 );
	}
	else
	{
		m_flTimeWeaponIdle = 2.2222223f;
		SendWeaponAnim( 0, 1 );
	}
}

void CTFShotgun::ItemPostFrame()
{
	if ( m_flPumpTime != SHOTGUN_PUMP_IDLE && m_flPumpTime < 0 )
	{
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usPumpShotgun, 0,
			(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );
		m_flPumpTime = SHOTGUN_PUMP_IDLE;
	}

	CBasePlayerWeapon::ItemPostFrame();
}
