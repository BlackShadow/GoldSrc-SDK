#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_kar, CKAR );

void CKAR::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_KAR].wmodel );
	m_iId = WEAPON_KAR;
	m_iDefaultAmmo = P_WpnInfo[WEAPON_KAR].ammo_default;
	FallInit();
}

void CKAR::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_KAR].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_KAR].wmodel );
	PRECACHE_SOUND( "weapons/kar_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/boltforward.wav" );
	PRECACHE_SOUND( "weapons/boltback.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );
	PRECACHE_SOUND( "weapons/knifeswing.wav" );
	PRECACHE_SOUND( "weapons/knifeswing2.wav" );
	PRECACHE_SOUND( "weapons/knife_slash1.wav" );
	PRECACHE_SOUND( "weapons/knife_slash2.wav" );
	PRECACHE_SOUND( "weapons/knife_hit1.wav" );
	PRECACHE_SOUND( "weapons/knife_hit2.wav" );
	PRECACHE_SOUND( "weapons/knife_hit3.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit2.wav" );
	m_usFireKar = PRECACHE_EVENT( 1, "events/weapons/kar.sc" );
}

int CKAR::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_16mm";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_KAR].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_KAR].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = WEAPON_KAR;
	p->iWeight = P_WpnInfo[WEAPON_KAR].misc_weight;
	return TRUE;
}

int CKAR::Classify()
{
	return 16;
}

BOOL CKAR::CanHolster()
{
	return TRUE;
}

int CKAR::AddToPlayer( CBasePlayer* pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		return TRUE;

	return FALSE;
}

BOOL CKAR::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_KAR].anim_drawtime;
	return DefaultDeploy( P_WpnInfo[WEAPON_KAR].vmodel, P_WpnInfo[WEAPON_KAR].pmodel, 3,
		P_WpnInfo[WEAPON_KAR].szAnimExt, P_WpnInfo[WEAPON_KAR].szAnimReloadExt, 0 );
}

void CKAR::Reload()
{
	DefaultReload( P_WpnInfo[WEAPON_KAR].ammo_maxclip, 2, P_WpnInfo[WEAPON_KAR].anim_reloadtime );
}

void CKAR::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		SendWeaponAnim( 0, 1 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

void CKAR::SecondaryAttack()
{
	RifleMeleeAttack( m_pPlayer, WEAPON_KAR );
	m_pPlayer->SetAnimation( PLAYER_ATTACK2 );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireKar, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, 0, 0, 1, 0, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.8;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.8;
}

void CKAR::PrimaryAttack()
{
	if ( PlayerIsWaterSniping() )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;
		return;
	}
	if ( m_fInAttack )
		return;
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
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_iClip--;
	float flSpread = P_WpnInfo[m_iId].base_accuracy;
	if ( m_pPlayer->pev->velocity.Length() > 45 )
		flSpread += P_WpnInfo[m_iId].accuracy_penalty;
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecNewShot = m_pPlayer->FireBulletsNC( vecSrc, vecAiming, flSpread, 8192,
		BULLET_PLAYER_KAR, 3, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireKar, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, vecNewShot.x, vecNewShot.y, 0, 0, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_KAR].anim_firedelay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + P_WpnInfo[WEAPON_KAR].anim_firedelay;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	m_fInAttack = TRUE;
#ifdef CLIENT_DLL
	flBoltHideXHair = P_WpnInfo[WEAPON_KAR].anim_firedelay;
#endif
	RemoveStamina( 2, m_pPlayer );
}

class CKarAmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( ammo_kar, CKarAmmoClip );

void CKarAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CKarAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_KAR].ammo_maxclip, "ammo_16mm", P_WpnInfo[WEAPON_KAR].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
