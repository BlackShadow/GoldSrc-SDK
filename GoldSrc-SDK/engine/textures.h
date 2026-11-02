#pragma once

#include "const.h"

qboolean	TEX_InitFromWad( char* path );
void		TEX_AddAnimatingTextures();
int			TEX_LoadLump( char* name, byte* destination, int destinationSize );
void		TEX_CleanupWadInfo();
