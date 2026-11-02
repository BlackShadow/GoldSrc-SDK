#pragma once

#include "const.h"
#include "SteamCommon.h"

#include "../external/steamworks/steam_api_common.h"

S_API bool S_CALLTYPE SteamAPI_Init();
S_API void S_CALLTYPE SteamAPI_Shutdown();
S_API void S_CALLTYPE SteamGameServer_Shutdown();
S_API void S_CALLTYPE SteamAPI_SetBreakpadAppID( uint32 appid );

int					Steam_GSInitiateGameConnection( void* data, int maxDataSize, uint64 serverID, uint32 serverIP,
						uint16 serverPort, qboolean secure );
void				Steam_GSTerminateGameConnection();
const char*			Steam_GetCommunityName();

void				Steam_ActivateInputSet( bool inMenu );
void				Steam_UpdateRichPresence();
void				Steam_SetRichPresenceOverride( const char* text );
void				Steam_SetRichPresenceInfo( const char* mapName );
void				Steam_SetRichPresenceGameMode( const char* text );

TSteamGlobalUserID	Steam_Steam3IDtoSteam2( uint64 steamID );
uint64				Steam_StringToSteamID( const char* text );
