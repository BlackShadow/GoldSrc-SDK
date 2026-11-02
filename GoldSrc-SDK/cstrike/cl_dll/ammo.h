/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef __AMMO_H__
#define __AMMO_H__

#include "cl_dll.h"
#include "wrect.h"

enum ClientWeaponIdType
{
	ID_NONE = 0,
	ID_P228,
	ID_SHIELD,
	ID_SCOUT,
	ID_HEGRENADE,
	ID_XM1014,
	ID_C4,
	ID_MAC10,
	ID_AUG,
	ID_SMOKEGRENADE,
	ID_ELITES,
	ID_FIVESEVEN,
	ID_UMP45,
	ID_SG550,
	ID_GALIL,
	ID_FAMAS,
	ID_USP,
	ID_GLOCK18,
	ID_AWP,
	ID_MP5N,
	ID_M249,
	ID_M3,
	ID_M4A1,
	ID_TMP,
	ID_G3SG1,
	ID_FLASHBANG,
	ID_DEAGLE,
	ID_SG552,
	ID_AK47,
	ID_KNIFE,
	ID_P90,
	ID_MAX = ID_P90
};

#define MAX_WEAPON_NAME 128


#define WEAPON_FLAGS_SELECTONEMPTY	1

#define WEAPON_IS_ONTARGET 0x40

struct WEAPON
{
	char	szName[MAX_WEAPON_NAME];
	int		iAmmoType;
	int		iAmmo2Type;
	int		iMax1;
	int		iMax2;
	int		iSlot;
	int		iSlotPos;
	int		iFlags;
	int		iId;
	int		iClip;

	int		iCount;		// # of itesm in plist

	HSPRITE hActive;
	wrect_t rcActive;
	HSPRITE hInactive;
	wrect_t rcInactive;
	HSPRITE	hAmmo;
	wrect_t rcAmmo;
	HSPRITE hAmmo2;
	wrect_t rcAmmo2;
	HSPRITE hCrosshair;
	wrect_t rcCrosshair;
	HSPRITE hAutoaim;
	wrect_t rcAutoaim;
	HSPRITE hZoomedCrosshair;
	wrect_t rcZoomedCrosshair;
	HSPRITE hZoomedAutoaim;
	wrect_t rcZoomedAutoaim;
};

typedef int AMMO;

extern int g_iPlayerFlags;
extern int g_iWeaponFlags;
extern float g_flPlayerSpeed;


#endif
