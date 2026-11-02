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
#include "netadr.h"
#include "hud_servers.h"
#include "vgui_int.h"
#include "vgui_CTFViewport.h"
#include "voice_status.h"
#include "interface.h"
#include "cl_dll/IGameClientExports.h"
//#include "vgui_schememanager.h"

extern "C"
{
#include "pm_shared.h"
}

#include <stddef.h>
#include <string.h>

cl_enginefunc_t gEngfuncs;
CHud gHUD;
CTFViewport* gViewPort = NULL;

extern int giR, giG, giB;
void InitInput();
void EV_HookEvents();
void IN_Commands();

int EXPORT HUD_GetHullBounds( int hull, float*, float* )
{
	return (unsigned int)hull <= 2;
}

int EXPORT HUD_ConnectionlessPacket( const netadr_s*, const char*, char*, int* responseSize )
{
	*responseSize = 0;
	return 0;
}

void EXPORT HUD_PlayerMoveInit( playermove_s* move )
{
//	RecClClientMoveInit(ppmove);

	PM_Init( move );
}

char EXPORT HUD_PlayerMoveTexture( char* name )
{
//	RecClClientTextureType(name);

	return PM_FindTextureType( name );
}

void EXPORT HUD_PlayerMove( playermove_s* move, int server )
{
//	RecClClientMove(ppmove, server);

	PM_Move( move, server );
}

int EXPORT Initialize( cl_enginefunc_t* engineFuncs, int version )
{
	const size_t engineFuncsSize = sizeof( cl_enginefunc_t );
	memcpy( &gEngfuncs, engineFuncs, engineFuncsSize );
	if ( version != CLDLL_INTERFACE_VERSION )
		return 0;

	memcpy( &gEngfuncs, engineFuncs, engineFuncsSize );
	EV_HookEvents();
	return 1;
}

int EXPORT HUD_VidInit()
{
//	RecClHudVidInit();
	gHUD.VidInit();
	VGui_Startup();
	return 1;
}

int EXPORT HUD_Init()
{
//	RecClHudInit();
	InitInput();
	giR = 0;
	giG = 160;
	giB = 0;
	gHUD.Init();
	Scheme_Init();
	return 1;
}


/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/

int EXPORT HUD_Redraw( float time, int intermission )
{
//	RecClHudRedraw(time, intermission);

	gHUD.Redraw( time, intermission );
	return 1;
}

int EXPORT HUD_UpdateClientData( client_data_t* data, float time )
{
//	RecClHudUpdateClientData(pcldata, flTime);

	IN_Commands();
	return gHUD.UpdateClientData( data, time );
}

int EXPORT HUD_Reset()
{
	gHUD.VidInit();
	return 1;
}

/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/

void EXPORT HUD_Frame( double time )
{
//	RecClHudFrame(time);

	ServersThink( time );
	GetClientVoiceMgr()->Frame( time );
}

void EXPORT HUD_VoiceStatus( int entity, qboolean talking )
{
////	RecClVoiceStatus(entindex, bTalking);

	GetClientVoiceMgr()->UpdateSpeakerStatus( entity, talking );
}

void EXPORT HUD_DirectorEvent( unsigned char command, unsigned int firstObject,
	unsigned int secondObject, unsigned int flags )
{
	gHUD.m_Spectator.DirectorEvent( command, firstObject, secondObject, flags );
}

void EXPORT HUD_ChatInputPosition( int*, int* y )
{
	if ( g_iUser1 || gEngfuncs.IsSpectateOnly() )
		*y = YRES( gHUD.m_Spectator.m_OverviewData.insetWindowHeight + 5 );
}

extern "C" CreateInterfaceFn EXPORT ClientFactory()
{
	return Sys_GetFactoryThis();
}

//-----------------------------------------------------------------------------
// Purpose: Exports functions that are used by the gameUI for UI dialogs
//-----------------------------------------------------------------------------
class CClientExports : public IGameClientExports
{
public:
	// returns the name of the server the user is connected to, if any
	const char* GetServerHostName() override
	{
		/*if (gViewPortInterface)
		{
			return gViewPortInterface->GetServerName();
		}*/
		return "";
	}

	bool IsPlayerGameVoiceMuted( int player ) override
	{
		return GetClientVoiceMgr() && GetClientVoiceMgr()->IsPlayerBlocked( player );
	}

	void MutePlayerGameVoice( int player ) override
	{
		if ( GetClientVoiceMgr() )
			GetClientVoiceMgr()->SetPlayerBlockedState( player, true );
	}

	void UnmutePlayerGameVoice( int player ) override
	{
		if ( GetClientVoiceMgr() )
			GetClientVoiceMgr()->SetPlayerBlockedState( player, false );
	}
};

EXPOSE_SINGLE_INTERFACE( CClientExports, IGameClientExports, GAMECLIENTEXPORTS_INTERFACE_VERSION );
