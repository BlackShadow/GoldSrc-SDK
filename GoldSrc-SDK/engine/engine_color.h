#pragma once

#include "mathlib.h"
#include "../common/const.h"

#define COLOR_LIGHTMASK			0xFF00
#define COLOR_LIGHTSCALE_BYTE		192
#define COLOR_LIGHTSCALE_FLOAT	49280.0
#define COLOR_FULLBRIGHT			49152

#define COLOR_REDMASK_555			0x7C00
#define COLOR_GREENMASK_555		0x03E0
#define COLOR_REDMASK_565			0xF800
#define COLOR_GREENMASK_565		0x07E0
#define COLOR_BLUEMASK			0x001F

#define COLOR_REDBLUEMASK_555		0x7C1F
#define COLOR_REDBLUEMASK_565		0xF81F
#define COLOR_PACKEDMASK_555		0x7C1F03E0
#define COLOR_PACKEDMASK_565		0xF81F07E0

extern word*	r_palette;

short	hlRGB( word* palette, int index );
short	PackedRGB( byte* palette, int index );
short	PutRGB( colorVec* color );
short	RGBToPacked( byte* color );
void	GetRGB( short packed, colorVec* color );
