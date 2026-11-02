#pragma once

inline float RoundToFloat( double value )
{
	volatile float rounded = (float)value;
	return rounded;
}

void	__cdecl Vector_ToSphericalDegrees( const float in[3], float out[3] );
void	__cdecl Vector_ToSphericalRadians( const float in[3], float out[3] );
