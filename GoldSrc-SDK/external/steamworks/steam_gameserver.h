// Copyright Valve Corporation. All rights reserved.

#pragma once

#include "steam_api_common.h"

enum EServerMode
{
	eServerModeInvalid = 0,
	eServerModeNoAuthentication = 1,
	eServerModeAuthentication = 2,
	eServerModeAuthenticationAndSecure = 3
};

const uint16 STEAMGAMESERVER_QUERY_PORT_SHARED = 0xffff;

S_API bool S_CALLTYPE SteamInternal_GameServer_Init( uint32 unIP,
	uint16 usLegacySteamPort, uint16 usGamePort, uint16 usQueryPort,
	EServerMode eServerMode, const char* pchVersionString );

S_API void S_CALLTYPE SteamGameServer_Shutdown();

static_assert( sizeof( EServerMode ) == 4 );
