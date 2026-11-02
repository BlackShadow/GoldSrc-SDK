#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

class CColtAmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn();
	BOOL AddAmmo( CBaseEntity* pOther );
};

LINK_ENTITY_TO_CLASS( weapon_colt, CCOLT );
LINK_ENTITY_TO_CLASS( ammo_colt, CColtAmmoClip );

void CCOLT::Spawn()
{
	CPistol::Spawn( WEAPON_COLT );
}

void CCOLT::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_COLT].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_COLT].wmodel );
	PRECACHE_SOUND( "weapons/colt_shoot.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/colt_reload_clipin.wav" );
	PRECACHE_SOUND( "weapons/colt_reload_clipout.wav" );
	PRECACHE_SOUND( "weapons/colt_draw_pullslide.wav" );
	m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/colt.sc" );
}

int CCOLT::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_12mm";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_COLT].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_COLT].ammo_maxclip;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 64;
	p->iId = m_iId = WEAPON_COLT;
	p->iWeight = P_WpnInfo[WEAPON_COLT].misc_weight;
	p->iBulletId = BULLET_PLAYER_COLT;
	p->flSpread = 0.035f;
	return TRUE;
}

int CCOLT::GetDrawAnim()
{
	return 5;
}

int CCOLT::GetIdleAnim()
{
	if ( m_iClip == 0 )
		return 7;
	else
		return 0;
}

int CCOLT::GetReloadAnim()
{
	if ( m_iClip == 0 )
		return 3;
	else
		return 4;
}

void CColtAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CColtAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_COLT].ammo_maxclip, "ammo_12mm", P_WpnInfo[WEAPON_COLT].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
