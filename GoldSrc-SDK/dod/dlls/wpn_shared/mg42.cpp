#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_mg42, CMG42 );

void CMG42::Spawn()
{
	CBipodWeapon::Spawn( WEAPON_MG42 );
}

void CMG42::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_MG42].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_MG42].wmodel );

	PRECACHE_SOUND( "weapons/weaponempty.wav" );
	PRECACHE_SOUND( "weapons/chainpull.wav" );
	PRECACHE_SOUND( "weapons/bulletchain.wav" );
	PRECACHE_SOUND( "weapons/clampdown.wav" );
	PRECACHE_SOUND( "weapons/mgbolt.wav" );
	PRECACHE_SOUND( "weapons/mg42_shoot.wav" );
	PRECACHE_SOUND( "weapons/tommy_draw_slideback.wav" );
	PRECACHE_SOUND( "weapons/mgoverheat.wav" );

	m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/mg42.sc" );

	CBipodWeapon::Precache();
}

int CMG42::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_66mm_ger";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_MG42].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_MG42].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = WEAPON_FLAG_OVERHEAT | ITEM_FLAG_PRIMARYWEAPON | ITEM_FLAG_NOAUTORELOAD;
	p->iId = m_iId = WEAPON_MG42;
	p->iWeight = P_WpnInfo[WEAPON_MG42].misc_weight;
	p->iBulletId = BULLET_PLAYER_MG42;
	p->flRecoil = 15;

	return TRUE;
}

int CMG42::Classify()
{
	return WEAPON_CLASS_MACHINEGUN;
}

int CMG42::GetReloadAnim()
{
	return 58;
}

int CMG42::GetDrawAnim()
{
	return GetDownToUpAnim();
}

int CMG42::GetIdleAnim()
{
	return -1;
}

int CMG42::GetUpToDownAnim()
{
	int i_Anim = 30;

	switch ( m_iClip )
	{
	case 8:
		i_Anim = 31;
		break;
	case 7:
		i_Anim = 32;
		break;
	case 6:
		i_Anim = 33;
		break;
	case 5:
		i_Anim = 34;
		break;
	case 4:
		i_Anim = 35;
		break;
	case 3:
		i_Anim = 36;
		break;
	case 2:
		i_Anim = 37;
		break;
	case 1:
		i_Anim = 38;
		break;
	case 0:
		i_Anim = 39;
		break;
	default:
		i_Anim = 30;
		break;
	}

	return i_Anim;
}

int CMG42::GetDownToUpAnim()
{
	int i_Anim = 20;

	switch ( m_iClip )
	{
	case 8:
		i_Anim = 21;
		break;
	case 7:
		i_Anim = 22;
		break;
	case 6:
		i_Anim = 23;
		break;
	case 5:
		i_Anim = 24;
		break;
	case 4:
		i_Anim = 25;
		break;
	case 3:
		i_Anim = 26;
		break;
	case 2:
		i_Anim = 27;
		break;
	case 1:
		i_Anim = 28;
		break;
	case 0:
		i_Anim = 29;
		break;
	default:
		i_Anim = 20;
		break;
	}

	return i_Anim;
}

class CMG42AmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( ammo_mg42, CMG42AmmoClip );

void CMG42AmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/axis_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CMG42AmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_MG42].ammo_maxclip, "ammo_66mm_ger", P_WpnInfo[WEAPON_MG42].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
