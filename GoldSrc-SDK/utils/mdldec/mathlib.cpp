// mathlib.cpp -- skeletal transforms and quaternion conversion

#include "mdldec.h"

float VectorNormalize( vec3_t vector )
{
	int i;
	double length;

	// Leftover debugging test from the SDK math library; it has no effect.
	if ( fabs( vector[1] - 0.000215956 ) < 0.0001 )
		i = 1;

	length = 0.0;
	for ( i = 0; i < 3; i++ )
		length = (double)vector[i] * vector[i] + length;

	length = sqrt( length );
	if ( length == 0.0 )
		return 0.0f;

	for ( i = 0; i < 3; i++ )
		vector[i] = (float)( vector[i] / length );

	return (float)length;
}

void R_ConcatTransforms( const matrix3x4_t first, const matrix3x4_t second, matrix3x4_t out )
{
	for ( int i = 0; i < 3; i++ )
	{
		for ( int j = 0; j < 3; j++ )
			out[i][j] = (float)( (double)first[i][0] * second[0][j]
					+ (double)first[i][1] * second[1][j] + (double)first[i][2] * second[2][j] );

		out[i][3] = (float)( (double)first[i][0] * second[0][3]
				+ (double)first[i][1] * second[1][3] + (double)first[i][2] * second[2][3] + first[i][3] );
	}
}

void VectorRotate( const vec3_t vector, const matrix3x4_t matrix, vec3_t out )
{
	for ( int i = 0; i < 3; i++ )
		out[i] = (float)( (double)vector[0] * matrix[i][0]
				+ (double)vector[1] * matrix[i][1] + (double)vector[2] * matrix[i][2] );
}

void VectorTransform( const vec3_t vector, const matrix3x4_t matrix, vec3_t out )
{
	for ( int i = 0; i < 3; i++ )
		out[i] = (float)( (double)vector[0] * matrix[i][0]
				+ (double)vector[1] * matrix[i][1] + (double)vector[2] * matrix[i][2] + matrix[i][3] );
}

void AngleQuaternion( const vec3_t angles, vec4_t quaternion )
{
	float angle = (float)( angles[2] * 0.5 );
	float sy = (float)sin( (double)angle );
	float cy = (float)cos( (double)angle );
	angle = (float)( angles[1] * 0.5 );
	float sp = (float)sin( (double)angle );
	float cp = (float)cos( (double)angle );
	angle = (float)( angles[0] * 0.5 );
	float sr = (float)sin( (double)angle );
	float cr = (float)cos( (double)angle );

	quaternion[0] = (float)( (double)sr * cp * cy - (double)cr * sp * sy );
	quaternion[1] = (float)( (double)cr * sp * cy + (double)sr * cp * sy );
	quaternion[2] = (float)( (double)cr * cp * sy - (double)sr * sp * cy );
	quaternion[3] = (float)( (double)cr * cp * cy + (double)sr * sp * sy );
}

void QuaternionMatrix( const vec4_t quaternion, matrix3x4_t matrix )
{
	matrix[0][0] = (float)( 1.0 - ( (double)quaternion[1] + quaternion[1] ) * quaternion[1]
			- ( (double)quaternion[2] + quaternion[2] ) * quaternion[2] );
	matrix[1][0] = (float)( ( (double)quaternion[0] + quaternion[0] ) * quaternion[1]
			+ ( (double)quaternion[3] + quaternion[3] ) * quaternion[2] );
	matrix[2][0] = (float)( ( (double)quaternion[0] + quaternion[0] ) * quaternion[2]
			- ( (double)quaternion[3] + quaternion[3] ) * quaternion[1] );
	matrix[0][1] = (float)( ( (double)quaternion[0] + quaternion[0] ) * quaternion[1]
			- ( (double)quaternion[3] + quaternion[3] ) * quaternion[2] );
	matrix[1][1] = (float)( 1.0 - ( (double)quaternion[0] + quaternion[0] ) * quaternion[0]
			- ( (double)quaternion[2] + quaternion[2] ) * quaternion[2] );
	matrix[2][1] = (float)( ( (double)quaternion[1] + quaternion[1] ) * quaternion[2]
			+ ( (double)quaternion[3] + quaternion[3] ) * quaternion[0] );
	matrix[0][2] = (float)( ( (double)quaternion[0] + quaternion[0] ) * quaternion[2]
			+ ( (double)quaternion[3] + quaternion[3] ) * quaternion[1] );
	matrix[1][2] = (float)( ( (double)quaternion[1] + quaternion[1] ) * quaternion[2]
			- ( (double)quaternion[3] + quaternion[3] ) * quaternion[0] );
	matrix[2][2] = (float)( 1.0 - ( (double)quaternion[0] + quaternion[0] ) * quaternion[0]
			- ( (double)quaternion[1] + quaternion[1] ) * quaternion[1] );
}
