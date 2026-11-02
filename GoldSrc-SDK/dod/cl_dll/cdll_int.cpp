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
//
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//

#include "hud.h"
#include "cl_util.h"
#include "Exports.h"
#include "netadr.h"
#include "FileSystem.h"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4018)
#endif
#include "particleman.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "tri.h"
#include "voice_status.h"
#include "game_controls/iviewport.h"
#include "cl_dll/IGameClientExports.h"
//#include "vgui_schememanager.h"

extern "C"
{
#include "pm_shared.h"
}

class TeamFortressViewport;

cl_enginefunc_t gEngfuncs;
CHud gHUD;
TeamFortressViewport* gViewPort;
bool b_StopDemo;
CSysModule* g_hFileSystemModule;
IFileSystem* g_pFileSystem;
modfuncs_t g_modfuncs;
cldll_func_dst_t* g_pcldstAddrs;
CSysModule* g_hTrackerModule;
CSysModule* g_hParticleManModule;
IParticleMan* g_pParticleMan;

class IFriendsUser;
IFriendsUser* g_pFriendsUser;

void InitInput();
void ShutdownInput();
void IN_Commands();
void EV_HookEvents();
void CL_LoadParticleMan();
CVoiceStatus* GetClientVoice();

int CL_DLLEXPORT HUD_GetPlayerTeam( int player )
{
	if ( (unsigned int)player <= 64 )
		return g_PlayerExtraInfo[player].teamId;
	return 0;
}

CreateInterfaceFn ClientFactory()
{
	return Sys_GetFactoryThis();
}

int CL_DLLEXPORT HUD_GetHullBounds( int hull, float* mins, float* maxs )
{
	g_pcldstAddrs->pGetHullBounds( &hull, &mins, &maxs );
	switch ( hull )
	{
	case 0:
		Vector( -16, -16, -36 ).CopyToArray( mins );
		Vector( 16, 16, 36 ).CopyToArray( maxs );
		return 1;
	case 1:
		Vector( -16, -16, -18 ).CopyToArray( mins );
		Vector( 16, 16, 18 ).CopyToArray( maxs );
		return 1;
	case 2:
		Vector( 0, 0, 0 ).CopyToArray( mins );
		Vector( 0, 0, 0 ).CopyToArray( maxs );
		return 1;
	}
	return 0;
}

int CL_DLLEXPORT HUD_ConnectionlessPacket( const netadr_s* from, const char* args,
	char* response, int* responseSize )
{
	g_pcldstAddrs->pConnectionlessPacket( &from, &args, &response, &responseSize );
	*responseSize = 0;
	return 0;
}

void CL_DLLEXPORT HUD_Reset()
{
	g_pcldstAddrs->pHudResetFunc();
	b_StopDemo = false;
}

void CL_DLLEXPORT HUD_DirectorMessage( int size, void* buffer )
{
	g_pcldstAddrs->pDirectorMessage( &size, &buffer );
	gHUD.m_Spectator.DirectorMessage( size, buffer );
}

void CL_DLLEXPORT HUD_VoiceStatus( int index, qboolean talking )
{
	g_pcldstAddrs->pVoiceStatus( &index, &talking );
	GetClientVoice()->UpdateSpeakerStatus( index, talking > 0 );
	gHUD.m_StatusBar.m_TargetTalking[index] = talking;
}

/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/

void CL_DLLEXPORT HUD_Frame( double time )
{
	g_pcldstAddrs->pHudFrame( &time );
	GetClientVoice()->Frame( time );
}

int CL_DLLEXPORT HUD_UpdateClientData( client_data_t* data, float time )
{
	g_pcldstAddrs->pHudUpdateClientDataFunc( &data, &time );
//	RecClHudUpdateClientData(pcldata, flTime);

	IN_Commands();
	return gHUD.UpdateClientData( data, time );
}


/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/

int CL_DLLEXPORT HUD_Redraw( float time, int intermission )
{
	g_pcldstAddrs->pHudRedrawFunc( &time, &intermission );
//	RecClHudRedraw(time, intermission);

	gHUD.Redraw( time, intermission );
	return 1;
}

void CL_DLLEXPORT HUD_Init()
{
	g_pcldstAddrs->pHudInitFunc();
//	RecClHudInit();
	InitInput();
	gHUD.Init();
}

int CL_DLLEXPORT HUD_VidInit()
{
	g_pcldstAddrs->pHudVidInitFunc();
	gHUD.VidInit();
	return 1;
}

void CL_DLLEXPORT HUD_PlayerMove( playermove_s* move, int server )
{
	g_pcldstAddrs->pClientMove( &move, &server );
	PM_Move( move, server );
}

char CL_DLLEXPORT HUD_PlayerMoveTexture( char* name )
{
	g_pcldstAddrs->pClientTextureType( &name );
//	RecClClientTextureType(name);

	return PM_FindTextureType( name );
}

void CL_DLLEXPORT HUD_PlayerMoveInit( playermove_s* move )
{
	g_pcldstAddrs->pClientMoveInit( &move );
	PM_Init( move );
}

void CL_UnloadParticleMan()
{
	Sys_UnloadModule( g_hParticleManModule );
	g_pParticleMan = NULL;
	g_hParticleManModule = NULL;
}

void CL_DLLEXPORT HUD_Shutdown()
{
	g_pcldstAddrs->pShutdown();
	ShutdownInput();
	CL_UnloadParticleMan();
	if ( g_hTrackerModule )
		Sys_UnloadModule( g_hTrackerModule );
	if ( g_hFileSystemModule )
		Sys_UnloadModule( g_hFileSystemModule );
}

void CL_LoadParticleMan()
{
	char path[512];
	if ( !gEngfuncs.COM_ExpandFilename( PARTICLEMAN_DLLNAME, path, sizeof( path ) ) )
	{
		g_pParticleMan = NULL;
		g_hParticleManModule = NULL;
		return;
	}
	g_hParticleManModule = Sys_LoadModule( path );
	CreateInterfaceFn factory = Sys_GetFactory( g_hParticleManModule );
	if ( !factory )
	{
		g_pParticleMan = NULL;
		g_hParticleManModule = NULL;
		return;
	}
	g_pParticleMan = (IParticleMan*)factory( PARTICLEMAN_INTERFACE, NULL );
	if ( g_pParticleMan )
	{
		g_pParticleMan->SetUp( &gEngfuncs );
		g_pParticleMan->AddCustomParticleClassSize( sizeof( CBaseDoDParticle ) );
		g_pParticleMan->AddCustomParticleClassSize( sizeof( CDoDParticle ) );
		g_pParticleMan->AddCustomParticleClassSize( sizeof( CDoDSnowFlake ) );
	}
}

int CL_DLLEXPORT Initialize( cl_enginefunc_t* engine, int version )
{
	memcpy( &gEngfuncs, engine, sizeof( gEngfuncs ) );
	g_pcldstAddrs->pInitFunc( &engine, &version );
	if ( version != CLDLL_INTERFACE_VERSION )
		return 0;
	EV_HookEvents();
#ifdef _WIN32
	char path[MAX_PATH];
	const char* filesystem = "filesystem_stdio.dll";
#else
	char path[4096];
	const char* filesystem = "filesystem_stdio.so";
#endif
	path[0] = '\0';
	if ( !gEngfuncs.COM_ExpandFilename( filesystem, path, sizeof( path ) ) )
		return 0;
	g_hFileSystemModule = Sys_LoadModule( path );
	if ( !g_hFileSystemModule )
		return 0;
	CreateInterfaceFn factory = Sys_GetFactory( g_hFileSystemModule );
	if ( !factory )
		return 0;
	g_pFileSystem = (IFileSystem*)factory( FILESYSTEM_INTERFACE_VERSION, NULL );
	if ( !g_pFileSystem )
		return 0;
	CL_LoadParticleMan();
#ifdef _WIN32
	char trackerPath[512];
	if ( !gEngfuncs.COM_ExpandFilename( "Bin/TrackerUI.dll", trackerPath, sizeof( trackerPath ) ) )
	{
		g_pFriendsUser = NULL;
		g_hTrackerModule = NULL;
		return 1;
	}
	g_hTrackerModule = Sys_LoadModule( trackerPath );
	CreateInterfaceFn trackerFactory = Sys_GetFactory( g_hTrackerModule );
	if ( !trackerFactory )
	{
		g_pFriendsUser = NULL;
		g_hTrackerModule = NULL;
		return 1;
	}
	g_pFriendsUser = (IFriendsUser*)trackerFactory( "FriendsUser001", NULL );
#else
	g_pFriendsUser = NULL;
#endif
	return 1;
}

extern "C" void CL_DLLEXPORT F( void* table )
{
	cldll_func_t* functions = (cldll_func_t*)table;
	memcpy( &g_modfuncs, (const void*)functions->pInitFunc, sizeof( g_modfuncs ) );
	g_pcldstAddrs = (cldll_func_dst_t*)functions->pHudVidInitFunc;
	cldll_func_t callbacks =
	{
		Initialize, HUD_Init, HUD_VidInit, HUD_Redraw, HUD_UpdateClientData, HUD_Reset,
		HUD_PlayerMove, HUD_PlayerMoveInit, HUD_PlayerMoveTexture,
		IN_ActivateMouse, IN_DeactivateMouse, IN_MouseEvent, IN_ClearStates, IN_Accumulate,
		CL_CreateMove, CL_IsThirdPerson, CL_CameraOffset, KB_Find, CAM_Think, V_CalcRefdef,
		HUD_AddEntity, HUD_CreateEntities, HUD_DrawNormalTriangles, HUD_DrawTransparentTriangles,
		HUD_StudioEvent, HUD_PostRunCmd, HUD_Shutdown, HUD_TxferLocalOverrides, HUD_ProcessPlayerState,
		HUD_TxferPredictionData, Demo_ReadBuffer, HUD_ConnectionlessPacket, HUD_GetHullBounds,
		HUD_Frame, HUD_Key_Event, HUD_TempEntUpdate, HUD_GetUserEntity, HUD_VoiceStatus,
		HUD_DirectorMessage, HUD_GetStudioModelInterface, HUD_ChatInputPosition,
		HUD_GetPlayerTeam, (CLIENTFACTORY)ClientFactory
	};
	*functions = callbacks;
}

//-----------------------------------------------------------------------------
// Purpose: Exports functions that are used by the gameUI for UI dialogs
//-----------------------------------------------------------------------------
class CClientExports : public IGameClientExports
{
public:
	const char* GetServerHostName() override
	{
		return gViewPortInterface ? gViewPortInterface->GetServerName() : "";
	}

	bool IsPlayerGameVoiceMuted( int player ) override
	{
		return GetClientVoice() ? GetClientVoice()->IsPlayerBlocked( player ) : false;
	}

	void MutePlayerGameVoice( int player ) override
	{
		if ( GetClientVoice() )
			GetClientVoice()->SetPlayerBlockedState( player, true );
	}

	void UnmutePlayerGameVoice( int player ) override
	{
		if ( GetClientVoice() )
			GetClientVoice()->SetPlayerBlockedState( player, false );
	}
};

EXPOSE_SINGLE_INTERFACE( CClientExports, IGameClientExports, GAMECLIENTEXPORTS_INTERFACE_VERSION );
