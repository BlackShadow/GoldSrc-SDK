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

#include <stdio.h>
#include <math.h>
#include "hud.h"
#include "cl_util.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846	// matches value in gcc v2 math.h
#endif

vec3_t vec3_origin( 0, 0, 0 );

float Length( const float* vector )
{
	float length = 0;
	for ( int i = 0; i < 3; i++ )
		length += vector[i] * vector[i];
	return sqrt( length );
}

void VectorAngles( const float* forward, float* angles )
{
	float yaw, pitch;
	if ( forward[1] == 0 && forward[0] == 0 )
	{
		yaw = 0;
		pitch = forward[2] > 0 ? 90 : 270;
	}
	else
	{
		yaw = atan2( forward[1], forward[0] ) * 180 / M_PI;
		if ( yaw < 0 )
			yaw += 360;
		float horizontal = sqrt( forward[0] * forward[0] + forward[1] * forward[1] );
		pitch = atan2( forward[2], horizontal ) * 180 / M_PI;
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
	length = sqrt( length );		// FIXME
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

void VectorScale( const float* input, float scale, float* output )
{
	output[0] = input[0] * scale;
	output[1] = input[1] * scale;
	output[2] = input[2] * scale;
}

void VectorMA( const float* input, float scale, const float* direction, float* output )
{
	output[0] = input[0] + scale * direction[0];
	output[1] = input[1] + scale * direction[1];
	output[2] = input[2] + scale * direction[2];
}

HSPRITE LoadSprite( const char* name )
{
	int resolution = ScreenWidth >= 640 ? 640 : 320;
	char path[256];
	sprintf( path, name, resolution );
	return SPR_Load( path );
}
