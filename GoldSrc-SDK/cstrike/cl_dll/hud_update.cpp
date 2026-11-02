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
//
//  hud_update.cpp
//

#include "hud.h"
#include "cl_util.h"

int CL_ButtonBits( int resetState );
void CL_ResetButtonBits( int bits );
float in_fov;
extern float v_idlescale;

int CHud::UpdateClientData( client_data_t* data, float )
{
	m_vecOrigin = data->origin;
	m_vecAngles = data->viewangles;
	m_iKeyBits = CL_ButtonBits( 0 );
	m_iWeaponBits = data->iWeaponBits;
	in_fov = data->fov;
	Think();
	data->fov = m_iFOV;
	v_idlescale = m_iConcussionEffect;
	CL_ResetButtonBits( m_iKeyBits );

	// return 1 if in anything in the client_data struct has been changed, 0 otherwise
	return 1;
}
