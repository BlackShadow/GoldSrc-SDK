#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_gerknife, CGerKnife );

void CGerKnife::Spawn()
{
	CMeleeWeapon::Spawn( WEAPON_GERKNIFE );
}

void CGerKnife::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_GERKNIFE].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_GERKNIFE].wmodel );
	PRECACHE_MODEL( P_WpnInfo[35].vmodel );
	CMeleeWeapon::Precache();
}

int CGerKnife::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_GERKNIFE;
	p->iWeight = 0;
	return 1;
}

int CGerKnife::GetDrawAnim()
{
	return 3;
}

int CGerKnife::GetIdleAnim()
{
	return -1;
}

int CGerKnife::GetSlashAnim( int iSwing )
{
	int i_Anim = 1;

	switch ( ( iSwing++ ) % 2 )
	{
	case 0:
		i_Anim = 1;
		break;
	case 1:
		i_Anim = 2;
		break;
	}

	return i_Anim;
}
