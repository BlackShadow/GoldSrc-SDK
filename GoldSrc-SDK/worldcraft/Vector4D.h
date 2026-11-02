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

#ifndef HAMMER_VECTOR4D_H
#define HAMMER_VECTOR4D_H
#pragma once
#include "vector.h"

class Vector4D
{
public:
	#pragma warning( suppress : 26495 )
	Vector4D() {}
	Vector4D( float X, float Y, float Z, float W ) : x( X ), y( Y ), z( Z ), w( W ) {}
	void Init( float X = 0, float Y = 0, float Z = 0, float W = 0 )
	{
		x = X;
		y = Y;
		z = Z;
		w = W;
	}
	float* Base() { return &x; }
	const float* Base() const { return &x; }
	float& operator[]( int i ) { return Base()[i]; }
	float operator[]( int i ) const { return Base()[i]; }
	Vector& AsVector3D() { return *(Vector*)&x; }
	const Vector& AsVector3D() const { return *(const Vector*)&x; }
	float x, y, z, w;
};
#endif
