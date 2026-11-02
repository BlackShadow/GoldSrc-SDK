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
// mathlib.h

#pragma once

#define PITCH	0
#define YAW		1
#define ROLL	2

struct mplane_s;

extern float	vec3_origin[3];

float		TriangleDiameter( const float* v1, float* v2, float* v3 );
float		VectorNormalize( float* v ); // returns vector length
float		VectorDistance( const float* v1, const float* v2 );

void		VectorAngles( const float* forward, float* angles );
void		VectorRAngles( const float* v, float* a );

int			BoxOnPlaneSide( float* emins, float* emaxs, mplane_s* p );

float		AngleBetweenVectors( const float* angle1, const float* angle2 );
float		RAngleBetweenVectors( const float* angle1, const float* angle2 );

void		NormalizeAngles( float* angles );
void		NormalizeRAngles( float* a );
void		AngleVectors( const float* angles, float* forward, float* right, float* up );
int			AngleLeftOfOther( const float* v1, const float* v2 );
float		RadiusFromBounds( const float* mins, const float* maxs );

bool		SolveLSE( float* v0, float* v1, float* v2, float* v3, float* x, float* y, float* z );
void		NormalizePoints( float* v0, float* v1 );
void		Normal2Plane( float* v1, float* v2, float* v3 );
