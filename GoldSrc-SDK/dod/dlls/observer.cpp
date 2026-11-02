//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Functionality for the observer chase camera
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "dod_shared.h"
#include "pm_shared.h"

extern cvar_t cvar_fadetoblack;
extern cvar_t cvar_forcechasecam;
extern cvar_t cvar_forcecamera;
extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern int gmsgSpectator;

enum
{
	TEAM_ALLIES = 1,
	TEAM_SPECTATOR = 3
};

bool IsTeamInfinite( int team )
{
	CDoDTeamPlay *Dodgamerules = (CDoDTeamPlay *)g_pGameRules;

	return Dodgamerules->IsTeamInfinite( team );
}

int GetForceCamera( void )
{
	if ( cvar_fadetoblack.value != 0 )
		return 2;

	int retVal = ( cvar_forcechasecam.value || cvar_forcecamera.value );

	return retVal;
}

bool CBasePlayer::Observer_PlayerValidTarget( CBaseEntity *pEnt, bool bSameTeam )
{
	if ( pEnt == NULL )
		return false;

	if ( pEnt == this )
		return false;

	if ( pEnt->has_disconnected )
		return false;

	if ( pEnt->pev->deadflag > DEAD_DYING )
		return false;

	// Don't spec observers or players who haven't picked a class yet
	if ( ( (CBasePlayer *)pEnt )->IsObserver() || ( pEnt->pev->effects & EF_NODRAW ) )
		return false;

	if ( IsTeamInfinite( pev->team ) && pev->team != TEAM_SPECTATOR && pEnt->pev->team != pev->team )
		return false;

	if ( bSameTeam && pEnt->pev->team != pev->team )
		return false;

	return true;
}

// Find the next client in the game for this player to spectate
void CBasePlayer::Observer_FindNextPlayer( bool bReverse )
{
	// MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
	//				only a subset of the players. e.g. Make it check the target's team.

	int		iStart;
	if ( m_hObserverTarget )
		iStart = ENTINDEX( m_hObserverTarget->edict() );
	else
		iStart = ENTINDEX( edict() );
	int	    iCurrent = iStart;
	m_hObserverTarget = NULL;
	int iDir = bReverse ? -1 : 1;

	bool bForceSameTeam = ( GetForceCamera() != 0 && pev->team != TEAM_SPECTATOR );

	do
	{
		iCurrent += iDir;

		// Loop through the clients
		if ( iCurrent > gpGlobals->maxClients )
			iCurrent = 1;
		else if ( iCurrent < 1 )
			iCurrent = gpGlobals->maxClients;

		CBaseEntity *pEnt = UTIL_PlayerByIndex( iCurrent );

		if ( Observer_PlayerValidTarget( pEnt, bForceSameTeam ) )
		{

			// MOD AUTHORS: Add checks on target here.

			m_hObserverTarget = pEnt;
			break;
		}

	} while ( iCurrent != iStart );

	// Did we find a target?
	if ( m_hObserverTarget )
	{
		// Move to the target
		UTIL_SetOrigin( this, m_hObserverTarget->pev->origin );

		// ALERT( at_console, "Now Tracking %s\n", STRING( m_hObserverTarget->pev->netname ) );

		// Store the target in pev so the physics DLL can get to it
		if ( pev->iuser1 != OBS_ROAMING )
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );

		if ( pev->iuser1 == OBS_IN_EYE )
			SendObserverTargetWeaponAnim();
	}
}

void CBasePlayer::SendObserverTargetWeaponAnim( void )
{
	int body = 0;

	if ( m_hObserverTarget )
	{
		int team = m_hObserverTarget->pev->team;
		int playerclass = m_hObserverTarget->pev->playerclass;

		if ( team == TEAM_ALLIES )
		{
			if ( playerclass >= PLAYERCLASS_BRIT_LIGHTINF && playerclass <= PLAYERCLASS_BRIT_MORTAR )
				body = 1;
			else
				body = 0;
		}
		else
		{
			body = 2;
		}

		MESSAGE_BEGIN( MSG_ONE, SVC_WEAPONANIM, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_BYTE( body );
		MESSAGE_END();
	}
}

void CBasePlayer::Observer_CheckProperties( void )
{
	// try to find a traget if we have no current one
	if ( pev->iuser1 == OBS_IN_EYE && m_hObserverTarget )
	{
		CBasePlayer *target = (CBasePlayer *)( UTIL_PlayerByIndex( ENTINDEX( m_hObserverTarget->edict() ) ) );

		if ( !target )
			return;

		int weapon = ( target->m_pActiveItem != NULL ) ? target->m_pActiveItem->m_iId : 0;
		// use fov of tracked client
		if ( m_iFOV != target->m_iFOV || m_iObserverWeapon != weapon )
		{
			m_iClientFOV = m_iFOV = target->m_iFOV;

			// write fov before wepon data, so zoomed crosshair is set correctly
			MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
				WRITE_BYTE( m_iFOV );
			MESSAGE_END();

			m_iObserverWeapon = weapon;
			//send weapon update
			MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
				WRITE_BYTE( 1 );	// 1 = current weapon, not on target
				WRITE_BYTE( m_iObserverWeapon );
				WRITE_BYTE( 0 );	// clip
			MESSAGE_END();
		}
	}
	else
	{
		m_iFOV = 90;

		if ( m_iObserverWeapon )
		{
			m_iObserverWeapon = 0;

			MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
				WRITE_BYTE( 1 );	// 1 = current weapon
				WRITE_BYTE( m_iObserverWeapon );
				WRITE_BYTE( 0 );	// clip
			MESSAGE_END();
		}
	}
}

// Attempt to change the observer mode
void CBasePlayer::Observer_SetMode( int iMode )
{
	// Just abort if we're changing to the mode we're already in
	if ( iMode == pev->iuser1 )
	{
		pev->iuser3 = 0;
		return;
	}

	int forcecamera = GetForceCamera();

	// is valid mode ?
	if ( iMode < OBS_CHASE_LOCKED || iMode > OBS_IN_EYE )
		iMode = OBS_IN_EYE; // now it is

	// verify observer target again
	if ( pev->team != TEAM_SPECTATOR )
	{
		if ( forcecamera == 1 && iMode == OBS_ROAMING )
			iMode = OBS_CHASE_LOCKED;
		else if ( forcecamera == 2 )
			iMode = OBS_IN_EYE;
	}

	if ( m_hObserverTarget )
	{
		CBaseEntity *pEnt = m_hObserverTarget;

		if ( ( pEnt == this ) || ( pEnt == NULL ) )
			m_hObserverTarget = NULL;
		else if ( pEnt->has_disconnected )
			m_hObserverTarget = NULL;
		else if ( ( (CBasePlayer *)pEnt )->IsObserver() || ( pEnt->pev->effects & EF_NODRAW ) )
			m_hObserverTarget = NULL;
		else if ( ( forcecamera != 0 ) && ( pEnt->pev->team != pev->team ) )
			m_hObserverTarget = NULL;
	}

	// set spectator mode
	pev->iuser1 = iMode;

	bool bError = false;
	(void)bError;

	// if we are not roaming, we need a valid target to track
	if ( ( iMode != OBS_ROAMING ) && ( m_hObserverTarget == NULL ) )
	{
		Observer_FindNextPlayer( false );

		// if we didn't find a valid target switch to roaming
		if ( m_hObserverTarget == NULL )
		{
			bError = true;
			pev->iuser1 = OBS_ROAMING;
		}
	}

	// set spectator mode
	if ( pev->iuser1 == OBS_ROAMING )
		pev->iuser2 = 0;
	else
		pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );

	pev->iuser3 = 0;	// clear second target from death cam

	if ( m_hObserverTarget )
		UTIL_SetOrigin( this, m_hObserverTarget->pev->origin );

	// print spepctaor mode on client screen
	{
		char modemsg[16];
		sprintf( modemsg, "#Spec_Mode%i", pev->iuser1 );
		ClientPrint( pev, HUD_PRINTCENTER, modemsg );
	}

	if ( pev->iuser1 == OBS_IN_EYE )
		SendObserverTargetWeaponAnim();

	m_iObserverLastMode = iMode;
}

void CBasePlayer::Observer_CheckTarget( void )
{
	if ( pev->iuser1 == OBS_ROAMING )
		return;

	// try to find a traget if we have no current one
	if ( !m_hObserverTarget )
	{
		Observer_FindNextPlayer( false );

		if ( !m_hObserverTarget )
		{
			// no target found at all
			int lastMode = pev->iuser1;

			Observer_SetMode( OBS_ROAMING );

			m_iObserverLastMode = lastMode;	// don't overwrite users lastmode

			return;	// we still have np target return
		}
	}

	CBasePlayer *target = (CBasePlayer *)( UTIL_PlayerByIndex( ENTINDEX( m_hObserverTarget->edict() ) ) );

	if ( !target )
	{
		Observer_FindNextPlayer( false );
		return;
	}

	// check taget
	if ( target->pev->deadflag == DEAD_DYING )
	{
		if ( ( target->m_fDeadTime + 2.0f ) < gpGlobals->time )
		{
			// 3 secs after death change target
			Observer_FindNextPlayer( false );
			return;
		}
	}
}

void CBasePlayer::Observer_HandleButtons( void )
{
	// Slow down mouse clicks
	if ( m_flNextObserverInput > gpGlobals->time )
		return;

	// Jump changes from modes: Chase to Roaming
	if ( m_afButtonPressed & IN_JUMP )
	{
		if ( pev->iuser1 == OBS_CHASE_LOCKED )
			Observer_SetMode( OBS_CHASE_FREE );

		else if ( pev->iuser1 == OBS_CHASE_FREE )
			Observer_SetMode( OBS_IN_EYE );

		else if ( pev->iuser1 == OBS_IN_EYE )
			Observer_SetMode( OBS_ROAMING );

		else if ( pev->iuser1 == OBS_ROAMING )
			Observer_SetMode( OBS_CHASE_FREE );

		else
			Observer_SetMode( OBS_CHASE_FREE );	// don't use OBS_CHASE_LOCKED anymore

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack moves to the next player
	if ( m_afButtonPressed & IN_ATTACK ) //&& pev->iuser1 != OBS_ROAMING )
	{
		Observer_FindNextPlayer( false );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack2 moves to the prev player
	if ( m_afButtonPressed & IN_ATTACK2 ) // && pev->iuser1 != OBS_ROAMING )
	{
		Observer_FindNextPlayer( true );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}
}

void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle )
{
	// clear any clientside entities attached to this player
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_KILLPLAYERATTACHMENTS );
		WRITE_BYTE( (BYTE)entindex() );
	MESSAGE_END();

	// Holster weapon immediately, to allow it to cleanup
	if ( m_pActiveItem )
		m_pActiveItem->Holster( 0 );

	if ( m_pTank != NULL )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0XFF );
		WRITE_BYTE( 0xFF );
	MESSAGE_END();

	// reset FOV
	m_iFOV = m_iClientFOV = 0;
	pev->fov = m_iFOV;
	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();

	CheckPlayerSpeed();

	// Setup flags
	m_iHideHUD = ( HIDEHUD_HEALTH | HIDEHUD_WEAPONS );
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin( this, vecPosition );
	ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
	ClearBits( pev->flags, FL_DUCKING );
	pev->deadflag = DEAD_DEAD;
	pev->health = 1;

	m_bIsObserver = TRUE;

	// Remove all the player's stuff
	RemoveAllItems( FALSE );

	// Find a player to watch
	m_flNextObserverInput = 0;
	Observer_SetMode( m_iObserverLastMode );

	g_engfuncs.pfnSetClientMaxspeed( ENT( pev ), 350 );

	MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
		WRITE_BYTE( ENTINDEX( edict() ) );
		WRITE_BYTE( 1 );
	MESSAGE_END();
}

void CBasePlayer::StopObserver( void )
{
	// Turn off spectator
	if ( pev->iuser1 || pev->iuser2 || m_bIsObserver )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
			WRITE_BYTE( ENTINDEX( edict() ) );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		pev->iuser1 = 0;
		pev->iuser2 = 0;
		m_bIsObserver = FALSE;
		m_hObserverTarget = NULL;
		m_imissedwave = 0;
		m_irdytospawn = 0;
	}

	m_iHideHUD = 0;
}

void CBasePlayer::Observer_SpectatePlayer( int index )
{
	if ( !pev->iuser1 )
		return;
	bool sameTeam = GetForceCamera() != 0 && pev->team != TEAM_SPECTATOR;
	CBaseEntity* target = UTIL_PlayerByIndex( index );
	if ( !Observer_PlayerValidTarget( target, sameTeam ) )
		return;
	m_hObserverTarget = target;
	if ( m_hObserverTarget )
	{
		if ( pev->iuser1 == OBS_ROAMING )
			Observer_SetMode( OBS_CHASE_FREE );
		pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
		UTIL_SetOrigin( this, m_hObserverTarget->pev->origin );
		ALERT( at_console, "Now Tracking %s\n", STRING( m_hObserverTarget->pev->netname ) );
		if ( pev->iuser1 == OBS_IN_EYE )
			SendObserverTargetWeaponAnim();
	}
	else
	{
		ALERT( at_console, "No observer targets.\n" );
		Observer_SetMode( OBS_ROAMING );
	}
}
