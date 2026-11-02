#pragma once

#include "const.h"

typedef enum
{
	BOTH,
	SINGLEPLAYER_ONLY,
	MULTIPLAYER_ONLY
} MOD_GAMEPLAY_TYPE_E;

typedef struct modinfo_s
{
	qboolean bIsMod;
	char szInfo[256];
	char szDL[256];
	char szHLVersion[32];
	int version;
	int size;
	qboolean svonly;
	qboolean cldll;
	qboolean secure;
	MOD_GAMEPLAY_TYPE_E type;
	int num_edicts;
	qboolean clientcrccheck;
} modinfo_t;

extern modinfo_t gmodinfo;
