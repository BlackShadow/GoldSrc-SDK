#pragma once

#include "decals.h"

typedef struct
{
	char name[DECAL_NAME_BYTES];
	unsigned char ucFlags;
} decalname_t;

extern decalname_t sv_decalnames[MAX_BASE_DECALS];
extern int sv_decalnamecount;
