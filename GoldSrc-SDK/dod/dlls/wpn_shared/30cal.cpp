#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_30cal, C30CAL );

void C30CAL::Spawn()
{
	CBipodWeapon::Spawn( WEAPON_30CAL );
}

void C30CAL::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_30CAL].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_30CAL].wmodel );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/chainpull.wav" );
	PRECACHE_SOUND( "weapons/bulletchain.wav" );
	PRECACHE_SOUND( "weapons/clampdown.wav" );
	PRECACHE_SOUND( "weapons/mgbolt.wav" );
	PRECACHE_SOUND( "weapons/30cal_shoot.wav" );
	PRECACHE_SOUND( "weapons/tommy_draw_slideback.wav" );
	m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/30cal.sc" );
}

int C30CAL::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ammo_66mm_us";
	p->iMaxAmmo1 = P_WpnInfo[WEAPON_30CAL].ammo_maxcarry;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P_WpnInfo[WEAPON_30CAL].ammo_maxclip;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_PRIMARYWEAPON | ITEM_FLAG_NOAUTORELOAD;
	p->iId = m_iId = WEAPON_30CAL;
	p->iWeight = P_WpnInfo[WEAPON_30CAL].misc_weight;
	p->iBulletId = BULLET_PLAYER_30CAL;
	p->flRecoil = 15;
	return TRUE;
}

int C30CAL::Classify()
{
	return WEAPON_CLASS_MACHINEGUN;
}

int C30CAL::GetReloadAnim()
{
	return 58;
}

int C30CAL::GetDrawAnim()
{
	return 20;
}

int C30CAL::GetIdleAnim()
{
	return -1;
}

int C30CAL::GetUpToDownAnim()
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

int C30CAL::GetDownToUpAnim()
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

class C30CALAmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

LINK_ENTITY_TO_CLASS( ammo_30cal, C30CALAmmoClip );

void C30CALAmmoClip::Spawn()
{
	SET_MODEL( ENT( pev ), "models/allied_ammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL C30CALAmmoClip::AddAmmo( CBaseEntity* pOther )
{
	int bResult = ( pOther->GiveAmmo( P_WpnInfo[WEAPON_30CAL].ammo_maxclip, "ammo_66mm_us", P_WpnInfo[WEAPON_30CAL].ammo_maxcarry ) != -1 );
	if ( bResult )
	{
		EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
	}
	return bResult;
}
