/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// util.cpp
//
// implementation of class-less helper functions
//

#include "hud.h"
#include "cl_util.h"
#include <math.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846	// matches value in gcc v2 math.h
#endif

extern "C"
{
	Vector vec3_origin( 0, 0, 0 );
}

float Length( const float* vector )
{
	float length = 0;
	for ( int i = 0; i < 3; ++i )
		length += vector[i] * vector[i];
	return (float)sqrt( (double)length );
}

void VectorAngles( const float* forward, float* angles )
{
	float yaw, pitch;
	if ( forward[1] == 0 && forward[0] == 0 )
	{
		yaw = 0;
		pitch = forward[2] > 0 ? 90.0f : 270.0f;
	}
	else
	{
		yaw = (float)( atan2( (double)forward[1], (double)forward[0] ) * 180 / M_PI );
		if ( yaw < 0 )
			yaw += 360;
		float length = (float)sqrt( (double)( forward[0] * forward[0] + forward[1] * forward[1] ) );
		pitch = (float)( atan2( (double)forward[2], (double)length ) * 180 / M_PI );
		if ( pitch < 0 )
			pitch += 360;
	}
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

float VectorNormalize( float* vector )
{
	float length = vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2];
	length = (float)sqrt( (double)length );
	if ( length )
	{
		float inverse = 1 / length;
		vector[0] *= inverse;
		vector[1] *= inverse;
		vector[2] *= inverse;
	}
	return length;
}

void VectorInverse( float* vector )
{
	vector[0] = -vector[0];
	vector[1] = -vector[1];
	vector[2] = -vector[2];
}

void VectorScale( const float* in, float scale, float* out )
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

void VectorMA( const float* a, float scale, const float* b, float* out )
{
	out[0] = a[0] + scale * b[0];
	out[1] = a[1] + scale * b[1];
	out[2] = a[2] + scale * b[2];
}

HSPRITE LoadSprite( const char* name )
{
	int resolution = ScreenWidth >= 640 ? 640 : 320;
	char path[256];
	sprintf( path, name, resolution );
	return SPR_Load( path );
}
