#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"

LINK_ENTITY_TO_CLASS( tf_weapon_gl, CTFGrenadeLauncher );
LINK_ENTITY_TO_CLASS( tf_weapon_pl, CTFPipebombLauncher );

void CTFGrenadeLauncher::Spawn()
{
	current_ammo = NULL;
	Precache();
	m_iAnim_Holster = 10;
	m_iAnim_Idle = 0;
	m_iAnim_ReloadDown = 4;
	m_iAnim_ReloadUp = 5;
	m_iDefaultAmmo = 50;
	m_iId = 4;
	m_iAnim_Deploy = 8;
	m_fReloadTime = 0.6666667f;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

void CTFPipebombLauncher::Spawn()
{
	current_ammo = NULL;
	Precache();
	m_iId = 4;
	m_iDefaultAmmo = 50;
	m_fReloadTime = 0.6666667f;
	pev->solid = SOLID_TRIGGER;
	m_iAnim_Holster = 11;
	m_iAnim_Idle = 1;
	m_iAnim_ReloadDown = 6;
	m_iAnim_ReloadUp = 7;
	SetTouch( &CBasePlayerItem::DefaultTouch );
	m_iAnim_Deploy = 9;
	m_usFireGL = PRECACHE_EVENT( 1, "events/wpn/tf_pipel.sc" );
}

void CTFGrenadeLauncher::Precache()
{
	PRECACHE_MODEL( "models/v_tfgl.mdl" );
	PRECACHE_MODEL( "models/p_glauncher.mdl" );
	PRECACHE_MODEL( "models/p_glauncher2.mdl" );
	PRECACHE_MODEL( "models/pipebomb.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/clipinsert1.wav" );
	PRECACHE_SOUND( "items/cliprelease1.wav" );
	PRECACHE_SOUND( "weapons/hks1.wav" );
	PRECACHE_SOUND( "weapons/hks2.wav" );
	PRECACHE_SOUND( "weapons/hks3.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/glauncher2.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	m_usFireGL = PRECACHE_EVENT( 1, "events/wpn/tf_gl.sc" );
	PRECACHE_EVENT( 1, "events/wpn/tf_pipel.sc" );
}

int CTFGrenadeLauncher::GetItemInfo( ItemInfo* info )
{
	info->pszName = STRING( pev->classname );
	info->pszAmmo1 = "rockets";
	info->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_rockets : 50;
	info->pszAmmo2 = NULL;
	info->iAmmo2 = -1;
	info->iMaxClip = 6;
	info->iSlot = 3;
	info->iPosition = 3;
	info->iId = m_iId = 12;
	info->iFlags = 0;
	info->iWeight = 15;
	return 1;
}

int CTFPipebombLauncher::GetItemInfo( ItemInfo* info )
{
	info->pszName = STRING( pev->classname );
	info->pszAmmo1 = "rockets";
	info->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_rockets : 50;
	info->pszAmmo2 = NULL;
	info->iAmmo2 = -1;
	info->iMaxClip = 6;
	info->iSlot = 4;
	info->iPosition = 4;
	info->iId = m_iId = 22;
	info->iFlags = 0;
	info->iWeight = 15;
	return 1;
}

int CTFGrenadeLauncher::AddToPlayer( CBasePlayer* player )
{
	if ( !CBasePlayerWeapon::AddToPlayer( player ) )
		return 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, player->pev );
		WRITE_BYTE( m_iId );
	MESSAGE_END();
	current_ammo = &m_pPlayer->ammo_rockets;
	return 1;
}

BOOL CTFGrenadeLauncher::Deploy()
{
	m_iClip = m_pPlayer->m_iGLClip;
	return DefaultDeploy( "models/v_tfgl.mdl", "models/p_glauncher.mdl", m_iAnim_Deploy, "mp5", 1 );
}

void CTFGrenadeLauncher::Holster( int )
{
	m_pPlayer->m_iGLClip = m_iClip;
	m_pPlayer->tfstate &= ~TFSTATE_RELOADING;
	m_fInSpecialReload = 0;
	m_flTimeWeaponIdle = 1000;
	m_pPlayer->m_flNextAttack = 0.5f;
	SendWeaponAnim( m_iAnim_Holster, 1 );
}

void CTFGrenadeLauncher::PrimaryAttack()
{
	if ( m_iClip == 0 )
	{
		SendWeaponAnim( m_iAnim_Idle, 1 );
		m_flNextPrimaryAttack = GetNextAttackDelay( 1 );
		PlayEmptySound();
		m_fInSpecialReload = 0;
		m_pPlayer->tfstate &= ~TFSTATE_RELOADING;
		return;
	}
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireGL, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector up = gpGlobals->v_up * -16;
	Vector right = gpGlobals->v_right * 8;
	Vector origin = m_pPlayer->GetGunPosition();
	origin = right + origin + up;
	CTFGrenade::CreateTFGrenade( origin, m_pPlayer->pev->v_angle, m_pPlayer, this );
	DB_LogShots( 1 );
	m_fInSpecialReload = 0;
	m_pPlayer->tfstate &= ~TFSTATE_RELOADING;
	m_flNextSecondaryAttack = 0.6f;
	m_iClip--;
	m_flNextPrimaryAttack = 0.6f;
	m_flTimeWeaponIdle = 0.6f;
}

void CTFPipebombLauncher::PrimaryAttack()
{
	if ( m_iClip == 0 )
	{
		Reload();
		if ( m_pPlayer->ammo_rockets == 0 )
		{
			PlayEmptySound();
			SendWeaponAnim( m_iAnim_Idle, 1 );
			m_flNextPrimaryAttack = GetNextAttackDelay( 1 );
		}
		return;
	}
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireGL, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector up = gpGlobals->v_up * -16;
	Vector right = gpGlobals->v_right * 8;
	Vector origin = m_pPlayer->GetGunPosition();
	origin = right + origin + up;
	CTFGrenade::CreateTFPipebomb( origin, m_pPlayer->pev->v_angle, m_pPlayer, this );
	DB_LogShots( 1 );
	m_fInSpecialReload = 0;
	m_pPlayer->tfstate &= ~TFSTATE_RELOADING;
	m_iClip--;
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.6f );
	m_flTimeWeaponIdle = 0.6f;
}

void CTFGrenadeLauncher::SecondaryAttack()
{
}

void CTFGrenadeLauncher::Reload()
{
	if ( !m_pPlayer->ammo_rockets || m_iClip == 6 || !( m_flNextReload <= 0 ) ||
		!( m_flNextPrimaryAttack <= 0 ) )
		return;
	if ( !m_fInSpecialReload )
	{
		m_fInSpecialReload = 1;
		m_pPlayer->tfstate |= TFSTATE_RELOADING;
		SendWeaponAnim( m_iAnim_ReloadDown, 1 );
		m_pPlayer->m_flNextAttack = 0.1f;
		m_flTimeWeaponIdle = 0.1f;
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.1f );
		m_flNextSecondaryAttack = 0.1f;
	}
	else if ( m_fInSpecialReload == 1 )
	{
		if ( m_flTimeWeaponIdle <= 0 )
		{
			m_fInSpecialReload = 2;
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

void CTFGrenadeLauncher::WeaponIdle()
{
	ResetEmptySound();
	if ( !( m_flTimeWeaponIdle <= 0 ) )
		return;
	if ( m_fInSpecialReload )
	{
		if ( m_iClip == 6 || m_pPlayer->ammo_rockets <= 0 )
		{
			SendWeaponAnim( m_iAnim_ReloadUp, 1 );
			m_fInSpecialReload = 0;
			m_flTimeWeaponIdle = 1.5f;
		}
		else
			Reload();
	}
	else if ( !m_iClip && m_pPlayer->ammo_rockets > 0 )
		Reload();
	else
	{
		m_flTimeWeaponIdle = 3;
		SendWeaponAnim( m_iAnim_Idle, 1 );
	}
}
