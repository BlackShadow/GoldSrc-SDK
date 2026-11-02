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
#include "hud.h"
#include "cl_util.h"
#include "demo.h"
#include "demo_api.h"
#include <memory.h>

int g_demosniper = 0;
int g_demosniperdamage = 0;
float g_demosniperorg[3];
float g_demosniperangles[3];
float g_demozoom;

// FIXME:  There should be buffer helper functions to avoid all of the *(int *)& crap.

/*
=====================
Demo_WriteBuffer

Write some data to the demo stream
=====================
*/
void Demo_WriteBuffer( int type, int size, unsigned char *buffer )
{
	int pos = 0;
	if ( size < 0 || size > 32 * 1024 - (int)sizeof( int ) || (size && !buffer) )
		return;
	unsigned char* buf = (unsigned char*)malloc( size + sizeof( int ) );
	if ( !buf )
		return;
	*( int * )&buf[pos] = type;
	pos+=sizeof( int );

	if ( size )
		memcpy( &buf[pos], buffer, size );

	// Write full buffer out
	gEngfuncs.pDemoAPI->WriteBuffer( size + sizeof( int ), buf );
	free( buf );
}

/*
=====================
Demo_ReadBuffer

Engine wants us to parse some data from the demo stream
=====================
*/
void CL_DLLEXPORT Demo_ReadBuffer( int size, unsigned char *buffer )
{
//	RecClReadDemoBuffer(size, buffer);

	if ( !buffer || size < (int)sizeof( int ) )
		return;

	int type;
	int i = 0;

	type = *( int * )buffer;
	i += sizeof( int );
	switch ( type )
	{
	case TYPE_SNIPERDOT:
		if ( size < i + (int)sizeof( int ) )
			return;
		if ( *(int *)&buffer[i] && size < i + (int)( sizeof( int ) * 2
			+ sizeof( g_demosniperangles ) + sizeof( g_demosniperorg ) ) )
			return;
		g_demosniper = *(int * )&buffer[ i ];
		i += sizeof( int );

		if ( g_demosniper )
		{
			g_demosniperdamage = *( int * )&buffer[ i ];
			i += sizeof( int );

			g_demosniperangles[ 0 ] = *(float *)&buffer[i];
			i += sizeof( float );
			g_demosniperangles[ 1 ] = *(float *)&buffer[i];
			i += sizeof( float );
			g_demosniperangles[ 2 ] = *(float *)&buffer[i];
			i += sizeof( float );
			g_demosniperorg[ 0 ] = *(float *)&buffer[i];
			i += sizeof( float );
			g_demosniperorg[ 1 ] = *(float *)&buffer[i];
			i += sizeof( float );
			g_demosniperorg[ 2 ] = *(float *)&buffer[i];
			i += sizeof( float );
		}
		break;
	case TYPE_ZOOM:
		if ( size < i + (int)sizeof( g_demozoom ) )
			return;
		g_demozoom = *(float * )&buffer[ i ];
		i += sizeof( float );
		break;
	default:
		gEngfuncs.Con_DPrintf( "Unknown demo buffer type, skipping.\n" );
		break;
	}
}
