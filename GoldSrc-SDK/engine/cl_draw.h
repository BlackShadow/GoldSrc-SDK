#pragma once

#include "engine_apiproxy.h"

HSPRITE		SPR_Load( const char* name );
int			SPR_Frames( HSPRITE sprite );
int			SPR_Width( HSPRITE sprite, int frame );
int			SPR_Height( HSPRITE sprite, int frame );
void		SPR_Set( HSPRITE sprite, int red, int green, int blue );
void		SPR_Draw( int frame, int x, int y, const wrect_t* rect );
void		SPR_DrawHoles( int frame, int x, int y, const wrect_t* rect );
void		SPR_DrawAdditive( int frame, int x, int y, const wrect_t* rect );
void		SPR_DrawGeneric( int frame, int x, int y, const wrect_t* rect, int source, int destination, int width,
				int height );
void		SPR_EnableScissor( int x, int y, int width, int height );
void		SPR_DisableScissor();

void		SetCrosshair( HSPRITE sprite, wrect_t rect, int red, int green, int blue );
void		SetFilterMode( int mode );
void		SetFilterColor( float red, float green, float blue );
void		SetFilterBrightness( float brightness );

void		UnpackPalette( unsigned short* pDest, unsigned short* pSource, int r, int g, int b );
