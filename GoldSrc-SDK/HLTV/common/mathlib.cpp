#include "HLTV/common/mathlib.h"

#include "HLTV/common/types.h"
#include "engine/model.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

vec3_t vec3_origin = { 0.0f, 0.0f, 0.0f };

float TriangleDiameter( const vec_t* v1, vec_t* v2, vec_t* v3 )
{
	vec3_t e1;
	vec3_t e2;
	vec3_t e3;
	float l1;
	float l2;
	float l3;
	int i;

	for ( i = 0; i < 3; i++ )
	{
		e1[i] = v2[i] - v1[i];
		e2[i] = v3[i] - v2[i];
		e3[i] = v1[i] - v3[i];
	}

	l1 = 0.0f;
	for ( i = 0; i < 3; i++ )
		l1 += e1[i] * e1[i];

	l2 = 0.0f;
	for ( i = 0; i < 3; i++ )
		l2 += e2[i] * e2[i];

	l3 = 0.0f;
	for ( i = 0; i < 3; i++ )
		l3 += e3[i] * e3[i];

	if ( l1 > l2 && l1 > l3 )
		return (float)sqrt( l1 );
	if ( l2 > l1 && l2 > l3 )
		return (float)sqrt( l2 );
	return (float)sqrt( l3 );
}

float VectorNormalize( vec_t* v )
{
	float length;
	float ilength;

	length = (float)sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
	if ( length != 0 )
	{
		ilength = 1.0f / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}

float VectorDistance( const vec_t* v1, const vec_t* v2 )
{
	vec3_t t;

	t[0] = v1[0] - v2[0];
	t[1] = v1[1] - v2[1];
	t[2] = v1[2] - v2[2];
	return (float)sqrt( t[0] * t[0] + t[1] * t[1] + t[2] * t[2] );
}

void VectorAngles( const float* forward, float* angles )
{
	float tmp;
	float yaw;
	float pitch;

	if ( forward[1] == 0 && forward[0] == 0 )
	{
		yaw = 0;
		if ( forward[2] > 0 )
			pitch = 90.0f;
		else
			pitch = 270.0f;
	}
	else
	{
		yaw = (float)( atan2( (double)forward[1], (double)forward[0] ) * ( 180.0f / (float)M_PI ) );
		if ( yaw < 0 )
			yaw += 360.0f;

		tmp = (float)sqrt( (double)( forward[0] * forward[0] + forward[1] * forward[1] ) );
		pitch = (float)( atan2( (double)forward[2], (double)tmp ) * ( 180.0f / (float)M_PI ) );
		if ( pitch < 0 )
			pitch += 360.0f;
	}

	angles[PITCH] = pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

void VectorRAngles( const float* v, float* a )
{
	a[0] = (float)atan2( v[2], sqrt( (double)v[0] * v[0] + (double)v[1] * v[1] ) );
	a[1] = (float)atan2( v[1], v[0] );
	NormalizeRAngles( a );
}

int BoxOnPlaneSide( vec_t* emins, vec_t* emaxs, mplane_t* p )
{
	float dist1;
	float dist2;
	int sides;

// general case
	switch ( p->signbits )
	{
	case 0:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 1:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 2:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 3:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 4:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	default:
		dist1 = 0;
		dist2 = 0;
		break;
	}

	sides = 0;
	if ( dist1 >= p->dist )
		sides = 1;
	if ( dist2 < p->dist )
		sides |= 2;

	return sides;
}

/*
===================
AngleBetweenVectors

===================
*/
float AngleBetweenVectors( const vec_t* angle1, const vec_t* angle2 )
{
	float d1;
	float d2;

	d1 = angle1[0] * angle2[0] + angle1[1] * angle2[1] + angle1[2] * angle2[2];
	d2 = (float)( sqrt( (double)( angle1[0] * angle1[0] + angle1[1] * angle1[1] + angle1[2] * angle1[2] ) )
		* sqrt( (double)( angle2[0] * angle2[0] + angle2[1] * angle2[1] + angle2[2] * angle2[2] ) ) );
	if ( d2 == 0.0f )
		return 0.0f;

	return (float)( acos( (double)( d1 / d2 ) ) * ( 180.0f / (float)M_PI ) );
}

float RAngleBetweenVectors( const vec_t* angle1, const vec_t* angle2 )
{
	float d1;
	float d2;
	float d3;

	d1 = (float)sqrt( (double)angle1[0] * angle1[0]
		+ (double)angle1[1] * angle1[1] + (double)angle1[2] * angle1[2] );
	d2 = (float)sqrt( (double)angle2[0] * angle2[0]
		+ (double)angle2[1] * angle2[1] + (double)angle2[2] * angle2[2] );
	if ( d1 * d2 == 0 )
		return 0;

	d3 = ( angle1[0] * angle2[0] + angle1[1] * angle2[1] + angle1[2] * angle2[2] ) / ( d1 * d2 );
	if ( d3 > 1.0f )
		return 0;
	if ( d3 < -1.0f )
		return (float)M_PI;
	return acosf( d3 );
}

void NormalizeAngles( vec_t* angles )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		if ( angles[i] > 180.0f )
			angles[i] -= 360.0f;
		else if ( angles[i] < -180.0f )
			angles[i] += 360.0f;
	}
}

void NormalizeRAngles( vec_t* a )
{
	if ( a[0] > M_PI )
		a[0] = (float)( a[0] - M_PI * 2 );
	else if ( a[0] < -M_PI )
		a[0] = (float)( a[0] + M_PI * 2 );

	if ( a[1] > M_PI )
		a[1] = (float)( a[1] - M_PI * 2 );
	else if ( a[1] < -M_PI )
		a[1] = (float)( a[1] + M_PI * 2 );

	a[2] = 0;
}

void AngleVectors( const vec_t* angles, vec_t* forward, vec_t* right, vec_t* up )
{
	float angle;
	float sr;
	float sp;
	float sy;
	float cr;
	float cp;
	float cy;

	angle = angles[YAW] * (float)( M_PI / 180.0 );
	sy = (float)sin( (double)angle );
	cy = (float)cos( (double)angle );
	angle = angles[PITCH] * (float)( M_PI / 180.0 );
	sp = (float)sin( (double)angle );
	cp = (float)cos( (double)angle );
	angle = angles[ROLL] * (float)( M_PI / 180.0 );
	sr = (float)sin( (double)angle );
	cr = (float)cos( (double)angle );

	if ( forward )
	{
		forward[0] = cy * cp;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}

	if ( right )
	{
		right[0] = cy * ( sp * -sr ) + sy * cr;
		right[1] = ( sp * -sr ) * sy - cy * cr;
		right[2] = -sr * cp;
	}

	if ( up )
	{
		up[0] = cy * ( sp * cr ) + sy * sr;
		up[1] = sy * ( sp * cr ) - cy * sr;
		up[2] = cr * cp;
	}
}

int AngleLeftOfOther( const vec_t* v1, const vec_t* v2 )
{
	float a;
	float b;

	a = v1[1];
	b = v2[1];
	if ( a >= 180.0f )
		return a <= b || b < a - 180.0f;

	return b > a && a + 180.0f >= b;
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( const vec_t* mins, const vec_t* maxs )
{
	int i;
	vec3_t corner;

	for ( i = 0; i < 3; i++ )
	{
		if ( fabs( mins[i] ) > fabs( maxs[i] ) )
			corner[i] = (float)fabs( mins[i] );
		else
			corner[i] = (float)fabs( maxs[i] );
	}

	return (float)sqrt( corner[0] * corner[0] + corner[1] * corner[1] + corner[2] * corner[2] );
}

bool SolveLSE( vec_t* v0, vec_t* v1, vec_t* v2, vec_t* v3, float* x, float* y, float* z )
{
	float d;
	float cx;
	float cy;
	float cz;

	cz = v2[0] * v3[1] - v2[1] * v3[0];
	cy = v3[0] * v2[2] - v2[0] * v3[2];
	cx = v2[1] * v3[2] - v3[1] * v2[2];
	d = v1[2] * cz + v1[1] * cy + v1[0] * cx;
	if ( d != 0 )
	{
		if ( x )
			*x = ( cz * v0[2] + cy * v0[1] + cx * v0[0] ) / d;
		if ( y )
		{
			*y = ( ( v0[0] * v3[1] - v0[1] * v3[0] ) * v1[2]
				+ ( v3[0] * v0[2] - v0[0] * v3[2] ) * v1[1]
				+ ( v3[2] * v0[1] - v3[1] * v0[2] ) * v1[0] ) / d;
		}
		if ( z )
		{
			*z = ( ( v2[0] * v0[1] - v2[1] * v0[0] ) * v1[2]
				+ ( v0[0] * v2[2] - v2[0] * v0[2] ) * v1[1]
				+ ( v2[1] * v0[2] - v0[1] * v2[2] ) * v1[0] ) / d;
		}
		return true;
	}

	return false;
}

void NormalizePoints( vec_t* v0, vec_t* v1 )
{
	vec3_t a;

	a[0] = v1[0] - v0[0];
	a[1] = v1[1] - v0[1];
	a[2] = v1[2] - v0[2];
	if ( sqrt( (double)a[0] * a[0] + (double)a[1] * a[1] + (double)a[2] * a[2] ) != 0 )
	{
		VectorNormalize( a );
		v1[0] = a[0] + v0[0];
		v1[1] = a[1] + v0[1];
		v1[2] = a[2] + v0[2];
	}
}

void Normal2Plane( vec_t* v1, vec_t* v2, vec_t* v3 )
{
	if ( v1[0] != 0 )
	{
		v2[0] = -v1[1] / v1[0];
		v2[1] = 1.0f;
		v2[2] = 0.0f;
		v3[0] = -v1[2] / v1[0];
		v3[1] = 0.0f;
		v3[2] = 1.0f;
	}
	else if ( v1[1] == 0 )
	{
		v2[0] = 1.0f;
		v2[1] = 0.0f;
		v2[2] = -v1[0] / v1[2];
		v3[0] = 0.0f;
		v3[1] = 1.0f;
		v3[2] = -v1[1] / v1[2];
	}
	else
	{
		v2[0] = 1.0f;
		v2[1] = -v1[0] / v1[1];
		v2[2] = 0.0f;
		v3[0] = 0.0f;
		v3[1] = -v1[2] / v1[1];
		v3[2] = 1.0f;
	}
}
