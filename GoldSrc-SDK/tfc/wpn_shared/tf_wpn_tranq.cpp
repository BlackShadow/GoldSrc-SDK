#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"

LINK_ENTITY_TO_CLASS( tf_weapon_tranq, CTFTranq );

void CTFTranq::Spawn()
{
	pev->classname = MAKE_STRING( "tf_weapon_tranq" );
	Precache();
	m_iId = 2;
	m_iDefaultAmmo = 17;
	pev->solid = SOLID_TRIGGER;
	SetTouch( &CBasePlayerItem::DefaultTouch );
}

void CTFTranq::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_pistol.mdl" );
	PRECACHE_MODEL( "models/p_spygun.mdl" );
	PRECACHE_MODEL( "models/p_9mmhandgun2.mdl" );
	PRECACHE_SOUND( "weapons/dartgun.wav" );
	m_usFireTranquilizer = PRECACHE_EVENT( 1, "events/wpn/tf_tranq.sc" );
}

int CTFTranq::GetItemInfo( ItemInfo* info )
{
	info->pszName = STRING( pev->classname );
	info->pszAmmo1 = "buckshot";
	info->iAmmo1 = m_pPlayer ? m_pPlayer->maxammo_shells : 40;
	info->pszAmmo2 = NULL;
	info->iAmmo2 = -1;
	info->iMaxClip = -1;
	info->iSlot = 1;
	info->iPosition = 4;
	info->iFlags = 0;
	info->iId = m_iId = 20;
	info->iWeight = 10;
	return 1;
}

BOOL CTFTranq::Deploy()
{
	pev->body = 1;
	BOOL deployed = DefaultDeploy( "models/v_tfc_pistol.mdl", "models/p_spygun.mdl", 7, "onehanded", 1 );
	if ( deployed )
		SendWeaponAnim( 7, 0 );
	return deployed;
}

void CTFTranq::PrimaryAttack()
{
	if ( m_pPlayer->ammo_shells <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.2f );
		return;
	}
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireTranquilizer, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	Vector origin = m_pPlayer->GetGunPosition();
	CTFNailgunNail::CreateTranqNail( origin, m_pPlayer->pev->v_angle, m_pPlayer, this );
	DB_LogShots( 1 );
	m_pPlayer->ammo_shells--;
	m_flNextPrimaryAttack = GetNextAttackDelay( 1.5f );
	m_flTimeWeaponIdle = 12.5f;
}

void CTFTranq::Reload()
{
}

void CTFTranq::WeaponIdle()
{
	ResetEmptySound();
	if ( !( m_flTimeWeaponIdle <= 0 ) )
		return;
	int animation;
	int choice = UTIL_SharedRandomLong( m_pPlayer->random_seed, 0, 3 );
	if ( choice % 3 == 0 )
	{
		m_flTimeWeaponIdle = 3.0625f;
		animation = 2;
	}
	else if ( ( choice + 1 ) % 3 == 0 )
	{
		m_flTimeWeaponIdle = 3.75f;
		animation = 0;
	}
	else
	{
		m_flTimeWeaponIdle = 2.5f;
		animation = 1;
	}
	SendWeaponAnim( animation, 1 );
}

void CTFNailgunNail::TranqTouch( CBaseEntity* other )
{
	if ( other->pev->takedamage )
	{
		CBaseEntity* owner = CBaseEntity::Instance( pev->owner );
		if ( other->Classify() == CLASS_PLAYER &&
			( !owner->IsAlly( other ) || !( (int)gpGlobals->teamplay & TEAMPLAY_TEAMDAMAGE ) ) )
		{
			if ( !( other->tfstate & TFSTATE_TRANQUILISED ) )
			{
				ClientPrint( other->pev, HUD_PRINTNOTIFY, "#Spy_tranq" );
				other->tfstate |= TFSTATE_TRANQUILISED;
#ifndef CLIENT_DLL
				const char* victimTeam = other->team_no ? GetTeamName( other->team_no ) : "SPECTATOR";
				const char* victimAuth = GETPLAYERAUTHID( other->edict() );
				int victimId = GETPLAYERUSERID( other->edict() );
				const char* victimName = STRING( other->pev->netname );
				const char* attackerTeam = owner->team_no ? GetTeamName( owner->team_no ) : "SPECTATOR";
				const char* attackerAuth = GETPLAYERAUTHID( owner->edict() );
				int attackerId = GETPLAYERUSERID( owner->edict() );
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Spy_Tranq\" against \"%s<%i><%s><%s>\"\n",
					STRING( owner->pev->netname ), attackerId, attackerAuth, attackerTeam,
					victimName, victimId, victimAuth, victimTeam );
#endif
				CBaseEntity* timer = other->CreateTimer( TF_TIMER_TRANQUILISATION );
				timer->pev->nextthink = gpGlobals->time + 15;
				timer->SetThink( &CBaseEntity::Timer_Tranquilisation );
				timer->team_no = owner->team_no;
				( (CBasePlayer*)other )->TeamFortress_SetSpeed();
			}
			else
			{
				CBaseEntity* timer = other->FindTimer( TF_TIMER_TRANQUILISATION );
				if ( timer )
					timer->pev->nextthink = gpGlobals->time + 15;
			}
		}
		other->TakeDamage( pev, owner->pev, 20, DMG_TRANQ | DMG_NAIL );
	}
	SUB_Remove();
}
