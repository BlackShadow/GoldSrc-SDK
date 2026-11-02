#pragma once

#include "../external/steamworks/steamtypes.h"

extern "C" bool		ISteamApps_BIsSubscribedApp( uint32 appid );
extern "C" uint64	ISteamGameServer_CreateUnauthenticatedUserConnection();
extern "C" bool		ISteamGameServer_BUpdateUserData( uint64 steamID, const char* playerName, uint32 score );
