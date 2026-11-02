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
#include "cl_util.h"
#include "event_args.h"

extern "C"
{
void EV_TrainPitchAdjust( event_args_t* args );
void EV_FireColt( event_args_t* args );
void EV_FireLuger( event_args_t* args );
void EV_FireM1Carbine( event_args_t* args );
void EV_FireGarand( event_args_t* args );
void EV_FireScopedKar( event_args_t* args );
void EV_FireThompson( event_args_t* args );
void EV_FireMP44( event_args_t* args );
void EV_FireSpring( event_args_t* args );
void EV_FireKar( event_args_t* args );
void EV_FireMP40( event_args_t* args );
void EV_Knife( event_args_t* args );
void EV_FireBAR( event_args_t* args );
void EV_FireMG42( event_args_t* args );
void EV_FireMG34( event_args_t* args );
void EV_Fire30CAL( event_args_t* args );
void EV_FireGreaseGun( event_args_t* args );
void EV_FireFG42( event_args_t* args );
void EV_FireK43( event_args_t* args );
void EV_FireEnfield( event_args_t* args );
void EV_FireSten( event_args_t* args );
void EV_FireBren( event_args_t* args );
void EV_FireWebley( event_args_t* args );
void EV_FireScopedEnfield( event_args_t* args );
void EV_FireBazooka( event_args_t* args );
void EV_FirePschreck( event_args_t* args );
void EV_FirePIAT( event_args_t* args );
void EV_FireMortar( event_args_t* args );
void EV_FireMelee( event_args_t* args );
void EV_Pain( event_args_t* args );
void EV_Smoke( event_args_t* args );
void EV_BloodSprite( event_args_t* args );
void EV_BloodStream( event_args_t* args );
void EV_BulletTracers( event_args_t* args );
void EV_BubbleTrails( event_args_t* args );
void EV_Bubbles( event_args_t* args );
void EV_Explosion( event_args_t* args );
void EV_SparkShower( event_args_t* args );
void EV_PlayWhizz( event_args_t* args );
void EV_USVoice( event_args_t* args );
void EV_GERVoice( event_args_t* args );
void EV_BodyDamage( event_args_t* args );
void EV_RoundReleaseSound( event_args_t* args );
void EV_DoDCamera( event_args_t* args );
void EV_PopHelmet( event_args_t* args );
void EV_RoundReset( event_args_t* args );
void EV_Overheat( event_args_t* args );
void EV_RocketTrail( event_args_t* args );
void EV_MortarShell( event_args_t* args );
}

/*
======================
Game_HookEvents

Associate script file name with callback functions.  Callback's must be extern "C" so
 the engine doesn't get confused about name mangling stuff.  Note that the format is
 always the same.  Of course, a clever mod team could actually embed parameters, behavior
 into the actual .sc files and create a .sc file parser and hook their functionality through
 that.. i.e., a scripting system.

That was what we were going to do, but we ran out of time...oh well.
======================
*/
void Game_HookEvents()
{
	gEngfuncs.pfnHookEvent( "events/misc/train.sc", EV_TrainPitchAdjust );
	gEngfuncs.pfnHookEvent( "events/weapons/colt.sc", EV_FireColt );
	gEngfuncs.pfnHookEvent( "events/weapons/luger.sc", EV_FireLuger );
	gEngfuncs.pfnHookEvent( "events/weapons/m1carbine.sc", EV_FireM1Carbine );
	gEngfuncs.pfnHookEvent( "events/weapons/garand.sc", EV_FireGarand );
	gEngfuncs.pfnHookEvent( "events/weapons/scopedkar.sc", EV_FireScopedKar );
	gEngfuncs.pfnHookEvent( "events/weapons/thompson.sc", EV_FireThompson );
	gEngfuncs.pfnHookEvent( "events/weapons/mp44.sc", EV_FireMP44 );
	gEngfuncs.pfnHookEvent( "events/weapons/spring.sc", EV_FireSpring );
	gEngfuncs.pfnHookEvent( "events/weapons/kar.sc", EV_FireKar );
	gEngfuncs.pfnHookEvent( "events/weapons/mp40.sc", EV_FireMP40 );
	gEngfuncs.pfnHookEvent( "events/weapons/knife.sc", EV_Knife );
	gEngfuncs.pfnHookEvent( "events/weapons/bar.sc", EV_FireBAR );
	gEngfuncs.pfnHookEvent( "events/weapons/mg42.sc", EV_FireMG42 );
	gEngfuncs.pfnHookEvent( "events/weapons/mg34.sc", EV_FireMG34 );
	gEngfuncs.pfnHookEvent( "events/weapons/30cal.sc", EV_Fire30CAL );
	gEngfuncs.pfnHookEvent( "events/weapons/greasegun.sc", EV_FireGreaseGun );
	gEngfuncs.pfnHookEvent( "events/weapons/fg42.sc", EV_FireFG42 );
	gEngfuncs.pfnHookEvent( "events/weapons/k43.sc", EV_FireK43 );
	gEngfuncs.pfnHookEvent( "events/weapons/enfield.sc", EV_FireEnfield );
	gEngfuncs.pfnHookEvent( "events/weapons/sten.sc", EV_FireSten );
	gEngfuncs.pfnHookEvent( "events/weapons/bren.sc", EV_FireBren );
	gEngfuncs.pfnHookEvent( "events/weapons/webley.sc", EV_FireWebley );
	gEngfuncs.pfnHookEvent( "events/weapons/scopedenfield.sc", EV_FireScopedEnfield );
	gEngfuncs.pfnHookEvent( "events/weapons/bazooka.sc", EV_FireBazooka );
	gEngfuncs.pfnHookEvent( "events/weapons/pschreck.sc", EV_FirePschreck );
	gEngfuncs.pfnHookEvent( "events/weapons/piat.sc", EV_FirePIAT );
	gEngfuncs.pfnHookEvent( "events/weapons/mortar.sc", EV_FireMortar );
	gEngfuncs.pfnHookEvent( "events/weapons/melee.sc", EV_FireMelee );
	gEngfuncs.pfnHookEvent( "events/misc/pain.sc", EV_Pain );
	gEngfuncs.pfnHookEvent( "events/effects/smoke.sc", EV_Smoke );
	gEngfuncs.pfnHookEvent( "events/effects/blood.sc", EV_BloodSprite );
	gEngfuncs.pfnHookEvent( "events/effects/bloodstream.sc", EV_BloodStream );
	gEngfuncs.pfnHookEvent( "events/effects/tracers.sc", EV_BulletTracers );
	gEngfuncs.pfnHookEvent( "events/effects/bubbletrails.sc", EV_BubbleTrails );
	gEngfuncs.pfnHookEvent( "events/effects/bubbles.sc", EV_Bubbles );
	gEngfuncs.pfnHookEvent( "events/effects/explosion.sc", EV_Explosion );
	gEngfuncs.pfnHookEvent( "events/effects/sparkshower.sc", EV_SparkShower );
	gEngfuncs.pfnHookEvent( "events/misc/whizz.sc", EV_PlayWhizz );
	gEngfuncs.pfnHookEvent( "events/misc/usvoice.sc", EV_USVoice );
	gEngfuncs.pfnHookEvent( "events/misc/gervoice.sc", EV_GERVoice );
	gEngfuncs.pfnHookEvent( "events/effects/bodydamage.sc", EV_BodyDamage );
	gEngfuncs.pfnHookEvent( "events/misc/roundrestart.sc", EV_RoundReleaseSound );
	gEngfuncs.pfnHookEvent( "events/misc/camera.sc", EV_DoDCamera );
	gEngfuncs.pfnHookEvent( "events/effects/helmet.sc", EV_PopHelmet );
	gEngfuncs.pfnHookEvent( "events/misc/roundreset.sc", EV_RoundReset );
	gEngfuncs.pfnHookEvent( "events/effects/overheat.sc", EV_Overheat );
	gEngfuncs.pfnHookEvent( "events/effects/rockettrail.sc", EV_RocketTrail );
	gEngfuncs.pfnHookEvent( "events/effects/mortarshell.sc", EV_MortarShell );
}
