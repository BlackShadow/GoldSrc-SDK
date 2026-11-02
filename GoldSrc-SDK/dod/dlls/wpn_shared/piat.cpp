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

class CPIATAmmo : public CBasePlayerAmmo
{
public:
	void Spawn();
	void Precache();
	BOOL AddAmmo( CBaseEntity* pOther );
};

LINK_ENTITY_TO_CLASS( weapon_piat, CPIAT );
LINK_ENTITY_TO_CLASS( ammo_piat, CPIATAmmo );

void CPIAT::Spawn()
{
	Precache();
	m_iId = WEAPON_PIAT;
	SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_PIAT].wmodel );
	m_iDefaultAmmo = P_WpnInfo[WEAPON_PIAT].ammo_default;
	FallInit();
	m_iClip = P_WpnInfo[WEAPON_PIAT].ammo_maxclip;
}

void CPIAT::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_PIAT].wmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_PIAT].vmodel );
	PRECACHE_SOUND( "items/ammopickup.wav" );
	PRECACHE_SOUND( "weapons/rocketfire1.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" );
	m_usFirePIAT = PRECACHE_EVENT( 1, "events/weapons/piat.sc" );
}

int CPIAT::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "rockets";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_PIAT].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 1;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_PIAT;
	p->iFlags = 642;
	p->iWeight = 20;
	return 1;
}

BOOL CPIAT::CanHolster()
{
	return !( m_iWeaponState & 1 );
}

BOOL CPIAT::Deploy()
{
	m_iWeaponState &= 1;
	int iAnim;

	if ( m_iClip == 1 )
		iAnim = 2;
	else
		iAnim = 3;

	return TimedDeploy( P_WpnInfo[WEAPON_PIAT].vmodel, P_WpnInfo[WEAPON_PIAT].pmodel, iAnim,
		P_WpnInfo[WEAPON_PIAT].szAnimExt, P_WpnInfo[WEAPON_PIAT].szAnimReloadExt,
		P_WpnInfo[WEAPON_PIAT].anim_drawtime, P_WpnInfo[WEAPON_PIAT].anim_drawtime, UseDecrement() != 0 );
}

void CPIAT::Holster( int skiplocal )
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

void CPIAT::ReSlow()
{
	m_iWeaponState = 1;
#ifdef CLIENT_DLL
	ClientSetSensitivity( 1 );
#else
	m_pPlayer->m_bBazookaDeployed = true;
	m_pPlayer->CheckPlayerSpeed();
#endif
}

void CPIAT::UnSlow()
{
	m_iWeaponState = 0;
#ifdef CLIENT_DLL
	ClientSetSensitivity( 0 );
#else
	m_pPlayer->m_bBazookaDeployed = false;
	m_pPlayer->CheckPlayerSpeed();
#endif
}

void CPIAT::Raise()
{
	SendWeaponAnim( m_iClip == 1 ? 7 : 8, 1 );
	ReSlow();
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PIAT].anim_down2uptime;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PIAT].anim_down2uptime;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PIAT].anim_down2uptime;
}

void CPIAT::Lower()
{
	SendWeaponAnim( m_iClip == 1 ? 9 : 10, 1 );
	UnSlow();
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PIAT].anim_up2downtime;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PIAT].anim_up2downtime;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PIAT].anim_up2downtime;
}

void CPIAT::SecondaryAttack()
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

void CPIAT::PrimaryAttack()
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
		CPIATShell* pRocket;
		pRocket = CPIATShell::CreateRocket( vecSrc, m_pPlayer->pev->v_angle, m_pPlayer );
#endif
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFirePIAT, 0,
			(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, m_iClip == 0, 0 );
		m_iClip--;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PIAT].anim_firedelay;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PIAT].anim_firedelay;
	}
	else
	{
#ifndef CLIENT_DLL
		UTIL_ShowMessage( "#game_shoulder_piat", m_pPlayer, true );
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PIAT].anim_firedelay;
	}
}

void CPIAT::Reload()
{
	if ( m_iClip > 0 )
		return;

	int iResult;

	if ( m_iWeaponState & 1 )
		iResult = DefaultReload( P_WpnInfo[WEAPON_PIAT].ammo_maxclip, 12, P_WpnInfo[WEAPON_PIAT].anim_reloadtime );
	else
		iResult = DefaultReload( P_WpnInfo[WEAPON_PIAT].ammo_maxclip, 11, P_WpnInfo[WEAPON_PIAT].anim_reloadtime );

	if ( iResult )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PIAT].anim_reloadtime;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_PIAT].anim_reloadtime;
		if ( m_iWeaponState & 1 )
		{
			UnSlow();
			m_iWeaponState = 2;
		}
	}
}

void CPIAT::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		if ( m_iWeaponState & 1 )
			SendWeaponAnim( m_iClip == 1 ? 4 : 5, 1 );
		else
			SendWeaponAnim( m_iClip == 1 ? 0 : 1, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4;
	}
}

void CPIATAmmo::Precache()
{
	PRECACHE_MODEL( "models/w_piat_rocket.mdl" );
}

void CPIATAmmo::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_piat_rocket.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CPIATAmmo::AddAmmo( CBaseEntity* pOther )
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
LINK_ENTITY_TO_CLASS( shell_piat, CPIATShell );

void CPIATShell::Precache()
{
	PRECACHE_MODEL( "models/w_piat_rocket.mdl" );
	PRECACHE_SOUND( "weapons/rocket2.wav" );
}

void CPIATShell::Spawn()
{
	Precache();
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/w_piat_rocket.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( this, pev->origin );
	pev->movetype = MOVETYPE_TOSS;
	pev->velocity = gpGlobals->v_forward * 1300;
	pev->gravity = 0.1f;
	pev->dmg = gSkillData.plrDmgPiatShell;
	pev->angles.x = -pev->angles.x;
	pev->angles.x -= 180;
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/rocket2.wav", 1, 0.5 );
	SetTouch( &CGrenade::RocketExplodeTouch );
	MAKE_STRING_CLASS( "shell_piat", pev );
}

void CPIATShell::RocketTouch( CBaseEntity* pOther )
{
	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket2.wav" );
	RocketExplodeTouch( pOther );
}

CPIATShell* CPIATShell::CreateRocket( Vector origin, Vector angles, CBaseEntity* pOwner )
{
	CPIATShell* pRocket = GetClassPtr( (CPIATShell*)NULL );
	UTIL_SetOrigin( pRocket, origin );
	pRocket->pev->angles = angles;
	pRocket->Spawn();
	pRocket->SetTouch( &CPIATShell::RocketTouch );
	pRocket->pev->owner = pOwner->edict();
	pRocket->pev->team = pOwner->pev->team;
	UTIL_SendDirectorMessage( pRocket, NULL, 135 );
	PLAYBACK_EVENT_FULL( FEV_GLOBAL, pRocket->edict(), ( (CDoDTeamPlay*)g_pGameRules )->StartTrail(), 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	return pRocket;
}
#endif
