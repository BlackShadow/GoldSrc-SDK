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
//  cl_dll.h
//

// 4-23-98  JOHN

//
//  This DLL is linked by the client when they first initialize.
// This DLL is responsible for the following tasks:
//		- Loading the HUD graphics upon initialization
//		- Drawing the HUD graphics every frame
//		- Handling the custum HUD-update packets
//
typedef unsigned char byte;
typedef unsigned short word;
typedef float vec_t;
typedef int (*pfnUserMsgHook)(const char *pszName, int iSize, void *pbuf);

#include "util_vector.h"
#ifdef _WIN32
#define EXPORT	_declspec( dllexport )
#else
#define EXPORT	__attribute__ ((visibility("default")))
#endif

#include "../../engine/cdll_int.h"
#include "../dlls/cdll_dll.h"

extern cl_enginefunc_t gEngfuncs;

#define CL_DLLEXPORT EXPORT

extern "C"
{
int EXPORT Initialize( cl_enginefunc_t* engineFuncs, int version );
int EXPORT HUD_Init();
int EXPORT HUD_VidInit();
int EXPORT HUD_Redraw( float time, int intermission );
int EXPORT HUD_UpdateClientData( client_data_t* data, float time );
int EXPORT HUD_Reset();
void EXPORT HUD_PlayerMove( struct playermove_s* move, int server );
void EXPORT HUD_PlayerMoveInit( struct playermove_s* move );
char EXPORT HUD_PlayerMoveTexture( char* name );
int EXPORT HUD_ConnectionlessPacket( const struct netadr_s* from, const char* args,
	char* response, int* responseSize );
int EXPORT HUD_GetHullBounds( int hull, float* mins, float* maxs );
void EXPORT HUD_Frame( double time );
void EXPORT HUD_VoiceStatus( int entity, qboolean talking );
void EXPORT HUD_PostRunCmd( struct local_state_s* from, struct local_state_s* to,
	struct usercmd_s* cmd, int runFunctions, double time, unsigned int seed );
void EXPORT HUD_DirectorEvent( unsigned char command, unsigned int firstObject,
	unsigned int secondObject, unsigned int flags );
void EXPORT Demo_ReadBuffer( int size, unsigned char* buffer );
int EXPORT HUD_AddEntity( int type, struct cl_entity_s* entity, const char* model );
void EXPORT HUD_CreateEntities();
void EXPORT HUD_StudioEvent( const struct mstudioevent_s* event, const struct cl_entity_s* entity );
void EXPORT HUD_TxferLocalOverrides( struct entity_state_s* state, const struct clientdata_s* client );
void EXPORT HUD_ProcessPlayerState( struct entity_state_s* dst, const struct entity_state_s* src );
void EXPORT HUD_TxferPredictionData( struct entity_state_s* state, const struct entity_state_s* previousState,
	struct clientdata_s* client, const struct clientdata_s* previousClient,
	struct weapon_data_s* weapons, const struct weapon_data_s* previousWeapons );
void EXPORT HUD_TempEntUpdate( double frametime, double clientTime, double gravity,
	struct tempent_s** freeList, struct tempent_s** activeList,
	int ( *addVisibleEntity )( struct cl_entity_s* ), void ( *playSound )( struct tempent_s*, float ) );
EXPORT struct cl_entity_s* HUD_GetUserEntity( int index );
void EXPORT CAM_Think();
int EXPORT CL_IsThirdPerson();
void EXPORT CL_CameraOffset( float* offset );
EXPORT struct kbutton_s* KB_Find( const char* name );
void EXPORT CL_CreateMove( float frametime, struct usercmd_s* cmd, int active );
void EXPORT HUD_Shutdown();
int EXPORT HUD_Key_Event( int down, int key, const char* binding );
void EXPORT IN_ActivateMouse();
void EXPORT IN_DeactivateMouse();
void EXPORT IN_MouseEvent( int buttons );
void EXPORT IN_ClearStates();
void EXPORT IN_Accumulate();
void EXPORT HUD_DrawNormalTriangles();
void EXPORT HUD_DrawTransparentTriangles();
void EXPORT V_CalcRefdef( struct ref_params_s* params );
}

void EXPORT HUD_ChatInputPosition( int* x, int* y );
