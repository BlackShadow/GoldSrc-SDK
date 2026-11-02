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
//
// ammohistory.h
//

// this is the max number of items in each bucket
#ifndef AMMOHISTORY_H
#define AMMOHISTORY_H

#include "ammo.h"

#define MAX_HUD_WEAPONS		32
#define MAX_WEAPON_POSITIONS	24
#define MAX_HISTORY			12

class WeaponsResource
{
private:
	WEAPON		rgWeapons[MAX_HUD_WEAPONS];
	WEAPON		rgAccessories[MAX_HUD_WEAPONS];

	// counts of weapons * ammo
	WEAPON*		rgSlots[MAX_WEAPON_SLOTS + 1][MAX_WEAPON_POSITIONS + 1];	// The slots currently in use by weapons.  The value is a pointer to the weapon;  if it's NULL, no weapon is there
	int			riAmmo[MAX_AMMO_TYPES];							// count of each ammo type

public:
	void Init();
	void Reset();

	int iOldWeaponBits;
	int iOldAccessoryBits;
	float m_hudOffTime;

	WEAPON* GetItem( int id, bool accessory ) { return accessory ? &rgAccessories[id] : &rgWeapons[id]; }
	WEAPON* GetWeapon( int id ) { return &rgWeapons[id]; }
	WEAPON* GetAccessory( int id ) { return &rgAccessories[id]; }
	void AddWeapon( WEAPON* weapon );
	void PickupWeapon( WEAPON* weapon ) { rgSlots[weapon->iSlot][weapon->iSlotPos] = weapon; }
	void DropWeapon( WEAPON* weapon ) { rgSlots[weapon->iSlot][weapon->iSlotPos] = NULL; }
	void DropAllWeapons();
	void AddAccessory( WEAPON* weapon );
	void PickupAccessory( WEAPON* weapon ) { rgSlots[weapon->iSlot][weapon->iSlotPos] = weapon; }
	void DropAccessory( WEAPON* weapon ) { rgSlots[weapon->iSlot][weapon->iSlotPos] = NULL; }
	void DropAllAccessories();
	WEAPON* GetWeaponSlot( int slot, int position ) { return rgSlots[slot][position]; }
	void LoadWeaponSprites( WEAPON* weapon );
	void LoadAllWeaponSprites();
	WEAPON* GetFirstPos( int slot );
	int GetNumberOfItemsInSlot( int slot );
	void SelectSlot( int slot, int advance, int direction );
	WEAPON* GetNextActivePos( int slot, int position );
	int HasAmmo( WEAPON* weapon );
	AMMO GetAmmo( int id ) { return id; }
	void SetAmmo( int id, int count ) { riAmmo[id] = count; }
	int CountAmmo( int id );
	HSPRITE* GetAmmoPicFromWeapon( int ammoId, wrect_t& rect, int accessory );
};

enum
{
	HISTSLOT_EMPTY,
	HISTSLOT_AMMO,
	HISTSLOT_WEAP,
	HISTSLOT_ITEM,
};

class HistoryResource
{
private:
	struct HIST_ITEM
	{
		int		type;
		float	DisplayTime;  // the time at which this item should be removed from the history
		int		iCount;
		int		iId;
		int		iIsAccessory;
	};

	HIST_ITEM rgAmmoHistory[MAX_HISTORY];

public:
	void Init() { Reset(); }
	void Reset() { memset( rgAmmoHistory, 0, sizeof( rgAmmoHistory ) ); }
	int iHistoryGap;
	int iCurrentHistorySlot;
	void AddToHistory( int type, int id, int accessory, int count );
	void AddToHistory( int type, const char* name, int count = 0 );
	void CheckClearHistory();
	int DrawAmmoHistory( float time );
};

extern WeaponsResource	gWR;
extern HistoryResource	gHR;

#endif
