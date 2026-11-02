#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_amerknife, CAmerKnife );

void CAmerKnife::Spawn()
{
	CMeleeWeapon::Spawn( WEAPON_AMERKNIFE );
}

void CAmerKnife::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_AMERKNIFE].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_AMERKNIFE].wmodel );
	PRECACHE_MODEL( P_WpnInfo[40].vmodel );
	PRECACHE_MODEL( P_WpnInfo[40].wmodel );
	CMeleeWeapon::Precache();
}

int CAmerKnife::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_AMERKNIFE;
	p->iWeight = 0;
	return 1;
}

int CAmerKnife::GetDrawAnim()
{
	return 3;
}

int CAmerKnife::GetIdleAnim()
{
	return -1;
}

int CAmerKnife::GetSlashAnim( int iSwing )
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
