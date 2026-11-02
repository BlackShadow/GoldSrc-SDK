/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
#include "event_api.h"
#include "r_efx.h"
#include "../../pm_shared/pm_defs.h"
extern "C"
{
#include "../../pm_shared/pm_shared.h"
}
#include "eventscripts.h"
#include "ev_hldm.h"
#include "com_weapons.h"
#include "particleman.h"
#include "materials/z_materials.h"
#include "effects/e_effectclasses.h"

extern cvar_t* cl_righthand;
extern int g_iShotsFired;
extern IParticleMan* g_pParticleMan;

static int tracerCount[32];
static int g_xWindDirection = 1;
static int g_yWindDirection = 1;
static float g_xWindMagnitude;
static float g_yWindMagnitude;
int blowTorchFiring;
TEMPENTITY* g_DeadPlayerModels[64];
char extern_i[25];
BEAM* pBeam;
BEAM* pBeam2;

static constexpr int FTENT_BODY_LANDED = 1 << 21;
static constexpr int FTENT_PLAYERBODY = 1 << 20;
static constexpr int SND_CHANGE_PITCH = 1 << 7;

void SmokeRise_Wind_Expand( TEMPENTITY* smoke, float, float )
{
	if ( smoke->entity.curstate.renderamt > 74 && gEngfuncs.pfnRandomLong( 0, 9 ) == 0 )
		--smoke->entity.curstate.renderamt;
	if ( smoke->entity.baseline.fuser1 > smoke->entity.curstate.scale )
		smoke->entity.curstate.scale = (float)( smoke->entity.curstate.scale + smoke->entity.curstate.scale / 40.0 );
}

void RemoveBody( TEMPENTITY* body, float frametime, float currentTime )
{
	const float start = body->entity.curstate.fuser2 + 5;
	if ( currentTime >= gEngfuncs.pfnGetCvarFloat( "cl_corpsestay" ) + start )
		body->entity.origin.z -= 5 * frametime;
}

void HitBody( TEMPENTITY* body, pmtrace_t* trace )
{
	if ( trace->plane.normal.z > 0 )
		body->flags |= FTENT_BODY_LANDED;
}

void HugWalls( TEMPENTITY* smoke, pmtrace_t* trace )
{
	const float speed = smoke->entity.baseline.origin.Length();
	const Vector direction = smoke->entity.baseline.origin.Normalize();
	const Vector tangent = CrossProduct( direction, trace->plane.normal );
	const Vector velocity = CrossProduct( tangent, trace->plane.normal );
	smoke->entity.baseline.origin = velocity * ( speed > 2000 ? 3000 : speed * 1.5f );
}

void SmokeRise( TEMPENTITY* smoke, float, float )
{
	if ( smoke->entity.curstate.frame > 7 )
	{
		smoke->entity.baseline.origin.x = (float)( smoke->entity.baseline.origin.x * 0.97 );
		smoke->entity.baseline.origin.y = (float)( smoke->entity.baseline.origin.y * 0.97 );
		float speed = (float)( smoke->entity.baseline.origin.z * 0.97 );
		speed = (float)( speed + 0.7 );
		smoke->entity.baseline.origin.z = speed > 70 ? 70 : speed;
	}
}

void SmokeRise_Wind( TEMPENTITY* smoke, float frametime, float currentTime )
{
	SmokeRise( smoke, frametime, currentTime );
	if ( smoke->entity.curstate.frame <= 6 )
		return;
	if ( g_xWindDirection == 0 || g_xWindDirection == 1 )
	{
		g_xWindMagnitude = (float)( g_xWindMagnitude + 0.075 );
		if ( g_xWindMagnitude > 5 ) g_xWindMagnitude = 5;
		smoke->entity.baseline.origin.x += g_xWindDirection ? g_xWindMagnitude : -g_xWindMagnitude;
	}
	if ( g_yWindDirection == 0 || g_yWindDirection == 1 )
	{
		g_yWindMagnitude = (float)( g_yWindMagnitude + 0.075 );
		if ( g_yWindMagnitude > 5 ) g_yWindMagnitude = 5;
		smoke->entity.baseline.origin.y += g_yWindDirection ? g_yWindMagnitude : -g_yWindMagnitude;
	}
	if ( gEngfuncs.pfnRandomLong( 0, 10 ) == 0 && g_yWindMagnitude > 3 )
	{
		g_yWindMagnitude = 0;
		g_yWindDirection = 1 - g_yWindDirection;
	}
	if ( gEngfuncs.pfnRandomLong( 0, 10 ) == 0 && g_xWindMagnitude > 3 )
	{
		g_xWindMagnitude = 0;
		g_xWindDirection = 1 - g_xWindDirection;
	}
}

void EV_HLDM_CreateLight( float*, float radius, int red, int green, int blue, float duration )
{
	dlight_t* light = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
	light->origin = gHUD.m_vecOrigin;
	light->radius = radius;
	light->color.r = (byte)red;
	light->color.g = (byte)green;
	light->color.b = (byte)blue;
	light->die = gHUD.m_flTime + duration;
}

#pragma warning( push )
#pragma warning( disable : 4701 )
void EV_HLDM_CreateSmoke( float* origin, float* direction, int speed, float scale,
	int red, int green, int blue, int smokeType, float* baseVelocity, bool wind, int frameRate )
{
	if ( smokeType != 0 && g_SmokePuffQueue.Full() )
		return;
	g_SmokePuffQueue.Add( gHUD.m_flTime );
	int model;
	switch ( smokeType )
	{
	case 0:
	case 1:
		strcpy( extern_i, "sprites/black_smoke1.spr" );
		extern_i[19] = (char)( '1' + gEngfuncs.pfnRandomLong( 0, 3 ) );
		model = gEngfuncs.pEventAPI->EV_FindModelIndex( extern_i );
		if ( gEngfuncs.pfnGetCvarFloat( "fastsprites" ) > 1 )
			model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/fast_wallpuff1.spr" );
		if ( !model ) model = 293;
		break;
	case 2:
		switch ( gEngfuncs.pfnRandomLong( 0, 3 ) )
		{
		case 1: model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/wall_puff2.spr" ); break;
		case 2: model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/wall_puff3.spr" ); break;
		case 3: model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/wall_puff4.spr" ); break;
		default: model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/wall_puff1.spr" ); break;
		}
		break;
	case 3:
		model = gEngfuncs.pEventAPI->EV_FindModelIndex( gEngfuncs.pfnRandomLong( 0, 1 ) ? "sprites/pistol_smoke1.spr" : "sprites/pistol_smoke2.spr" );
		break;
	case 4:
		switch ( gEngfuncs.pfnRandomLong( 0, 2 ) )
		{
		case 1: model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/rifle_smoke2.spr" ); break;
		case 2: model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/rifle_smoke3.spr" ); break;
		default: model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/rifle_smoke1.spr" ); break;
		}
		break;
	default:
		break;
	}
	TEMPENTITY* smoke = gEngfuncs.pEfxAPI->R_DefaultSprite( origin, model, (float)frameRate );
	if ( !smoke )
		return;
	smoke->flags |= FTENT_CLIENTCUSTOM | FTENT_COLLIDEWORLD;
	smoke->entity.curstate.scale = gEngfuncs.pfnRandomFloat( 0, scale * 0.5f ) + scale;
	if ( (unsigned int)smokeType <= 1 )
	{
		if ( gEngfuncs.pfnGetCvarFloat( "fastsprites" ) == 1 )
		{
			smoke->entity.curstate.rendermode = kRenderTransAdd;
			smoke->entity.curstate.renderamt = 200;
		}
		else if ( gEngfuncs.pfnGetCvarFloat( "fastsprites" ) <= 1 )
		{
			smoke->entity.curstate.rendermode = kRenderTransAlpha;
			smoke->entity.curstate.renderamt = 200;
		}
		else smoke->entity.curstate.rendermode = kRenderNormal;
		smoke->callback = SmokeRise_Wind_Expand;
		if ( smokeType == 1 ) smoke->entity.baseline.fuser1 = 1;
		if ( gEngfuncs.pfnGetCvarFloat( "fastsprites" ) > 1 ) smoke->entity.baseline.fuser1 -= 3;
	}
	else
	{
		smoke->entity.curstate.renderamt = 150;
		smoke->callback = wind ? SmokeRise_Wind : SmokeRise;
		smoke->entity.curstate.rendermode = kRenderTransAdd;
		smoke->entity.curstate.renderfx = kRenderFxFadeSlow;
	}
	smoke->hitcallback = HugWalls;
	smoke->entity.curstate.rendercolor.r = (byte)red;
	smoke->entity.curstate.rendercolor.g = (byte)green;
	smoke->entity.curstate.rendercolor.b = (byte)blue;
	if ( speed ) speed += gEngfuncs.pfnRandomLong( 0, 5 );
	if ( origin[0] != direction[0] && origin[1] != direction[1] && origin[2] != direction[2] )
	{
		smoke->entity.origin = origin;
		smoke->entity.baseline.origin = Vector( direction ) * (float)speed;
		if ( baseVelocity )
		{
			smoke->entity.baseline.origin.x += 0.9f * baseVelocity[0];
			smoke->entity.baseline.origin.y += 0.9f * baseVelocity[1];
			smoke->entity.baseline.origin.z += 0.5f * baseVelocity[2];
		}
	}
}
#pragma warning( pop )

static void PlayToolEvent( event_args_t* args, int animation, const char* sound )
{
	Vector origin( args->origin );
	if ( EV_IsLocal( args->entindex ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( animation, 2 );
	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON, sound,
		1, 0.4f, 0, gEngfuncs.pfnRandomLong( 0, 15 ) + 94 );
}

void EV_FireCamera( event_args_t* args ) { PlayToolEvent( args, 1, "weapons/camera-1.wav" ); }
void EV_FireFiberOpticCamera( event_args_t* ) {}
void EV_HolsterBlowtorch( event_args_t* ) { blowTorchFiring = 0; }
void EV_IdleBlowtorch( event_args_t* ) { blowTorchFiring = 1; }
void EV_FireRadioControlledBomb( event_args_t* args ) { PlayToolEvent( args, 2, "weapons/blowtorch-1.wav" ); }
void EV_FireLaws( event_args_t* args ) { PlayToolEvent( args, 2, "weapons/law_fire.wav" ); }
void EV_FireBriefcase( event_args_t* args ) { PlayToolEvent( args, 3, "weapons/briefcase_use.wav" ); }
void EV_FireMedkit( event_args_t* args ) { PlayToolEvent( args, 2, "weapons/blowtorch-1.wav" ); }
void EV_FireSyringe( event_args_t* args ) { PlayToolEvent( args, 2, "weapons/syringe_use.wav" ); }
void EV_FireRadio( event_args_t* args ) { PlayToolEvent( args, 1, "weapons/radio_activate.wav" ); }

void EV_FireZipline( event_args_t* args )
{
	if ( EV_IsLocal( args->entindex ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 3, 2 );
}

void EV_RemoveAllDecals( event_args_t* )
{
	for ( int index = 0; index < (int)gEngfuncs.pfnGetCvarFloat( "r_decals" ); ++index )
		gEngfuncs.pEfxAPI->R_DecalRemoveAll( index );
	if ( g_pParticleMan )
		g_pParticleMan->ResetParticles();
	if ( g_DeadPlayerModels[0] )
	{
		for ( int index = 0; index < 64; ++index )
		{
			if ( g_DeadPlayerModels[index] )
			{
				g_DeadPlayerModels[index]->die = 0;
				g_DeadPlayerModels[index] = NULL;
			}
		}
	}
}

int EV_TFC_IsAllyTeam( int, int )
{
	return 0;
}

char* EV_HLDM_DamageDecal( physent_t* entity )
{
	static char decal[32];
	if ( entity->classnumber == 1 )
		sprintf( decal, "{break%i", gEngfuncs.pfnRandomLong( 0, 2 ) + 1 );
	else if ( entity->rendermode == kRenderNormal )
		sprintf( decal, "{shot%i", gEngfuncs.pfnRandomLong( 0, 4 ) + 1 );
	else
		strcpy( decal, "{bproof1" );
	return decal;
}

char EV_HLDM_PlayTextureSound( int, pmtrace_t* trace, float* source, float* end, int bulletType )
{
	char textureType = 0;
	const int index = gEngfuncs.pEventAPI->EV_IndexFromTrace( trace );
	if ( !index )
	{
		const char* texture = gEngfuncs.pEventAPI->EV_TraceTexture( trace->ent, source, end );
		if ( texture )
		{
			char name[64], buffer[64];
			strcpy( name, texture );
			char* start = name;
			if ( *start == '+' || *start == '-' ) start += 2;
			if ( *start == '~' || *start == '{' || *start == ' ' || *start == '!' ) ++start;
			strcpy( buffer, start );
			buffer[12] = 0;
			textureType = PM_FindTextureType( buffer );
		}
	}
	else
	{
		cl_entity_t* entity = gEngfuncs.GetEntityByIndex( index );
		if ( entity && ( entity->curstate.eflags & EFLAG_FLESH_SOUND ) ) textureType = 'F';
	}
	const char* sounds[4];
	int count;
	float volume;
	float attenuation = ATTN_NORM;
	switch ( textureType )
	{
	case 'D':
		sounds[0] = "player/pl_dirt1.wav"; sounds[1] = "player/pl_dirt2.wav"; sounds[2] = "player/pl_dirt3.wav";
		count = 3; volume = 0.9f; break;
	case 'G':
		sounds[0] = "player/pl_grate1.wav"; sounds[1] = "player/pl_grate4.wav";
		count = 2; volume = 0.9f; break;
	case 'M':
		sounds[0] = "player/pl_metal1.wav"; sounds[1] = "player/pl_metal2.wav";
		count = 2; volume = 0.9f; break;
	case 'N':
		sounds[0] = "player/pl_snow1.wav"; sounds[1] = "player/pl_snow2.wav";
		sounds[2] = "player/pl_snow3.wav"; sounds[3] = "player/pl_snow4.wav";
		count = 4; volume = 0.7f; break;
	case 'P':
	case 'Y':
		sounds[0] = "debris/glass1.wav"; sounds[1] = "debris/glass2.wav"; sounds[2] = "debris/glass3.wav";
		count = 3; volume = 0.8f; break;
	case 'S':
		sounds[0] = "player/pl_slosh1.wav"; sounds[1] = "player/pl_slosh3.wav";
		sounds[2] = "player/pl_slosh2.wav"; sounds[3] = "player/pl_slosh4.wav";
		count = 4; volume = 0.9f; break;
	case 'T':
		sounds[0] = "player/pl_tile1.wav"; sounds[1] = "player/pl_tile3.wav";
		sounds[2] = "player/pl_tile2.wav"; sounds[3] = "player/pl_tile4.wav";
		count = 4; volume = 0.8f; break;
	case 'V':
		sounds[0] = "player/pl_duct1.wav"; sounds[1] = "player/pl_duct1.wav";
		count = 2; volume = 0.5f; break;
	case 'W':
		sounds[0] = "debris/wood1.wav"; sounds[1] = "debris/wood2.wav";
		count = 2; volume = 0.9f; break;
	case 'F':
		if ( bulletType == BULLET_PLAYER_CROWBAR ) return 'F';
		sounds[0] = "weapons/bullet_hit1.wav"; sounds[1] = "weapons/bullet_hit2.wav";
		count = 2; volume = 1; attenuation = 1; break;
	default:
		sounds[0] = "player/pl_step1.wav"; sounds[1] = "player/pl_step2.wav";
		count = 2; volume = 0.9f; break;
	}
	const int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 96;
	const int sound = gEngfuncs.pfnRandomLong( 0, count - 1 );
	gEngfuncs.pEventAPI->EV_PlaySound( 0, trace->endpos, CHAN_STATIC, sounds[sound], volume, attenuation, 0, pitch );
	return textureType;
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t* trace, char* decal, char textureType )
{
	const int random = gEngfuncs.pfnRandomLong( 0, 100 );
	if ( random > 50 )
	{
		const char* sound;
		if ( textureType == 'M' || textureType == 'V' )
			sound = random & 1 ? "weapons/ric_metal-1.wav" : "weapons/ric_metal-2.wav";
		else if ( textureType == 'N' || textureType == 'L' )
		{
			static const char* snow[] = { "player/pl_snow1.wav", "player/pl_snow2.wav", "player/pl_snow3.wav", "player/pl_snow4.wav", "player/pl_snow5.wav" };
			sound = snow[random % 5];
		}
		else
		{
			static const char* ricochet[] = { "weapons/ric1.wav", "weapons/ric2.wav", "weapons/ric3.wav", "weapons/ric4.wav", "weapons/ric5.wav", "weapons/ric_conc-1.wav" };
			sound = ricochet[random % 6];
		}
		gEngfuncs.pEventAPI->EV_PlaySound( -1, trace->endpos, CHAN_AUTO, sound, 1, ATTN_NORM, 0, 100 );
	}
	physent_t* entity = gEngfuncs.pEventAPI->EV_GetPhysent( trace->ent );
	if ( decal && *decal && entity && ( entity->solid == SOLID_BSP || entity->movetype == MOVETYPE_PUSHSTEP ) &&
		gEngfuncs.pfnGetCvarFloat( "r_decals" ) != 0 )
	{
		const int index = gEngfuncs.pEventAPI->EV_IndexFromTrace( trace );
		const int decalIndex = gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decal );
		gEngfuncs.pEfxAPI->R_DecalShoot( gEngfuncs.pEfxAPI->Draw_DecalIndex( decalIndex ), index, 0, trace->endpos, 0 );
	}
}

int EV_HLDM_CheckTracer( int index, float* source, float* end, float* forward, float* right,
	int, int frequency, int* count )
{
	const bool player = index > 0 && index <= gEngfuncs.GetMaxClients();
	if ( !frequency || ( *count )++ % frequency != 0 )
		return 0;
	Vector start( source );
	if ( player ) start = start + Vector( 0, 0, -4 ) + Vector( right ) * 2 + Vector( forward ) * 16;
	EV_CreateTracer( start, end );
	return frequency != 1;
}

char UTIL_TextureHit( pmtrace_t* trace, Vector source, Vector end )
{
	const int index = gEngfuncs.pEventAPI->EV_IndexFromTrace( trace );
	if ( index > 0 ) return index <= gEngfuncs.GetMaxClients() ? 'F' : 0;
	if ( index ) return 0;
	const char* texture = gEngfuncs.pEventAPI->EV_TraceTexture( trace->ent, source, end );
	if ( !texture ) return 0;
	char name[64], buffer[64];
	strcpy( name, texture );
	char* start = name;
	if ( *start == '+' || *start == '-' ) start += 2;
	if ( *start == '~' || *start == '{' || *start == ' ' || *start == '!' ) ++start;
	strcpy( buffer, start );
	buffer[12] = 0;
	return PM_FindTextureType( buffer );
}

char* vstr( float* vector )
{
	static int index;
	static char strings[16][1024];
	index = ( index + 1 ) & 15;
	sprintf( strings[index], "%.4f %.4f %.4f", vector[0], vector[1], vector[2] );
	return strings[index];
}

void EV_SpinGauss( event_args_t* args )
{
	Vector origin( args->origin );
	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON, "ambience/pulsemachine.wav",
		1, ATTN_NORM, args->bparam1 ? SND_CHANGE_PITCH : 0, args->iparam1 );
}

static void AdjustPitch( event_args_t* args, const char* const* sounds )
{
	const int sound = ( (unsigned short)args->iparam1 >> 12 ) & 7;
	if ( sound == 0 || sound == 7 )
		return;
	Vector origin( args->origin );
	if ( args->bparam1 )
		gEngfuncs.pEventAPI->EV_StopSound( args->entindex, CHAN_STATIC, sounds[sound - 1] );
	else
		gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_STATIC, sounds[sound - 1],
			(float)( ( args->iparam1 & 63 ) / 40.0 ), ATTN_NORM, SND_CHANGE_PITCH,
			( ( (unsigned short)args->iparam1 >> 6 ) & 63 ) * 10 );
}

void EV_TrainPitchAdjust( event_args_t* args )
{
	static const char* sounds[] = { "plats/ttrain1.wav", "plats/ttrain2.wav", "plats/ttrain3.wav",
		"plats/ttrain4.wav", "plats/ttrain6.wav", "plats/ttrain7.wav" };
	AdjustPitch( args, sounds );
}

void EV_VehiclePitchAdjust( event_args_t* args )
{
	static const char* sounds[] = { "plats/vehicle1.wav", "plats/vehicle2.wav", "plats/vehicle3.wav",
		"plats/vehicle4.wav", "plats/vehicle6.wav", "plats/vehicle7.wav" };
	AdjustPitch( args, sounds );
}

void EV_FireUSP( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin( args->origin ), velocity( args->velocity ), angles( args->angles );
	const float handedness = cl_righthand->value;
	angles.x = (float)( angles.x + args->iparam1 / 100.0 );
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	const int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( args->entindex ) )
	{
		++g_iShotsFired;
		if ( args->bparam2 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( args->bparam1 ? 4 : gEngfuncs.pfnRandomLong( 0, 2 ) + 1, 2 );
		else
		{
			EV_MuzzleFlash();
			gEngfuncs.pEventAPI->EV_WeaponAnimation( args->bparam1 ? 12 : gEngfuncs.pfnRandomLong( 0, 2 ) + 9, 2 );
		}
		if ( gHUD.m_bRenderGunSmoke )
		{
			Vector smokeOrigin = view->attachment[0] - forward * 3;
			const float scale = gEngfuncs.pfnRandomFloat( 0.1f, 0.25f );
			EV_HLDM_CreateSmoke( smokeOrigin, forward, 0, scale, 7, 7, 7, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( smokeOrigin, forward, 20, scale + 0.1f, 10, 10, 10, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( smokeOrigin, forward, 40, scale, 13, 13, 13, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( args->entindex ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			36, -14, handedness == 0 ? -14.0f : 14.0f, false );
		shellOrigin = view->attachment[1];
		shellVelocity = shellVelocity * 0.5f;
		shellVelocity.z += 45;
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
		shellOrigin = origin + forward * 10;
	}
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, args->entindex, 5 );
	if ( args->bparam2 )
		gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON, "weapons/usp1.wav", 1, 2, 0,
			gEngfuncs.pfnRandomLong( 0, 15 ) + 94 );
	else
		gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON, "weapons/usp_unsil-1.wav", 1, ATTN_NORM, 0,
			gEngfuncs.pfnRandomLong( 0, 18 ) + 87 );
	Vector source, spread( args->fparam1, args->fparam2, 0.0f );
	EV_GetGunPosition( args, source, origin );
	EV_HLDM_FireBullets( args->entindex, forward, right, up, 1, source, forward, spread,
		4096, BULLET_PLAYER_45ACP, 0, &tracerCount[args->entindex - 1], 2 );
}

struct RifleEvent
{
	const char* shellModel;
	const char* sound;
	int animationFirst, animationCount;
	float smokeScale;
	int smokeColor, smokeType;
	float shellForwardLeft, shellForwardRight, shellUp, shellRight;
	float shellVelocityScale, shellVelocityZ;
	int shellAngularVelocity;
	float attenuation;
	int bullet;
	bool reverseShell, remoteShellAtMuzzle, layeredSmoke, recoilAfterVectors;
	int smokeSpeed = 3;
	bool countAfterSmoke = false;
	int smokeAttachment = 0;
	bool smokeEnabled = true;
	int recoilDivisor = 100;
	bool integralRecoil = false;
	const char* alternateSound = NULL;
};

static void AddEventRecoil( Vector& angles, event_args_t* args, const RifleEvent& event )
{
	if ( event.integralRecoil )
	{
		angles.x += (float)( args->iparam1 / event.recoilDivisor );
		angles.y += (float)( args->iparam2 / event.recoilDivisor );
	}
	else
	{
		angles.x = (float)( angles.x + (double)args->iparam1 / event.recoilDivisor );
		angles.y = (float)( angles.y + (double)args->iparam2 / event.recoilDivisor );
	}
}

static void FireRifleEvent( event_args_t* args, const RifleEvent& event )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin( args->origin ), velocity( args->velocity ), angles( args->angles );
	const float handedness = cl_righthand->value;
	if ( !event.recoilAfterVectors )
		AddEventRecoil( angles, args, event );
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	if ( event.recoilAfterVectors )
		AddEventRecoil( angles, args, event );
	const int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( event.shellModel );
	if ( EV_IsLocal( args->entindex ) )
	{
		if ( !event.countAfterSmoke ) ++g_iShotsFired;
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( gEngfuncs.pfnRandomLong( 0, event.animationCount - 1 ) + event.animationFirst, 2 );
		if ( event.smokeEnabled && gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[event.smokeAttachment], forward, event.smokeSpeed, event.smokeScale,
				event.smokeColor, event.smokeColor, event.smokeColor, event.smokeType, velocity, false, 35 );
			if ( event.layeredSmoke )
			{
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 35, 0.35f, 30, 30, 30, 2, velocity, false, 35 );
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 70, 0.3f, 25, 25, 25, 2, velocity, false, 35 );
			}
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( args->entindex ) )
	{
		if ( event.countAfterSmoke ) ++g_iShotsFired;
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			handedness == 0 ? event.shellForwardLeft : event.shellForwardRight, event.shellUp,
			handedness == 0 ? -event.shellRight : event.shellRight, event.reverseShell );
		shellOrigin = view->attachment[1];
		shellVelocity = shellVelocity * event.shellVelocityScale;
		shellVelocity.z += event.shellVelocityZ;
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
		if ( event.remoteShellAtMuzzle ) shellOrigin = origin + forward * 10;
	}
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, args->entindex, event.shellAngularVelocity );
	const char* sound = event.sound;
	if ( event.alternateSound && gEngfuncs.pfnRandomLong( 0, 1 ) == 0 ) sound = event.alternateSound;
	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON, sound, 1, event.attenuation,
		0, gEngfuncs.pfnRandomLong( 0, 15 ) + 94 );
	Vector source, spread( args->fparam1, args->fparam2, event.layeredSmoke ? args->fparam1 : 0.0f );
	EV_GetGunPosition( args, source, origin );
	EV_HLDM_FireBullets( args->entindex, forward, right, up, 1, source, forward, spread,
		8192, event.bullet, 0, &tracerCount[args->entindex - 1], 2 );
}

void EV_FireMP5N( event_args_t* args )
{
	static const RifleEvent event = { "models/pshell.mdl", "weapons/mp5-1.wav", 3, 3,
		0.25f, 10, 3, 34, 34, -10, 11, 1, -50, 13, 0.64f, BULLET_PLAYER_9MM,
		false, true, false, false };
	FireRifleEvent( args, event );
}

void EV_FireAK47( event_args_t* args )
{
	static const RifleEvent event = { "models/rshell.mdl", "weapons/ak47-1.wav", 3, 3,
		0.35f, 20, 4, 20, 20, -8, 10, 1, -75, 9, 0.4f, BULLET_PLAYER_762MM,
		false, true, false, false };
	FireRifleEvent( args, event );
}

void EV_FireAug( event_args_t* args )
{
	static const RifleEvent event = { "models/rshell.mdl", "weapons/aug-1.wav", 3, 3,
		0.26f, 15, 4, 17, 17, -8, 14, 1.25f, -122, 8, 0.48f, BULLET_PLAYER_556MM,
		true, true, false, false };
	FireRifleEvent( args, event );
}

void EV_FireG3SG1( event_args_t* args )
{
	static const RifleEvent event = { "models/rshell.mdl", "weapons/g3sg1-1.wav", 1, 2,
		0.3f, 35, 4, 20, 17, -8, 10, 1.5f, -50, 17, 0.4f, BULLET_PLAYER_762MM,
		false, false, true, false };
	FireRifleEvent( args, event );
}

void EV_FireSG550( event_args_t* args )
{
	static const RifleEvent event = { "models/rshell.mdl", "weapons/sg550-1.wav", 1, 2,
		0.3f, 35, 4, 20, 17, -8, 10, 1.5f, -50, 17, 0.4f, BULLET_PLAYER_556MM,
		false, false, true, true };
	FireRifleEvent( args, event );
}

void EV_FireSG552( event_args_t* args )
{
	static const RifleEvent event = { "models/rshell.mdl", "weapons/sg552-1.wav", 3, 3,
		0.2f, 18, 4, 20, 20, -8, 10, 1.65f, -120, 15, 0.4f, BULLET_PLAYER_556MM,
		false, false, false, false };
	FireRifleEvent( args, event );
}

void EV_FireMac10( event_args_t* args )
{
	static const RifleEvent event = { "models/pshell.mdl", "weapons/mac10-1.wav", 3, 3,
		0.25f, 8, 3, 32, 32, -9, 11, 0.75f, -65, 7, 0.72f, BULLET_PLAYER_9MM,
		false, true, false, false, 2, true };
	FireRifleEvent( args, event );
}

void EV_FireTMP( event_args_t* args )
{
	static const RifleEvent event = { "models/pshell.mdl", "weapons/tmp-1.wav", 3, 3,
		0.2f, 10, 3, 32, 32, -6, 11, 1.2f, -50, 8, 1.6f, BULLET_PLAYER_9MM,
		false, true, false, false };
	FireRifleEvent( args, event );
}

void EV_FireP90( event_args_t* args )
{
	static const RifleEvent event = { "models/rshell.mdl", "weapons/p90-1.wav", 3, 3,
		0.16f, 7, 4, 35, 35, -16, 22, 1.25f, -50, 10, 0.64f, BULLET_PLAYER_57MM,
		true, false, false, false, 3, true };
	FireRifleEvent( args, event );
}

void EV_FireUMP45( event_args_t* args )
{
	static const RifleEvent event = { "models/pshell.mdl", "weapons/ump45-1.wav", 3, 3,
		0.25f, 10, 3, 34, 34, -10, 11, 1, -50, 13, 0.64f, BULLET_PLAYER_45ACP,
		false, false, false, false };
	FireRifleEvent( args, event );
}

void EV_FireM249( event_args_t* args )
{
	static const RifleEvent event = { "models/rshell.mdl", "weapons/m249-1.wav", 1, 2,
		0.26f, 15, 4, 20, 20, -10, 13, 1.2f, -75, 10, 0.52f, BULLET_PLAYER_556MM,
		true, false, false, false };
	FireRifleEvent( args, event );
}

void EV_FireM60( event_args_t* args )
{
	static const RifleEvent event = { "models/rshell.mdl", "weapons/m60-1.wav", 1, 2,
		0.26f, 15, 4, 20, 20, -10, 13, 1.2f, -75, 10, 0.52f, BULLET_PLAYER_762MM,
		true, true, false, false };
	FireRifleEvent( args, event );
}

void EV_FireGalil( event_args_t* args )
{
	static const RifleEvent event = { "models/rshell.mdl", "weapons/galil-1.wav", 3, 3,
		0, 0, 0, 20, 20, -8, 10, 1, -75, 9, 0.4f, BULLET_PLAYER_762MM,
		false, false, false, false, 3, false, 0, false, 10000000, true, "weapons/galil-2.wav" };
	FireRifleEvent( args, event );
}

void EV_FireFamas( event_args_t* args )
{
	static const RifleEvent event = { "models/rshell.mdl", "weapons/famas-1.wav", 3, 3,
		0, 0, 0, 17, 17, -8, 14, 1.25f, -122, 8, 0.48f, BULLET_PLAYER_556MM,
		true, false, false, false, 3, false, 0, false, 10000000, true, "weapons/famas-2.wav" };
	FireRifleEvent( args, event );
}

void EV_FireM4A1( event_args_t* args )
{
	RifleEvent event = { "models/rshell.mdl", "weapons/m4a1-1.wav", 1, 3,
		0.3f, 16, 3, 20, 20, -8, 10, 1, -45, 10, 1.4f, BULLET_PLAYER_556MM,
		false, true, false, false, 3, true };
	if ( !args->bparam1 )
	{
		event.animationFirst = 8;
		event.smokeScale = 0.2f;
		event.smokeType = 4;
		event.smokeAttachment = 2;
		event.sound = "weapons/m4a1_unsil-1.wav";
		event.alternateSound = "weapons/m4a1_unsil-2.wav";
		event.attenuation = 0.52f;
	}
	FireRifleEvent( args, event );
}

static void FireSniperEvent( event_args_t* args, bool awp )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin( args->origin ), velocity( args->velocity ), angles( args->angles );
	angles.x = (float)( angles.x + args->iparam1 / 100.0 );
	angles.y = (float)( angles.y + args->iparam2 / 100.0 );
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	if ( EV_IsLocal( args->entindex ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( gEngfuncs.pfnRandomLong( 0, awp ? 2 : 1 ) + 1, 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, awp ? 0.5f : 0.45f,
				awp ? 20 : 25, awp ? 20 : 25, awp ? 20 : 25, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, awp ? 40 : 35, awp ? 0.5f : 0.45f,
				awp ? 15 : 20, awp ? 15 : 20, awp ? 15 : 20, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, awp ? 80 : 70, awp ? 0.45f : 0.35f,
				awp ? 10 : 15, awp ? 10 : 15, awp ? 10 : 15, 2, velocity, false, 35 );
		}
	}
	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON,
		awp ? "weapons/awp1.wav" : "weapons/scout_fire-1.wav", 1, awp ? 0.28f : 1.6f,
		0, gEngfuncs.pfnRandomLong( 0, 15 ) + 94 );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector spread = awp ? Vector( args->fparam1, args->fparam2, 0.0f ) :
		Vector( (float)( args->fparam1 / 1000.0 ), (float)( args->fparam2 / 1000.0 ), 0.0f );
	EV_HLDM_FireBullets( args->entindex, forward, right, up, 1, source, forward, spread,
		8192, awp ? BULLET_PLAYER_338MAG : BULLET_PLAYER_762MM, 0, &tracerCount[args->entindex - 1], awp ? 3 : 2 );
}

void EV_FireAWP( event_args_t* args ) { FireSniperEvent( args, true ); }
void EV_FireScout( event_args_t* args ) { FireSniperEvent( args, false ); }

enum PistolEvent
{
	PISTOL_DEAGLE,
	PISTOL_P228,
	PISTOL_FIVESEVEN,
	PISTOL_GLOCK18,
	PISTOL_SHIELD
};

static void FirePistolEvent( event_args_t* args, PistolEvent pistol )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin( args->origin ), velocity( args->velocity ), angles( args->angles );
	const float handedness = cl_righthand->value;
	const bool lateRecoil = pistol == PISTOL_FIVESEVEN || pistol == PISTOL_SHIELD;
	if ( !lateRecoil )
	{
		angles.x = (float)( angles.x + args->iparam1 / 100.0 );
		angles.y = (float)( angles.y + args->iparam2 / 100.0 );
	}
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	if ( lateRecoil )
	{
		if ( pistol == PISTOL_SHIELD )
		{
			angles.x += (float)( args->iparam1 / 10000000 );
			angles.y += (float)( args->iparam2 / 10000000 );
		}
		else
		{
			angles.x = (float)( angles.x + args->iparam1 / 100.0 );
			angles.y = (float)( angles.y + args->iparam2 / 100.0 );
		}
	}
	const int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( args->entindex ) )
	{
		if ( pistol != PISTOL_GLOCK18 ) ++g_iShotsFired;
		EV_MuzzleFlash();
		int animation;
		if ( pistol == PISTOL_GLOCK18 ) animation = args->bparam1 ? 6 : 5;
		else if ( args->bparam1 ) animation = pistol == PISTOL_P228 ? 4 : 3;
		else animation = gEngfuncs.pfnRandomLong( 0, pistol == PISTOL_P228 ? 2 : 1 ) + 1;
		gEngfuncs.pEventAPI->EV_WeaponAnimation( animation, 2 );
		if ( pistol != PISTOL_SHIELD && gHUD.m_bRenderGunSmoke )
		{
			if ( pistol == PISTOL_GLOCK18 )
			{
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 0, 0.2f, 10, 10, 10, 3, velocity, false, 35 );
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 40, 0.4f, 20, 20, 20, 2, velocity, false, 35 );
			}
			else
			{
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 0, 0.25f, 10, 10, 10, 3, velocity, false, 35 );
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 25, 0.3f, 15, 15, 15, 2, velocity, false, 35 );
				const int color = pistol == PISTOL_DEAGLE ? 25 : 20;
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 50, 0.2f, color, color, color, 2, velocity, false, 35 );
			}
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( args->entindex ) )
	{
		if ( pistol == PISTOL_GLOCK18 ) ++g_iShotsFired;
		const float forwardScale = pistol == PISTOL_DEAGLE ? 35.0f : 36.0f;
		const float upScale = pistol == PISTOL_DEAGLE ? -11.0f : -14.0f;
		const float rightScale = pistol == PISTOL_DEAGLE ? 16.0f : 14.0f;
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			forwardScale, upScale, handedness == 0 ? -rightScale : rightScale, false );
		shellOrigin = view->attachment[1];
		shellVelocity = shellVelocity * ( pistol == PISTOL_GLOCK18 ? 0.65f : 0.75f );
		shellVelocity.z += pistol == PISTOL_DEAGLE || pistol == PISTOL_GLOCK18 ? 25.0f : 35.0f;
	}
	else EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, args->entindex, pistol == PISTOL_DEAGLE ? 5 : 4 );
	const char* sound;
	int bullet;
	float attenuation = ATTN_NORM;
	switch ( pistol )
	{
	case PISTOL_DEAGLE: sound = "weapons/deagle-1.wav"; bullet = BULLET_PLAYER_50AE; attenuation = 0.6f; break;
	case PISTOL_P228: sound = "weapons/p228-1.wav"; bullet = BULLET_PLAYER_357SIG; break;
	case PISTOL_GLOCK18: sound = "weapons/glock18-2.wav"; bullet = BULLET_PLAYER_9MM; break;
	default: sound = "weapons/fiveseven-1.wav"; bullet = BULLET_PLAYER_57MM; break;
	}
	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON, sound, 1, attenuation, 0,
		gEngfuncs.pfnRandomLong( 0, 15 ) + 94 );
	Vector source, spread( args->fparam1, args->fparam2, pistol == PISTOL_GLOCK18 ? args->fparam1 : 0.0f );
	EV_GetGunPosition( args, source, origin );
	EV_HLDM_FireBullets( args->entindex, forward, right, up, 1, source, forward, spread,
		pistol == PISTOL_GLOCK18 ? 4096.0f : 8192.0f, bullet, 0, &tracerCount[args->entindex - 1], 2 );
}

void EV_FireDeagle( event_args_t* args ) { FirePistolEvent( args, PISTOL_DEAGLE ); }
void EV_FireP228( event_args_t* args ) { FirePistolEvent( args, PISTOL_P228 ); }
void EV_FireFiveSeven( event_args_t* args ) { FirePistolEvent( args, PISTOL_FIVESEVEN ); }
void EV_FireGlock18( event_args_t* args ) { FirePistolEvent( args, PISTOL_GLOCK18 ); }
void EV_FireShieldGun( event_args_t* args ) { FirePistolEvent( args, PISTOL_SHIELD ); }

void EV_Knife( event_args_t* args )
{
	if ( EV_IsLocal( args->entindex ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( args->iparam1, 2 );
}

void EV_FireBlowtorch( event_args_t* args )
{
	blowTorchFiring = 2;
	Vector origin( args->origin );
	if ( EV_IsLocal( args->entindex ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 1, 2 );
	Vector angles( args->angles );
	angles.x += (float)( args->iparam1 / 10000000 );
	angles.y += (float)( args->iparam2 / 10000000 );
	Vector forward, right, up, source;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	EV_GetGunPosition( args, source, origin );
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( args->entindex, forward, right, up, 1, source, forward, spread,
		36, BULLET_PLAYER_BLOWTORCH, 0, &tracerCount[args->entindex - 1], 2 );
}

static void FireShotgunEvent( event_args_t* args, bool automatic )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin( args->origin ), velocity( args->velocity ), angles( args->angles );
	Vector forward, right, up;
	float handedness = 0;
	int shell = 0;
	if ( automatic )
	{
		handedness = cl_righthand->value;
		angles.x = (float)( angles.x + args->iparam1 / 100.0 );
		angles.y = (float)( angles.y + args->iparam2 / 100.0 );
	}
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	if ( automatic ) shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shotgunshell.mdl" );
	if ( EV_IsLocal( args->entindex ) )
	{
		if ( !automatic ) ++g_iShotsFired;
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( gEngfuncs.pfnRandomLong( 0, 1 ) + 1, 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			if ( automatic )
			{
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.45f, 15, 15, 15, 3, velocity, false, 35 );
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 40, 0.35f, 9, 9, 9, 2, velocity, false, 35 );
			}
			else
			{
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.5f, 25, 25, 25, 3, velocity, false, 35 );
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 25, 0.5f, 20, 20, 20, 2, velocity, false, 35 );
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 50, 0.45f, 15, 15, 15, 2, velocity, false, 35 );
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 75, 0.35f, 7, 7, 7, 2, velocity, false, 35 );
			}
		}
	}
	if ( automatic )
	{
		Vector shellVelocity, shellOrigin;
		if ( EV_IsLocal( args->entindex ) )
		{
			++g_iShotsFired;
			EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
				22, -9, handedness == 0 ? -11.0f : 11.0f, false );
			shellOrigin = view->attachment[1];
			shellVelocity = shellVelocity * 1.25f;
			shellVelocity.z -= 50;
		}
		else
		{
			EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
			shellOrigin = origin + forward * 10;
		}
		EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHOTSHELL, args->entindex, 3 );
	}
	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON,
		automatic ? "weapons/xm1014-1.wav" : "weapons/m3-1.wav", 1, automatic ? 0.52f : 0.48f,
		0, gEngfuncs.pfnRandomLong( 0, 15 ) + 94 );
	const float cone = automatic ? 0.0725f : 0.0675f;
	Vector source, spread( cone, cone, 0.0f );
	EV_GetGunPosition( args, source, origin );
	EV_HLDM_FireBullets( args->entindex, forward, right, up, automatic ? 6 : 9, source, forward,
		spread, 8192, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[args->entindex - 1], 1 );
}

void EV_FireM3( event_args_t* args ) { FireShotgunEvent( args, false ); }
void EV_FireXM1014( event_args_t* args ) { FireShotgunEvent( args, true ); }

static void FireEliteEvent( event_args_t* args, bool rightHand )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin( args->origin ), velocity( args->velocity ), angles( args->angles );
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	const int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( args->entindex ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = -1;
		if ( args->iparam2 <= 1 ) animation = 7;
		else if ( args->fparam1 >= 0.5 ) animation = 6;
		else if ( args->fparam1 >= 0.4 ) animation = 5;
		else if ( args->fparam1 >= 0.3 ) animation = 4;
		else if ( args->fparam1 >= 0.2 ) animation = 3;
		else if ( args->fparam1 >= 0 ) animation = 2;
		if ( animation >= 0 ) gEngfuncs.pEventAPI->EV_WeaponAnimation( animation + ( rightHand ? 6 : 0 ), 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			Vector muzzle = view->attachment[rightHand ? 1 : 0];
			EV_HLDM_CreateSmoke( muzzle, forward, 0, 0.25f, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( muzzle, forward, 25, 0.3f, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( muzzle, forward, 50, 0.2f, 25, 25, 25, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( args->entindex ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			35, -11, rightHand ? 16.0f : -16.0f, false );
		shellOrigin = view->attachment[rightHand ? 3 : 2];
		shellVelocity = shellVelocity * 0.75f;
		shellVelocity.z += 25;
	}
	else EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, args->entindex, 5 );
	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON, "weapons/elite_fire.wav", 1, ATTN_NORM,
		0, gEngfuncs.pfnRandomLong( 0, 15 ) + 94 );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	source = source + right * ( rightHand ? 5.0f : -5.0f );
	Vector spread( args->fparam2, (float)( args->iparam1 / 100.0 ), 0.0f );
	EV_HLDM_FireBullets( args->entindex, forward, right, up, 1, source, forward, spread,
		8192, BULLET_PLAYER_50AE, 0, &tracerCount[args->entindex - 1], 2 );
}

void EV_FireEliteLeft( event_args_t* args ) { FireEliteEvent( args, false ); }
void EV_FireEliteRight( event_args_t* args ) { FireEliteEvent( args, true ); }

void CreateCorpse( Vector origin, Vector angles, const char* model, float animationTime, int sequence, int body )
{
	gEngfuncs.pEventAPI->EV_FindModelIndex( model );
	const int modelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( model );
	Vector velocity( 0, 0, 0 );
	TEMPENTITY* corpse = gEngfuncs.pEfxAPI->R_TempModel( origin, velocity, angles, 100, modelIndex, 0 );
	if ( !corpse ) return;
	corpse->flags |= FTENT_PLAYERBODY | FTENT_CLIENTCUSTOM | FTENT_PERSIST | FTENT_SPRANIMATE | FTENT_COLLIDEWORLD;
	corpse->entity.curstate.framerate = 1;
	corpse->frameMax = 255;
	corpse->entity.curstate.animtime = animationTime;
	corpse->entity.curstate.renderamt = 255;
	corpse->entity.curstate.sequence = sequence;
	corpse->entity.curstate.frame = 0;
	corpse->entity.curstate.body = body;
	corpse->entity.curstate.fuser1 = gHUD.m_flTime + 1;
	const float currentTime = gHUD.m_flTime;
	corpse->entity.curstate.fuser2 = gEngfuncs.pfnGetCvarFloat( "cl_corpsestay" ) + currentTime;
	corpse->callback = RemoveBody;
	corpse->hitcallback = HitBody;
	corpse->bounceFactor = 0;
	corpse->die = gEngfuncs.GetClientTime() + corpse->entity.curstate.fuser2 + 5;
	for ( int index = 0; index < 64; ++index )
		if ( !g_DeadPlayerModels[index] )
		{
			g_DeadPlayerModels[index] = corpse;
			return;
		}
}

void EV_CreateGlass( event_args_t* args )
{
	const Vector direction( args->angles );
	if ( args->bparam1 )
		DecayGlass( args->origin[0], args->origin[1], args->iparam2, args->iparam1,
			args->fparam1, args->fparam2, direction );
	else
		CreateTriangle( Vector( args->origin ), direction, args->fparam1, args->fparam2, args->iparam1 );
}

static void CreateSmokeRing( event_args_t* args, bool poison )
{
	for ( int ring = 0; ring < 2; ++ring )
	{
		for ( int angle = 0; angle < 360; angle += 45 )
		{
			Vector angles( 0.0f, (float)angle, 0.0f ), forward, right, up;
			gEngfuncs.pfnAngleVectors( angles, forward, right, up );
			forward = forward.Normalize();
			const Vector velocity = forward * 375 + up * ( ring ? 265.0f : 165.0f );
			if ( poison ) CreatePoisonSmoke( Vector( args->origin ), velocity, false, false, true );
			else CreateGasSmoke( Vector( args->origin ), velocity, false, false, true );
		}
	}
	for ( int index = 0; index < 2; ++index )
	{
		Vector angles( 0, index ? 270 : 45, 0 ), forward, right, up;
		gEngfuncs.pfnAngleVectors( angles, forward, right, up );
		forward = forward.Normalize();
		const Vector velocity = forward * 375 + up * 120;
		if ( poison ) CreatePoisonSmoke( Vector( args->origin ), velocity, false, index != 0, true );
		else CreateGasSmoke( Vector( args->origin ), velocity, false, index != 0, true );
	}
}

void CreateSmokePop( event_args_t* args )
{
	CreateSmokeRing( args, false );
}

void CreatePoisonPop( event_args_t* args )
{
	const int sound = gEngfuncs.pfnRandomLong( 0, 2 );
	Vector origin( args->origin );
	if ( sound >= 0 && sound <= 2 )
	{
		static const char* sounds[] = { "weapons/explode3.wav", "weapons/explode4.wav", "weapons/explode5.wav" };
		for ( int index = sound; index < 3; ++index )
			gEngfuncs.pEventAPI->EV_PlaySound( -1, origin, CHAN_AUTO,
				args->bparam2 ? "weapons/law_explode.wav" : sounds[index], 1, ATTN_NORM, 0, 100 );
	}
	CreateSmokeRing( args, true );
}

void EV_CreateSmoke( event_args_t* args )
{
	if ( args->iparam2 == 1 ) { CreateSmokePop( args ); return; }
	if ( args->iparam2 == 2 ) { CreatePoisonPop( args ); return; }
	Vector origin( args->origin );
	Vector direction( args->fparam1, args->fparam2, (float)( args->iparam1 / 100.0 ) );
	const int color = gEngfuncs.pfnRandomLong( 155, 175 );
	const int speed = (int)gEngfuncs.pfnRandomFloat( 5, 10 );
	EV_HLDM_CreateSmoke( origin, direction, speed, 1, color, color, color, 1, NULL, true, 25 );
	const float x = (float)( 0.9659274596269048 * direction.x - 0.25881294932579024 * direction.y );
	const float y = (float)( 0.9659274596269048 * direction.y + 0.25881294932579024 * direction.x );
	direction = Vector( x * 0.75f, y * 0.75f, direction.z * 0.75f );
	const int nextSpeed = (int)gEngfuncs.pfnRandomFloat( 2, 5 );
	EV_HLDM_CreateSmoke( origin, direction, nextSpeed, 1.2f, color, color, color, 1, NULL, true, 20 );
}

void EV_GrenadeExplosion( event_args_t* args )
{
	Vector origin( args->origin );
	for ( int angle = 0; angle < 360; angle += 45 )
	{
		Vector angles( 0.0f, (float)angle, 0.0f ), forward, right, up;
		gEngfuncs.pfnAngleVectors( angles, forward, right, up );
		forward = forward.Normalize();
		Vector down = origin - Vector( 0, 0, 100 );
		pmtrace_t trace;
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( origin, down, PM_NORMAL, -1, &trace );
		if ( trace.fraction != 1 )
		{
			const float scale = gEngfuncs.pfnRandomFloat( 75, 80 );
			CreateDustRing( trace.endpos, forward * 500 + up * 25, scale );
		}
	}
	for ( int angle = 0; angle < 180; angle += 45 )
	{
		Vector angles( 0.0f, (float)angle, 0.0f ), forward, right, up;
		gEngfuncs.pfnAngleVectors( angles, forward, right, up );
		const float height = gEngfuncs.pfnRandomFloat( 5, 25 );
		const Vector smokeOrigin = origin + up * height;
		forward = forward.Normalize();
		CreateGrenadeSmoke( smokeOrigin, up * ( 55 - height * 2 ), true );
	}
	for ( int index = 0; index < 16; ++index )
	{
		Vector angles( 0, index * 45, 0 ), forward, right, up;
		gEngfuncs.pfnAngleVectors( angles, forward, right, up );
		forward = forward.Normalize();
		const float rise = gEngfuncs.pfnRandomFloat( 25, 40 ) * index;
		const float speed = gEngfuncs.pfnRandomFloat( 300, 400 );
		CreateGrenadeSmoke( origin + up * (float)( 5 * index ), forward * speed + up * rise, false );
	}
	const int sound = gEngfuncs.pfnRandomLong( 0, 2 );
	if ( sound >= 0 && sound <= 2 )
	{
		static const char* sounds[] = { "weapons/explode3.wav", "weapons/explode4.wav", "weapons/explode5.wav" };
		for ( int index = sound; index < 3; ++index )
			gEngfuncs.pEventAPI->EV_PlaySound( -1, origin, CHAN_AUTO,
				args->bparam2 ? "weapons/law_explode.wav" : sounds[index], 1, ATTN_NORM, 0, 100 );
	}
	dlight_t* light = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
	light->origin = origin;
	light->radius = 300;
	light->color.r = 255;
	light->color.g = 190;
	light->color.b = 40;
	light->die = gEngfuncs.GetClientTime() + 1;
	light->decay = 275;
	Vector down = origin - Vector( 0, 0, 100 );
	pmtrace_t trace;
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, down, PM_NORMAL, -1, &trace );
	CreateHEGrenadeFlash( trace.endpos );
	const int materialIndex = g_pMaterialSystem.GetMaterialIndexAtPos( &trace, trace.endpos, down );
	if ( !g_pMaterialSystem.GetMaterialByIndex( materialIndex ) ) return;
	materials_t gib = *g_pMaterialSystem.GetMaterialByIndex( materialIndex );
	if ( !gib.m_iSpawnDebris ) return;
	const int count = gEngfuncs.pfnRandomLong( 30, 55 );
	for ( int index = 0; index < count; ++index )
	{
		const float x = trace.endpos.x + gEngfuncs.pfnRandomFloat( -50, 50 );
		const float y = trace.endpos.y + gEngfuncs.pfnRandomFloat( -50, 50 );
		gib.m_flGravity = 0.3f;
		gib.m_flSize = gib.m_flSize >= 3 ? 10.0f : 20.0f;
		CreateCustomGib( Vector( x, y, trace.endpos.z + 1 ), trace.plane.normal, gib.m_szTextName, 3, &gib );
	}
}

void V_PunchAxis( int axis, float punch );

void EV_FireGauss( event_args_t* args )
{
	const int index = args->entindex;
	const bool primary = args->bparam1 != 0;
	float damage = args->fparam1;
	Vector origin( args->origin ), angles( args->angles ), source, forward, right, up;
	gEngfuncs.pEventAPI->EV_KillEvents( index, "events/gaussspin.sc" );
	EV_GetGunPosition( args, source, origin );
	const int beam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/smoke.spr" );
	const int balls = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/hotglow.spr" );
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	Vector destination = source + forward * 8192;
	if ( EV_IsLocal( index ) )
	{
		V_PunchAxis( 0, -2 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 6, 2 );
	}
	gEngfuncs.pEventAPI->EV_StopSound( index, CHAN_WEAPON, "ambience/pulsemachine.wav" );
	const int pitch = gEngfuncs.pfnRandomLong( 0, 31 ) + 85;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/gauss2.wav", damage * 0.0025f + 0.5f, ATTN_NORM, 0, pitch );
	bool first = true;
	bool punched = false;
	Vector zero( 0, 0, 0 );
	for ( int hits = 0; damage > 10 && hits < 10; ++hits )
	{
		pmtrace_t trace;
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( FALSE, TRUE );
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( index - 1 );
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( source, destination, PM_STUDIO_BOX, -1, &trace );
		gEngfuncs.pEventAPI->EV_PopPMStates();
		if ( trace.allsolid ) break;
		physent_t* entity = gEngfuncs.pEventAPI->EV_GetPhysent( trace.ent );
		if ( !entity ) break;
		if ( first )
		{
			if ( EV_IsLocal( index ) ) EV_MuzzleFlash();
			gEngfuncs.pEfxAPI->R_BeamEntPoint( index | 0x1000, trace.endpos, beam, 0.1f,
				primary ? 1.0f : 2.5f, 0, primary ? 128.0f : damage, 0, 0, 0, 255, primary ? 128.0f : 255.0f, primary ? 0.0f : 255.0f );
		}
		else
			gEngfuncs.pEfxAPI->R_BeamPoints( source, trace.endpos, beam, 0.1f,
				primary ? 1.0f : 2.5f, 0, primary ? 128.0f : damage, 0, 0, 0, 255, primary ? 128.0f : 255.0f, primary ? 0.0f : 255.0f );
		first = false;
		if ( entity->solid != SOLID_BSP )
		{
			source = trace.endpos + forward;
			continue;
		}
		const float incidence = -DotProduct( trace.plane.normal, forward );
		if ( incidence < 0.5 )
		{
			forward = forward + trace.plane.normal * ( 2 * incidence );
			source = trace.endpos + forward * 8;
			destination = source + forward * 8192;
			const float reflectedDamage = damage * incidence;
			gEngfuncs.pEfxAPI->R_TempSprite( trace.endpos, zero, 0.2f, balls, kRenderGlow, kRenderFxNoDissipation,
				reflectedDamage / 255, reflectedDamage * 0.5f * 0.1f, FTENT_FADEOUT );
			Vector end = trace.endpos + trace.plane.normal;
			gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, trace.endpos, end, balls, (int)( reflectedDamage * 0.3 ), 0.1f,
				gEngfuncs.pfnRandomFloat( 10, 20 ) / 100, 100, 255, 200 );
			damage = incidence == 0 ? damage * 0.9f : damage * ( 1 - incidence );
			continue;
		}
		EV_HLDM_DecalGunshot( &trace, BULLET_MONSTER_12MM, 0, 0, 0, 0, false, 'C', -1 );
		gEngfuncs.pEfxAPI->R_TempSprite( trace.endpos, zero, 1, balls, kRenderGlow, kRenderFxNoDissipation,
			damage / 255, 6, FTENT_FADEOUT );
		if ( punched ) return;
		punched = true;
		if ( primary )
		{
			gEngfuncs.pEfxAPI->R_TempSprite( trace.endpos, zero, 0.2f, balls, kRenderGlow, kRenderFxNoDissipation,
				200.0f / 255, 0.3f, FTENT_FADEOUT );
			Vector end = trace.endpos + trace.plane.normal;
			gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, trace.endpos, end, balls, 8, 0.6f,
				gEngfuncs.pfnRandomFloat( 10, 20 ) / 100, 100, 255, 200 );
			damage = 0;
			continue;
		}
		Vector start = trace.endpos + forward * 8;
		pmtrace_t exitTrace;
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( index - 1 );
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( start, destination, PM_STUDIO_BOX, -1, &exitTrace );
		if ( exitTrace.allsolid ) damage = 0;
		else
		{
			gEngfuncs.pEventAPI->EV_PlayerTrace( exitTrace.endpos, trace.endpos, PM_STUDIO_BOX, -1, &exitTrace );
			const float thickness = ( exitTrace.endpos - trace.endpos ).Length();
			if ( damage > thickness )
			{
				damage -= thickness == 0 ? 1.0f : thickness;
				Vector end = trace.endpos - forward;
				gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, trace.endpos, end, balls, 3, 0.1f,
					gEngfuncs.pfnRandomFloat( 10, 20 ) / 100, 100, 255, 100 );
				EV_HLDM_DecalGunshot( &exitTrace, BULLET_MONSTER_12MM, 0, 0, 0, 0, false, 'C', -1 );
				gEngfuncs.pEfxAPI->R_TempSprite( exitTrace.endpos, zero, 0.1f, balls, kRenderGlow, kRenderFxNoDissipation,
					damage / 255, 6, FTENT_FADEOUT );
				end = exitTrace.endpos - forward;
				gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, exitTrace.endpos, end, balls, (int)( damage * 0.3 ), 0.1f,
					gEngfuncs.pfnRandomFloat( 10, 20 ) / 100, 200, 255, 40 );
				source = exitTrace.endpos + forward;
			}
		}
		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

static void FireLegacyGlock( event_args_t* args, bool secondary )
{
	Vector origin( args->origin ), angles( args->angles ), velocity( args->velocity );
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	const int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shell.mdl" );
	if ( EV_IsLocal( args->entindex ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( secondary ? 3 : ( args->bparam1 ? 4 : 3 ), 2 );
	}
	Vector shellVelocity, shellOrigin;
	EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, args->entindex, 1 );
	const int pitch = gEngfuncs.pfnRandomLong( 0, 3 ) + 98;
	const float volume = gEngfuncs.pfnRandomFloat( 0.92f, 1 );
	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", volume, ATTN_NORM, 0, pitch );
	Vector source, spread = secondary ? Vector( 0.1f, 0.1f, 0.0f ) : Vector( 0.01f, 0.01f, 0.0f );
	EV_GetGunPosition( args, source, origin );
	EV_HLDM_FireBullets( args->entindex, forward, right, up, 1, source, forward, spread,
		8192, BULLET_PLAYER_9MM, 0, &tracerCount[args->entindex - 1], 1 );
	if ( EV_IsLocal( args->entindex ) ) V_PunchAxis( 0, -2 );
}

void EV_FireGlock1( event_args_t* args ) { FireLegacyGlock( args, false ); }
void EV_FireGlock2( event_args_t* args ) { FireLegacyGlock( args, true ); }

void EV_FirePython( event_args_t* args )
{
	Vector origin( args->origin ), angles( args->angles ), forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	if ( EV_IsLocal( args->entindex ) )
	{
		const int maxClients = gEngfuncs.GetMaxClients();
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 2, maxClients != 1 );
	}
	const int sound = gEngfuncs.pfnRandomLong( 0, 1 );
	if ( sound == 0 || sound == 1 )
	{
		const float volume = gEngfuncs.pfnRandomFloat( 0.8f, 0.9f );
		gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON,
			sound ? "weapons/357_shot2.wav" : "weapons/357_shot1.wav", volume, ATTN_NORM, 0, 100 );
	}
	Vector source, spread( 0.00873f, 0.00873f, 0.0f );
	EV_GetGunPosition( args, source, origin );
	EV_HLDM_FireBullets( args->entindex, forward, right, up, 1, source, forward, spread,
		8192, BULLET_PLAYER_357, 0, &tracerCount[args->entindex - 1], 1 );
	if ( EV_IsLocal( args->entindex ) ) V_PunchAxis( 0, -10 );
}

void EV_FireMP5( event_args_t* args )
{
	Vector origin( args->origin ), angles( args->angles ), velocity( args->velocity );
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	const int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shell.mdl" );
	if ( EV_IsLocal( args->entindex ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( gEngfuncs.pfnRandomLong( 0, 2 ) + 5, 2 );
	}
	Vector shellVelocity, shellOrigin;
	EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHOTSHELL, args->entindex, 1 );
	const int sound = gEngfuncs.pfnRandomLong( 0, 1 );
	if ( sound == 0 || sound == 1 )
		gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON,
			sound ? "weapons/hks2.wav" : "weapons/hks1.wav", 1, ATTN_NORM, 0, gEngfuncs.pfnRandomLong( 0, 15 ) + 94 );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	const float cone = gEngfuncs.GetMaxClients() <= 1 ? 0.02618f : 0.05234f;
	Vector spread( cone, cone, cone );
	EV_HLDM_FireBullets( args->entindex, forward, right, up, 1, source, forward, spread,
		8192, BULLET_PLAYER_MP5, 2, &tracerCount[args->entindex - 1], 1 );
	if ( EV_IsLocal( args->entindex ) ) V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -2, 2 ) );
}

static void FireLegacyShotgun( event_args_t* args, bool doubleShot )
{
	Vector origin( args->origin ), angles( args->angles ), velocity( args->velocity );
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	const int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shotgunshell.mdl" );
	if ( EV_IsLocal( args->entindex ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( doubleShot ? 2 : 1, 2 );
	}
	for ( int index = 0; index < ( doubleShot ? 2 : 1 ); ++index )
	{
		Vector shellVelocity, shellOrigin;
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 32, -12, 6, false );
		EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHOTSHELL, args->entindex, 1 );
	}
	const int pitch = gEngfuncs.pfnRandomLong( 0, 31 ) + ( doubleShot ? 85 : 93 );
	const float volume = gEngfuncs.pfnRandomFloat( doubleShot ? 0.98f : 0.95f, 1 );
	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, origin, CHAN_WEAPON,
		doubleShot ? "weapons/dbarrel1.wav" : "weapons/sbarrel1.wav", volume, ATTN_NORM, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	const bool multiplayer = gEngfuncs.GetMaxClients() > 1;
	Vector spread = multiplayer ? Vector( doubleShot ? 0.17365f : 0.08716f, 0.04362f, 0.0f ) : Vector( 0.08716f, 0.08716f, 0.08716f );
	const int shots = ( multiplayer ? 4 : 6 ) * ( doubleShot ? 2 : 1 );
	EV_HLDM_FireBullets( args->entindex, forward, right, up, shots, source, forward, spread,
		2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[args->entindex - 1], 1 );
	if ( EV_IsLocal( args->entindex ) ) V_PunchAxis( 0, doubleShot ? -10.0f : -5.0f );
}

void EV_FireShotGunSingle( event_args_t* args ) { FireLegacyShotgun( args, false ); }
void EV_FireShotGunDouble( event_args_t* args ) { FireLegacyShotgun( args, true ); }

void MAT_AddMetalSpark( Vector origin, Vector normal )
{
	const int model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/metalspark.spr" );
	const int count = gEngfuncs.pfnRandomLong( 4, 6 );
	float length = gEngfuncs.pfnRandomFloat( 8, 10 );
	if ( !model ) return;
	if ( gEngfuncs.pfnRandomLong( 0, 3 ) == 1 )
		gEngfuncs.pEventAPI->EV_PlaySound( -1, origin, CHAN_AUTO,
			gEngfuncs.pfnRandomLong( 0, 1 ) ? "debris/r_metal3.wav" : "debris/r_metal4.wav", 1, ATTN_NORM, 0, 100 );
	if ( count <= 0 ) return;
	Vector randomNormal;
	for ( int axis = 0; axis < 3; ++axis )
		randomNormal[axis] = gEngfuncs.pfnRandomFloat( -0.2f, 0.2f ) + normal[axis];
	Vector end = origin + randomNormal * length;
	length *= 0.75f;
	const float step = length / count;
	gEngfuncs.pEfxAPI->R_BeamPoints( origin, end, model, 0.05f, 0.8f, 0, 255, 0, 0, 0, 250, 220, 192 );
	for ( int index = 1; index < count; ++index )
	{
		Vector direction;
		for ( int axis = 0; axis < 3; ++axis )
			direction[axis] = gEngfuncs.pfnRandomFloat( -0.2f, 0.2f ) + randomNormal[axis];
		length -= step;
		direction = direction.Normalize();
		end = origin + direction * length;
		gEngfuncs.pEfxAPI->R_BeamPoints( origin, end, model, 0.05f, 0.8f, 0, 255, 0, 0, 0, 250, 220, 192 );
	}
}

void EV_DebriSystem( char, pmtrace_t* trace, int bulletType, int materialIndex )
{
	if ( materialIndex == -1 ) return;
	const int entityIndex = gEngfuncs.pEventAPI->EV_IndexFromTrace( trace );
	if ( entityIndex > 0 && entityIndex <= gEngfuncs.GetMaxClients() ) return;
	materials_t* material = g_pMaterialSystem.GetMaterialByIndex( materialIndex );
	if ( !material ) return;
	const Vector position = trace->endpos + trace->plane.normal * 5;
	int count = 4;
	int sparks = 5;
	float velocityScale = 1;
	switch ( bulletType )
	{
	case BULLET_PLAYER_9MM:
	case BULLET_PLAYER_45ACP: count += gEngfuncs.pfnRandomLong( 0, 2 ); velocityScale = 1.05f; break;
	case BULLET_PLAYER_BUCKSHOT: count += gEngfuncs.pfnRandomLong( 1, 3 ); break;
	case BULLET_PLAYER_338MAG: count += gEngfuncs.pfnRandomLong( 3, 5 ); velocityScale = 1.5f; break;
	case BULLET_PLAYER_762MM: count += gEngfuncs.pfnRandomLong( 2, 4 ); velocityScale = 1.2f; break;
	case BULLET_PLAYER_556MM: count += gEngfuncs.pfnRandomLong( 1, 3 ); velocityScale = 1.3f; break;
	case BULLET_PLAYER_50AE:
	case BULLET_PLAYER_357SIG: count += gEngfuncs.pfnRandomLong( 1, 3 ); velocityScale = 1.15f; break;
	case BULLET_PLAYER_57MM: count += gEngfuncs.pfnRandomLong( 0, 2 ); velocityScale = 1.15f; break;
	case BULLET_PLAYER_BLOWTORCH:
		count = gEngfuncs.pfnRandomLong( 0, 2 );
		material->m_iSpawnSmoke = 0;
		material->m_iSpawnSparks = 1;
		material->m_iSpawnDebris = 0;
		velocityScale = 0.2f;
		sparks = 1;
		break;
	}
	if ( bulletType != BULLET_PLAYER_BLOWTORCH && material->m_iSpawnSmoke &&
		gEngfuncs.PM_PointContents( (float*)&position, NULL ) != CONTENTS_WATER )
	{
		gEngfuncs.pfnRandomFloat( 0, 0.2f );
		for ( int index = 0; index < 4; ++index )
		{
			const Vector puffOrigin = position + trace->plane.normal * (float)gEngfuncs.pfnRandomLong( 1, 3 );
			CreateDebrisWallPuff( puffOrigin, trace->plane.normal, material->m_vSmokeColor, index, false );
		}
	}
	if ( material->m_iSpawnSparks )
	{
		MAT_AddMetalSpark( trace->endpos, trace->plane.normal );
		for ( int index = 0; index < sparks; ++index )
			CreateSpark( trace->endpos, trace->plane.normal, "sprites/richo1.spr" );
		dlight_t* light = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
		light->origin = trace->endpos;
		light->radius = 55;
		light->color.r = light->color.g = 240;
		light->color.b = 180;
		light->die = gHUD.m_flTime + 0.1f;
		light->decay = 5;
	}
	if ( material->m_iSpawnDebris )
		for ( int index = 0; index < count; ++index )
			CreateCustomGib( position, trace->plane.normal, material->m_szTextName, velocityScale, material );
}

void EV_HLDM_DecalGunshot( pmtrace_t* trace, int bulletType, float, int, int, int,
	bool, char textureType, int materialIndex )
{
	physent_t* entity = gEngfuncs.pEventAPI->EV_GetPhysent( trace->ent );
	if ( !entity || entity->solid != SOLID_BSP ) return;
	materials_t* material = g_pMaterialSystem.GetMaterialByIndex( materialIndex );
	char decal[255];
	strcpy( decal, material ? material->m_szBulletImpactName : EV_HLDM_DamageDecal( entity ) );
	if ( bulletType != BULLET_PLAYER_BLOWTORCH )
		EV_HLDM_GunshotDecalTrace( trace, decal, material ? material->m_chMaterialType : textureType );
	EV_DebriSystem( textureType, trace, bulletType, materialIndex );
}

#pragma warning( push )
#pragma warning( disable : 4701 )
void EV_HLDM_FireBullets( int index, float* forward, float* right, float* up, int shots,
	float* source, float* aiming, float* spread, float distance, int bulletType,
	int frequency, int* count, int penetration )
{
	static const float penetrationDistances[] = { 800, 0, 0, 500, 0, 0, 0, 0, 500, 8000, 5000, 4000, 1000, 2000, 800 };
	static const int penetrationPowers[] = { 42, 0, 0, 16, 0, 0, 0, 0, 30, 90, 78, 70, 60, 60, 50 };
	const bool knownBullet = bulletType >= 1 && bulletType <= 15;
	const float penetrationDistance = knownBullet ? penetrationDistances[bulletType - 1] : 0;
	int power = knownBullet ? penetrationPowers[bulletType - 1] : 0;
	const Vector originalRight( right );
	char textureType;
	for ( int shot = 0; shot < shots; ++shot )
	{
		Vector direction;
		if ( bulletType == BULLET_PLAYER_BUCKSHOT )
		{
			float x, y;
			do
			{
				x = gEngfuncs.pfnRandomFloat( -0.5f, 0.5f );
				x += gEngfuncs.pfnRandomFloat( -0.5f, 0.5f );
				y = gEngfuncs.pfnRandomFloat( -0.5f, 0.5f );
				y += gEngfuncs.pfnRandomFloat( -0.5f, 0.5f );
			}
			while ( x * x + y * y > 1 );
			direction = Vector( aiming ) + originalRight * ( x * spread[0] ) + Vector( up ) * ( y * spread[1] );
		}
		else direction = Vector( aiming ) + Vector( right ) * spread[0] + Vector( up ) * spread[1];
		Vector end = Vector( source ) + direction * distance;
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( FALSE, TRUE );
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( index - 1 );
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		bool soundPlayed = false;
		int remaining = penetration;
		while ( remaining > 0 )
		{
			pmtrace_t trace;
			gEngfuncs.pEventAPI->EV_PlayerTrace( source, end, PM_NORMAL, -1, &trace );
			const int tracer = EV_HLDM_CheckTracer( index, source, trace.endpos, forward, right, bulletType, frequency, count );
			const float travelled = distance * trace.fraction;
			if ( travelled <= 0.01 ) break;
			--remaining;
			if ( travelled > penetrationDistance ) remaining = 0;
			if ( bulletType != BULLET_PLAYER_BUCKSHOT && bulletType != BULLET_PLAYER_BLOWTORCH &&
				!soundPlayed && ( bulletType < 1 || bulletType > 3 || !tracer ) )
			{
				soundPlayed = true;
				textureType = EV_HLDM_PlayTextureSound( index, &trace, source, end, bulletType );
			}
			switch ( textureType )
			{
			case 'C': power = (int)( power * 0.25 ); break;
			case 'G':
			case 'V': power = (int)( power * 0.5 ); break;
			case 'M': power = (int)( power * 0.15 ); break;
			case 'P': power = (int)( power * 0.4 ); break;
			case 'T': power = (int)( power * 0.65 ); break;
			case 'Y': power = 0; remaining = 0; break;
			}
			const int materialIndex = g_pMaterialSystem.GetMaterialIndexAtPos( &trace, trace.endpos, end );
			physent_t* entity = gEngfuncs.pEventAPI->EV_GetPhysent( trace.ent );
			if ( entity && entity->solid == SOLID_BSP )
			{
				materials_t* material = g_pMaterialSystem.GetMaterialByIndex( materialIndex );
				char decal[255];
				strcpy( decal, material ? material->m_szBulletImpactName : EV_HLDM_DamageDecal( entity ) );
				if ( bulletType != BULLET_PLAYER_BLOWTORCH )
					EV_HLDM_GunshotDecalTrace( &trace, decal, material ? material->m_chMaterialType : textureType );
				EV_DebriSystem( textureType, &trace, bulletType, materialIndex );
			}
			if ( remaining == 0 ) break;
			distance = ( distance - travelled ) * 0.5f;
			const Vector nextSource = trace.endpos + direction * (float)power;
			nextSource.CopyToArray( source );
			end = nextSource + direction * distance;
		}
		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}
#pragma warning( pop )
