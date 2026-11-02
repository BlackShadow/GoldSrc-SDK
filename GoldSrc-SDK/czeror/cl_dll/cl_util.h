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
// cl_util.h
//

#ifndef CL_UTIL_H
#define CL_UTIL_H

#include "cvardef.h"
#include <stdio.h> // for safe_sprintf()
#include <stdarg.h>  // "
#include <string.h> // for strncpy()

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define HOOK_MESSAGE( x ) gEngfuncs.pfnHookUserMsg( #x, __MsgFunc_##x )
#define DECLARE_MESSAGE( y, x ) int __MsgFunc_##x( const char* name, int size, void* buffer ) \
	{ return gHUD.y.MsgFunc_##x( name, size, buffer ); }
#define HOOK_COMMAND( x, y ) gEngfuncs.pfnAddCommand( x, __CmdFunc_##y )
#define DECLARE_COMMAND( y, x ) void __CmdFunc_##x() { gHUD.y.UserCmd_##x(); }

inline float CVAR_GET_FLOAT( const char* name ) { return gEngfuncs.pfnGetCvarFloat( (char*)name ); }
inline char* CVAR_GET_STRING( const char* name ) { return gEngfuncs.pfnGetCvarString( (char*)name ); }
inline cvar_t* CVAR_CREATE( const char* name, const char* value, int flags )
{
	return gEngfuncs.pfnRegisterVariable( (char*)name, (char*)value, flags );
}

#define SPR_Load (*gEngfuncs.pfnSPR_Load)
#define SPR_Set (*gEngfuncs.pfnSPR_Set)
#define SPR_Frames (*gEngfuncs.pfnSPR_Frames)
#define SPR_GetList (*gEngfuncs.pfnSPR_GetList)

// SPR_Draw  draws a the current sprite as solid
#define SPR_Draw (*gEngfuncs.pfnSPR_Draw)
// SPR_DrawHoles  draws the current sprites,  with color index255 not drawn (transparent)
#define SPR_DrawHoles (*gEngfuncs.pfnSPR_DrawHoles)
// SPR_DrawAdditive  adds the sprites RGB values to the background  (additive transulency)
#define SPR_DrawAdditive (*gEngfuncs.pfnSPR_DrawAdditive)

// SPR_EnableScissor  sets a clipping rect for HUD sprites.  (0,0) is the top-left hand corner of the screen.
#define SPR_EnableScissor (*gEngfuncs.pfnSPR_EnableScissor)
// SPR_DisableScissor  disables the clipping rect
#define SPR_DisableScissor (*gEngfuncs.pfnSPR_DisableScissor)
//
#define FillRGBA (*gEngfuncs.pfnFillRGBA)


// ScreenHeight returns the height of the screen, in pixels
#define ScreenHeight (gHUD.m_scrinfo.iHeight)
// ScreenWidth returns the width of the screen, in pixels
#define ScreenWidth (gHUD.m_scrinfo.iWidth)
#define BASE_XRES 640.f

// use this to project world coordinates to screen coordinates
#define XPROJECT( x ) ( ( 1.0f + ( x ) ) * ScreenWidth * 0.5f )
#define YPROJECT( y ) ( ( 1.0f - ( y ) ) * ScreenHeight * 0.5f )
#define XRES( x ) ( ( x ) * ( (float)ScreenWidth / 640 ) )
#define YRES( y ) ( ( y ) * ( (float)ScreenHeight / 480 ) )
#define GetScreenInfo (*gEngfuncs.pfnGetScreenInfo)
#define ServerCmd (*gEngfuncs.pfnServerCmd)
#define EngineClientCmd (*gEngfuncs.pfnClientCmd)
#define SetCrosshair (*gEngfuncs.pfnSetCrosshair)
#define AngleVectors (*gEngfuncs.pfnAngleVectors)

inline int SPR_Height( HSPRITE sprite, int frame ) { return gEngfuncs.pfnSPR_Height( sprite, frame ); }
inline int SPR_Width( HSPRITE sprite, int frame ) { return gEngfuncs.pfnSPR_Width( sprite, frame ); }
inline client_textmessage_t* TextMessageGet( const char* name ) { return gEngfuncs.pfnTextMessageGet( name ); }
inline int TextMessageDrawChar( int x, int y, int number, int r, int g, int b )
{
	return gEngfuncs.pfnDrawCharacter( x, y, number, r, g, b );
}

inline int DrawConsoleString( int x, int y, const char* text )
{
	return gEngfuncs.pfnDrawConsoleString( x, y, (char*)text );
}

inline void GetConsoleStringSize( const char* text, int* width, int* height )
{
	gEngfuncs.pfnDrawConsoleStringLen( text, width, height );
}

inline int ConsoleStringLen( const char* text )
{
	int width, height;
	GetConsoleStringSize( text, &width, &height );
	return width;
}

#define fabs(x)	   ((x) > 0 ? (x) : 0 - (x))

inline void ConsolePrint( const char* text ) { gEngfuncs.pfnConsolePrint( text ); }
inline void CenterPrint( const char* text ) { gEngfuncs.pfnCenterPrint( text ); }

inline char* safe_strcpy( char* dest, const char* source, int size )
{
	if ( size <= 0 )
		return NULL;
	strncpy( dest, source, size );
	dest[size - 1] = '\0';
	return dest;
}

inline int safe_sprintf( char* dest, int size, const char* format, ... )
{
	if ( size <= 0 )
		return -1;
	va_list args;
	va_start( args, format );
	_vsnprintf( dest, size, format, args );
	va_end( args );
	dest[size - 1] = '\0';
	return 0;
}

inline void PlaySound( char* name, float volume ) { gEngfuncs.pfnPlaySoundByName( name, volume ); }
inline void PlaySound( int sound, float volume ) { gEngfuncs.pfnPlaySoundByIndex( sound, volume ); }

#define DotProduct( x, y ) ( ( x )[0] * ( y )[0] + ( x )[1] * ( y )[1] + ( x )[2] * ( y )[2] )
#define VectorSubtract( a, b, c ) { ( c )[0] = ( a )[0] - ( b )[0]; ( c )[1] = ( a )[1] - ( b )[1]; ( c )[2] = ( a )[2] - ( b )[2]; }
#define VectorAdd( a, b, c ) { ( c )[0] = ( a )[0] + ( b )[0]; ( c )[1] = ( a )[1] + ( b )[1]; ( c )[2] = ( a )[2] + ( b )[2]; }
#define VectorCopy( a, b ) { ( b )[0] = ( a )[0]; ( b )[1] = ( a )[1]; ( b )[2] = ( a )[2]; }
inline void VectorClear( float* vector ) { vector[0] = vector[1] = vector[2] = 0; }

void	ScaleColors( int& r, int& g, int& b, int a );
float	Length( const float* vector );
void	VectorMA( const float* a, float scale, const float* b, float* out );
void	VectorScale( const float* in, float scale, float* out );
float	VectorNormalize( float* vector );
void	VectorInverse( float* vector );
void	VectorAngles( const float* forward, float* angles );
HSPRITE LoadSprite( const char* name );
extern "C" Vector vec3_origin;

inline void UnpackRGB( int& r, int& g, int& b, unsigned long color )
{
	r = ( color & 0xFF0000 ) >> 16;
	g = ( color & 0xFF00 ) >> 8;
	b = color & 0xFF;
}

#endif
