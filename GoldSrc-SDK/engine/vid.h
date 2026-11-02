// vid.h -- video driver defs

#pragma once

#include "const.h"

typedef byte pixel_t;

typedef enum
{
	VT_None,
	VT_Software,
	VT_OpenGL,
	VT_Direct3D
} VidTypes;

typedef struct viddef_s
{
	pixel_t* buffer;   // invisible buffer
	pixel_t* colormap; // 256 * VID_GRADES size
	unsigned short* colormap16; // 256 * VID_GRADES size
	int fullbright; // index of first fullbright color
	int bits;
	int is15bit;
	unsigned int rowbytes; // may be > width if displayed in a window
	unsigned int width;
	unsigned int height;
	float aspect; // width / height -- < 0 is taller than wide
	int numpages;
	int recalc_refdef; // if true, recalc vid-based stuff
	pixel_t* conbuffer;
	int conrowbytes;
	unsigned int conwidth;
	unsigned int conheight;
	unsigned int maxwarpwidth;
	unsigned int maxwarpheight;
	pixel_t* direct; // direct drawing to framebuffer, if not
									//  NULL
	VidTypes vidtype;
} viddef_t;

extern viddef_t vid; // global video state

#if defined( SW_RENDERER )
#define WARP_WIDTH	640

extern byte*	vid_surfcache;
extern int		vid_surfcachesize;
extern void		( *VID_FlipScreen )();
#endif //VIDH

extern "C" int		VID_Init( unsigned short* palette );
extern "C" float	GetXMouseAspectRatioAdjustment();
extern "C" float	GetYMouseAspectRatioAdjustment();
