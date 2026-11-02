#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_mg34, CMG34 );

void CMG34::Spawn()
{
	CBipodWeapon::Spawn( WEAPON_MG34 );
}

void CMG34::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_MG34].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_MG34].wmodel );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipin.wav" );
	PRECACHE_SOUND( "weapons/tommy_reload_clipout.wav" );
	PRECACHE_SOUND( "weapons/clampdown.wav" );
	PRECACHE_SOUND( "weapons/mgbolt.wav" );
	PRECACHE_SOUND( "weapons/mg34_shoot.wav" );
	m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/mg34.sc" );
}

int CMG34::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_66mm";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_MG34].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_MG34].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON | ITEM_FLAG_NOAUTORELOAD;
	p->iId = m_iId = WEAPON_MG34;
	p->iWeight = P_WpnInfo[WEAPON_MG34].misc_weight;
	p->iBulletId = BULLET_PLAYER_MG34;
	p->flRecoil = 15;
	return TRUE;
}

int CMG34::Classify()
{
	return WEAPON_CLASS_MACHINEGUN;
}

int CMG34::GetReloadAnim()
{
	return 12;
}

int CMG34::GetDrawAnim()
{
	return GetDownToUpAnim();
}

int CMG34::GetIdleAnim()
{
	if ( IsDeployed() )
		return ( m_iClip <= 0 );
	else
		return ( m_iClip <= 0 ) + 6;
}

int CMG34::GetUpToDownAnim()
{
	return (m_iClip <= 0) + 8;
}

int CMG34::GetDownToUpAnim()
{
	return (m_iClip <= 0) + 2;
}

class CMG34AmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( ammo_mg34, CMG34AmmoClip );

void CMG34AmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CMG34AmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_MG34].ammo_maxclip, "ammo_66mm", P_WpnInfo[WEAPON_MG34].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
