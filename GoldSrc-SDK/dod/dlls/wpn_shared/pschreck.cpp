#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "../../cl_dll/dod_shared.h"

#ifndef CLIENT_DLL
extern int gmsgResetSens;
#endif

class CPschreckAmmo : public CBasePlayerAmmo
{
public:
	void Spawn();
	void Precache();
	BOOL AddAmmo( CBaseEntity* pOther );
};

LINK_ENTITY_TO_CLASS( weapon_pschreck, CPschreck );
LINK_ENTITY_TO_CLASS( ammo_pschreck, CPschreckAmmo );

void CPschreck::Spawn()
{
	Precache();
	m_iId = WEAPON_PSCHRECK;
	SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_PSCHRECK].wmodel );
	m_iDefaultAmmo = P_WpnInfo[WEAPON_PSCHRECK].ammo_default;
	FallInit();
	m_iClip = P_WpnInfo[WEAPON_PSCHRECK].ammo_maxclip;
}

void CPschreck::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_PSCHRECK].wmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_PSCHRECK].vmodel );
	PRECACHE_SOUND( "items/ammopickup.wav" );
	PRECACHE_SOUND( "weapons/rocketfire1.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" );
	m_usFirePschreck = PRECACHE_EVENT( 1, "events/weapons/pschreck.sc" );
}

int CPschreck::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "rockets";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_PSCHRECK].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 1;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_PSCHRECK;
	p->iFlags = 642;
	p->iWeight = 20;
	return 1;
}

BOOL CPschreck::CanHolster()
{
	return !( m_iWeaponState & 1 );
}

BOOL CPschreck::Deploy()
{
	m_iWeaponState &= 1;
	return TimedDeploy( P_WpnInfo[WEAPON_PSCHRECK].vmodel, P_WpnInfo[WEAPON_PSCHRECK].pmodel, 1,
		P_WpnInfo[WEAPON_PSCHRECK].szAnimExt, P_WpnInfo[WEAPON_PSCHRECK].szAnimReloadExt,
		P_WpnInfo[WEAPON_PSCHRECK].anim_drawtime, P_WpnInfo[WEAPON_PSCHRECK].anim_drawtime, UseDecrement() != 0 );
}

void CPschreck::Holster( int skiplocal )
{
#ifdef CLIENT_DLL
	ClientSetSensitivity( 0 );
#else
	m_pPlayer->m_bBazookaDeployed = false;
	m_pPlayer->CheckPlayerSpeed();
	MESSAGE_BEGIN( MSG_ONE, gmsgResetSens, NULL, m_pPlayer->pev );
	MESSAGE_END();
#endif
	m_iWeaponState &= ~3;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	CBasePlayerWeapon::Holster( skiplocal );
}

void CPschreck::ReSlow()
{
	m_iWeaponState = 1;
#ifdef CLIENT_DLL
	ClientSetSensitivity( 1 );
#else
	m_pPlayer->m_bBazookaDeployed = true;
	m_pPlayer->CheckPlayerSpeed();
#endif
}

void CPschreck::UnSlow()
{
	m_iWeaponState = 0;
#ifdef CLIENT_DLL
	ClientSetSensitivity( 0 );
#else
	m_pPlayer->m_bBazookaDeployed = false;
	m_pPlayer->CheckPlayerSpeed();
#endif
}

void CPschreck::Raise()
{
	SendWeaponAnim( 4, 1 );
#ifndef CLIENT_DLL
	m_pPlayer->m_bBazookaDeployed = true;
	m_pPlayer->CheckPlayerSpeed();
#endif
	m_iWeaponState |= 1;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_down2uptime;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_down2uptime;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_down2uptime;
}

void CPschreck::Lower()
{
	SendWeaponAnim( 5, 1 );
#ifndef CLIENT_DLL
	m_pPlayer->m_bBazookaDeployed = false;
	m_pPlayer->CheckPlayerSpeed();
#endif
	m_iWeaponState &= ~1;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_up2downtime;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_up2downtime;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_up2downtime;
}

void CPschreck::SecondaryAttack()
{
	if ( m_pPlayer->pev->waterlevel > 1 )
	{
		if ( m_iWeaponState & 1 )
			Lower();
	}
	else
	{
		if ( m_iWeaponState & 1 )
			Lower();
		else
			Raise();
	}
}

void CPschreck::PrimaryAttack()
{
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
	if ( m_pPlayer->pev->waterlevel > 1 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
		return;
	}
	if ( m_iWeaponState & 1 )
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 32;
		CPschreckShell* pRocket;
		pRocket = CPschreckShell::CreateRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer );
#endif
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFirePschreck, 0,
			(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, m_iClip == 0, 0 );
		m_iClip--;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_firedelay;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.17;
	}
	else
	{
#ifndef CLIENT_DLL
		UTIL_ShowMessage( "#game_shoulder_pschreck", m_pPlayer, true );
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_firedelay;
	}
}

void CPschreck::Reload()
{
	if ( m_iClip > 0 )
		return;

	int iResult;

	if ( m_iWeaponState & 1 )
	{
		iResult = DefaultReload( P_WpnInfo[WEAPON_PSCHRECK].ammo_maxclip, 6, P_WpnInfo[WEAPON_PSCHRECK].anim_reloadtime );
		if ( iResult )
		{
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_reloadtime;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_reloadtime;
		}
	}
	else
	{
		iResult = DefaultReload( P_WpnInfo[WEAPON_PSCHRECK].ammo_maxclip, 7, P_WpnInfo[WEAPON_PSCHRECK].anim_reloadtime2 );
		if ( iResult )
		{
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_reloadtime2;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PSCHRECK].anim_reloadtime2;
		}
	}

	if ( iResult )
	{
		if ( m_iWeaponState & 1 )
		{
			UnSlow();
			m_iWeaponState = 2;
		}
	}
}

void CPschreck::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		if ( m_iWeaponState & 1 )
			SendWeaponAnim( 2, 1 );
		else
			SendWeaponAnim( 0, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4;
	}
}

void CPschreckAmmo::Precache()
{
	PRECACHE_MODEL( "models/w_pschreck_rocket.mdl" );
}

void CPschreckAmmo::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_pschreck_rocket.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CPschreckAmmo::AddAmmo( CBaseEntity* pOther )
{
	int iGive = 2;

	if ( pOther->GiveAmmo( iGive, "rockets", 5 ) != -1 )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
		return TRUE;
	}
	return FALSE;
}

#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS( shell_pschreck, CPschreckShell );

void CPschreckShell::Precache()
{
	PRECACHE_MODEL( "models/w_pschreck_rocket.mdl" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
}

void CPschreckShell::Spawn()
{
	Precache();
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/w_pschreck_rocket.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( this, pev->origin );
	pev->movetype = MOVETYPE_TOSS;
	pev->velocity = gpGlobals->v_forward * 1300;
	pev->gravity = 0.1f;
	pev->dmg = gSkillData.plrDmgPschreckShell;
	pev->angles.x = -pev->angles.x;
	pev->angles.x -= 180;
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );
	SetTouch( &CGrenade::RocketExplodeTouch );
	MAKE_STRING_CLASS( "shell_pschreck", pev );
}

void CPschreckShell::RocketTouch( CBaseEntity* pOther )
{
	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav" );
	RocketExplodeTouch( pOther );
}

CPschreckShell* CPschreckShell::CreateRocket( Vector origin, Vector angles, CBaseEntity* pOwner )
{
	CPschreckShell* pRocket = GetClassPtr( (CPschreckShell*)NULL );
	UTIL_SetOrigin( pRocket, origin );
	pRocket->pev->angles = angles;
	pRocket->Spawn();
	pRocket->SetTouch( &CPschreckShell::RocketTouch );
	pRocket->pev->owner = pOwner->edict();
	pRocket->pev->team = pOwner->pev->team;
	UTIL_SendDirectorMessage( pRocket, NULL, 135 );
	PLAYBACK_EVENT_FULL( FEV_GLOBAL, pRocket->edict(), ( (CDoDTeamPlay*)g_pGameRules )->StartTrail(), 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	return pRocket;
}
#endif
