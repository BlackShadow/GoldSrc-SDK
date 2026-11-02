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

#include "hud.h"
#include "ev_hldm.h"

void Game_HookEvents()
{
	gEngfuncs.pfnHookEvent( "events/usp.sc", EV_FireUSP );
	gEngfuncs.pfnHookEvent( "events/mp5n.sc", EV_FireMP5N );
	gEngfuncs.pfnHookEvent( "events/ak47.sc", EV_FireAK47 );
	gEngfuncs.pfnHookEvent( "events/aug.sc", EV_FireAug );
	gEngfuncs.pfnHookEvent( "events/deagle.sc", EV_FireDeagle );
	gEngfuncs.pfnHookEvent( "events/g3sg1.sc", EV_FireG3SG1 );
	gEngfuncs.pfnHookEvent( "events/sg550.sc", EV_FireSG550 );
	gEngfuncs.pfnHookEvent( "events/glock18.sc", EV_FireGlock18 );
	gEngfuncs.pfnHookEvent( "events/m249.sc", EV_FireM249 );
	gEngfuncs.pfnHookEvent( "events/m60.sc", EV_FireM60 );
	gEngfuncs.pfnHookEvent( "events/m3.sc", EV_FireM3 );
	gEngfuncs.pfnHookEvent( "events/m4a1.sc", EV_FireM4A1 );
	gEngfuncs.pfnHookEvent( "events/mac10.sc", EV_FireMac10 );
	gEngfuncs.pfnHookEvent( "events/p90.sc", EV_FireP90 );
	gEngfuncs.pfnHookEvent( "events/p228.sc", EV_FireP228 );
	gEngfuncs.pfnHookEvent( "events/awp.sc", EV_FireAWP );
	gEngfuncs.pfnHookEvent( "events/scout.sc", EV_FireScout );
	gEngfuncs.pfnHookEvent( "events/sg552.sc", EV_FireSG552 );
	gEngfuncs.pfnHookEvent( "events/tmp.sc", EV_FireTMP );
	gEngfuncs.pfnHookEvent( "events/fiveseven.sc", EV_FireFiveSeven );
	gEngfuncs.pfnHookEvent( "events/ump45.sc", EV_FireUMP45 );
	gEngfuncs.pfnHookEvent( "events/xm1014.sc", EV_FireXM1014 );
	gEngfuncs.pfnHookEvent( "events/elite_left.sc", EV_FireEliteLeft );
	gEngfuncs.pfnHookEvent( "events/elite_right.sc", EV_FireEliteRight );
	gEngfuncs.pfnHookEvent( "events/knife.sc", EV_Knife );
	gEngfuncs.pfnHookEvent( "events/galil.sc", EV_FireGalil );
	gEngfuncs.pfnHookEvent( "events/camera.sc", EV_FireCamera );
	gEngfuncs.pfnHookEvent( "events/fiberopticcamera.sc", EV_FireFiberOpticCamera );
	gEngfuncs.pfnHookEvent( "events/famas.sc", EV_FireFamas );
	gEngfuncs.pfnHookEvent( "events/decal_reset.sc", EV_RemoveAllDecals );
	gEngfuncs.pfnHookEvent( "events/shieldgun.sc", EV_FireShieldGun );
	gEngfuncs.pfnHookEvent( "events/blowtorchholster.sc", EV_HolsterBlowtorch );
	gEngfuncs.pfnHookEvent( "events/blowtorchidle.sc", EV_IdleBlowtorch );
	gEngfuncs.pfnHookEvent( "events/blowtorch.sc", EV_FireBlowtorch );
	gEngfuncs.pfnHookEvent( "events/radiocontrolledbomb.sc", EV_FireRadioControlledBomb );
	gEngfuncs.pfnHookEvent( "events/laws.sc", EV_FireLaws );
	gEngfuncs.pfnHookEvent( "events/briefcase.sc", EV_FireBriefcase );
	gEngfuncs.pfnHookEvent( "events/medkit.sc", EV_FireMedkit );
	gEngfuncs.pfnHookEvent( "events/syringe.sc", EV_FireSyringe );
	gEngfuncs.pfnHookEvent( "events/radio.sc", EV_FireRadio );
	gEngfuncs.pfnHookEvent( "events/zipline.sc", EV_FireZipline );
	gEngfuncs.pfnHookEvent( "events/create_glass.sc", EV_CreateGlass );
	gEngfuncs.pfnHookEvent( "events/explosion.sc", EV_GrenadeExplosion );
	gEngfuncs.pfnHookEvent( "events/createsmoke.sc", EV_CreateSmoke );
	gEngfuncs.pfnHookEvent( "events/createexplo.sc", EV_CreateSmoke );
}
