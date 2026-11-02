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
// pm_defs.h
#pragma once

#include "engine_edict.h"
#include "../common/usercmd.h"
#include "../common/pmtrace.h"
#include "../pm_shared/pm_info.h"

#define MAX_PHYSENTS 600 // Must have room for all entities in the world.
#define MAX_MOVEENTS 64
#define MAX_PLAYER_HULLS 4
#define PHYSENT_NAME_BYTES 32
#define PM_TEXTURE_NAME_BYTES 256

#define PM_NORMAL 0x00000000
#define PM_STUDIO_IGNORE 0x00000001 // Skip studio models
#define PM_STUDIO_BOX 0x00000002	// Use boxes for non-complex studio models (even in traceline)
#define PM_GLASS_IGNORE 0x00000004	// Ignore entities with non-normal rendermode
#define PM_WORLD_ONLY 0x00000008	// Only trace against the world

struct model_s;
struct movevars_s;
struct hull_s;

// physent_t
typedef struct physent_s
{
	char name[PHYSENT_NAME_BYTES]; // Name of model, or "player" or "world".
	int player;
	vec3_t origin;			 // Model's origin in world coordinates.
	model_s* model;			 // only for bsp models
	model_s* studiomodel;	 // SOLID_BBOX, but studio clip intersections.
	vec3_t mins, maxs;		 // only for non-bsp models
	int info;				 // For client or server to use to identify (index into edicts or cl_entities)
	vec3_t angles;			 // rotated entities need this info for hull testing to work.

	int solid;		// Triggers and func_door type WATER brushes are SOLID_NOT
	int skin;		// BSP Contents for such things like fun_door water brushes.
	int rendermode; // So we can ignore glass

	// Complex collision detection.
	float frame;
	int sequence;
	byte controller[4];
	byte blending[2];

	int movetype;
	int takedamage;
	int blooddecal;
	int team;
	int classnumber;

	// For mods
	int iuser1, iuser2, iuser3, iuser4;
	float fuser1, fuser2, fuser3, fuser4;
	vec3_t vuser1, vuser2, vuser3, vuser4;
} physent_t;

typedef struct playermove_s
{
	int player_index; // So we don't try to run the PM_CheckStuck nudging too quickly.
	qboolean server;  // For debugging, are we running physics code on server side?

	qboolean multiplayer; // 1 == multiplayer server
	float time;			  // realtime on host, for reckoning duck timing
	float frametime;	  // Duration of this frame

	vec3_t forward, right, up; // Vectors for angles

	// player state
	vec3_t origin;		 // Movement origin.
	vec3_t angles;		 // Movement view angles.
	vec3_t oldangles;	 // Angles before movement view angles were looked at.
	vec3_t velocity;	 // Current movement direction.
	vec3_t movedir;		 // For waterjumping, a forced forward velocity so we can fly over lip of ledge.
	vec3_t basevelocity; // Velocity of the conveyor we are standing, e.g.

	// For ducking/dead
	vec3_t view_ofs; // Our eye position.
	float flDuckTime; // Time we started duck
	qboolean bInDuck; // In process of ducking or ducked already?

	// For walking/falling
	int flTimeStepSound; // Next time we can play a step sound
	int iStepLeft;

	float flFallVelocity;
	vec3_t punchangle;

	float flSwimTime;

	float flNextPrimaryAttack;

	int effects; // MUZZLE FLASH, e.g.

	int flags;	 // FL_ONGROUND, FL_DUCKING, etc.
	int usehull; // 0 = regular player hull, 1 = ducked player hull, 2 = point hull
	float gravity; // Our current gravity and friction.
	float friction;
	int oldbuttons; // Buttons last usercmd
	float waterjumptime; // Amount of time left in jumping out of water cycle.
	qboolean dead; // Are we a dead player?
	int deadflag;
	int spectator; // Should we use spectator physics model?
	int movetype;  // Our movement type, NOCLIP, WALK, FLY

	int onground;
	int waterlevel;
	int watertype;
	int oldwaterlevel;

	char sztexturename[PM_TEXTURE_NAME_BYTES];
	char chtexturetype;

	float maxspeed;
	float clientmaxspeed; // Player specific maxspeed

	// For mods
	int iuser1, iuser2, iuser3, iuser4;
	float fuser1, fuser2, fuser3, fuser4;
	vec3_t vuser1, vuser2, vuser3, vuser4;

	// world state
	// Number of entities to clip against.
	int numphysent;
	physent_t physents[MAX_PHYSENTS];
	// Number of momvement entities (ladders)
	int nummoveent;
	// just a list of ladders
	physent_t moveents[MAX_MOVEENTS];

	// All things being rendered, for tracing against things you don't actually collide with
	int numvisent;
	physent_t visents[MAX_PHYSENTS];

	// input to run through physics.
	usercmd_t cmd;

	// Trace results for objects we collided with.
	int numtouch;
	pmtrace_t touchindex[MAX_PHYSENTS];

	char physinfo[MAX_PHYSINFO_STRING]; // Physics info string

	movevars_s* movevars;
	vec3_t player_mins[MAX_PLAYER_HULLS];
	vec3_t player_maxs[MAX_PLAYER_HULLS];

	// Common functions
	const char* ( *PM_Info_ValueForKey )( const char* info, const char* key );
	void ( *PM_Particle )( float* origin, int color, float life, int zpos, int zvel );
	int ( *PM_TestPlayerPosition )( float* pos, pmtrace_t* trace );
	void ( *Con_NPrintf )( int index, char* format, ... );
	void ( *Con_DPrintf )( const char* format, ... );
	void ( *Con_Printf )( const char* format, ... );
	double ( *Sys_FloatTime )();
	void ( *PM_StuckTouch )( int hitent, pmtrace_t* trace );
	int ( *PM_PointContents )( float* point, int* truecontents );
	int ( *PM_TruePointContents )( float* point );
	int ( *PM_HullPointContents )( hull_s* hull, int num, float* point );
	pmtrace_t ( *PM_PlayerTrace )( float* start, float* end, int traceFlags, int ignore_pe );
	pmtrace_t* ( *PM_TraceLine )( float* start, float* end, int flags, int usehull, int ignore_pe );
	int ( *RandomLong )( int low, int high );
	float ( *RandomFloat )( float low, float high );
	int ( *PM_GetModelType )( model_s* model );
	void ( *PM_GetModelBounds )( model_s* model, float* mins, float* maxs );
	hull_s* ( *PM_HullForBsp )( physent_t* entity, float* offset );
	float ( *PM_TraceModel )( physent_t* entity, float* start, float* end, trace_t* trace );
	int ( *COM_FileSize )( char* filename );
	byte* ( *COM_LoadFile )( char* path, int usehunk, int* length );
	void ( *COM_FreeFile )( void* buffer );
	char* ( *memfgets )( byte* file, int fileSize, int* position, char* buffer, int bufferSize );

	// Functions
	// Run functions for this frame?
	qboolean runfuncs;
	void ( *PM_PlaySound )( int channel, const char* sample, float volume, float attenuation, int flags, int pitch );
	const char* ( *PM_TraceTexture )( int ground, float* start, float* end );
	void ( *PM_PlaybackEventFull )( int flags, int clientindex, unsigned short eventindex, float delay, float* origin,
		float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );

	pmtrace_t ( *PM_PlayerTraceEx )( float* start, float* end, int flags, int ( *ignore )( physent_t* ) );
	int ( *PM_TestPlayerPositionEx )( float* pos, pmtrace_t* trace, int ( *ignore )( physent_t* ) );
	pmtrace_t* ( *PM_TraceLineEx )( float* start, float* end, int flags, int usehull, int ( *ignore )( physent_t* ) );
} playermove_t;

extern playermove_t* pmove;
extern movevars_s movevars;
extern vec3_t player_mins[MAX_PLAYER_HULLS];
extern vec3_t player_maxs[MAX_PLAYER_HULLS];

void			PM_Init( playermove_t* move );
void			PM_InitBoxHull();
void			PM_StuckTouch( int hitEntity, pmtrace_t* trace );
int				PM_TestPlayerPosition( float* position, pmtrace_t* trace );
int				PM_TestPlayerPositionEx( float* position, pmtrace_t* trace, int ( *ignore )( physent_t* ) );
int				PM_PointContents( float* point, int* trueContents );
int				PM_TruePointContents( float* point );
int				PM_HullPointContents( hull_s* hull, int node, float* point );
int				PM_LinkContents( float* point, int* index );
int				PM_WaterEntity( float* point );
pmtrace_t		PM_PlayerTrace( float* start, float* end, int flags, int ignoreEntity );
pmtrace_t		PM_PlayerTraceEx( float* start, float* end, int flags, int ( *ignore )( physent_t* ) );
pmtrace_t*		PM_TraceLine( float* start, float* end, int flags, int useHull, int ignoreEntity );
pmtrace_t*		PM_TraceLineEx( float* start, float* end, int flags, int useHull, int ( *ignore )( physent_t* ) );
int				PM_GetModelType( model_s* model );
void			PM_GetModelBounds( model_s* model, float* mins, float* maxs );
hull_s*			PM_HullForBsp( physent_t* entity, float* offset );
float			PM_TraceModel( physent_t* entity, float* start, float* end, trace_t* trace );

void			PM_CL_PlaySound( int channel, const char* sample, float volume, float attenuation, int flags, int pitch );
const char*		PM_CL_TraceTexture( int ground, float* start, float* end );
void			PM_CL_PlaybackEventFull( int flags, int clientIndex, unsigned short eventIndex, float delay, float* origin,
					float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
