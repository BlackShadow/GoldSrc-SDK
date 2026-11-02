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

#include "const.h"

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4]; // x,y,z,w
typedef int fixed8_t;
typedef int fixed16_t;

#ifndef M_PI
#define M_PI 3.14159265358979323846 // matches value in gcc v2 math.h
#endif

// up / down
#define PITCH 0
// left / right
#define YAW 1
// fall over
#define ROLL 2

struct mplane_s;

extern vec3_t vec3_origin;
extern int nanmask;

#ifndef DotProduct
#define DotProduct( x, y ) ( ( x )[0] * ( y )[0] + ( x )[1] * ( y )[1] + ( x )[2] * ( y )[2] )
#endif

#define VectorClear( a ) { ( a )[0] = 0.0f; ( a )[1] = 0.0f; ( a )[2] = 0.0f; }
#define VectorSubtract( a, b, c ) { ( c )[0] = ( a )[0] - ( b )[0]; ( c )[1] = ( a )[1] - ( b )[1]; ( c )[2] = ( a )[2] - ( b )[2]; }
#define VectorAdd( a, b, c ) { ( c )[0] = ( a )[0] + ( b )[0]; ( c )[1] = ( a )[1] + ( b )[1]; ( c )[2] = ( a )[2] + ( b )[2]; }
#define VectorCopy( a, b ) { ( b )[0] = ( a )[0]; ( b )[1] = ( a )[1]; ( b )[2] = ( a )[2]; }

vec_t				_DotProduct( vec_t* v1, vec_t* v2 );
void				_VectorSubtract( vec_t* veca, vec_t* vecb, vec_t* out );
void				_VectorAdd( vec_t* veca, vec_t* vecb, vec_t* out );
void				_VectorCopy( vec_t* in, vec_t* out );

qboolean			VectorCompare( const vec_t* first, const vec_t* second );
void				VectorMA( const vec_t* start, float scale, const vec_t* direction, vec_t* out );
void				VectorScale( const vec_t* in, float scale, vec_t* out );
void				VectorInverse( vec_t* v );
void				CrossProduct( const vec_t* first, const vec_t* second, vec_t* out );
float				Length( const vec_t* vector );
float				VectorNormalize( vec_t* vector ); // returns vector length
void				VectorMatrix( vec_t* forward, vec_t* right, vec_t* up );
void				VectorTransform( const vec_t* vector, float matrix[3][4], vec_t* output );

float				anglemod( float angle );
void				NormalizeAngles( float* angles );
void				InterpolateAngles( float* start, float* end, float* output, float fraction );
void				VectorAngles( const vec_t* forward, vec_t* angles );
void				AngleVectors( const vec_t* angles, vec_t* forward, vec_t* right, vec_t* up );
void				AngleVectorsTranspose( const vec_t* angles, vec_t* forward, vec_t* right, vec_t* up );

void				AngleMatrix( const float* angles, float matrix[3][4] );
void				AngleIMatrix( const float* angles, float matrix[3][4] );
int					InvertMatrix( const float* matrix, float* inverse );
void				R_ConcatRotations( float first[3][3], float second[3][3], float output[3][3] );
void				R_ConcatTransforms( float first[3][4], float second[3][4], float output[3][4] );
void				QuaternionMatrix( const float* quaternion, float matrix[3][4] );
void				QuaternionSlerp( const float* p, float* q, float t, float* qt );

int					BoxOnPlaneSide( vec_t* mins, vec_t* maxs, mplane_s* plane );
[[noreturn]] void	BOPS_Error();

int					Q_log2( int val );
void				FloorDivMod( double numer, double denom, int* quotient, int* rem );
int					GreatestCommonDivisor( int i1, int i2 );
fixed16_t			Invert24To16( fixed16_t val );
