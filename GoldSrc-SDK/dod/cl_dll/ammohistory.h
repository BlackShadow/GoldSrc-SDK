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

class WeaponsResource
{
public:
	void LoadWeaponSprites( WEAPON *weapon );
	void LoadAllWeaponSprites();
	WEAPON *GetFirstPos( int slot );
	void SelectSlot( int slot, int advance, int direction );
	WEAPON *GetNextActivePos( int slot, int position );
	int HasAmmo( WEAPON *weapon );
	int CountAmmo( int id );
	HSPRITE *GetAmmoPicFromWeapon( int ammoId, wrect_t &rect );
	WEAPON *GetWeapon( int id ) { return &rgWeapons[id]; }
	WEAPON *GetWeaponSlot( int slot, int position ) { return rgSlots[slot][position]; }
	void PickupWeapon( WEAPON *weapon ) { rgSlots[weapon->iSlot][weapon->iSlotPos] = weapon; }
	void DropWeapon( WEAPON *weapon ) { rgSlots[weapon->iSlot][weapon->iSlotPos] = NULL; }
	void SetAmmo( int id, int count ) { riAmmo[id] = count; }

	WEAPON rgWeapons[64];
	WEAPON *rgSlots[6][13];
	int riAmmo[32];
	WEAPON scoped_fg42;
	WEAPON folding_carbine;
	WEAPON gravity_knife;
	WEAPON scoped_enfield;
	WEAPON brit_knife;
	WEAPON brit_grenade;
	WEAPON ger_binoculars;

///// WEAPON /////
	int iOldWeaponBits;
	int iOldWeaponBits2;
};

extern WeaponsResource gWR;

#endif
