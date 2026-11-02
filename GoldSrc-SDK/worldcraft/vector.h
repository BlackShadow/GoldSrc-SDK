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
#ifndef HAMMER_VECTOR_H
#define HAMMER_VECTOR_H
#pragma once

#include <math.h>

class Vector2D
{
public:
	#pragma warning( suppress : 26495 )
	Vector2D() {}
	Vector2D( float X, float Y ) : x( X ), y( Y ) {}
	void Init( float X = 0, float Y = 0 )
	{
		x = X;
		y = Y;
	}
	float* Base() { return &x; }
	const float* Base() const { return &x; }
	float& operator[]( int i ) { return Base()[i]; }
	float operator[]( int i ) const { return Base()[i]; }
	Vector2D operator+( const Vector2D& v ) const { return Vector2D( x + v.x, y + v.y ); }
	Vector2D operator-( const Vector2D& v ) const { return Vector2D( x - v.x, y - v.y ); }
	Vector2D operator*( float scale ) const { return Vector2D( x * scale, y * scale ); }
	Vector2D operator/( float scale ) const { return Vector2D( x / scale, y / scale ); }
	Vector2D& operator+=( const Vector2D& v )
	{
		x += v.x;
		y += v.y;
		return *this;
	}
	Vector2D& operator-=( const Vector2D& v )
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}
	Vector2D& operator*=( float scale )
	{
		x *= scale;
		y *= scale;
		return *this;
	}
	float LengthSqr() const { return x * x + y * y; }
	float Length() const { return sqrtf( LengthSqr() ); }
	float x, y;
};

class Vector						// same data-layout as engine's vec3_t,
{								//		which is a vec_t[3]
public:
	#pragma warning( suppress : 26495 )
	Vector() {}
	Vector( float X, float Y, float Z ) : x( X ), y( Y ), z( Z ) {}
	Vector( const float* v ) : x( v[0] ), y( v[1] ), z( v[2] ) {}
	void Init( float X = 0, float Y = 0, float Z = 0 )
	{
		x = X;
		y = Y;
		z = Z;
	}
	float* Base() { return &x; } // Vectors will now automatically convert to float * when needed
	const float* Base() const { return &x; }
	operator float*() { return Base(); }
	operator const float*() const { return Base(); }
	float& operator[]( int i ) { return Base()[i]; }
	float operator[]( int i ) const { return Base()[i]; }
	Vector operator-() const { return Vector( -x, -y, -z ); }
	Vector operator+( const Vector& v ) const { return Vector( x + v.x, y + v.y, z + v.z ); }
	Vector operator-( const Vector& v ) const { return Vector( x - v.x, y - v.y, z - v.z ); }
	Vector operator*( float scale ) const { return Vector( x * scale, y * scale, z * scale ); }
	Vector operator*( const Vector& v ) const { return Vector( x * v.x, y * v.y, z * v.z ); }
	Vector operator/( float scale ) const { return Vector( x / scale, y / scale, z / scale ); }
	Vector& operator+=( const Vector& v )
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}
	Vector& operator-=( const Vector& v )
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}
	Vector& operator*=( float scale )
	{
		x *= scale;
		y *= scale;
		z *= scale;
		return *this;
	}
	Vector& operator/=( float scale )
	{
		x /= scale;
		y /= scale;
		z /= scale;
		return *this;
	}
	bool operator==( const Vector& v ) const { return x == v.x && y == v.y && z == v.z; }
	bool operator!=( const Vector& v ) const { return !( *this == v ); }
	float LengthSqr() const { return x * x + y * y + z * z; }
	float Length() const { return sqrtf( LengthSqr() ); }
	float Length2D() const { return sqrtf( x * x + y * y ); }
	float DistToSqr( const Vector& v ) const { return ( *this - v ).LengthSqr(); }
	float DistTo( const Vector& v ) const { return ( *this - v ).Length(); }
	float Dot( const Vector& v ) const { return x * v.x + y * v.y + z * v.z; }
	Vector Cross( const Vector& v ) const { return Vector( y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x ); }
	float NormalizeInPlace()
	{
		float length = Length();
		if ( length != 0 )
			*this /= length;
		return length;
	}
	void Negate()
	{
		x = -x;
		y = -y;
		z = -z;
	}
	void CopyToArray( float* v ) const
	{
		v[0] = x;
		v[1] = y;
		v[2] = z;
	}
	float x, y, z;
};

inline Vector operator*( float scale, const Vector& v )
{
	return v * scale;
}
inline Vector2D operator*( float scale, const Vector2D& v )
{
	return v * scale;
}
typedef Vector QAngle;
typedef float  matrix3x4_t[3][4];

#endif
