#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

class CBARAmmoClip : public CBasePlayerAmmo
{
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( weapon_bar, CBAR );
LINK_ENTITY_TO_CLASS( ammo_bar, CBARAmmoClip );

void CBAR::Spawn()
{
	CBipodWeapon::Spawn( 11 );
}

void CBAR::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[11].vmodel );
	PRECACHE_MODEL( P_WpnInfo[11].wmodel );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/bar_shoot.wav" );
	m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/bar.sc" );
}

int CBAR::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_55mm";
	p->iMaxAmmo1 = P_WpnInfo[11].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[11].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = 11;
	p->iWeight = P_WpnInfo[11].misc_weight;
	p->iBulletId = BULLET_PLAYER_BAR;
	p->flRecoil = 2;
	return 1;
}

int CBAR::Classify()
{
	return 256;
}

int CBAR::GetDrawAnim()
{
	return 2;
}

int CBAR::GetUpToDownAnim()
{
	return 4;
}

int CBAR::GetDownToUpAnim()
{
	return 8;
}

int CBAR::GetIdleAnim()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return -1;
	if ( IsDeployed() )
		return 5;
	return 0;
}

int CBAR::GetReloadAnim()
{
	if ( IsDeployed() )
		return 6;
	else
		return 1;
}

void CBARAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CBARAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[11].ammo_maxclip, "ammo_55mm", P_WpnInfo[11].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
