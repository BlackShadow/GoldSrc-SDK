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
#include "Exports.h"
#include "../public/utlvector.h"

int g_demosniper;
int g_demosniperdamage;
float g_demosniperorg[3];
float g_demosniperangles[3];
float g_demozoom;

void Demo_WriteBuffer( int type, int size, unsigned char* buffer )
{
	if ( size < 0 || size > 32 * 1024 - (int)sizeof( type ) || ( size && !buffer ) )
		return;
	CUtlVector<unsigned char> message;
	message.AddMultipleToTail( size + sizeof( type ) );
	memcpy( message.Base(), &type, sizeof( type ) );
	if ( size )
		memcpy( message.Base() + sizeof( type ), buffer, size );
	gEngfuncs.pDemoAPI->WriteBuffer( size + sizeof( type ), message.Base() );
}

void CL_DLLEXPORT Demo_ReadBuffer( int size, unsigned char* buffer )
{
	g_pcldstAddrs->pReadDemoBuffer( &size, &buffer );
	int type = *(int*)buffer;
	buffer += sizeof( type );
	switch ( type )
	{
	case TYPE_SNIPERDOT:
		g_demosniper = *(int*)buffer;
		buffer += sizeof( g_demosniper );
		if ( g_demosniper )
		{
			g_demosniperdamage = *(int*)buffer;
			buffer += sizeof( g_demosniperdamage );
			memcpy( g_demosniperangles, buffer, sizeof( g_demosniperangles ) );
			buffer += sizeof( g_demosniperangles );
			memcpy( g_demosniperorg, buffer, sizeof( g_demosniperorg ) );
		}
		break;
	case TYPE_ZOOM:
		g_demozoom = *(float*)buffer;
		break;
	default:
		gEngfuncs.Con_DPrintf( "Unknown demo buffer type, skipping.\n" );
		break;
	}
}
