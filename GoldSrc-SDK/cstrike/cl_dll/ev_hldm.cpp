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
#include "const.h"
#include "r_efx.h"
#include "pm_defs.h"
extern "C"
{
#include "pm_shared.h"
}
#include "com_weapons.h"
#include "eventscripts.h"
#include "ev_hldm.h"
#include "r_studioint.h"
#include "particleman.h"
#include "tri.h"

#define SND_CHANGE_PITCH ( 1 << 7 )

extern engine_studio_api_t IEngineStudio;
extern IParticleMan* g_pParticleMan;
extern int g_iShotsFired;
extern cvar_t* cl_righthand;
extern bool g_bHoldingShield;
static int tracerCount[32];
static int g_xWindDirection = 1;
static int g_yWindDirection = 1;
static float g_xWindMagnitude;
static float g_yWindMagnitude;
char extern_i[25];
TEMPENTITY* g_DeadPlayerModels[64];

void V_PunchAxis( int axis, float punch );

void SmokeRise_Wind_Expand( TEMPENTITY* entity, float, float )
{
	if ( entity->entity.curstate.renderamt > 74 && gEngfuncs.pfnRandomLong( 0, 9 ) == 0 )
		--entity->entity.curstate.renderamt;
	if ( entity->entity.baseline.fuser1 > entity->entity.curstate.scale )
		entity->entity.curstate.scale += entity->entity.curstate.scale / 40.0;
}

void RemoveBody( TEMPENTITY* entity, float frameTime, float currentTime )
{
	float time = entity->entity.curstate.fuser2 + 5.0;
	if ( currentTime >= time + gEngfuncs.pfnGetCvarFloat( "cl_corpsestay" ) )
		entity->entity.origin.z -= 5.0 * frameTime;
}

void HitBody( TEMPENTITY* entity, pmtrace_t* trace )
{
	if ( trace->plane.normal.z > 0 )
		entity->flags |= 0x200000;
}

void HugWalls( TEMPENTITY* entity, pmtrace_t* trace )
{
	Vector velocity = entity->entity.baseline.origin;
	float speed = velocity.Length();
	Vector direction( 0, 0, 0 );
	if ( speed != 0 )
		direction = velocity / speed;
	Vector normal = trace->plane.normal;
	Vector tangent = CrossProduct( direction, normal );
	Vector alongWall = CrossProduct( normal, tangent );
	entity->entity.baseline.origin = alongWall * min( speed * 1.5, 3000.0 );
}

void SmokeRise_Wind( TEMPENTITY* entity, float, float )
{
	if ( entity->entity.curstate.frame > 7 )
	{
		entity->entity.baseline.origin.x *= 0.97;
		entity->entity.baseline.origin.y *= 0.97;
		float vertical = entity->entity.baseline.origin.z * 0.97;
		vertical += 0.7;
		entity->entity.baseline.origin.z = min( vertical, 70.0f );
	}
	if ( entity->entity.curstate.frame > 6 )
	{
		if ( g_xWindDirection == 0 || g_xWindDirection == 1 )
		{
			float magnitude = g_xWindMagnitude + 0.075;
			g_xWindMagnitude = min( magnitude, 5.0f );
			if ( g_xWindDirection == 1 )
				entity->entity.baseline.origin.x += g_xWindMagnitude;
			else
				entity->entity.baseline.origin.x -= g_xWindMagnitude;
		}
		if ( g_yWindDirection == 0 || g_yWindDirection == 1 )
		{
			float magnitude = g_yWindMagnitude + 0.075;
			g_yWindMagnitude = min( magnitude, 5.0f );
			if ( g_yWindDirection == 1 )
				entity->entity.baseline.origin.y += g_yWindMagnitude;
			else
				entity->entity.baseline.origin.y -= g_yWindMagnitude;
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
}

void SmokeRise( TEMPENTITY* entity, float, float )
{
	if ( entity->entity.curstate.frame > 7 )
	{
		entity->entity.baseline.origin.x *= 0.97;
		entity->entity.baseline.origin.y *= 0.97;
		float vertical = entity->entity.baseline.origin.z * 0.97;
		vertical += 0.7;
		entity->entity.baseline.origin.z = min( vertical, 70.0f );
	}
}

void EV_HLDM_CreateLight( float*, float radius, int red, int green, int blue, float time )
{
	dlight_t* light = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
	light->origin = gHUD.m_vecOrigin;
	light->radius = radius;
	light->color.r = red;
	light->color.g = green;
	light->color.b = blue;
	light->die = gHUD.m_flTime + time;
}

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4701 )
#endif
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
		extern_i[19] = '1' + gEngfuncs.pfnRandomLong( 0, 3 );
		model = gEngfuncs.pEventAPI->EV_FindModelIndex( extern_i );
		if ( gEngfuncs.pfnGetCvarFloat( "fastsprites" ) > 1 )
			model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/fast_wallpuff1.spr" );
		if ( !model )
		{
			const int fallbackSmokeModel = 293;
			model = fallbackSmokeModel;
		}
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
	TEMPENTITY* entity = gEngfuncs.pEfxAPI->R_DefaultSprite( origin, model, (float)frameRate );
	if ( !entity )
		return;
	entity->flags |= FTENT_CLIENTCUSTOM | FTENT_COLLIDEWORLD;
	entity->entity.curstate.scale = scale + gEngfuncs.pfnRandomFloat( 0, scale * 0.5 );
	if ( (unsigned int)smokeType <= 1 )
	{
		if ( gEngfuncs.pfnGetCvarFloat( "fastsprites" ) == 1 )
		{
			entity->entity.curstate.rendermode = kRenderTransAdd;
			entity->entity.curstate.renderamt = 200;
		}
		else if ( gEngfuncs.pfnGetCvarFloat( "fastsprites" ) <= 1 )
		{
			entity->entity.curstate.rendermode = kRenderTransAlpha;
			entity->entity.curstate.renderamt = 200;
		}
		else
			entity->entity.curstate.rendermode = kRenderNormal;
		entity->callback = SmokeRise_Wind_Expand;
		entity->entity.baseline.fuser1 = smokeType == 1 ? 1.0f : 8.5f;
		if ( gEngfuncs.pfnGetCvarFloat( "fastsprites" ) > 1 )
			entity->entity.baseline.fuser1 -= 3;
	}
	else
	{
		entity->entity.curstate.renderamt = 150;
		entity->callback = wind ? SmokeRise_Wind : SmokeRise;
		entity->entity.curstate.rendermode = kRenderTransAdd;
		entity->entity.curstate.renderfx = kRenderFxFadeSlow;
	}
	entity->hitcallback = HugWalls;
	entity->entity.curstate.rendercolor.r = red;
	entity->entity.curstate.rendercolor.g = green;
	entity->entity.curstate.rendercolor.b = blue;
	if ( speed )
		speed += gEngfuncs.pfnRandomLong( 0, 5 );
	if ( origin[0] != direction[0] && origin[1] != direction[1] && origin[2] != direction[2] )
	{
		entity->entity.origin = Vector( origin );
		entity->entity.baseline.origin = Vector( direction ) * speed;
		if ( baseVelocity )
		{
			entity->entity.baseline.origin.x += 0.9 * baseVelocity[0];
			entity->entity.baseline.origin.y += 0.9 * baseVelocity[1];
			entity->entity.baseline.origin.z += 0.5 * baseVelocity[2];
		}
	}
}
#ifdef _MSC_VER
#pragma warning( pop )
#endif

char EV_HLDM_PlayTextureSound( int, pmtrace_t* trace, float* source, float* end, int bulletType )
{
	char textureType = 0;
	int entityIndex = gEngfuncs.pEventAPI->EV_IndexFromTrace( trace );
	if ( !entityIndex )
	{
		const char* texture = gEngfuncs.pEventAPI->EV_TraceTexture( trace->ent, source, end );
		if ( texture )
		{
			char name[64], buffer[64];
			strcpy( name, texture );
			char* start = name;
			if ( *start == '-' || *start == '+' )
				start += 2;
			if ( *start == '~' || *start == '{' || *start == ' ' || *start == '!' )
				++start;
			strcpy( buffer, start );
			buffer[12] = 0;
			textureType = PM_FindTextureType( buffer );
		}
	}
	else
	{
		cl_entity_t* entity = gEngfuncs.GetEntityByIndex( entityIndex );
		if ( entity && ( entity->curstate.eflags & EFLAG_FLESH_SOUND ) )
			textureType = 'F';
	}
	const char* sounds[4];
	int count;
	float volume, attenuation = ATTN_NORM;
	switch ( textureType )
	{
	case 'D':
		sounds[0] = "player/pl_dirt1.wav"; sounds[1] = "player/pl_dirt2.wav"; sounds[2] = "player/pl_dirt3.wav";
		count = 3; volume = 0.9; break;
	case 'G':
		sounds[0] = "player/pl_grate1.wav"; sounds[1] = "player/pl_grate4.wav";
		count = 2; volume = 0.9; break;
	case 'M':
		sounds[0] = "player/pl_metal1.wav"; sounds[1] = "player/pl_metal2.wav";
		count = 2; volume = 0.9; break;
	case 'N':
		sounds[0] = "player/pl_snow1.wav"; sounds[1] = "player/pl_snow2.wav"; sounds[2] = "player/pl_snow3.wav"; sounds[3] = "player/pl_snow4.wav";
		count = 4; volume = 0.7; break;
	case 'P':
	case 'Y':
		sounds[0] = "debris/glass1.wav"; sounds[1] = "debris/glass2.wav"; sounds[2] = "debris/glass3.wav";
		count = 3; volume = 0.8; break;
	case 'S':
		sounds[0] = "player/pl_slosh1.wav"; sounds[1] = "player/pl_slosh3.wav"; sounds[2] = "player/pl_slosh2.wav"; sounds[3] = "player/pl_slosh4.wav";
		count = 4; volume = 0.9; break;
	case 'T':
		sounds[0] = "player/pl_tile1.wav"; sounds[1] = "player/pl_tile3.wav"; sounds[2] = "player/pl_tile2.wav"; sounds[3] = "player/pl_tile4.wav";
		count = 4; volume = 0.8; break;
	case 'V':
		sounds[0] = sounds[1] = "player/pl_duct1.wav";
		count = 2; volume = 0.5; break;
	case 'W':
		sounds[0] = "debris/wood1.wav"; sounds[1] = "debris/wood2.wav"; sounds[2] = "debris/wood3.wav";
		count = 3; volume = 0.9; break;
	case 'F':
		if ( bulletType == BULLET_PLAYER_CROWBAR )
			return 'F';
		sounds[0] = "weapons/bullet_hit1.wav"; sounds[1] = "weapons/bullet_hit2.wav";
		count = 2; volume = 1; attenuation = 1; break;
	default:
		sounds[0] = "player/pl_step1.wav"; sounds[1] = "player/pl_step2.wav";
		count = 2; volume = 0.9; break;
	}
	int pitch = 96 + gEngfuncs.pfnRandomLong( 0, 15 );
	int sound = gEngfuncs.pfnRandomLong( 0, count - 1 );
	gEngfuncs.pEventAPI->EV_PlaySound( 0, trace->endpos, CHAN_STATIC, sounds[sound], volume, attenuation, 0, pitch );
	return textureType;
}

char* EV_HLDM_DamageDecal( physent_t* entity )
{
	static char name[32];
	int index;
	if ( entity->classnumber == 1 )
	{
		index = gEngfuncs.pfnRandomLong( 0, 2 );
		sprintf( name, "{break%i", index + 1 );
	}
	else if ( entity->rendermode != kRenderNormal )
	{
		sprintf( name, "{bproof1" );
	}
	else
	{
		index = gEngfuncs.pfnRandomLong( 0, 4 );
		sprintf( name, "{shot%i", index + 1 );
	}
	return name;
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t* trace, char* decalName, char textureType )
{
	gEngfuncs.pEfxAPI->R_BulletImpactParticles( trace->endpos );
	int random = gEngfuncs.pfnRandomLong( 0, 0x7fff );
	if ( random < 0x7fff / 2 )
	{
		const char* ricochet;
		if ( textureType == 'M' || textureType == 'V' )
			ricochet = random & 1 ? "weapons/ric_metal-1.wav" : "weapons/ric_metal-2.wav";
		else
		{
			static const char* sounds[] = { "weapons/ric1.wav", "weapons/ric2.wav", "weapons/ric3.wav", "weapons/ric4.wav", "weapons/ric5.wav", "weapons/ric_conc-1.wav" };
			ricochet = sounds[random % 6];
		}
		gEngfuncs.pEventAPI->EV_PlaySound( -1, trace->endpos, CHAN_AUTO, ricochet, 1, ATTN_NORM, 0, PITCH_NORM );
	}
	physent_t* entity = gEngfuncs.pEventAPI->EV_GetPhysent( trace->ent );
	if ( decalName && *decalName && entity && ( entity->solid == SOLID_BSP || entity->movetype == MOVETYPE_PUSHSTEP ) && gEngfuncs.pfnGetCvarFloat( "r_decals" ) != 0 )
	{
		int entityIndex = gEngfuncs.pEventAPI->EV_IndexFromTrace( trace );
		int decalIndex = gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName );
		decalIndex = gEngfuncs.pEfxAPI->Draw_DecalIndex( decalIndex );
		gEngfuncs.pEfxAPI->R_DecalShoot( decalIndex, entityIndex, 0, trace->endpos, 0 );
	}
}

void EV_HLDM_DecalGunshot( pmtrace_t* trace, int, float scale, int red, int green, int blue, bool sparks, char textureType )
{
	physent_t* entity = gEngfuncs.pEventAPI->EV_GetPhysent( trace->ent );
	if ( !entity || entity->solid != SOLID_BSP )
		return;
	EV_HLDM_GunshotDecalTrace( trace, EV_HLDM_DamageDecal( entity ), textureType );
	if ( sparks )
	{
		Vector direction;
		direction.x = gEngfuncs.pfnRandomFloat( 4, 10 ) * trace->plane.normal.x;
		direction.y = gEngfuncs.pfnRandomFloat( 4, 10 ) * trace->plane.normal.y;
		direction.z = gEngfuncs.pfnRandomFloat( 4, 10 ) * trace->plane.normal.z;
		float speed = gEngfuncs.pfnRandomFloat( 4, 10 );
		int count = gEngfuncs.pfnRandomLong( 15, 30 );
		gEngfuncs.pEfxAPI->R_StreakSplash( trace->endpos, direction, 4, count, speed, -75, 75 );
	}
	Vector origin = trace->endpos + trace->plane.normal * 5;
	EV_HLDM_CreateSmoke( origin, trace->plane.normal, 25, scale, red, green, blue, 2, NULL, true, 35 );
}

int EV_HLDM_CheckTracer( int index, float* source, float* end, float* forward, float* right, int, int frequency, int* count )
{
	bool player = index > 0 && index <= gEngfuncs.GetMaxClients();
	if ( !frequency || ( *count )++ % frequency != 0 )
		return 0;
	Vector start = source;
	if ( player )
		start = start + Vector( 0, 0, -4 ) + Vector( right ) * 2 + Vector( forward ) * 16;
	EV_CreateTracer( start, end );
	return frequency != 1;
}

char UTIL_TextureHit( pmtrace_t* trace, Vector source, Vector end )
{
	int index = gEngfuncs.pEventAPI->EV_IndexFromTrace( trace );
	if ( index > 0 )
		return index <= gEngfuncs.GetMaxClients() ? 'F' : 0;
	if ( index )
		return 0;
	const char* texture = gEngfuncs.pEventAPI->EV_TraceTexture( trace->ent, source, end );
	if ( !texture )
		return 0;
	char name[64], buffer[64];
	strcpy( name, texture );
	char* start = name;
	if ( *start == '+' || *start == '-' )
		start += 2;
	if ( *start == '~' || *start == '{' || *start == ' ' || *start == '!' )
		++start;
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

int EV_TFC_IsAllyTeam( int, int )
{
	return 0;
}

void EV_HLDM_FireBullets( int index, float* forward, float* right, float* up, int shots,
	float* source, float* shootingDirection, float* spread, float distance,
	int bulletType, int tracerFrequency, int* count, int penetration )
{
	int penetrationPower = 0;
	float penetrationDistance = 0;
	switch ( bulletType )
	{
	case BULLET_PLAYER_9MM: penetrationPower = 42; penetrationDistance = 800; break;
	case BULLET_PLAYER_BUCKSHOT: penetrationPower = 16; penetrationDistance = 500; break;
	case BULLET_PLAYER_45ACP: penetrationPower = 30; penetrationDistance = 500; break;
	case BULLET_PLAYER_338MAG: penetrationPower = 90; penetrationDistance = 8000; break;
	case BULLET_PLAYER_762MM: penetrationPower = 78; penetrationDistance = 5000; break;
	case BULLET_PLAYER_556MM: penetrationPower = 70; penetrationDistance = 4000; break;
	case BULLET_PLAYER_50AE: penetrationPower = 60; penetrationDistance = 1000; break;
	case BULLET_PLAYER_57MM: penetrationPower = 60; penetrationDistance = 2000; break;
	case BULLET_PLAYER_357SIG: penetrationPower = 50; penetrationDistance = 800; break;
	}
	Vector originalRight = right;
	int red = 40, green = 40, blue = 40;
	float smokeScale = 0.4;
	char textureType = 0;
	for ( int shot = 1; shot <= shots; ++shot )
	{
		Vector direction;
		if ( bulletType == BULLET_PLAYER_BUCKSHOT )
		{
			float x, y;
			do
			{
				float first = gEngfuncs.pfnRandomFloat( -0.5, 0.5 );
				x = first + gEngfuncs.pfnRandomFloat( -0.5, 0.5 );
				first = gEngfuncs.pfnRandomFloat( -0.5, 0.5 );
				y = first + gEngfuncs.pfnRandomFloat( -0.5, 0.5 );
			} while ( x * x + y * y > 1 );
			direction = Vector( shootingDirection ) + originalRight * ( x * spread[0] ) + Vector( up ) * ( y * spread[1] );
		}
		else
			direction = Vector( shootingDirection ) + Vector( right ) * spread[0] + Vector( up ) * spread[1];
		Vector end = Vector( source ) + direction * distance;
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( FALSE, TRUE );
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( index - 1 );
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		bool sparks = false;
		int remaining = penetration;
		while ( remaining )
		{
			pmtrace_t trace;
			gEngfuncs.pEventAPI->EV_PlayerTrace( source, end, PM_NORMAL, -1, &trace );
			int tracer = EV_HLDM_CheckTracer( index, source, trace.endpos, forward, right, bulletType, tracerFrequency, count );
			float travelled = distance * trace.fraction;
			if ( travelled == 0 )
				break;
			--remaining;
			if ( travelled > penetrationDistance )
				remaining = 0;
			switch ( bulletType )
			{
			case BULLET_PLAYER_9MM:
			case BULLET_PLAYER_MP5:
			case BULLET_PLAYER_357:
				if ( !tracer )
					textureType = EV_HLDM_PlayTextureSound( index, &trace, source, end, bulletType );
				break;
			case BULLET_PLAYER_BUCKSHOT:
				break;
			default:
				textureType = EV_HLDM_PlayTextureSound( index, &trace, source, end, bulletType );
				break;
			}
			switch ( textureType )
			{
			case 'C':
				sparks = true;
				red = green = blue = 65;
				penetrationPower *= 0.25;
				break;
			case 'G':
			case 'V':
				penetrationPower *= 0.5;
				sparks = true;
				break;
			case 'M':
				penetrationPower *= 0.15;
				sparks = true;
				break;
			case 'P':
				penetrationPower *= 0.4;
				sparks = true;
				break;
			case 'T':
				penetrationPower *= 0.65;
				sparks = true;
				break;
			case 'W':
				sparks = false;
				smokeScale = 0.5;
				red = 75; green = 42; blue = 15;
				break;
			}
			EV_HLDM_DecalGunshot( &trace, bulletType, smokeScale, red, green, blue, sparks, textureType );
			if ( !remaining )
				break;
			distance = ( distance - travelled ) * 0.5;
			Vector start = trace.endpos + direction * penetrationPower;
			VectorCopy( start, source );
			end = start + direction * distance;
			for ( int step = 1; step < penetrationPower; ++step )
			{
				Vector startPos = trace.endpos + direction * step;
				Vector endPos = startPos + direction;
				pmtrace_t originalTrace;
				gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
				gEngfuncs.pEventAPI->EV_PlayerTrace( startPos, endPos, PM_NORMAL, -1, &originalTrace );
				if ( originalTrace.startsolid && originalTrace.inopen )
				{
					gEngfuncs.pEventAPI->EV_PlayerTrace( endPos, startPos, PM_NORMAL, -1, &originalTrace );
					EV_HLDM_DecalGunshot( &originalTrace, bulletType, smokeScale, red, green, blue, sparks, textureType );
					break;
				}
			}
		}
		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

void CreateCorpse( Vector origin, Vector angles, const char* model, float animationTime, int sequence, int body )
{
	gEngfuncs.pEventAPI->EV_FindModelIndex( model );
	int modelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( model );
	Vector velocity( 0, 0, 0 );
	TEMPENTITY* entity = gEngfuncs.pEfxAPI->R_TempModel( origin, velocity, angles, 100, modelIndex, 0 );
	if ( !entity )
		return;
	entity->flags |= FTENT_COLLIDEWORLD | FTENT_FADEOUT | FTENT_SPRANIMATE | FTENT_PERSIST | FTENT_CLIENTCUSTOM | 0x100000;
	entity->entity.curstate.framerate = 1;
	entity->entity.curstate.animtime = animationTime;
	entity->frameMax = 250;
	entity->entity.baseline.renderamt = 255;
	entity->entity.curstate.renderamt = 255;
	entity->entity.curstate.sequence = sequence;
	entity->entity.curstate.frame = 0;
	entity->entity.curstate.body = body;
	float time = gHUD.m_flTime;
	entity->entity.curstate.fuser2 = time + 1;
	entity->entity.curstate.fuser3 = time + gEngfuncs.pfnGetCvarFloat( "cl_corpsestay" );
	float clientTime = gEngfuncs.GetClientTime();
	entity->die = clientTime + gEngfuncs.pfnGetCvarFloat( "cl_corpsestay" ) + 9.0;
	entity->callback = RemoveBody;
	entity->hitcallback = HitBody;
	entity->bounceFactor = 0;
	for ( int i = 0; i < 64; ++i )
	{
		if ( !g_DeadPlayerModels[i] )
		{
			g_DeadPlayerModels[i] = entity;
			return;
		}
	}
}

void CreateSmokePop( event_args_t* args )
{
	Vector forward, right, up;
	for ( int yaw = 0; yaw < 360; yaw += 45 )
	{
		Vector angles( 0.0f, (float)yaw, 0.0f );
		gEngfuncs.pfnAngleVectors( angles, forward, right, up );
		forward = forward.Normalize();
		CreateGasSmoke( Vector( args->origin ), forward * 375 + up * 165, false, false, true );
	}
	for ( int yaw = 0; yaw < 360; yaw += 45 )
	{
		Vector angles( 0.0f, (float)yaw, 0.0f );
		gEngfuncs.pfnAngleVectors( angles, forward, right, up );
		forward = forward.Normalize();
		CreateGasSmoke( Vector( args->origin ), forward * 375 + up * 265, false, false, true );
	}
	Vector angles( 0, 45, 0 );
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	forward = forward.Normalize();
	CreateGasSmoke( Vector( args->origin ), forward * 375 + up * 120, false, false, true );
	angles = Vector( 0, 270, 0 );
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	forward = forward.Normalize();
	CreateGasSmoke( Vector( args->origin ), forward * 375 + up * 120, false, true, true );
}

extern "C" void EV_CreateSmoke( event_args_t* args )
{
	if ( IEngineStudio.IsHardware() && args->iparam2 == 1 )
	{
		CreateSmokePop( args );
		return;
	}
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector direction( args->fparam1, args->fparam2, (float)( args->iparam1 / 100.0 ) );
	int color = gEngfuncs.pfnRandomLong( 155, 175 );
	if ( IEngineStudio.IsHardware() )
		EV_HLDM_CreateSmoke( origin, direction, (int)gEngfuncs.pfnRandomFloat( 5, 10 ), 1, color, color, color, 1, NULL, true, 25 );
	else
		EV_HLDM_CreateSmoke( angles, direction, (int)gEngfuncs.pfnRandomFloat( 5, 10 ), 2, color, color, color, 0, NULL, true, 4 );
	float oldX = direction.x;
	float rotatedX = direction.x * 0.9659274596269048 - direction.y * 0.25881294932579024;
	direction.x = rotatedX * 0.75;
	float rotatedY = direction.y * 0.9659274596269048 + oldX * 0.25881294932579024;
	direction.y = rotatedY * 0.75;
	direction.z *= 0.75;
	if ( IEngineStudio.IsHardware() )
		EV_HLDM_CreateSmoke( origin, direction, (int)gEngfuncs.pfnRandomFloat( 2, 5 ), 1.2f, color, color, color, 1, NULL, true, 20 );
	else
		EV_HLDM_CreateSmoke( angles, direction, (int)gEngfuncs.pfnRandomFloat( 2, 5 ), 1.2f, color, color, color, 0, NULL, true, 4 );
}

extern "C" void EV_RemoveAllDecals( event_args_t* )
{
	for ( int i = 0; i < (int)gEngfuncs.pfnGetCvarFloat( "r_decals" ); ++i )
		gEngfuncs.pEfxAPI->R_DecalRemoveAll( i );
	if ( g_pParticleMan )
		g_pParticleMan->ResetParticles();
	if ( g_DeadPlayerModels[0] )
	{
		for ( int i = 0; i < 64; ++i )
		{
			if ( g_DeadPlayerModels[i] )
			{
				g_DeadPlayerModels[i]->die = 0;
				g_DeadPlayerModels[i] = NULL;
			}
		}
	}
}

extern "C" void EV_TrainPitchAdjust( event_args_t* args )
{
	int idx;
	vec3_t origin;
	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;
	char sz[256];

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	us_params = (unsigned short)args->iparam1;
	stop = args->bparam1;

	m_flVolume = (float)( us_params & 0x003f ) / 40.0;
	noise = (int)( ( us_params >> 12 ) & 0x0007 );
	pitch = (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch ( noise )
	{
	case 1: strcpy( sz, "plats/ttrain1.wav" ); break;
	case 2: strcpy( sz, "plats/ttrain2.wav" ); break;
	case 3: strcpy( sz, "plats/ttrain3.wav" ); break;
	case 4: strcpy( sz, "plats/ttrain4.wav" ); break;
	case 5: strcpy( sz, "plats/ttrain6.wav" ); break;
	case 6: strcpy( sz, "plats/ttrain7.wav" ); break;
	default:
		// no sound
		strcpy( sz, "" );
		return;
	}

	if ( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, sz );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

extern "C" void EV_VehiclePitchAdjust( event_args_t* args )
{
	int idx;
	vec3_t origin;
	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;
	char sz[256];

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	us_params = (unsigned short)args->iparam1;
	stop = args->bparam1;

	m_flVolume = (float)( us_params & 0x003f ) / 40.0;
	noise = (int)( ( us_params >> 12 ) & 0x0007 );
	pitch = (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch ( noise )
	{
	case 1: strcpy( sz, "plats/vehicle1.wav" ); break;
	case 2: strcpy( sz, "plats/vehicle2.wav" ); break;
	case 3: strcpy( sz, "plats/vehicle3.wav" ); break;
	case 4: strcpy( sz, "plats/vehicle4.wav" ); break;
	case 5: strcpy( sz, "plats/vehicle6.wav" ); break;
	case 6: strcpy( sz, "plats/vehicle7.wav" ); break;
	default:
		// no sound
		strcpy( sz, "" );
		return;
	}

	if ( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, sz );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

extern "C" void EV_SpinGauss( event_args_t* args )
{
	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, args->origin, CHAN_WEAPON,
		"ambience/pulsemachine.wav", 1, ATTN_NORM, args->bparam1 ? SND_CHANGE_PITCH : 0, args->iparam1 );
}

extern "C" void EV_FireGlock1( event_args_t* args )
{
	Vector origin = args->origin, angles = args->angles, velocity = args->velocity;
	Vector forward, right, up, shellVelocity, shellOrigin, source;
	int index = args->entindex;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( args->bparam1 ? GLOCK_SHOOT_EMPTY : GLOCK_SHOOT, 2 );
	}
	EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 1 );
	int pitch = 98 + gEngfuncs.pfnRandomLong( 0, 3 );
	float volume = gEngfuncs.pfnRandomFloat( 0.92f, 1 );
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", volume, ATTN_NORM, 0, pitch );
	EV_GetGunPosition( args, source, origin );
	Vector spread( 0.01, 0.01, 0.01 );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, forward, spread, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[index - 1], 1 );
	if ( EV_IsLocal( index ) )
		V_PunchAxis( 0, -2 );
}

extern "C" void EV_FireGlock2( event_args_t* args )
{
	Vector origin = args->origin, angles = args->angles, velocity = args->velocity;
	Vector forward, right, up, shellVelocity, shellOrigin, source;
	int index = args->entindex;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( GLOCK_SHOOT, 2 );
	}
	EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 1 );
	int pitch = 98 + gEngfuncs.pfnRandomLong( 0, 3 );
	float volume = gEngfuncs.pfnRandomFloat( 0.92f, 1 );
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", volume, ATTN_NORM, 0, pitch );
	EV_GetGunPosition( args, source, origin );
	Vector spread( 0.1, 0.1, 0.1 );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, forward, spread, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[index - 1], 1 );
	if ( EV_IsLocal( index ) )
		V_PunchAxis( 0, -2 );
}

extern "C" void EV_FireShotGunDouble( event_args_t* args )
{
	Vector origin = args->origin, angles = args->angles, velocity = args->velocity;
	Vector forward, right, up, shellVelocity, shellOrigin, source;
	int index = args->entindex;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shotgunshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SHOTGUN_FIRE2, 2 );
	}
	for ( int i = 0; i < 2; ++i )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 32, -12, 6, false );
		EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHOTSHELL, index, 1 );
	}
	int pitch = 85 + gEngfuncs.pfnRandomLong( 0, 31 );
	float volume = gEngfuncs.pfnRandomFloat( 0.98f, 1 );
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/dbarrel1.wav", volume, ATTN_NORM, 0, pitch );
	EV_GetGunPosition( args, source, origin );
	if ( gEngfuncs.GetMaxClients() <= 1 )
	{
		Vector spread( 0.08716, 0.08716, 0.08716 );
		EV_HLDM_FireBullets( index, forward, right, up, 12, source, forward, spread, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[index - 1], 1 );
	}
	else
	{
		Vector spread( 0.17365, 0.04362, 0.0 );
		EV_HLDM_FireBullets( index, forward, right, up, 8, source, forward, spread, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[index - 1], 1 );
	}
	if ( EV_IsLocal( index ) )
		V_PunchAxis( 0, -10 );
}

extern "C" void EV_FireShotGunSingle( event_args_t* args )
{
	Vector origin = args->origin, angles = args->angles, velocity = args->velocity;
	Vector forward, right, up, shellVelocity, shellOrigin, source;
	int index = args->entindex;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shotgunshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SHOTGUN_FIRE, 2 );
	}
	EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 32, -12, 6, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHOTSHELL, index, 1 );
	int pitch = 93 + gEngfuncs.pfnRandomLong( 0, 31 );
	float volume = gEngfuncs.pfnRandomFloat( 0.95f, 1 );
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/sbarrel1.wav", volume, ATTN_NORM, 0, pitch );
	EV_GetGunPosition( args, source, origin );
	if ( gEngfuncs.GetMaxClients() <= 1 )
	{
		Vector spread( 0.08716, 0.08716, 0.08716 );
		EV_HLDM_FireBullets( index, forward, right, up, 6, source, forward, spread, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[index - 1], 1 );
	}
	else
	{
		Vector spread( 0.08716, 0.04362, 0.0 );
		EV_HLDM_FireBullets( index, forward, right, up, 4, source, forward, spread, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[index - 1], 1 );
	}
	if ( EV_IsLocal( index ) )
		V_PunchAxis( 0, -5 );
}

extern "C" void EV_FireMP5( event_args_t* args )
{
	Vector origin = args->origin, angles = args->angles, velocity = args->velocity;
	Vector forward, right, up, shellVelocity, shellOrigin, source;
	int index = args->entindex;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5_FIRE1 + gEngfuncs.pfnRandomLong( 0, 2 ), 2 );
	}
	EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHOTSHELL, index, 1 );
	const char* sound = gEngfuncs.pfnRandomLong( 0, 1 ) ? "weapons/hks2.wav" : "weapons/hks1.wav";
	int pitch = 94 + gEngfuncs.pfnRandomLong( 0, 15 );
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, sound, 1, ATTN_NORM, 0, pitch );
	EV_GetGunPosition( args, source, origin );
	Vector spread;
	if ( gEngfuncs.GetMaxClients() <= 1 )
		spread = Vector( 0.02618, 0.02618, 0.02618 );
	else
		spread = Vector( 0.05234, 0.05234, 0.05234 );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, forward, spread, 8192, BULLET_PLAYER_MP5, 2, &tracerCount[index - 1], 1 );
	if ( EV_IsLocal( index ) )
		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -2, 2 ) );
}

extern "C" void EV_FirePython( event_args_t* args )
{
	Vector origin = args->origin, angles = args->angles;
	Vector forward, right, up, source;
	int index = args->entindex;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	if ( EV_IsLocal( index ) )
	{
		int clients = gEngfuncs.GetMaxClients();
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( PYTHON_FIRE1, clients != 1 );
	}
	const char* sound = gEngfuncs.pfnRandomLong( 0, 1 ) ? "weapons/357_shot2.wav" : "weapons/357_shot1.wav";
	float volume = gEngfuncs.pfnRandomFloat( 0.8f, 0.9f );
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, sound, volume, ATTN_NORM, 0, PITCH_NORM );
	EV_GetGunPosition( args, source, origin );
	Vector spread( 0.00873, 0.00873, 0.00873 );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, forward, spread, 8192, BULLET_PLAYER_357, 0, &tracerCount[index - 1], 1 );
	if ( EV_IsLocal( index ) )
		V_PunchAxis( 0, -10 );
}

extern "C" void EV_FireGauss( event_args_t* args )
{
	int index = args->entindex;
	float damage = args->fparam1;
	int primaryFire = args->bparam1;
	Vector origin = args->origin, angles = args->angles;
	Vector source, forward, right, up;
	gEngfuncs.pEventAPI->EV_KillEvents( index, "events/gaussspin.sc" );
	EV_GetGunPosition( args, source, origin );
	int beam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/smoke.spr" );
	int balls = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/hotglow.spr" );
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	Vector destination = source + forward * 8192;
	if ( EV_IsLocal( index ) )
	{
		V_PunchAxis( 0, -2 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( GAUSS_FIRE2, 2 );
	}
	gEngfuncs.pEventAPI->EV_StopSound( index, CHAN_WEAPON, "ambience/pulsemachine.wav" );
	int pitch = 85 + gEngfuncs.pfnRandomLong( 0, 31 );
	float volume = 0.5 + damage * 0.0025;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/gauss2.wav", volume, ATTN_NORM, 0, pitch );
	bool firstBeam = true;
	bool punched = false;
	Vector zero( 0, 0, 0 );
	int maxHits = 10;
	while ( damage > 10 && maxHits > 0 )
	{
		--maxHits;
		pmtrace_t trace;
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( FALSE, TRUE );
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( index - 1 );
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( source, destination, PM_STUDIO_BOX, -1, &trace );
		gEngfuncs.pEventAPI->EV_PopPMStates();
		if ( trace.allsolid )
			break;
		physent_t* entity = gEngfuncs.pEventAPI->EV_GetPhysent( trace.ent );
		if ( !entity )
			break;
		if ( firstBeam )
		{
			if ( EV_IsLocal( index ) )
				EV_MuzzleFlash();
			gEngfuncs.pEfxAPI->R_BeamEntPoint( index | 0x1000, trace.endpos, beam, 0.1,
				primaryFire ? 1 : 2.5, 0, primaryFire ? 128 : damage, 0, 0, 0, 250,
				primaryFire ? 128 : 255, primaryFire ? 0 : 255 );
		}
		else
			gEngfuncs.pEfxAPI->R_BeamPoints( source, trace.endpos, beam, 0.1,
				primaryFire ? 1 : 2.5, 0, primaryFire ? 128 : damage, 0, 0, 0, 250,
				primaryFire ? 128 : 255, primaryFire ? 0 : 255 );
		firstBeam = false;
		if ( entity->solid != SOLID_BSP )
		{
			source = trace.endpos + forward;
			continue;
		}
		float incidence = -DotProduct( trace.plane.normal, forward );
		if ( incidence < 0.5 )
		{
			forward = forward + trace.plane.normal * ( 2 * incidence );
			source = trace.endpos + forward * 8;
			destination = source + forward * 8192;
			float reflectedDamage = damage * incidence;
			gEngfuncs.pEfxAPI->R_TempSprite( trace.endpos, zero, 0.2, balls, kRenderGlow,
				kRenderFxNoDissipation, reflectedDamage / 255.0, reflectedDamage * 0.5 * 0.1, FTENT_FADEOUT );
			Vector end = trace.endpos + trace.plane.normal;
			float scale = gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0;
			gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, trace.endpos, end, balls,
				(int)( reflectedDamage * 0.3 ), 0.1, scale, 100, 255, 200 );
			if ( incidence == 0 )
				damage *= 0.9f;
			else
				damage *= 1.0 - incidence;
		}
		else
		{
			EV_HLDM_DecalGunshot( &trace, BULLET_MONSTER_12MM, 0.5, 75, 75, 75, false, 'C' );
			gEngfuncs.pEfxAPI->R_TempSprite( trace.endpos, zero, 1, balls, kRenderGlow,
				kRenderFxNoDissipation, damage / 255.0, 6, FTENT_FADEOUT );
			if ( punched )
				return;
			if ( primaryFire )
			{
				gEngfuncs.pEfxAPI->R_TempSprite( trace.endpos, zero, 0.2, balls, kRenderGlow,
					kRenderFxNoDissipation, 0.8, 0.3, FTENT_FADEOUT );
				Vector end = trace.endpos + trace.plane.normal;
				float scale = gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0;
				gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, trace.endpos, end, balls, 8, 0.6, scale, 100, 255, 200 );
				punched = true;
				damage = 0;
			}
			else
			{
				Vector start = trace.endpos + forward * 8;
				pmtrace_t beamTrace;
				gEngfuncs.pEventAPI->EV_PushPMStates();
				gEngfuncs.pEventAPI->EV_SetSolidPlayers( index - 1 );
				gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
				gEngfuncs.pEventAPI->EV_PlayerTrace( start, destination, PM_STUDIO_BOX, -1, &beamTrace );
				if ( beamTrace.allsolid )
					damage = 0;
				else
				{
					gEngfuncs.pEventAPI->EV_PlayerTrace( beamTrace.endpos, trace.endpos, PM_STUDIO_BOX, -1, &beamTrace );
					float thickness = ( beamTrace.endpos - trace.endpos ).Length();
					if ( damage > thickness )
					{
						if ( thickness == 0 )
							thickness = 1;
						damage -= thickness;
						Vector end = trace.endpos - forward;
						float scale = gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0;
						gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, trace.endpos, end, balls, 3, 0.1, scale, 100, 255, 100 );
						EV_HLDM_DecalGunshot( &beamTrace, BULLET_MONSTER_12MM, 0.65f, 65, 65, 65, false, 'C' );
						gEngfuncs.pEfxAPI->R_TempSprite( beamTrace.endpos, zero, 0.1, balls, kRenderGlow,
							kRenderFxNoDissipation, damage / 255.0, 6, FTENT_FADEOUT );
						end = beamTrace.endpos - forward;
						scale = gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0;
						gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, beamTrace.endpos, end, balls, (int)( damage * 0.3 ), 0.1, scale, 200, 255, 40 );
						source = beamTrace.endpos + forward;
					}
				}
				gEngfuncs.pEventAPI->EV_PopPMStates();
				punched = true;
			}
		}
	}
}

void EV_FireEliteLeft( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	int index = args->entindex;
	float timeDiff = args->fparam1;
	int bulletsLeft = args->iparam2;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		if ( bulletsLeft <= 1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFTLAST, 2 );
		else if ( timeDiff >= 0.5 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFT5, 2 );
		else if ( timeDiff >= 0.4 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFT4, 2 );
		else if ( timeDiff >= 0.3 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFT3, 2 );
		else if ( timeDiff >= 0.2 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFT2, 2 );
		else if ( timeDiff >= 0 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFT1, 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 0, 0.25f, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 25, 0.3f, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 50, 0.2f, 25, 25, 25, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 35, -11, -16, false );
		shellOrigin = view->attachment[2];
		shellVelocity.x *= 0.75;
		shellVelocity.y *= 0.75;
		shellVelocity.z = shellVelocity.z * 0.75 + 25;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 5 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/elite_fire.wav", 1, ATTN_NORM, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam2, (float)( args->iparam1 / 100.0 ), 0.0f );
	source.x -= 5.0 * right.x;
	source.y -= 5.0 * right.y;
	source.z -= 5.0 * right.z;
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_50AE, 0, &tracerCount[index - 1], 2 );
}

void EV_FireEliteRight( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	int index = args->entindex;
	float timeDiff = args->fparam1;
	int bulletsLeft = args->iparam2;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		if ( bulletsLeft <= 1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHTLAST, 2 );
		else if ( timeDiff >= 0.5 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHT5, 2 );
		else if ( timeDiff >= 0.4 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHT4, 2 );
		else if ( timeDiff >= 0.3 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHT3, 2 );
		else if ( timeDiff >= 0.2 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHT2, 2 );
		else if ( timeDiff >= 0 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHT1, 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[1], forward, 0, 0.25f, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[1], forward, 25, 0.3f, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[1], forward, 50, 0.2f, 25, 25, 25, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 35, -11, 16, false );
		shellOrigin = view->attachment[3];
		shellVelocity.x *= 0.75;
		shellVelocity.y *= 0.75;
		shellVelocity.z = shellVelocity.z * 0.75 + 25;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 5 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/elite_fire.wav", 1, ATTN_NORM, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	source.x += 5.0 * right.x;
	source.y += 5.0 * right.y;
	source.z += 5.0 * right.z;
	Vector spread( args->fparam2, (float)( args->iparam1 / 100.0 ), args->fparam2 );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_50AE, 0, &tracerCount[index - 1], 2 );
}

void EV_FireUSP( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	int silenced = args->bparam2;
	int empty = args->bparam1;
	angles.x += args->iparam1 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		if ( g_bHoldingShield )
		{
			if ( empty )
				gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_SHIELD_SHOOT_EMPTY, 2 );
			else
			{
				int animation = gEngfuncs.pfnRandomLong( 0, 1 );
				gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_SHIELD_SHOOT1 + animation, 2 );
			}
		}
		else if ( silenced )
		{
			if ( empty )
				gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_SHOOT_EMPTY, 2 );
			else
			{
				int animation = gEngfuncs.pfnRandomLong( 0, 2 );
				gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_SHOOT1 + animation, 2 );
			}
		}
		else
		{
			EV_MuzzleFlash();
			if ( empty )
				gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_UNSIL_SHOOT_EMPTY, 2 );
			else
			{
				int animation = gEngfuncs.pfnRandomLong( 0, 2 );
				gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_UNSIL_SHOOT1 + animation, 2 );
			}
		}
		if ( gHUD.m_bRenderGunSmoke )
		{
			Vector smokeOrigin = Vector( view->attachment[0] ) - forward * 3;
			float scale = gEngfuncs.pfnRandomFloat( 0.1f, 0.25f );
			EV_HLDM_CreateSmoke( smokeOrigin, forward, 0, scale, 7, 7, 7, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( smokeOrigin, forward, 20, scale + 0.1, 10, 10, 10, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( smokeOrigin, forward, 40, scale, 13, 13, 13, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			36, -14, rightHand == 0 ? -14 : 14, false );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 0.5;
		shellVelocity.y *= 0.5;
		shellVelocity.z = shellVelocity.z * 0.5 + 45;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 5 );
	if ( silenced )
	{
		int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
		gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/usp1.wav", 1, ATTN_IDLE, 0, pitch );
	}
	else
	{
		int pitch = gEngfuncs.pfnRandomLong( 0, 18 ) + 87;
		gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/usp_unsil-1.wav", 1, ATTN_NORM, 0, pitch );
	}
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 4096,
		BULLET_PLAYER_45ACP, 0, &tracerCount[index - 1], 2 );
}

void EV_FireGlock18( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	int empty = args->bparam1;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		EV_MuzzleFlash();
		if ( g_bHoldingShield )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? GLOCK18_SHIELD_SHOOT_EMPTY : GLOCK18_SHIELD_SHOOT, 2 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? GLOCK18_SHOOT_EMPTY : GLOCK18_SHOOT3, 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 0, 0.2f, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 40, 0.4f, 20, 20, 20, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			36, -14, rightHand == 0 ? -14 : 14, false );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 0.65f;
		shellVelocity.y *= 0.65f;
		shellVelocity.z = shellVelocity.z * 0.65f + 25;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 4 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/glock18-2.wav", 1, ATTN_NORM, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, args->fparam1 );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 4096,
		BULLET_PLAYER_9MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireM4A1( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	int silenced = args->bparam1;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		EV_MuzzleFlash();
		if ( silenced )
		{
			int animation = gEngfuncs.pfnRandomLong( 0, 2 );
			gEngfuncs.pEventAPI->EV_WeaponAnimation( M4A1_SHOOT1 + animation, 2 );
			if ( gHUD.m_bRenderGunSmoke )
				EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.3f, 16, 16, 16, 3, velocity, false, 35 );
		}
		else
		{
			int animation = gEngfuncs.pfnRandomLong( 0, 2 );
			gEngfuncs.pEventAPI->EV_WeaponAnimation( M4A1_UNSIL_SHOOT1 + animation, 2 );
			if ( gHUD.m_bRenderGunSmoke )
				EV_HLDM_CreateSmoke( view->attachment[2], forward, 3, 0.2f, 16, 16, 16, 4, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			20, -8, rightHand == 0 ? -10 : 10, false );
		shellOrigin = view->attachment[1];
		shellVelocity.z -= 45;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 10 );
	if ( silenced )
	{
		int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
		gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/m4a1-1.wav", 1, 1.4f, 0, pitch );
	}
	else
	{
		int sound = gEngfuncs.pfnRandomLong( 0, 1 );
		int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
		gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON,
			sound ? "weapons/m4a1_unsil-1.wav" : "weapons/m4a1_unsil-2.wav", 1, 0.52f, 0, pitch );
	}
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_556MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireAK47( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 2 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( AK47_SHOOT1 + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.35f, 20, 20, 20, 4, velocity, false, 35 );
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			20, -8, rightHand == 0 ? -10 : 10, false );
		shellOrigin = view->attachment[1];
		shellVelocity.z -= 75;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 9 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/ak47-1.wav", 1, 0.4f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_762MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireUMP45( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 2 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( UMP45_SHOOT1 + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.25f, 10, 10, 10, 3, velocity, false, 35 );
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			34, -10, rightHand == 0 ? -11 : 11, false );
		shellOrigin = view->attachment[1];
		shellVelocity.z -= 50;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 13 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/ump45-1.wav", 1, 0.64f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_45ACP, 0, &tracerCount[index - 1], 2 );
}

void EV_FireScout( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 1 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SCOUT_SHOOT + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.45f, 25, 25, 25, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 35, 0.45f, 20, 20, 20, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 70, 0.35f, 15, 15, 15, 2, velocity, false, 35 );
		}
	}
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/scout_fire-1.wav", 1, 1.6f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1 / 1000.0, args->fparam2 / 1000.0, 0.0 );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_762MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireAWP( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 2 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( AWP_SHOOT + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.5f, 20, 20, 20, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 40, 0.5f, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 80, 0.45f, 10, 10, 10, 2, velocity, false, 35 );
		}
	}
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/awp1.wav", 1, 0.36f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_338MAG, 0, &tracerCount[index - 1], 3 );
}

void EV_FireDeagle( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	int empty = args->bparam1;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		if ( empty )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( DEAGLE_SHOOT_EMPTY, 2 );
		else
		{
			int animation = gEngfuncs.pfnRandomLong( 0, 1 );
			gEngfuncs.pEventAPI->EV_WeaponAnimation( DEAGLE_SHOOT1 + animation, 2 );
		}
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 0, 0.25f, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 25, 0.3f, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 50, 0.2f, 25, 25, 25, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			35, -11, rightHand == 0 ? -16 : 16, false );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 0.75;
		shellVelocity.y *= 0.75;
		shellVelocity.z = shellVelocity.z * 0.75 + 25;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 5 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/deagle-1.wav", 1, 0.6f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_50AE, 0, &tracerCount[index - 1], 2 );
}

void EV_FireFiveSeven( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	int empty = args->bparam1;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		if ( empty )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( FIVESEVEN_SHOOT_EMPTY, 2 );
		else
		{
			int animation = gEngfuncs.pfnRandomLong( 0, 1 );
			gEngfuncs.pEventAPI->EV_WeaponAnimation( FIVESEVEN_SHOOT1 + animation, 2 );
		}
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 0, 0.25f, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 25, 0.3f, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 50, 0.2f, 20, 20, 20, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			36, -14, rightHand == 0 ? -14 : 14, false );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 0.75;
		shellVelocity.y *= 0.75;
		shellVelocity.z = shellVelocity.z * 0.75 + 35;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 4 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/fiveseven-1.wav", 1, ATTN_NORM, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_57MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireP228( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	int empty = args->bparam1;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		if ( g_bHoldingShield )
		{
			if ( empty )
				gEngfuncs.pEventAPI->EV_WeaponAnimation( P228_SHIELD_SHOOT_EMPTY, 2 );
			else
			{
				int animation = gEngfuncs.pfnRandomLong( 0, 1 );
				gEngfuncs.pEventAPI->EV_WeaponAnimation( P228_SHIELD_SHOOT1 + animation, 2 );
			}
		}
		else if ( empty )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( P228_SHOOT_EMPTY, 2 );
		else
		{
			int animation = gEngfuncs.pfnRandomLong( 0, 2 );
			gEngfuncs.pEventAPI->EV_WeaponAnimation( P228_SHOOT1 + animation, 2 );
		}
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 0, 0.25f, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 25, 0.3f, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 50, 0.2f, 20, 20, 20, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			36, -14, rightHand == 0 ? -14 : 14, false );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 0.75;
		shellVelocity.y *= 0.75;
		shellVelocity.z = shellVelocity.z * 0.75 + 35;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 4 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/p228-1.wav", 1, ATTN_NORM, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_357SIG, 0, &tracerCount[index - 1], 2 );
}

void EV_Knife( event_args_t* args )
{
	if ( EV_IsLocal( args->entindex ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( args->iparam1, 2 );
}

void EV_FireM3( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	int index = args->entindex;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 1 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( M3_FIRE1 + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.5f, 25, 25, 25, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 25, 0.5f, 20, 20, 20, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 50, 0.45f, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 75, 0.35f, 7, 7, 7, 2, velocity, false, 35 );
		}
	}
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/m3-1.wav", 1, ATTN_NORM * 0.6f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( 0.0675f, 0.0675f, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 9, source, direction, spread, 8192,
		BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[index - 1], 1 );
}

void EV_FireXM1014( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shotgunshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 1 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( XM1014_FIRE1 + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.45f, 15, 15, 15, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 40, 0.35f, 9, 9, 9, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			22, -9, rightHand == 0 ? -11 : 11, false );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 1.25;
		shellVelocity.y *= 1.25;
		shellVelocity.z = shellVelocity.z * 1.25 - 50;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHOTSHELL, index, 3 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/xm1014-1.wav", 1, 0.52f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( 0.0725f, 0.0725f, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 6, source, direction, spread, 8192,
		BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[index - 1], 1 );
}

void EV_FireG3SG1( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 1 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( G3SG1_SHOOT + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.3f, 35, 35, 35, 4, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 35, 0.35f, 30, 30, 30, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 70, 0.3f, 25, 25, 25, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			rightHand == 0 ? 20 : 17, -8, rightHand == 0 ? -10 : 10, false );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 1.5;
		shellVelocity.y *= 1.5;
		shellVelocity.z = shellVelocity.z * 1.5 - 50;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 17 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/g3sg1-1.wav", 1, 0.4f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, args->fparam1 );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_762MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireSG550( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 1 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SG550_SHOOT + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
		{
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.3f, 35, 35, 35, 4, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 35, 0.35f, 30, 30, 30, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 70, 0.3f, 25, 25, 25, 2, velocity, false, 35 );
		}
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			rightHand == 0 ? 20 : 17, -8, rightHand == 0 ? -10 : 10, false );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 1.5;
		shellVelocity.y *= 1.5;
		shellVelocity.z = shellVelocity.z * 1.5 - 50;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 17 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/sg550-1.wav", 1, 0.4f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, args->fparam1 );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_556MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireSG552( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 2 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SG552_SHOOT1 + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.2f, 18, 18, 18, 4, velocity, false, 35 );
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			20, -8, rightHand == 0 ? -10 : 10, false );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 1.65f;
		shellVelocity.y *= 1.65f;
		shellVelocity.z = shellVelocity.z * 1.65f - 120;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 15 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/sg552-1.wav", 1, 0.4f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_556MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireTMP( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 2 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( TMP_SHOOT1 + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.2f, 10, 10, 10, 3, velocity, false, 35 );
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			32, -6, rightHand == 0 ? -11 : 11, false );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 1.2f;
		shellVelocity.y *= 1.2f;
		shellVelocity.z = shellVelocity.z * 1.2f - 50;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 8 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/tmp-1.wav", 1, 1.6f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_9MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireMac10( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 2 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MAC10_SHOOT1 + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 2, 0.25f, 8, 8, 8, 3, velocity, false, 35 );
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			32, -9, rightHand == 0 ? -11 : 11, false );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 0.75;
		shellVelocity.y *= 0.75;
		shellVelocity.z = shellVelocity.z * 0.75 - 65;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 7 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/mac10-1.wav", 1, 0.72f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_9MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireP90( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 2 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( P90_SHOOT1 + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.16f, 7, 7, 7, 4, velocity, false, 35 );
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			35, -16, rightHand == 0 ? -22 : 22, true );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 1.25;
		shellVelocity.y *= 1.25;
		shellVelocity.z = shellVelocity.z * 1.25 - 50;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 10 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/p90-1.wav", 1, 0.64f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_57MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireM249( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 1 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( M249_SHOOT1 + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.26f, 15, 15, 15, 4, velocity, false, 35 );
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			20, -10, rightHand == 0 ? -13 : 13, true );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 1.2f;
		shellVelocity.y *= 1.2f;
		shellVelocity.z = shellVelocity.z * 1.2f - 75;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 10 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/m249-1.wav", 1, 0.52f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_556MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireMP5N( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 2 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5N_SHOOT1 + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.25f, 10, 10, 10, 3, velocity, false, 35 );
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			34, -10, rightHand == 0 ? -11 : 11, false );
		shellOrigin = view->attachment[1];
		shellVelocity.z -= 50;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 13 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/mp5-1.wav", 1, 0.64f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_9MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireAug( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 100.0;
	angles.y += args->iparam2 / 100.0;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 2 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( AUG_SHOOT1 + animation, 2 );
		if ( gHUD.m_bRenderGunSmoke )
			EV_HLDM_CreateSmoke( view->attachment[0], forward, 3, 0.26f, 15, 15, 15, 4, velocity, false, 35 );
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			17, -8, rightHand == 0 ? -14 : 14, true );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 1.25;
		shellVelocity.y *= 1.25;
		shellVelocity.z = shellVelocity.z * 1.25 - 122;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 8 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON, "weapons/aug-1.wav", 1, ATTN_NORM * 0.6f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_556MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireGalil( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 10000000;
	angles.y += args->iparam2 / 10000000;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 2 );
		const int firstFireAnimation = 3;
		gEngfuncs.pEventAPI->EV_WeaponAnimation( firstFireAnimation + animation, 2 );
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			20, -8, rightHand == 0 ? -10 : 10, false );
		shellOrigin = view->attachment[1];
		shellVelocity.z -= 75;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 9 );
	int sound = gEngfuncs.pfnRandomLong( 0, 1 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON,
		sound ? "weapons/galil-1.wav" : "weapons/galil-2.wav", 1, 0.4f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_762MM, 0, &tracerCount[index - 1], 2 );
}

void EV_FireFamas( event_args_t* args )
{
	cl_entity_t* view = GetViewEntity();
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector velocity = args->velocity;
	float rightHand = cl_righthand->value;
	int index = args->entindex;
	angles.x += args->iparam1 / 10000000;
	angles.y += args->iparam2 / 10000000;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	if ( EV_IsLocal( index ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		int animation = gEngfuncs.pfnRandomLong( 0, 2 );
		const int firstFireAnimation = 3;
		gEngfuncs.pEventAPI->EV_WeaponAnimation( firstFireAnimation + animation, 2 );
	}
	Vector shellVelocity, shellOrigin;
	if ( EV_IsLocal( index ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up,
			17, -8, rightHand == 0 ? -14 : 14, true );
		shellOrigin = view->attachment[1];
		shellVelocity.x *= 1.25;
		shellVelocity.y *= 1.25;
		shellVelocity.z = shellVelocity.z * 1.25 - 122;
	}
	else
		EV_GetDefaultShellInfo( args, origin, velocity, shellVelocity, shellOrigin, forward, right, up, 20, -12, 4, false );
	EV_EjectBrass( shellOrigin, shellVelocity, angles.y, shell, TE_BOUNCE_SHELL, index, 8 );
	int sound = gEngfuncs.pfnRandomLong( 0, 1 );
	int pitch = gEngfuncs.pfnRandomLong( 0, 15 ) + 94;
	gEngfuncs.pEventAPI->EV_PlaySound( index, origin, CHAN_WEAPON,
		sound ? "weapons/famas-1.wav" : "weapons/famas-2.wav", 1, ATTN_NORM * 0.6f, 0, pitch );
	Vector source;
	EV_GetGunPosition( args, source, origin );
	Vector direction = forward;
	Vector spread( args->fparam1, args->fparam2, 0.0f );
	EV_HLDM_FireBullets( index, forward, right, up, 1, source, direction, spread, 8192,
		BULLET_PLAYER_556MM, 0, &tracerCount[index - 1], 2 );
}
