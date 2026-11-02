/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// CBaseSpectator

// YWB:  UNDONE

// Spectator functions
// 
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "spectator.h"

/*
===========
Spawn

  Called when spectator is initialized:
  UNDONE:  Is this actually being called because spectators are not allocated in normal fashion?
============
*/
void CBaseSpectator::Spawn()
{
	pev->flags = FL_SPECTATOR;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NOCLIP;
	m_pGoalEnt = NULL;
}

void CBaseSpectator::SpectatorConnect()
{
	pev->flags = FL_SPECTATOR;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NOCLIP;
	m_pGoalEnt = NULL;
}

/*
===========
SpectatorDisconnect

called when a spectator disconnects from a server
============
*/
void CBaseSpectator::SpectatorDisconnect()
{
}

void CBaseSpectator::SpectatorImpulseCommand()
{
	static edict_t* pGoal;
	if ( pev->impulse == 1 )
	{
		CBaseEntity* previous = pGoal ? CBaseEntity::Instance( pGoal ) : NULL;
		CBaseEntity* current = previous;
		for ( ;; )
		{
			current = UTIL_FindEntityByClassname( current, "info_player_deathmatch" );
			if ( current == previous )
			{
				ALERT( at_console, "Could not find a spawn spot.\n" );
				break;
			}
			if ( current )
			{
				pGoal = current->edict();
				UTIL_SetOrigin( this, pGoal->v.origin );
				pev->angles = pGoal->v.angles;
				pev->fixangle = FALSE;
				break;
			}
		}
	}
	else
	{
		ALERT( at_console, "Unknown spectator impulse\n" );
	}
	pev->impulse = 0;
}

/*
================
SpectatorThink

Called every frame after physics are run
================
*/
void CBaseSpectator::SpectatorThink()
{
	if ( !( pev->flags & FL_SPECTATOR ) )
		pev->flags = FL_SPECTATOR;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NOCLIP;
	if ( pev->impulse )
		SpectatorImpulseCommand();
}
