#include "mathlib.h"

#include <math.h>

void VectorScale( const vec_t* in, float scale, vec_t* out )
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

void VectorAngles( const vec_t* forward, vec_t* angles )
{
	float yaw, pitch;

	if ( forward[1] == 0 && forward[0] == 0 )
	{
		yaw = 0;
		pitch = forward[2] > 0 ? 90.0f : 270.0f;
	}
	else
	{
		yaw = (float)( atan2( (double)forward[1], (double)forward[0] ) * 180.0 / M_PI );
		if ( yaw < 0 )
			yaw += 360;

		float horizontal = (float)sqrt( (double)( forward[0] * forward[0] + forward[1] * forward[1] ) );
		pitch = (float)( atan2( (double)forward[2], (double)horizontal ) * 180.0 / M_PI );
		if ( pitch < 0 )
			pitch += 360;
	}

	angles[PITCH] = pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

float VectorNormalize( vec_t* v )
{
	float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = (float)sqrt( (double)length );

	if ( length != 0 )
	{
		float inverseLength = 1.0f / length;
		v[0] *= inverseLength;
		v[1] *= inverseLength;
		v[2] *= inverseLength;
	}

	return length;
}

void VectorMA( const vec_t* start, float scale, const vec_t* direction, vec_t* out )
{
	out[0] = start[0] + scale * direction[0];
	out[1] = start[1] + scale * direction[1];
	out[2] = start[2] + scale * direction[2];
}
