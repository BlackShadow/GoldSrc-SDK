#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

class CBRENAmmoClip : public CBasePlayerAmmo
{
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( weapon_bren, CBREN );
LINK_ENTITY_TO_CLASS( ammo_bren, CBRENAmmoClip );

void CBREN::Spawn()
{
	CBipodWeapon::Spawn( 27 );
}

void CBREN::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[27].vmodel );
	PRECACHE_MODEL( P_WpnInfo[27].wmodel );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/bren_shoot.wav" );
	m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/bren.sc" );
}

int CBREN::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_55mm";
	p->iMaxAmmo1 = P_WpnInfo[27].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[27].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON;
	p->iId = m_iId = 27;
	p->iWeight = P_WpnInfo[27].misc_weight;
	p->iBulletId = BULLET_PLAYER_BREN;
	p->flRecoil = 2;
	return 1;
}

int CBREN::Classify()
{
	return 256;
}

int CBREN::GetDrawAnim()
{
	return 2;
}

int CBREN::GetUpToDownAnim()
{
	return 4;
}

int CBREN::GetDownToUpAnim()
{
	return 8;
}

int CBREN::GetIdleAnim()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return -1;
	if ( IsDeployed() )
		return 5;
	return 0;
}

int CBREN::GetReloadAnim()
{
	if ( IsDeployed() )
		return 6;
	else
		return 1;
}

void CBRENAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CBRENAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[27].ammo_maxclip, "ammo_55mm", P_WpnInfo[27].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
