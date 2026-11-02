#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "../../cl_dll/dod_shared.h"

LINK_ENTITY_TO_CLASS( weapon_spade, CSpade );

void CSpade::Spawn()
{
	CMeleeWeapon::Spawn( WEAPON_SPADE );
}

void CSpade::Precache()
{
	PRECACHE_MODEL( P_WpnInfo[WEAPON_SPADE].vmodel );
	PRECACHE_MODEL( P_WpnInfo[WEAPON_SPADE].wmodel );
	CMeleeWeapon::Precache();
}

int CSpade::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = WEAPON_SPADE;
	p->iWeight = 0;
	return 1;
}

int CSpade::GetDrawAnim()
{
	return 3;
}

int CSpade::GetIdleAnim()
{
	return -1;
}

int CSpade::GetSlashAnim( int iSwing )
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
