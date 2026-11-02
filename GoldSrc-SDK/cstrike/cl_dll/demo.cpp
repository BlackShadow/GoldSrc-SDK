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
#include <string.h>

int g_demosniper = 0;
int g_demosniperdamage = 0;
float g_demosniperorg[3];
float g_demosniperangles[3];
float g_demozoom;

#ifdef _MSC_VER
#pragma warning( suppress : 6262 )
#endif
void Demo_WriteBuffer( int type, int size, unsigned char* buffer )
{
	unsigned char data[32 * 1024];
	if ( size < 0 || size > (int)( sizeof( data ) - sizeof( int ) ) || ( size && !buffer ) )
		return;
	*(int*)data = type;
	if ( size )
		memcpy( data + sizeof( int ), buffer, size );
	gEngfuncs.pDemoAPI->WriteBuffer( size + sizeof( int ), data );
}

extern "C" void EXPORT Demo_ReadBuffer( int, unsigned char* buffer )
{
	switch ( *(int*)buffer )
	{
	case TYPE_SNIPERDOT:
		g_demosniper = *(int*)( buffer + 4 );
		if ( g_demosniper )
		{
			g_demosniperdamage = *(int*)( buffer + 8 );
			memcpy( g_demosniperangles, buffer + 12, sizeof( g_demosniperangles ) );
			memcpy( g_demosniperorg, buffer + 24, sizeof( g_demosniperorg ) );
		}
		break;
	case TYPE_ZOOM:
		g_demozoom = *(float*)( buffer + 4 );
		break;
	default:
		gEngfuncs.Con_DPrintf( "Unknown demo buffer type, skipping.\n" );
		break;
	}
}
