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
#include "pm_shared.h"

DECLARE_MESSAGE( m_Ammo, CurWeapon )
DECLARE_MESSAGE( m_Ammo, WeaponList )
DECLARE_MESSAGE( m_Ammo, AmmoPickup )
DECLARE_MESSAGE( m_Ammo, WeapPickup )
DECLARE_MESSAGE( m_Ammo, ItemPickup )
DECLARE_MESSAGE( m_Ammo, HideWeapon )
DECLARE_MESSAGE( m_Ammo, AmmoX )
DECLARE_MESSAGE( m_Ammo, AccesBits )

DECLARE_COMMAND( m_Ammo, Slot1 )
DECLARE_COMMAND( m_Ammo, Slot2 )
DECLARE_COMMAND( m_Ammo, Slot3 )
DECLARE_COMMAND( m_Ammo, Slot4 )
DECLARE_COMMAND( m_Ammo, Slot5 )
DECLARE_COMMAND( m_Ammo, Slot6 )
DECLARE_COMMAND( m_Ammo, Slot7 )
DECLARE_COMMAND( m_Ammo, Slot8 )
DECLARE_COMMAND( m_Ammo, Slot9 )
DECLARE_COMMAND( m_Ammo, Slot10 )
DECLARE_COMMAND( m_Ammo, Close )
DECLARE_COMMAND( m_Ammo, NextWeapon )
DECLARE_COMMAND( m_Ammo, PrevWeapon )

WeaponsResource gWR;
WEAPON* gpActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
						// this points to the active weapon menu item
WEAPON* gpLastSel;		// Last weapon menu selection 
int g_weaponselect;
int g_iShotsFired;
int g_iPlayerFlags;
int g_iWeaponFlags;
float g_flPlayerSpeed;
int giBucketHeight;
int giBucketWidth;
int giABHeight;
int giABWidth;
HSPRITE ghsprBuckets;					// Sprite for top row of weapons menu
cvar_t* cl_dynamiccrosshair;

extern "C" void EV_HolsterBlowtorch( struct event_args_s* args );

void WeaponsResource::Init()
{
	memset( rgWeapons, 0, sizeof( rgWeapons ) );
	memset( rgAccessories, 0, sizeof( rgAccessories ) );
	Reset();
}

void WeaponsResource::Reset()
{
	iOldWeaponBits = 0;
	iOldAccessoryBits = 0;
	memset( rgSlots, 0, sizeof( rgSlots ) );
	memset( riAmmo, 0, sizeof( riAmmo ) );
	m_hudOffTime = 0;
}

void WeaponsResource::AddWeapon( WEAPON* weapon )
{
	rgWeapons[weapon->iId] = *weapon;
	LoadWeaponSprites( &rgWeapons[weapon->iId] );
}

void WeaponsResource::AddAccessory( WEAPON* weapon )
{
	rgAccessories[weapon->iId] = *weapon;
	LoadWeaponSprites( &rgAccessories[weapon->iId] );
}

void WeaponsResource::DropAllWeapons()
{
	for ( int i = 0; i < MAX_HUD_WEAPONS; ++i )
	{
		if ( rgWeapons[i].iId )
			DropWeapon( &rgWeapons[i] );
	}
}

void WeaponsResource::DropAllAccessories()
{
	for ( int i = 0; i < MAX_HUD_WEAPONS; ++i )
	{
		if ( rgAccessories[i].iId )
			DropAccessory( &rgAccessories[i] );
	}
}

int CountAmmo( int ammoIndex )
{
	return gWR.CountAmmo( ammoIndex );
}

int CountClipAmmo( int id, bool accessory )
{
	return gWR.GetItem( id, accessory )->iClip;
}

int WeaponsResource::CountAmmo( int id )
{
	return id < 0 ? 0 : riAmmo[id];
}

int WeaponsResource::HasAmmo( WEAPON* weapon )
{
	if ( !weapon )
		return FALSE;
	if ( weapon->iMax1 == -1 ||
		( weapon->bIsAccessory && weapon->iId == ACCESSORY_RADIOCONTROLLEDBOMB ) )
		return TRUE;
	return weapon->iAmmoType == -1 || weapon->iClip > 0 || CountAmmo( weapon->iAmmoType ) ||
		CountAmmo( weapon->iAmmo2Type ) || ( weapon->iFlags & WEAPON_FLAGS_SELECTONEMPTY );
}

WEAPON* WeaponsResource::GetFirstPos( int slot )
{
	for ( int position = 0; position < MAX_WEAPON_POSITIONS; ++position )
	{
		if ( rgSlots[slot][position] )
			return rgSlots[slot][position];
	}
	return NULL;
}

WEAPON* WeaponsResource::GetNextActivePos( int slot, int position )
{
	while ( position <= MAX_WEAPON_POSITIONS - 1 && slot <= MAX_WEAPON_SLOTS - 1 )
	{
		WEAPON* weapon = gWR.GetWeaponSlot( slot, ++position );
		if ( weapon && gWR.HasAmmo( weapon ) )
			return weapon;
	}
	return NULL;
}

int WeaponsResource::GetNumberOfItemsInSlot( int slot )
{
	int count = 0;
	for ( int i = 0; i < MAX_WEAPON_POSITIONS; ++i )
	{
		if ( rgSlots[slot][i] )
			++count;
	}
	return count;
}

void WeaponsResource::LoadAllWeaponSprites()
{
	for ( int i = 0; i < MAX_HUD_WEAPONS; ++i )
	{
		if ( rgWeapons[i].iId )
			LoadWeaponSprites( &rgWeapons[i] );
	}
	for ( int i = 0; i < MAX_HUD_WEAPONS; ++i )
	{
		if ( rgAccessories[i].iId )
			LoadWeaponSprites( &rgAccessories[i] );
	}
}

HSPRITE* WeaponsResource::GetAmmoPicFromWeapon( int ammoId, wrect_t& rect, int accessory )
{
	WEAPON* weapons = accessory ? rgAccessories : rgWeapons;
	for ( int i = 0; i < MAX_HUD_WEAPONS; ++i )
	{
		if ( weapons[i].iAmmoType == ammoId )
		{
			rect = weapons[i].rcAmmo;
			return &weapons[i].hAmmo;
		}
		if ( weapons[i].iAmmo2Type == ammoId )
		{
			rect = weapons[i].rcAmmo2;
			return &weapons[i].hAmmo2;
		}
	}
	return NULL;
}

client_sprite_t* GetSpriteList( client_sprite_t* list, const char* name, int resolution, int count )
{
	if ( !list || !count )
		return NULL;
	while ( strcmp( name, list->szName ) || list->iRes != resolution )
	{
		++list;
		if ( !--count )
			return NULL;
	}
	return list;
}

int GetWeaponMaxSpeed( int weaponId )
{
	switch ( weaponId )
	{
	case 5:
	case 14:
	case 16:
	case 20:
	case 21:
		return 140;
	case 22:
		return 170;
	default:
		return 0;
	}
}

int GetWeaponAccuracyFlags( int weaponId )
{
	switch ( weaponId )
	{
	case 1:
	case 7:
	case 19:
		return 7;
	case 4:
	case 8:
	case 12:
	case 17:
		return 1;
	case 5:
	case 14:
	case 20:
	case 21:
	case 22:
		return 3;
	case 10:
		return ( g_iWeaponFlags & 1 ) ? 15 : 7;
	case 11:
		return ( g_iWeaponFlags & 2 ) ? 23 : 7;
	case 16:
		return ( g_iWeaponFlags & 4 ) ? 11 : 3;
	default:
		return 0;
	}
}

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
	HOOK_MESSAGE( AccesBits );
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
	HOOK_COMMAND( "cancelselect", Close );
	HOOK_COMMAND( "invnext", NextWeapon );
	HOOK_COMMAND( "invprev", PrevWeapon );
	Reset();
	CVAR_CREATE( "hud_drawhistory_time", "5", 0 );
	CVAR_CREATE( "hud_fastswitch", "0", FCVAR_ARCHIVE );		// controls whether or not weapons can be selected in one keypress
	m_iFlags |= HUD_ACTIVE;
	m_R = 50;
	m_G = 250;
	m_B = 50;
	m_iCurrentCrosshair = 0;
	gWR.Init();
	gHR.Init();
	return 1;
}

void CHudAmmo::Reset()
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE;
	m_pWeapon = NULL;
	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;
	gWR.Reset();
	gHR.Reset();
}

void CHudAmmo::Think()
{
	if ( gHUD.m_fPlayerDead )
		return;
	if ( gHUD.m_iWeaponBits != gWR.iOldWeaponBits )
	{
		gWR.iOldWeaponBits = gHUD.m_iWeaponBits;
		for ( int i = MAX_HUD_WEAPONS - 1; i > 0; --i )
		{
			WEAPON* weapon = gWR.GetWeapon( i );
			if ( ( (unsigned int)gHUD.m_iWeaponBits >> weapon->iId ) & 1 )
				gWR.PickupWeapon( weapon );
			else
				gWR.DropWeapon( weapon );
		}
	}
	if ( gpActiveSel && ( gHUD.m_iKeyBits & IN_ATTACK ) )
	{
		if ( gpActiveSel != (WEAPON*)1 )
		{
			ServerCmd( gpActiveSel->szName );
			g_weaponselect = gpActiveSel->iId;
		}
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gWR.m_hudOffTime = 0;
		gHUD.m_iKeyBits &= ~IN_ATTACK;
		gEngfuncs.pfnPlaySoundByName( "common/wpn_select.wav", 1 );
	}
}

int CHudAmmo::MsgFunc_AmmoX( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int id = READ_BYTE();
	int count = READ_BYTE();
	if ( id >= MAX_AMMO_TYPES )
		return 1;
	gWR.SetAmmo( id, abs( count ) );
	return 1;
}

int CHudAmmo::MsgFunc_AmmoPickup( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int id = READ_BYTE();
	int count = READ_BYTE();
	gHR.AddToHistory( HISTSLOT_AMMO, id, 0, abs( count ) );
	return 1;
}

int CHudAmmo::MsgFunc_WeapPickup( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int id = READ_BYTE();
	int accessory = READ_BYTE();
	if ( id >= MAX_HUD_WEAPONS )
		return 1;
	gHR.AddToHistory( HISTSLOT_WEAP, id, accessory, 0 );
	return 1;
}

int CHudAmmo::MsgFunc_ItemPickup( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	gHR.AddToHistory( HISTSLOT_ITEM, READ_STRING(), 0 );
	return 1;
}

int CHudAmmo::MsgFunc_AccesBits( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	unsigned int bits = READ_LONG();
	for ( int i = MAX_HUD_WEAPONS - 1; i > 0; --i )
	{
		WEAPON* weapon = gWR.GetAccessory( i );
		if ( ( bits >> weapon->iId ) & 1 )
			gWR.PickupAccessory( weapon );
		else
			gWR.DropAccessory( weapon );
	}
	return 1;
}

int CHudAmmo::MsgFunc_HideWeapon( const char*, int size, void* buffer )
{
	static wrect_t nullrc;
	BEGIN_READ( buffer, size );
	gHUD.m_iHideHUDDisplay = READ_BYTE();
	if ( gEngfuncs.IsSpectateOnly() )
		return 1;
	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
	{
		gpActiveSel = NULL;
		gWR.m_hudOffTime = 0;
		SetCrosshair( 0, nullrc, 0, 0, 0 );
	}
	else if ( m_pWeapon )
		SetCrosshair( m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255 );
	return 1;
}

int CHudAmmo::MsgFunc_CurWeapon( const char*, int size, void* buffer )
{
	wrect_t nullrc = { 0, 0, 0, 0 };
	BEGIN_READ( buffer, size );
	int state = READ_BYTE();
	int id = READ_CHAR();
	int clip = READ_CHAR();
	int accessory = READ_BYTE();
	if ( id <= 0 && !m_bObserverCrosshair )
	{
		SetCrosshair( 0, nullrc, 0, 0, 0 );
		return 0;
	}
	if ( id == -1 && clip == -1 )
	{
		gHUD.m_fPlayerDead = TRUE;
		gpActiveSel = NULL;
		gWR.m_hudOffTime = 0;
		return 1;
	}
	gHUD.m_fPlayerDead = FALSE;
	if ( id < 0 || id >= MAX_HUD_WEAPONS )
		return 0;
	WEAPON* weapon = gWR.GetItem( id, accessory != 0 );
	if ( clip <= -2 )
		clip = -clip;
	weapon->iClip = clip;
	weapon->bIsAccessory = accessory;
	if ( state )
	{
		m_pWeapon = weapon;
		if ( !( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
		{
			if ( gHUD.m_iFOV <= 89 )
				SetCrosshair( weapon->hZoomedCrosshair, weapon->rcZoomedCrosshair, 255, 255, 255 );
			else
				SetCrosshair( 0, nullrc, 0, 0, 0 );
			m_bObserverCrosshair = FALSE;
		}
		m_fFade = 200; //!!!
		m_iFlags |= HUD_ACTIVE;
	}
	return 1;
}

int CHudAmmo::MsgFunc_WeaponList( const char*, int size, void* buffer )
{
	WEAPON weapon;
	BEGIN_READ( buffer, size );
	strncpy( weapon.szName, READ_STRING(), sizeof( weapon.szName ) );
	weapon.szName[sizeof( weapon.szName ) - 1] = 0;
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
	weapon.iId = READ_CHAR();
	weapon.bIsAccessory = READ_CHAR();
	weapon.iFlags = READ_BYTE();
	weapon.iClip = 0;
	if ( weapon.iId < 0 || weapon.iId > MAX_HUD_WEAPONS - 1 )
		return 0;
	if ( (unsigned int)weapon.iSlot > MAX_WEAPON_SLOTS ||
		(unsigned int)weapon.iSlotPos > MAX_WEAPON_POSITIONS ||
		(unsigned int)( weapon.iAmmoType + 1 ) > MAX_AMMO_TYPES ||
		(unsigned int)( weapon.iAmmo2Type + 1 ) > MAX_AMMO_TYPES ||
		( weapon.iAmmoType != -1 && !weapon.iMax1 ) ||
		( weapon.iAmmo2Type != -1 && !weapon.iMax2 ) )
		return 0;
	if ( weapon.bIsAccessory )
		gWR.AddAccessory( &weapon );
	else
		gWR.AddWeapon( &weapon );
	EV_HolsterBlowtorch( NULL );
	return 1;
}

int CHudAmmo::VidInit()
{
	// Load sprites for buckets (top row of weapon menu)
	m_HUD_bucket0 = gHUD.GetSpriteIndex( "bucket1" );
	m_HUD_selection = gHUD.GetSpriteIndex( "selection" );
	ghsprBuckets = gHUD.GetSprite( m_HUD_bucket0 );
	wrect_t& bucket = gHUD.GetSpriteRect( m_HUD_bucket0 );
	giBucketWidth = bucket.right - bucket.left;
	giBucketHeight = bucket.bottom - bucket.top;
	if ( gHR.iHistoryGap < giBucketHeight )
		gHR.iHistoryGap = giBucketHeight;

	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();
	giABHeight = ScreenWidth >= 640 ? 4 : 2;
	giABWidth = ScreenWidth >= 640 ? 20 : 10;
	int count;
	client_sprite_t* list = SPR_GetList( "sprites/observer.txt", &count );
	int resolution = ScreenWidth < 640 ? 320 : 640;
	if ( list )
	{
		client_sprite_t* sprite = GetSpriteList( list, "crosshair", resolution, count );
		if ( sprite )
		{
			char path[32];
			sprintf( path, "sprites/%s.spr", sprite->szSprite );
			m_hObserverCrosshair = SPR_Load( path );
			m_rcObserverCrosshair = sprite->rc;
		}
		else
			m_hObserverCrosshair = 0;
	}
	return 1;
}

void CHudAmmo::SlotInput( int slot )
{
	if ( gHUD.m_Menu.m_fMenuDisplayed )
		gHUD.m_Menu.SelectMenuItem( slot + 1 );
	else if ( slot <= MAX_WEAPON_SLOTS && !gHUD.m_fPlayerDead &&
		!( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
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
	if ( gHUD.m_Menu.m_fMenuDisplayed )
		gHUD.m_Menu.SelectMenuItem( 7 );
}

void CHudAmmo::UserCmd_Slot8()
{
	if ( gHUD.m_Menu.m_fMenuDisplayed )
		gHUD.m_Menu.SelectMenuItem( 8 );
}

void CHudAmmo::UserCmd_Slot9()
{
	if ( gHUD.m_Menu.m_fMenuDisplayed )
		gHUD.m_Menu.SelectMenuItem( 9 );
}

void CHudAmmo::UserCmd_Slot10()
{
	if ( gHUD.m_Menu.m_fMenuDisplayed )
		gHUD.m_Menu.SelectMenuItem( 10 );
}

void CHudAmmo::UserCmd_Close()
{
	if ( gpActiveSel )
	{
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gWR.m_hudOffTime = 0;
		gEngfuncs.pfnPlaySoundByName( "common/wpn_hudoff.wav", 1 );
	}
	else
		EngineClientCmd( "escape" );
}


// Selects the next item in the weapon menu
void CHudAmmo::UserCmd_NextWeapon()
{
	gWR.m_hudOffTime = gEngfuncs.GetClientTime() + 2.5f;
	if ( gHUD.m_fPlayerDead || ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
		return;
	if ( (unsigned int)gpActiveSel <= 1 )
		gpActiveSel = m_pWeapon;
	int slot = gpActiveSel ? gpActiveSel->iSlot : 0;
	int position = gpActiveSel ? gpActiveSel->iSlotPos + 1 : 0;
	for ( int pass = 0; pass < 2; ++pass )
	{
		for ( ; slot < MAX_WEAPON_SLOTS; ++slot, position = 0 )
		{
			for ( ; position < MAX_WEAPON_POSITIONS; ++position )
			{
				WEAPON* weapon = gWR.GetWeaponSlot( slot, position );
				if ( weapon && gWR.HasAmmo( weapon ) )
				{
					gpActiveSel = weapon;
					return;
				}
			}
		}
		slot = 0;  // start looking from the first slot again
	}
	gpActiveSel = NULL;
	gWR.m_hudOffTime = 0;
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_PrevWeapon()
{
	gWR.m_hudOffTime = gEngfuncs.GetClientTime() + 2.5f;
	if ( gHUD.m_fPlayerDead || ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
		return;
	if ( (unsigned int)gpActiveSel <= 1 )
		gpActiveSel = m_pWeapon;
	int slot = gpActiveSel ? gpActiveSel->iSlot : MAX_WEAPON_SLOTS - 1;
	int position = gpActiveSel ? gpActiveSel->iSlotPos - 1 : MAX_WEAPON_POSITIONS - 1;
	for ( int pass = 0; pass < 2; ++pass )
	{
		for ( ; slot >= 0; --slot, position = MAX_WEAPON_POSITIONS - 1 )
		{
			for ( ; position >= 0; --position )
			{
				WEAPON* weapon = gWR.GetWeaponSlot( slot, position );
				if ( weapon && gWR.HasAmmo( weapon ) )
				{
					gpActiveSel = weapon;
					return;
				}
			}
		}
		slot = MAX_WEAPON_SLOTS - 1;
	}
	gpActiveSel = NULL;
	gWR.m_hudOffTime = 0;
}

void WeaponsResource::SelectSlot( int slot, int advance, int direction )
{
	static int gCurrentSlot = -1;
	if ( gHUD.m_Menu.m_fMenuDisplayed && !advance && direction == 1 )
	{
		gHUD.m_Menu.SelectMenuItem( slot + 1 );  // slots are one off the key numbers
		return;
	}
	if ( slot > MAX_WEAPON_SLOTS || gHUD.m_fPlayerDead ||
		( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
		return;
	float fastSwitch = CVAR_GET_FLOAT( "hud_fastswitch" );
	if ( fastSwitch != 0 && gCurrentSlot == slot && GetNumberOfItemsInSlot( slot ) == 1 )
		return;
	WEAPON* weapon = NULL;
	if ( (unsigned int)gpActiveSel > 1 && slot == gpActiveSel->iSlot )
	{
		gEngfuncs.pfnPlaySoundByName( "common/wpn_moveselect.wav", 1 );
		if ( gpActiveSel )
			weapon = GetNextActivePos( gpActiveSel->iSlot, gpActiveSel->iSlotPos );
		if ( !weapon )
		{
			weapon = GetFirstPos( slot );
			if ( !HasAmmo( weapon ) )
				weapon = GetNextActivePos( weapon->iSlot, weapon->iSlotPos );
			if ( !weapon )
				weapon = GetFirstPos( slot );
		}
	}
	else
	{
		gEngfuncs.pfnPlaySoundByName( "common/wpn_hudon.wav", 1 );
		weapon = GetFirstPos( slot );
		if ( weapon && !HasAmmo( weapon ) )
		{
			weapon = GetNextActivePos( weapon->iSlot, weapon->iSlotPos );
			if ( !weapon )
				weapon = GetFirstPos( slot );
		}
		if ( weapon && fastSwitch != 0 && !GetNextActivePos( weapon->iSlot, weapon->iSlotPos ) )
		{
			ServerCmd( weapon->szName );
			g_weaponselect = weapon->iId;
			gpActiveSel = NULL;
			gCurrentSlot = -1;
			gWR.m_hudOffTime = 0;
			return;
		}
	}
	if ( !weapon )
	{
		if ( fastSwitch == 0 )
			gpActiveSel = (WEAPON*)1;
		else
		{
			gpActiveSel = NULL;
			gWR.m_hudOffTime = 0;
		}
	}
	else
	{
		gpActiveSel = weapon;
		gCurrentSlot = weapon->iSlot;
	}
	m_hudOffTime = gEngfuncs.GetClientTime() + 2.5f;
}

void CHudAmmo::UserCmd_Adjust_Crosshair()
{
	if ( ++m_iCurrentCrosshair > 4 )
		m_iCurrentCrosshair = 0;
	switch ( m_iCurrentCrosshair )
	{
	case 1:
		m_R = 250;
		m_G = 50;
		m_B = 50;
		break;
	case 2:
		m_R = 50;
		m_G = 50;
		m_B = 250;
		break;
	case 3:
		m_R = 250;
		m_G = 250;
		m_B = 50;
		break;
	case 4:
		m_R = 50;
		m_G = 250;
		m_B = 250;
		break;
	default:
		m_R = 50;
		m_G = 250;
		m_B = 50;
		break;
	}
}

void WeaponsResource::LoadWeaponSprites( WEAPON* weapon )
{
	int resolution = ScreenWidth > 639 ? 640 : 320;
	if ( !weapon )
		return;
	wrect_t empty = { 0, 0, 0, 0 };
	weapon->rcActive = empty;
	weapon->rcInactive = empty;
	weapon->rcAmmo = empty;
	weapon->rcAmmo2 = empty;
	weapon->hInactive = 0;
	weapon->hActive = 0;
	weapon->hAmmo = 0;
	weapon->hAmmo2 = 0;
	char path[128];
	sprintf( path, "sprites/%s.txt", weapon->szName );
	int count;
	client_sprite_t* list = SPR_GetList( path, &count );
	if ( !list )
		return;
	client_sprite_t* sprite = GetSpriteList( list, "crosshair", resolution, count );
	if ( sprite )
	{
		sprintf( path, "sprites/%s.spr", sprite->szSprite );
		weapon->hCrosshair = SPR_Load( path );
		weapon->rcCrosshair = sprite->rc;
	}
	else
		weapon->hCrosshair = 0;
	sprite = GetSpriteList( list, "autoaim", resolution, count );
	if ( sprite )
	{
		sprintf( path, "sprites/%s.spr", sprite->szSprite );
		weapon->hAutoaim = SPR_Load( path );
		weapon->rcAutoaim = sprite->rc;
	}
	else
		weapon->hAutoaim = 0;
	sprite = GetSpriteList( list, "zoom", resolution, count );
	if ( sprite )
	{
		sprintf( path, "sprites/%s.spr", sprite->szSprite );
		weapon->hZoomedCrosshair = SPR_Load( path );
		weapon->rcZoomedCrosshair = sprite->rc;
	}
	else
	{
		weapon->hZoomedCrosshair = weapon->hCrosshair;
		weapon->rcZoomedCrosshair = weapon->rcCrosshair;
	}
	sprite = GetSpriteList( list, "zoom_autoaim", resolution, count );
	if ( sprite )
	{
		sprintf( path, "sprites/%s.spr", sprite->szSprite );
		weapon->hZoomedAutoaim = SPR_Load( path );
		weapon->rcZoomedAutoaim = sprite->rc;
	}
	else
	{
		weapon->hZoomedAutoaim = weapon->hZoomedCrosshair;
		weapon->rcZoomedAutoaim = weapon->rcZoomedCrosshair;
	}
	sprite = GetSpriteList( list, "weapon", resolution, count );
	if ( sprite )
	{
		sprintf( path, "sprites/%s.spr", sprite->szSprite );
		weapon->hInactive = SPR_Load( path );
		weapon->rcInactive = sprite->rc;
		if ( gHR.iHistoryGap < weapon->rcActive.bottom - weapon->rcActive.top )
			gHR.iHistoryGap = weapon->rcActive.bottom - weapon->rcActive.top;
	}
	else
		weapon->hInactive = 0;
	sprite = GetSpriteList( list, "weapon_s", resolution, count );
	if ( sprite )
	{
		sprintf( path, "sprites/%s.spr", sprite->szSprite );
		weapon->hActive = SPR_Load( path );
		weapon->rcActive = sprite->rc;
	}
	else
		weapon->hActive = 0;
	sprite = GetSpriteList( list, "ammo", resolution, count );
	if ( sprite )
	{
		sprintf( path, "sprites/%s.spr", sprite->szSprite );
		weapon->hAmmo = SPR_Load( path );
		weapon->rcAmmo = sprite->rc;
		if ( gHR.iHistoryGap < weapon->rcActive.bottom - weapon->rcActive.top )
			gHR.iHistoryGap = weapon->rcActive.bottom - weapon->rcActive.top;
	}
	else
		weapon->hAmmo = 0;
	sprite = GetSpriteList( list, "ammo2", resolution, count );
	if ( sprite )
	{
		sprintf( path, "sprites/%s.spr", sprite->szSprite );
		weapon->hAmmo2 = SPR_Load( path );
		weapon->rcAmmo2 = sprite->rc;
		if ( gHR.iHistoryGap < weapon->rcActive.bottom - weapon->rcActive.top )
			gHR.iHistoryGap = weapon->rcActive.bottom - weapon->rcActive.top;
	}
	else
		weapon->hAmmo2 = 0;
}

int DrawBar( int x, int y, int width, int height, float fraction )
{
	int remaining = width;
	if ( fraction >= 0 )
	{
		if ( fraction > 1 )
			fraction = 1;
		if ( fraction != 0 )
		{
			int filled = (int)(float)( (double)fraction * width );
			if ( filled <= 0 )
				filled = 1;
			FillRGBA( x, y, filled, height, 0, 160, 0, 255 );
			x += filled;
			remaining -= filled;
		}
	}
	FillRGBA( x, y, remaining, height, 64, 255, 64, 128 );
	return x + remaining;
}

void DrawAmmoBar( WEAPON* weapon, int x, int y, int width, int height )
{
	if ( !weapon || weapon->iAmmoType < 0 || !gWR.CountAmmo( weapon->iAmmoType ) )
		return;
	float fraction = (float)gWR.CountAmmo( weapon->iAmmoType ) / weapon->iMax1;
	x = DrawBar( x, y, width, height, fraction );
	if ( weapon->iAmmo2Type != -1 )
	{
		fraction = (float)gWR.CountAmmo( weapon->iAmmo2Type ) / weapon->iMax2;
		DrawBar( x + 5, y, width, height, fraction );
	}
}




//
// Draw Weapon Menu
//
int CHudAmmo::DrawWList( float )
{
	if ( !gpActiveSel )
		return 0;
	int activeSlot = gpActiveSel == (WEAPON*)1 ? -1 : gpActiveSel->iSlot;
	bool outOfAmmo = activeSlot == -1 || !gWR.HasAmmo( gpActiveSel );
	WEAPON* first = gWR.GetFirstPos( activeSlot );
	if ( first && first->bIsAccessory && first->iId == ACCESSORY_RADIOCONTROLLEDBOMB )
		outOfAmmo = false;
	int x = 10;
	for ( int slot = 0; slot < MAX_WEAPON_SLOTS; ++slot )
	{
		int r = 64, g = 255, b = 64;
		ScaleColors( r, g, b, 255 );
		SPR_Set( gHUD.GetSprite( m_HUD_bucket0 + slot ), r, g, b );
		int width = giBucketWidth;
		if ( slot == activeSlot )
		{
			WEAPON* weapon = gWR.GetFirstPos( slot );
			if ( weapon )
				width = weapon->rcActive.right - weapon->rcActive.left;
		}
		SPR_DrawAdditive( 0, x, 10, &gHUD.GetSpriteRect( m_HUD_bucket0 + slot ) );
		x += width + 5;
	}
	x = 10;
	for ( int slot = 0; slot < MAX_WEAPON_SLOTS; ++slot )
	{
		int y = giBucketHeight + 10;
		int width = giBucketWidth;
		if ( slot == activeSlot )
		{
			WEAPON* weapon = gWR.GetFirstPos( slot );
			if ( weapon )
				width = weapon->rcActive.right - weapon->rcActive.left;
			for ( int position = 0; position < MAX_WEAPON_POSITIONS; ++position )
			{
				weapon = gWR.GetWeaponSlot( slot, position );
				if ( !weapon || !weapon->iId )
					continue;
				int r = 64, g = 255, b = 64;
				if ( gpActiveSel == weapon )
				{
					if ( outOfAmmo )
					{
						r = 255;
						g = 16;
						b = 16;
						ScaleColors( r, g, b, 128 );
					}
					SPR_Set( weapon->hActive, r, g, b );
					SPR_DrawAdditive( 0, x, y, &weapon->rcActive );
					SPR_Set( gHUD.GetSprite( m_HUD_selection ), r, g, b );
					SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( m_HUD_selection ) );
				}
				else
				{
					// Draw Weapon if Red if no ammo

					if ( gWR.HasAmmo( weapon ) )
						ScaleColors( r, g, b, 192 );
					else
					{
						r = 255;
						g = 16;
						b = 16;
						ScaleColors( r, g, b, 128 );
					}
					SPR_Set( weapon->hInactive, r, g, b );
					SPR_DrawAdditive( 0, x, y, &weapon->rcInactive );
				}
				DrawAmmoBar( weapon, x + giABWidth / 2, y, giABWidth, giABHeight );
				y += weapon->rcActive.bottom - weapon->rcActive.top + 5;
			}
		}
		else
		{
			for ( int position = 0; position < MAX_WEAPON_POSITIONS; ++position )
			{
				WEAPON* weapon = gWR.GetWeaponSlot( slot, position );
				if ( !weapon || !weapon->iId )
					continue;
				if ( gWR.HasAmmo( weapon ) )
					FillRGBA( x, y, giBucketWidth, giBucketHeight, 64, 255, 64, 128 );
				else
					FillRGBA( x, y, giBucketWidth, giBucketHeight, 255, 16, 16, 96 );
				y += giBucketHeight + 5;
			}
		}
		x += width + 5;
	}
	return 1;
}

int CHudAmmo::Draw( float time )
{
	if ( gEngfuncs.IsSpectateOnly() || ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) ) )
		return 1;
	DrawWList( time );
	gHR.DrawAmmoHistory( time );
	if ( !( m_iFlags & HUD_ACTIVE ) )
		return 0;
	WEAPON* weapon = m_pWeapon;
	if ( !weapon )
		return 0;
	if ( gHUD.m_iFOV > 89 && ( weapon->iId & ~16 ) != 2 && ( weapon->iId & ~4 ) != 9 &&
		!( gHUD.m_iHideHUDDisplay & HIDEHUD_CROSSHAIR ) )
		DrawCrosshair( time, weapon->iId );
	if ( weapon->iAmmoType < 0 && weapon->iAmmo2Type < 0 )
		return 0;
	wrect_t& number = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 );
	int width = number.right - number.left;
	int alpha = (int)m_fFade < 200 ? 200 : (int)m_fFade;
	if ( m_fFade > 0 )
		m_fFade = (float)( m_fFade - 20 * gHUD.m_flTimeDelta );
	int r = 64, g = 255, b = 64;
	ScaleColors( r, g, b, alpha );
	int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

	// Does weapon have any ammo at all?
	if ( m_pWeapon->iAmmoType > 0 )
	{
		int iconWidth = m_pWeapon->rcAmmo.right - m_pWeapon->rcAmmo.left;
		int x;
		if ( weapon->iClip < 0 )
		{
			if ( weapon->iMax1 * 0.15f >= gWR.CountAmmo( weapon->iAmmoType ) )
			{
				r = 255;
				g = 16;
				b = 16;
			}
			x = gHUD.DrawHudNumber( ScreenWidth - 4 * width - iconWidth, y, DHN_DRAWZERO | DHN_3DIGITS,
				gWR.CountAmmo( weapon->iAmmoType ), r, g, b );
		}
		else
		{
			if ( UTIL_GetMaxClip( weapon ) * 0.15f >= weapon->iClip )
			{
				r = 255;
				g = 16;
				b = 16;
			}
			x = gHUD.DrawHudNumber( ScreenWidth - 8 * width - iconWidth, y, DHN_DRAWZERO | DHN_3DIGITS,
				weapon->iClip, r, g, b );
			r = 64;
			g = 255;
			b = 64;
			x += width / 2;
			FillRGBA( x, y, width / 10, gHUD.m_iFontHeight, r, g, b, alpha );
			if ( weapon->iMax1 * 0.15f >= gWR.CountAmmo( weapon->iAmmoType ) )
			{
				r = 255;
				g = 16;
				b = 16;
			}
			ScaleColors( r, g, b, alpha );
			x = gHUD.DrawHudNumber( x + width / 10 + width / 2, y, DHN_DRAWZERO | DHN_3DIGITS,
				gWR.CountAmmo( weapon->iAmmoType ), r, g, b );
		}
		r = 64;
		g = 255;
		b = 64;
		int height = m_pWeapon->rcAmmo.bottom - m_pWeapon->rcAmmo.top;
		SPR_Set( m_pWeapon->hAmmo, r, g, b );
		SPR_DrawAdditive( 0, x, y - height / 8, &m_pWeapon->rcAmmo );
	}
	if ( weapon->iAmmo2Type > 0 && gWR.CountAmmo( weapon->iAmmo2Type ) > 0 )
	{
		int iconWidth = m_pWeapon->rcAmmo2.right - m_pWeapon->rcAmmo2.left;
		int secondaryY = y - gHUD.m_iFontHeight / 4 - gHUD.m_iFontHeight;
		int x = gHUD.DrawHudNumber( ScreenWidth - 4 * width - iconWidth, secondaryY,
			DHN_DRAWZERO | DHN_3DIGITS, gWR.CountAmmo( weapon->iAmmo2Type ), r, g, b );

		// Draw the ammo Icon
		SPR_Set( m_pWeapon->hAmmo2, r, g, b );
		SPR_DrawAdditive( 0, x, secondaryY - ( m_pWeapon->rcAmmo2.bottom - m_pWeapon->rcAmmo2.top ) / 8,
			&m_pWeapon->rcAmmo2 );
	}
	if ( gEngfuncs.GetClientTime() > gWR.m_hudOffTime )
	{
		gpActiveSel = NULL;
		gWR.m_hudOffTime = 0;
	}
	return 1;
}

int CHudAmmo::DrawCrosshair( float, int weaponId )
{
	int distance = 4;
	int delta = 3;
	switch ( weaponId )
	{
	case 1:
	case 7:
	case 10:
	case 11:
	case 13:
	case 19:
		distance = 8;
		break;
	case 2:
	case 9:
	case 20:
		distance = 5;
		break;
	case 3:
		distance = 9;
		delta = 4;
		break;
	case 4:
		distance = 9;
		break;
	case 5:
		distance = 3;
		break;
	case 6:
	case 8:
	case 14:
		distance = 6;
		break;
	case 12:
		distance = 6;
		delta = 2;
		break;
	case 15:
		distance = 8;
		delta = 6;
		break;
	case 17:
	case 22:
		distance = 7;
		break;
	case 18:
		distance = 6;
		delta = 4;
		break;
	case 21:
		delta = 4;
		break;
	}
	int accuracy = GetWeaponAccuracyFlags( weaponId );
	if ( accuracy && cl_dynamiccrosshair && cl_dynamiccrosshair->value != 0 &&
		!( gHUD.m_iHideHUDDisplay & HIDEHUD_WEAPONS ) )
	{
		if ( !( g_iPlayerFlags & FL_ONGROUND ) && ( accuracy & 1 ) )
			distance = (int)(float)( (double)distance + distance );
		else if ( ( g_iPlayerFlags & FL_DUCKING ) && ( accuracy & 4 ) )
			distance = (int)(float)( distance * 0.5 );
		else if ( (int)g_flPlayerSpeed > GetWeaponMaxSpeed( weaponId ) && ( accuracy & 2 ) )
			distance = (int)(float)( distance * 1.5 );
		if ( accuracy & 8 )
			distance = (int)(float)( distance * (double)1.4f );
		if ( accuracy & 16 )
			distance = (int)(float)( distance * (double)1.4f );
	}
	if ( m_iAmmoLastCheck >= g_iShotsFired )
	{
		m_flCrosshairDistance = (float)( m_flCrosshairDistance - ( 0.013 * m_flCrosshairDistance + 0.1 ) );
		m_iAlpha += 2;
	}
	else
	{
		m_flCrosshairDistance += delta;
		m_iAlpha -= 40;
		if ( !( m_flCrosshairDistance <= 15 ) )
			m_flCrosshairDistance = 15;
		if ( m_iAlpha <= 119 )
			m_iAlpha = 120;
	}
	if ( g_iShotsFired > 600 )
		g_iShotsFired = 1;
	m_iAmmoLastCheck = g_iShotsFired;
	if ( distance > m_flCrosshairDistance )
		m_flCrosshairDistance = (float)distance;
	if ( m_iAlpha > 255 )
		m_iAlpha = 255;
	int length = (int)(float)( ( m_flCrosshairDistance - distance ) * 0.5 ) + 5;
	int gap = (int)m_flCrosshairDistance;
	int x = ScreenWidth / 2;
	int y = ScreenHeight / 2;
	FillRGBA( x - gap - length + 1, y, length, 1, m_R, m_G, m_B, m_iAlpha );
	FillRGBA( x + (int)m_flCrosshairDistance, y, length, 1, m_R, m_G, m_B, m_iAlpha );
	FillRGBA( x, y - (int)m_flCrosshairDistance - length, 1, length, m_R, m_G, m_B, m_iAlpha );
	FillRGBA( x, y + (int)m_flCrosshairDistance, 1, length, m_R, m_G, m_B, m_iAlpha );
	return 1;
}
