#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

LINK_ENTITY_TO_CLASS( tf_weapon_ng, CTFNailgun );
LINK_ENTITY_TO_CLASS( tf_weapon_superng, CTFSuperNailgun );

void CTFNailgun::Spawn()
{
	current_ammo = NULL;
	Precache();
	m_iId = 4;
	m_iDefaultAmmo = 50;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

void CTFNailgun::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_nailgun.mdl" );
	PRECACHE_MODEL( "models/v_tfc_supernailgun.mdl" );
	PRECACHE_MODEL( "models/p_nailgun.mdl" );
	PRECACHE_MODEL( "models/p_nailgun2.mdl" );
	PRECACHE_MODEL( "models/p_snailgun.mdl" );
	PRECACHE_MODEL( "models/p_snailgun2.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/clipinsert1.wav" );
	PRECACHE_SOUND( "items/cliprelease1.wav" );
	PRECACHE_SOUND( "weapons/airgun_1.wav" );
	PRECACHE_SOUND( "weapons/spike2.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	m_usFireNailGun = PRECACHE_EVENT( 1, "events/wpn/tf_nail.sc" );
}

void CTFSuperNailgun::Precache()
{
	CTFNailgun::Precache();
	m_usFireSuperNailGun = PRECACHE_EVENT( 1, "events/wpn/tf_snail.sc" );
}

int CTFNailgun::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_nails : 200;
	p->pszAmmo2 = NULL;
	p->iAmmo2 = -1;
	p->iMaxClip = 0;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iId = m_iId = 10;
	p->iFlags = 0;
	p->iWeight = 15;
	return 1;
}

int CTFSuperNailgun::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_nails : 150;
	p->pszAmmo2 = NULL;
	p->iAmmo2 = -1;
	p->iMaxClip = 0;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = 11;
	p->iFlags = 0;
	p->iWeight = 15;
	return 1;
}

int CTFNailgun::AddToPlayer( CBasePlayer* player )
{
	if ( !CBasePlayerWeapon::AddToPlayer( player ) )
		return FALSE;
	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, player->pev );
	WRITE_BYTE( m_iId );
	MESSAGE_END();
	current_ammo = &m_pPlayer->ammo_nails;
	return TRUE;
}

BOOL CTFNailgun::Deploy()
{
	return DefaultDeploy( "models/v_tfc_nailgun.mdl", "models/p_nailgun.mdl", 4, "mp5", 1 );
}

BOOL CTFSuperNailgun::Deploy()
{
	return DefaultDeploy( "models/v_tfc_supernailgun.mdl", "models/p_snailgun.mdl", 4, "mp5", 1 );
}

void CTFNailgun::SecondaryAttack()
{
}

void CTFNailgun::Reload()
{
}

void CTFNailgun::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= 0 )
	{
		m_flTimeWeaponIdle = 15;
		SendWeaponAnim( 1, 1 );
	}
}

void CTFNailgun::PrimaryAttack()
{
	if ( m_pPlayer->ammo_nails <= 0 )
	{
		PlayEmptySound();
		return;
	}
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireNailGun, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs
		+ gpGlobals->v_right * 2 + gpGlobals->v_up * -4;
	CTFNailgunNail::CreateNail( FALSE, vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this, TRUE );
	DB_LogShots( 1 );
	m_pPlayer->ammo_nails--;
	if ( m_pPlayer->ammo_nails < 0 )
		m_pPlayer->ammo_nails = 0;
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.1 );
	m_flTimeWeaponIdle = 10;
}

void CTFSuperNailgun::PrimaryAttack()
{
	if ( m_pPlayer->ammo_nails <= 0 )
	{
		PlayEmptySound();
		return;
	}
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireSuperNailGun, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs
		+ gpGlobals->v_right * 2 + gpGlobals->v_up * -4;
	if ( m_pPlayer->ammo_nails <= 3 )
		CTFNailgunNail::CreateNail( FALSE, vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this, TRUE );
	else
		CTFNailgunNail::CreateSuperNail( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this );
	DB_LogShots( 1 );
	m_pPlayer->ammo_nails -= 2;
	if ( m_pPlayer->ammo_nails < 0 )
		m_pPlayer->ammo_nails = 0;
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.1 );
	m_flTimeWeaponIdle = 10;
}
