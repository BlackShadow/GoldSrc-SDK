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
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ammohistory.h"
#include "dod_shared.h"
#include "demo_api.h"
#include "in_buttons.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_WEAPON_SLOTS 5
#define MAX_WEAPON_POSITIONS 12

extern int g_iAlive;
extern int g_iDeadFlag;
extern bool ShowHudElement( int element );

WEAPON* gpActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
						// this points to the active weapon menu item
WEAPON* gpLastSel;		// Last weapon menu selection 
WeaponsResource gWR;
int g_weaponselect;
int g_iWeaponFlags;
float g_flWeaponHeat;
int giABHeight, giABWidth;
int giBucketHeight, giBucketWidth;

DECLARE_MESSAGE( m_Ammo, CurWeapon );	// Current weapon and clip
DECLARE_MESSAGE( m_Ammo, WeaponList );	// new weapon type
DECLARE_MESSAGE( m_Ammo, AmmoPickup );	// flashes an ammo pickup record
DECLARE_MESSAGE( m_Ammo, WeapPickup );    // flashes a weapon pickup record
DECLARE_MESSAGE( m_Ammo, ItemPickup );
DECLARE_MESSAGE( m_Ammo, HideWeapon );	// hides the weapon, ammo, and crosshair displays temporarily
DECLARE_MESSAGE( m_Ammo, AmmoX );			// update known ammo type's count
DECLARE_MESSAGE( m_Ammo, AmmoShort );
DECLARE_MESSAGE( m_Ammo, ReloadDone );
DECLARE_COMMAND( m_Ammo, Slot1 );
DECLARE_COMMAND( m_Ammo, Slot2 );
DECLARE_COMMAND( m_Ammo, Slot3 );
DECLARE_COMMAND( m_Ammo, Slot4 );
DECLARE_COMMAND( m_Ammo, Slot5 );
DECLARE_COMMAND( m_Ammo, Slot6 );
DECLARE_COMMAND( m_Ammo, Slot7 );
DECLARE_COMMAND( m_Ammo, Slot8 );
DECLARE_COMMAND( m_Ammo, Slot9 );
DECLARE_COMMAND( m_Ammo, Slot10 );
DECLARE_COMMAND( m_Ammo, Close );
DECLARE_COMMAND( m_Ammo, NextWeapon );
DECLARE_COMMAND( m_Ammo, PrevWeapon );

int CHudAmmo::Init()
{
	gHUD.AddHudElem( this );
	HOOK_MESSAGE( CurWeapon );
	HOOK_MESSAGE( WeaponList );
	HOOK_MESSAGE( AmmoPickup );
	HOOK_MESSAGE( WeapPickup );
	HOOK_MESSAGE( ItemPickup );
	HOOK_MESSAGE( HideWeapon );
	HOOK_MESSAGE( AmmoX );
	HOOK_MESSAGE( AmmoShort );
	HOOK_MESSAGE( ReloadDone );
	HOOK_COMMAND( "slot1", Slot1 );
	HOOK_COMMAND( "slot2", Slot2 );
	HOOK_COMMAND( "slot3", Slot3 );
	HOOK_COMMAND( "slot4", Slot4 );
	HOOK_COMMAND( "slot5", Slot5 );
	HOOK_COMMAND( "slot6", Slot6 );
	HOOK_COMMAND( "slot7", Slot7 );
	HOOK_COMMAND( "slot8", Slot8 );
	HOOK_COMMAND( "slot9", Slot9 );
	HOOK_COMMAND( "slot10", Slot10 );
	HOOK_COMMAND( "slot0", Slot10 );
	HOOK_COMMAND( "cancelselect", Close );
	HOOK_COMMAND( "invnext", NextWeapon );
	HOOK_COMMAND( "invprev", PrevWeapon );
	Reset();
	m_iFlags |= HUD_ACTIVE;
	memset( gWR.rgWeapons, 0, sizeof( gWR.rgWeapons ) );
	memset( gWR.riAmmo, 0, sizeof( gWR.riAmmo ) );
	gWR.iOldWeaponBits = gWR.iOldWeaponBits2 = 0;
	memset( gWR.rgSlots, 0, sizeof( gWR.rgSlots ) );
	return 1;
}

void CHudAmmo::Reset()
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE; //!!!
	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;
	gWR.iOldWeaponBits = gWR.iOldWeaponBits2 = 0;
	memset( gWR.rgSlots, 0, sizeof( gWR.rgSlots ) );
}

void CHudAmmo::PlayerDied()
{
	gHUD.m_fPlayerDead = TRUE;
	gpActiveSel = NULL;
}

void CHudAmmo::Think()
{
	if ( gHUD.m_fPlayerDead )
		return;
	int weapons = gHUD.m_iWeaponBits;
	int weapons2 = g_iWeaponBits2;
	if ( weapons != gWR.iOldWeaponBits || weapons2 != gWR.iOldWeaponBits2 )
	{
		memset( gWR.rgSlots, 0, MAX_WEAPON_SLOTS * MAX_WEAPON_POSITIONS * sizeof( WEAPON* ) );
		gWR.iOldWeaponBits = weapons;
		for ( int i = 31; i > 0; i-- )
		{
			WEAPON* weapon = &gWR.rgWeapons[i];
			if ( (weapons >> weapon->iId) & 1 )
				gWR.PickupWeapon( weapon );
		}
		gWR.iOldWeaponBits2 = weapons2;
		for ( int i = 0; i < 31; i++ )
		{
			if ( (weapons2 >> i) & 1 )
				gWR.PickupWeapon( &gWR.rgWeapons[i + 32] );
		}
	}
	if ( m_pWeapon )
		m_pWeapon->iLastWeaponState = g_iWeaponFlags;
	if ( gpActiveSel && (gHUD.m_iKeyBits & IN_ATTACK) )
	{
		if ( gpActiveSel != (WEAPON*)1 )
		{
			ServerCmd( gpActiveSel->szName );
			g_weaponselect = gpActiveSel->iId;
		}
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;
	}
}

int WeaponsResource::CountAmmo( int id )
{
	return id < 0 ? 0 : riAmmo[id];
}

int WeaponsResource::HasAmmo( WEAPON* )
{
	return TRUE;
}

void WeaponsResource::LoadWeaponSprites( WEAPON* weapon )
{
	if ( !weapon )
		return;
	memset( &weapon->rcActive, 0, sizeof( weapon->rcActive ) );
	memset( &weapon->rcInactive, 0, sizeof( weapon->rcInactive ) );
	memset( &weapon->rcAmmo, 0, sizeof( weapon->rcAmmo ) );
	memset( &weapon->rcAmmo2, 0, sizeof( weapon->rcAmmo2 ) );
	weapon->hActive = weapon->hInactive = weapon->hAmmo = weapon->hAmmo2 = 0;
	int sprite = gHUD.GetSpriteIndex( weapon->szName );
	weapon->hActive = gHUD.GetSprite( sprite );
	weapon->rcActive = gHUD.GetSpriteRect( sprite );
}

void WeaponsResource::LoadAllWeaponSprites()
{
	for ( int i = 0; i < 64; i++ )
	{
		if ( rgWeapons[i].iId )
			LoadWeaponSprites( &rgWeapons[i] );
	}
	WEAPON* weapons[] = { &scoped_fg42, &folding_carbine, &gravity_knife, &scoped_enfield,
		&brit_knife, &brit_grenade, &ger_binoculars };
	const char* names[] = { "weapon_scopedfg42", "weapon_fcarb", "weapon_paraknife", "weapon_scopedenfield",
		"weapon_britknife", "weapon_britgrenade", "weapon_gerbinoculars" };
	for ( int i = 0; i < 7; i++ )
	{
		int sprite = gHUD.GetSpriteIndex( names[i] );
		weapons[i]->hActive = gHUD.GetSprite( sprite );
		weapons[i]->rcActive = gHUD.GetSpriteRect( sprite );
	}
}

WEAPON* WeaponsResource::GetFirstPos( int slot )
{
	for ( int i = 0; i < MAX_WEAPON_POSITIONS; i++ )
	{
		if ( rgSlots[slot][i] )
			return rgSlots[slot][i];
	}
	return NULL;
}

WEAPON* WeaponsResource::GetNextActivePos( int slot, int position )
{
	while ( position <= 11 && slot <= 4 )
	{
		WEAPON* weapon = gWR.rgSlots[slot][++position];
		if ( weapon )
			return weapon;
	}
	return NULL;
}

HSPRITE* WeaponsResource::GetAmmoPicFromWeapon( int ammo, wrect_t& rect )
{
	for ( int i = 0; i < 64; i++ )
	{
		if ( rgWeapons[i].iAmmoType == ammo )
		{
			rect = rgWeapons[i].rcAmmo;
			return &rgWeapons[i].hAmmo;
		}
		if ( rgWeapons[i].iAmmo2Type == ammo )
		{
			rect = rgWeapons[i].rcAmmo2;
			return &rgWeapons[i].hAmmo2;
		}
	}
	return NULL;
}

void WeaponsResource::SelectSlot( int slot, int advance, int direction )
{
	if ( !advance && gHUD.m_Menu.m_fMenuDisplayed && direction == 1 )
	{
		gHUD.m_Menu.SelectMenuItem( slot + 1 );  // slots are one off the key numbers
		return;
	}
	if ( slot > 5 || gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & 5) )
		return;
	bool fast = gHUD.hud_fastswitch && gHUD.hud_fastswitch->value != 0;
	if ( (unsigned int)gpActiveSel > 1 && gpActiveSel->iSlot == slot )
	{
		WEAPON* weapon = GetNextActivePos( slot, gpActiveSel->iSlotPos );
		if ( !weapon )
			weapon = GetFirstPos( slot );
		gpActiveSel = weapon ? weapon : (WEAPON*)!fast;
		return;
	}
	WEAPON* weapon = GetFirstPos( slot );
	if ( !weapon )
		gpActiveSel = (WEAPON*)!fast;
	else if ( fast && !GetNextActivePos( weapon->iSlot, weapon->iSlotPos ) )
	{
		ServerCmd( weapon->szName );
		g_weaponselect = weapon->iId;
	}
	else
		gpActiveSel = weapon;
}

void CHudAmmo::SlotInput( int slot )
{
	gWR.SelectSlot( slot, FALSE, 1 );
}

void CHudAmmo::UserCmd_Slot1()
{
	SlotInput( 0 );
}

void CHudAmmo::UserCmd_Slot2()
{
	SlotInput( 1 );
}

void CHudAmmo::UserCmd_Slot3()
{
	SlotInput( 2 );
}

void CHudAmmo::UserCmd_Slot4()
{
	SlotInput( 3 );
}

void CHudAmmo::UserCmd_Slot5()
{
	SlotInput( 4 );
}

void CHudAmmo::UserCmd_Slot6()
{
	SlotInput( 5 );
}

void CHudAmmo::UserCmd_Slot7()
{
	SlotInput( 6 );
}

void CHudAmmo::UserCmd_Slot8()
{
	SlotInput( 7 );
}

void CHudAmmo::UserCmd_Slot9()
{
	SlotInput( 8 );
}

void CHudAmmo::UserCmd_Slot10()
{
	SlotInput( 9 );
}

void CHudAmmo::UserCmd_Close()
{
	if ( gHUD.m_Menu.m_fMenuDisplayed )
	{
		if ( gHUD.m_Menu.CanCancel )
		{
			gHUD.m_Menu.m_fMenuDisplayed = FALSE;
			gHUD.m_Menu.m_iFlags &= ~HUD_ACTIVE;
		}
	}
	else if ( gpActiveSel )
	{
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
	}
	else
		EngineClientCmd( "escape" );
}

void CHudAmmo::UserCmd_NextWeapon()
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & 5) )
		return;
	bool fast = gHUD.hud_fastswitch && gHUD.hud_fastswitch->value != 0;
	if ( (unsigned int)gpActiveSel <= 1 )
		gpActiveSel = m_pWeapon;
	int slot = gpActiveSel ? gpActiveSel->iSlot : 0;
	int position = gpActiveSel ? gpActiveSel->iSlotPos + 1 : 0;
	for ( int pass = 0; pass < 2; pass++, slot = 0 )
	{
		for ( ; slot < MAX_WEAPON_SLOTS; slot++, position = 0 )
		{
			for ( ; position < MAX_WEAPON_POSITIONS; position++ )
			{
				WEAPON* weapon = gWR.rgSlots[slot][position];
				if ( !weapon )
					continue;
				if ( fast )
				{
					g_weaponselect = weapon->iId;
					ServerCmd( weapon->szName );
					gpActiveSel = NULL;
				}
				else
					gpActiveSel = weapon;
				return;
			}
		}
	}
	gpActiveSel = NULL;
}

void CHudAmmo::UserCmd_PrevWeapon()
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & 5) )
		return;
	bool fast = gHUD.hud_fastswitch && gHUD.hud_fastswitch->value != 0;
	if ( (unsigned int)gpActiveSel <= 1 )
		gpActiveSel = m_pWeapon;
	int slot = gpActiveSel ? gpActiveSel->iSlot : 4;
	int position = gpActiveSel ? gpActiveSel->iSlotPos - 1 : 11;
	for ( int pass = 0; pass < 2; pass++, slot = 4 )
	{
		for ( ; slot >= 0; slot--, position = 11 )
		{
			for ( ; position >= 0; position-- )
			{
				WEAPON* weapon = gWR.rgSlots[slot][position];
				if ( !weapon )
					continue;
				if ( fast )
				{
					g_weaponselect = weapon->iId;
					ServerCmd( weapon->szName );
					gpActiveSel = NULL;
				}
				else
					gpActiveSel = weapon;
				return;
			}
		}
	}
	gpActiveSel = NULL;
}

int CHudAmmo::MsgFunc_AmmoX( const char*, int size, void* data )
{
	BEGIN_READ( data, size );
	int id = READ_BYTE();
	gWR.SetAmmo( id, abs( READ_BYTE() ) );
	return 1;
}

int CHudAmmo::MsgFunc_AmmoShort( const char*, int size, void* data )
{
	BEGIN_READ( data, size );
	int id = READ_BYTE();
	gWR.SetAmmo( id, abs( READ_SHORT() ) );
	return 1;
}

int CHudAmmo::MsgFunc_AmmoPickup( const char*, int size, void* data )
{
	BEGIN_READ( data, size );
	READ_BYTE();
	READ_BYTE();
	return 1;
}

int CHudAmmo::MsgFunc_WeapPickup( const char*, int size, void* data )
{
	BEGIN_READ( data, size );
	READ_BYTE();
	return 1;
}

int CHudAmmo::MsgFunc_ItemPickup( const char*, int size, void* data )
{
	BEGIN_READ( data, size );
	READ_STRING();
	return 1;
}

int CHudAmmo::MsgFunc_HideWeapon( const char*, int size, void* data )
{
	BEGIN_READ( data, size );
	gHUD.m_iHideHUDDisplay = READ_BYTE();
	if ( gHUD.m_iHideHUDDisplay & 5 )
		gpActiveSel = NULL;
	return 1;
}

int CHudAmmo::MsgFunc_ReloadDone( const char*, int size, void* data )
{
	BEGIN_READ( data, size );
	gHUD.m_bAutoReloadComplete = true;
	return 1;
}

int CHudAmmo::MsgFunc_CurWeapon( const char*, int size, void* data )
{
	BEGIN_READ( data, size );
	int state = READ_BYTE();
	int id = READ_CHAR();
	int clip = READ_BYTE();
	if ( id <= 0 )
	{
		static const wrect_t nullrc = { 0, 0, 0, 0 };
		SetCrosshair( 0, nullrc, 0, 0, 0 );
		return 0;
	}
	gHUD.m_fPlayerDead = FALSE;
	WEAPON* weapon = gWR.GetWeapon( id );
	weapon->iClip = clip < -1 ? -clip : clip;
	if ( !state )
		return 1;
	m_pWeapon = weapon;
	if ( !(gHUD.m_iHideHUDDisplay & 5) && gHUD.m_iFOV <= 89 )
	{
		if ( state > 1 && weapon->hZoomedAutoaim )
			SetCrosshair( weapon->hZoomedAutoaim, weapon->rcZoomedAutoaim, 255, 255, 255 );
		else
			SetCrosshair( weapon->hZoomedCrosshair, weapon->rcZoomedCrosshair, 255, 255, 255 );
	}
	m_iFlags |= HUD_ACTIVE;
	m_fFade = 200;
	return 1;
}

int CHudAmmo::MsgFunc_WeaponList( const char*, int size, void* data )
{
	BEGIN_READ( data, size );
	WEAPON weapon;
	weapon.iAmmoType = READ_CHAR();
	weapon.iMax1 = READ_BYTE();
	if ( weapon.iMax1 == 255 )
		weapon.iMax1 = -1;
	weapon.iAmmo2Type = READ_CHAR();
	weapon.iMax2 = READ_BYTE();
	if ( weapon.iMax2 == 255 )
		weapon.iMax2 = -1;
	weapon.iSlot = READ_CHAR();
	weapon.iSlotPos = READ_CHAR();
	weapon.iId = READ_SHORT();
	weapon.iFlags = READ_BYTE();
	weapon.iClip = 0;
	weapon.iClipMax = READ_BYTE();
	weapon.iLastWeaponState = 0;
	if ( (unsigned int)weapon.iId > 63 || (unsigned int)weapon.iSlot > 5 || (unsigned int)weapon.iSlotPos > 12 ||
		(unsigned int)(weapon.iAmmoType + 1) > 32 || (unsigned int)(weapon.iAmmo2Type + 1) > 32 ||
		(weapon.iAmmoType != -1 && !weapon.iMax1) || (weapon.iAmmo2Type != -1 && !weapon.iMax2) || weapon.iClipMax <= 0 )
		return 0;
	strncpy( weapon.szName, weaponnames[weapon.iId], sizeof( weapon.szName ) );
	weapon.szName[sizeof( weapon.szName ) - 1] = 0;
	gWR.rgWeapons[weapon.iId] = weapon;
	gWR.LoadWeaponSprites( &gWR.rgWeapons[weapon.iId] );
	return 1;
}

int CHudAmmo::GetCurrentWeaponId()
{
	return m_pWeapon ? m_pWeapon->iId : -1;
}

ClipInfo* CHudAmmo::GetCurrentGun( WEAPON* weapon )
{
	for ( int i = 0; i < 64; i++ )
	{
		if ( ClipInfoArray[i].weapon_id == weapon->iId )
			return &ClipInfoArray[i];
	}
	return NULL;
}

int CHudAmmo::DrawWList( float )
{
	return 0;
}

int CHudAmmo::VidInit()
{
	const int ids[28] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 13, 0, 17, 18, 20, 21, 22,
		23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };
	const char* full[28] =
	{
		"clip_colt_full", "clip_luger_full", "clip_garand_full", "clip_kar_full", "clip_tommy_full",
		"clip_mp44_full", "clip_spring_full", "clip_kar_full", "clip_bar_full", "clip_mp40_full",
		"clip_stick_full", "clip_grenade_full", NULL, "clip_mp40_full", "clip_mp40_full", "clip_m1carbine_full",
		"clip_mp40_full", "clip_grease_full", "clip_fg42_full", "clip_k43_full", "clip_enfield_full",
		"clip_sten_full", "clip_bren_full", "clip_webley_full", "clip_bazooka_full", "clip_pschreck_full",
		"clip_piat_full", "clip_mortar"
	};
	const char* empty[28] =
	{
		"clip_colt_empty", "clip_luger_empty", "clip_garand_empty", "clip_kar_empty", "clip_tommy_empty",
		"clip_mp44_empty", "clip_spring_empty", "clip_kar_empty", "clip_bar_empty", "clip_mp40_empty",
		NULL, NULL, NULL, "clip_mg42_empty", "clip_30cal_empty", "clip_m1carbine_empty", "clip_mg34_empty",
		"clip_grease_empty", "clip_fg42_empty", "clip_k43_empty", "clip_enfield_empty", "clip_sten_empty",
		"clip_bren_empty", "clip_webley_full", "clip_bazooka_empty", "clip_pschreck_empty", "clip_piat_empty", "clip_mortar"
	};
	const char* extra[28] =
	{
		"clip_colt_extra", "clip_luger_extra", "clip_garand_extra", "clip_kar_extra", "clip_tommy_extra",
		"clip_mp44_extra", "clip_spring_extra", "clip_kar_extra", "clip_bar_extra", "clip_mp40_extra",
		"clip_stick_extra", "clip_grenade_extra", NULL, "clip_mg42_extra", "clip_30cal_extra", "clip_m1carbine_extra",
		"clip_mg34_extra", "clip_grease_extra", "clip_fg42_extra", "clip_k43_extra", "clip_enfield_extra",
		"clip_sten_extra", "clip_bren_extra", "clip_webley_extra", "clip_bazooka_extra", "clip_pschreck_extra",
		"clip_piat_extra", "clip_mortar"
	};
	const float lastDrop[24] =
	{
		0.09803922f, 0.18907563f, 0.125f, 0.069767445f, 0.038f, 0.0671f, 0.069767445f, 0.069767445f,
		0.24347825f, 0.052f, 0, 0, 0, 0.023f, 0.023f, 0.15151516f, 0.023f, 0.045f, 0.24347825f,
		0.1388889f, 0.069767445f, 0.03846154f, 0.09375f, 0
	};
	const float subsequentDrop[24] =
	{
		0.10784314f, 0.0798f, 0.104166664f, 0.18604651f, 0.03076923f, 0.030075189f, 0.18604651f,
		0.18604651f, 0.034782607f, 0.030075189f, 0, 0, 0, 0.031f, 0.031f, 0.045454547f, 0.031f,
		0.030075189f, 0.034782607f, 0.074074075f, 0.18604651f, 0.03076923f, 0.020833334f, 0
	};
	for ( int i = 0; i < 28; i++ )
	{
		if ( !ids[i] )
			continue;
		ClipInfo& clip = ClipInfoArray[i];
		clip.weapon_id = ids[i];
		clip.full_index = gHUD.GetSpriteIndex( full[i] );
		if ( empty[i] )
			clip.empty_index = gHUD.GetSpriteIndex( empty[i] );
		clip.extra_index = gHUD.GetSpriteIndex( extra[i] );
		if ( i < 10 || (i >= 13 && i <= 23) )
		{
			clip.lastDrop = lastDrop[i];
			clip.subseqDrop = subsequentDrop[i];
		}
	}
	for ( int i = 0; i < 64; i++ )
	{
		ClipInfo& clip = ClipInfoArray[i];
		clip.FullSprite = gHUD.GetSprite( clip.full_index );
		clip.FullArea = &gHUD.GetSpriteRect( clip.full_index );
		clip.EmptySprite = gHUD.GetSprite( clip.empty_index );
		clip.EmptyArea = &gHUD.GetSpriteRect( clip.empty_index );
		clip.ExtraSprite = gHUD.GetSprite( clip.extra_index );
		clip.ExtraArea = &gHUD.GetSpriteRect( clip.extra_index );
	}
	BritGrenClipInfo.weapon_id = WEAPON_HANDGRENADE;
	BritGrenClipInfo.extra_index = gHUD.GetSpriteIndex( "clip_britgrenade_extra" );
	BritGrenClipInfo.ExtraSprite = gHUD.GetSprite( BritGrenClipInfo.extra_index );
	BritGrenClipInfo.ExtraArea = &gHUD.GetSpriteRect( BritGrenClipInfo.extra_index );
	int sprite = gHUD.GetSpriteIndex( "hud_barrel" );
	MGBarrelHUD = gHUD.GetSprite( sprite );
	MGBarrelHUDArea = &gHUD.GetSpriteRect( sprite );
	sprite = gHUD.GetSpriteIndex( "hud_barrelo" );
	MGBarrel2HUD = gHUD.GetSprite( sprite );
	MGBarrel2HUDArea = &gHUD.GetSpriteRect( sprite );
	giBucketWidth = giBucketHeight = 12;

	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();
	giABWidth = ScreenWidth < 640 ? 10 : 20;
	giABHeight = ScreenWidth < 640 ? 2 : 4;
	return 1;
}

int CHudAmmo::DrawWeaponList( float )
{
	if ( !gpActiveSel )
		return 0;
	int y = ScreenHeight / 2 - 150;
	for ( int slot = 0; slot < MAX_WEAPON_SLOTS; slot++ )
	{
		int drawn = 0;
		for ( int position = 0; position < MAX_WEAPON_POSITIONS; position++ )
		{
			WEAPON* weapon = gWR.rgSlots[slot][position];
			if ( !weapon || !weapon->iId )
				continue;
			y += 50;
			drawn++;
			int x = ScreenWidth - 5 - (weapon->rcActive.right - weapon->rcActive.left);
			int brightness = weapon == gpActiveSel ? 255 : 80;
			WEAPON* spriteWeapon = weapon;
			switch ( weapon->iId )
			{
			case WEAPON_FG42:
				if ( g_iWeaponFlags & 1 )
					spriteWeapon = &gWR.scoped_fg42;
				break;
			case WEAPON_M1CARBINE:
				if ( gHUD.m_bParatrooper )
					spriteWeapon = &gWR.folding_carbine;
				break;
			case WEAPON_GERKNIFE:
				if ( gHUD.m_bParatrooper )
					spriteWeapon = &gWR.gravity_knife;
				break;
			case WEAPON_ENFIELD:
				if ( weapon->iLastWeaponState & 1 )
					spriteWeapon = &gWR.scoped_enfield;
				break;
			case WEAPON_AMERKNIFE:
				if ( gHUD.m_bBritish )
					spriteWeapon = &gWR.brit_knife;
				break;
			case WEAPON_HANDGRENADE:
				if ( gHUD.m_bBritish )
					spriteWeapon = &gWR.brit_grenade;
				break;
			case WEAPON_BINOCULARS:
				if ( g_iTeamNumber == 2 )
					spriteWeapon = &gWR.ger_binoculars;
				break;
			}
			SPR_Set( spriteWeapon->hActive, brightness, brightness, brightness );
			SPR_DrawHoles( 0, x, y, &spriteWeapon->rcActive );
		}
		if ( !drawn )
			y += 50;
	}
	return 1;
}

int CHudAmmo::Draw( float time )
{
	if ( g_iVuser1z || (!gHUD.m_iWeaponBits && !g_iWeaponBits2) || (gHUD.m_iHideHUDDisplay & 5) ||
		g_iUser1 || g_iUser2 || !g_iAlive || g_iDeadFlag || !g_iTeamNumber || !g_iPlayerClass )
		return 1;
	DrawWeaponList( time );
	if ( !(m_iFlags & HUD_ACTIVE) || !m_pWeapon )
		return 0;
	WEAPON* weapon = m_pWeapon;
	gHUD.m_iClipSize = weapon->iClip;
	if ( gEngfuncs.pDemoAPI->IsPlayingback() )
		gHUD.g_iClip = weapon->iClip;
	if ( weapon->iAmmoType < 0 && weapon->iAmmo2Type < 0 )
		return 0;
	if ( m_fFade > 0 )
		m_fFade -= 20 * gHUD.m_flTimeDelta;
	ClipInfo* clip = GetCurrentGun( weapon );
	if ( !clip )
		return 0;
	int fontHeight = gHUD.m_iFontHeight;
	if ( m_pWeapon->iAmmoType > 0 )
	{
		wrect_t full = *clip->FullArea;
		wrect_t second = full;
		wrect_t* extraArea = clip->ExtraArea;
		int height = full.bottom - full.top;
		int width = full.right - full.left;
		int extraWidth = extraArea->right - extraArea->left;
		int extraHeight = extraArea->bottom - extraArea->top;
		int y = ScreenHeight - fontHeight - fontHeight / 2;
		int id = clip->weapon_id;
		char countText[16];
		if ( id == WEAPON_MG42 || id == WEAPON_30CAL || id == WEAPON_MG34 || id == WEAPON_WEBLEY )
		{
			int rounds = gHUD.g_iClip;
			wrect_t& zero = gHUD.GetSpriteRect( gHUD.m_MG_number_0 );
			int digitWidth = zero.right - zero.left;
			if ( ShowHudElement( 3 ) )
			{
				int x = ScreenWidth - 30 - (extraWidth + width);
				SPR_Set( clip->EmptySprite, 255, 255, 255 );
				SPR_DrawHoles( 0, x - 10, fontHeight + y - height, clip->EmptyArea );
				x += 26;
				if ( rounds > 99 )
				{
					int sprite = gHUD.m_MG_number_0 + rounds / 100;
					SPR_Set( gHUD.GetSprite( sprite ), 255, 255, 255 );
					SPR_DrawHoles( 0, x, y + 3, &gHUD.GetSpriteRect( sprite ) );
				}
				x += digitWidth;
				if ( rounds > 9 )
				{
					int sprite = gHUD.m_MG_number_0 + rounds % 100 / 10;
					SPR_Set( gHUD.GetSprite( sprite ), 255, 255, 255 );
					SPR_DrawHoles( 0, x, y + 3, &gHUD.GetSpriteRect( sprite ) );
				}
				x += digitWidth;
				int sprite = gHUD.m_MG_number_0 + rounds % 10;
				SPR_Set( gHUD.GetSprite( sprite ), 255, 255, 255 );
				SPR_DrawHoles( 0, x, y + 3, &gHUD.GetSpriteRect( sprite ) );
				int reserve = gWR.CountAmmo( weapon->iAmmoType ) / weapon->iClipMax;
				if ( reserve > 0 )
				{
					x = ScreenWidth - extraWidth - 15;
					SPR_Set( clip->ExtraSprite, 255, 255, 255 );
					SPR_DrawHoles( 0, x, fontHeight + y - extraHeight, clip->ExtraArea );
					sprintf( countText, "x %d", reserve );
					x += extraWidth / 2;
					gHUD.DrawHudString( x, y, x + 64, countText, 255, 255, 255 );
				}
			}
		}
		else if ( id == WEAPON_HANDGRENADE || id == WEAPON_STICKGRENADE || id == WEAPON_MORTAR )
		{
			HSPRITE extraSprite = clip->ExtraSprite;
			if ( id == WEAPON_HANDGRENADE && gHUD.m_bBritish )
			{
				extraSprite = BritGrenClipInfo.ExtraSprite;
				extraArea = BritGrenClipInfo.ExtraArea;
			}
			int count = gWR.CountAmmo( weapon->iAmmoType );
			if ( ShowHudElement( 3 ) && count > 0 )
			{
				int x = ScreenWidth - extraWidth - 30;
				SPR_Set( extraSprite, 255, 255, 255 );
				SPR_DrawHoles( 0, x, fontHeight + y - extraHeight, extraArea );
				sprintf( countText, "x %d", count );
				x += extraWidth;
				gHUD.DrawHudString( x, y, x + 64, countText, 255, 255, 255 );
			}
		}
		else if ( id >= WEAPON_BAZOOKA && id <= WEAPON_MORTAR )
		{
			int reserve = gWR.CountAmmo( weapon->iAmmoType );
			if ( ShowHudElement( 3 ) )
			{
				int x = ScreenWidth - (extraWidth + width) - 30;
				SPR_Set( clip->EmptySprite, 255, 255, 255 );
				SPR_DrawHoles( 0, x, fontHeight + y - height, clip->EmptyArea );
				if ( gHUD.g_iClip > 0 )
				{
					SPR_Set( clip->FullSprite, 255, 255, 255 );
					SPR_DrawHoles( 0, x, fontHeight + y - height, clip->FullArea );
				}
				if ( reserve > 0 )
				{
					x += clip->FullArea->right - clip->FullArea->left;
					SPR_Set( clip->ExtraSprite, 255, 255, 255 );
					SPR_DrawHoles( 0, x, fontHeight + y - extraHeight, clip->ExtraArea );
					sprintf( countText, "x %d", reserve );
					x += extraWidth;
					gHUD.DrawHudString( x, y, x + 64, countText, 255, 255, 255 );
				}
			}
		}
		else
		{
			float lastDrop = weapon->iClip > 0 ? clip->lastDrop : 0;
			if ( id == WEAPON_ENFIELD )
			{
				int rounds = min( gHUD.g_iClip, 5 );
				second = full;
				full.top += (1.0 - (rounds * clip->subseqDrop + lastDrop)) * height;
				second.top += (1.0 - (lastDrop + clip->subseqDrop * (gHUD.g_iClip - rounds))) * height;
			}
			else
				full.top += height * (1.0 - (lastDrop + gHUD.g_iClip * clip->subseqDrop));
			if ( ShowHudElement( 3 ) )
			{
				int x = ScreenWidth - (extraWidth + width) - 45;
				SPR_Set( clip->EmptySprite, 255, 255, 255 );
				SPR_DrawHoles( 0, x, fontHeight + y - height, clip->EmptyArea );
				if ( id == WEAPON_ENFIELD )
				{
					SPR_Set( clip->EmptySprite, 255, 255, 255 );
					SPR_DrawHoles( 0, x, fontHeight + y - height - (height - 2), clip->EmptyArea );
				}
				if ( full.bottom > full.top )
				{
					SPR_Set( clip->FullSprite, 255, 255, 255 );
					SPR_DrawHoles( 0, x, fontHeight + full.top + y - clip->EmptyArea->top - height, &full );
				}
				if ( id == WEAPON_ENFIELD && second.bottom > second.top )
				{
					SPR_Set( clip->FullSprite, 255, 255, 255 );
					SPR_DrawHoles( 0, x, fontHeight + second.top + y - clip->EmptyArea->top - height - (height - 2), &second );
				}
				int reserve = gWR.CountAmmo( weapon->iAmmoType ) / weapon->iClipMax;
				if ( reserve > 0 )
				{
					x = ScreenWidth - extraWidth - 45;
					SPR_Set( clip->ExtraSprite, 255, 255, 255 );
					SPR_DrawHoles( 0, x, fontHeight + y - extraHeight, clip->ExtraArea );
					sprintf( countText, "x %d", reserve );
					gHUD.DrawHudString( ScreenWidth - 45, y, ScreenWidth - 45 + 64, countText, 255, 255, 255 );
				}
			}
		}
	}
	if ( clip->weapon_id == WEAPON_MG42 )
	{
		wrect_t cold = *MGBarrelHUDArea;
		wrect_t hot = *MGBarrel2HUDArea;
		int height = cold.bottom - cold.top;
		int hotHeight = min( g_flWeaponHeat, 100.0f ) * height / 100.0;
		int y = (ScreenHeight - height) / 2;
		int x = ScreenWidth + MGBarrelHUDArea->left - MGBarrelHUDArea->right;
		hot.top = hot.bottom - hotHeight;
		cold.bottom -= hotHeight;
		SPR_Set( MGBarrelHUD, 255, 255, 255 );
		SPR_DrawHoles( 0, x, y, &cold );
		if ( hot.bottom - hot.top > 0 )
		{
			SPR_Set( MGBarrel2HUD, 255, 255, 255 );
			SPR_DrawHoles( 0, x, y + height - hotHeight, &hot );
		}
	}
	return 1;
}

client_sprite_t* GetSpriteList( client_sprite_t* sprites, const char* name, int resolution, int count )
{
	if ( !sprites || !count )
		return NULL;
	do
	{
		if ( !strcmp( name, sprites->szName ) && sprites->iRes == resolution )
			return sprites;
		sprites++;
	}
	while ( --count );
	return NULL;
}
