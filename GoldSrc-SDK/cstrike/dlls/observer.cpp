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
#include "game.h"
#include "pm_shared.h"

int GetForceCamera()
{
	if ( fadetoblack.value != 0 )
		return 2;

	int camera = (int)CVAR_GET_FLOAT( "mp_forcechasecam" );
	if ( !camera )
		camera = (int)CVAR_GET_FLOAT( "mp_forcecamera" );
	return camera;
}

static void UpdateClientEffects( CBasePlayer* pObserver, int oldMode )
{
	bool clearProgress = true;
	bool clearFade = fadetoblack.value == 0;

	if ( oldMode != OBS_IN_EYE && pObserver->pev->iuser1 != OBS_IN_EYE )
		return;

	CBasePlayer* target = NULL;
	if ( pObserver->pev->iuser1 == OBS_IN_EYE && pObserver->m_hObserverTarget
		&& pObserver->m_hObserverTarget->IsPlayer() )
		target = (CBasePlayer*)UTIL_PlayerByIndex( ENTINDEX( pObserver->m_hObserverTarget->edict() ) );
	if ( !target )
	{
		pObserver->SetProgressBarTime( 0 );
		if ( clearFade )
			UTIL_ScreenFade( pObserver, Vector( 0, 0, 0 ), 0.001f, 0, 0, 0 );

		MESSAGE_BEGIN( MSG_ONE, gmsgNVGToggle, NULL, pObserver->pev );
		WRITE_BYTE( 0 );
		MESSAGE_END();
		pObserver->m_bNightVisionOn = false;
		return;
	}

	if ( target )
	{
		if ( target->m_progressStart != 0 && target->m_progressEnd > target->m_progressStart
			&& target->m_progressEnd > gpGlobals->time )
		{
			float duration = target->m_progressEnd - target->m_progressStart;
			float elapsed = gpGlobals->time - target->m_progressStart;
			pObserver->SetProgressBarTime2( (int)duration, elapsed );
			clearProgress = false;
		}

		if ( clearFade && target->m_blindStartTime != 0 && target->m_blindFadeTime != 0
			&& target->m_blindStartTime + target->m_blindHoldTime + target->m_blindFadeTime > gpGlobals->time )
		{
			float fadeTime = target->m_blindFadeTime;
			float fadeHold = target->m_blindStartTime + target->m_blindHoldTime - gpGlobals->time;
			int alpha = target->m_blindAlpha;
			if ( fadeHold <= 0 )
			{
				float fraction = ( target->m_blindStartTime + target->m_blindHoldTime
					+ target->m_blindFadeTime - gpGlobals->time ) / target->m_blindFadeTime;
				float scaledAlpha = alpha * fraction;
				alpha = (int)scaledAlpha;
				fadeTime *= fraction;
				fadeHold = 0;
			}

			UTIL_ScreenFade( pObserver, Vector( 255, 255, 255 ), fadeTime, fadeHold, alpha, 0 );
			clearFade = false;
		}
	}

	if ( pObserver->m_bNightVisionOn != target->m_bNightVisionOn )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgNVGToggle, NULL, pObserver->pev );
		WRITE_BYTE( target->m_bNightVisionOn );
		MESSAGE_END();
		pObserver->m_bNightVisionOn = target->m_bNightVisionOn;
	}

	if ( clearProgress )
		pObserver->SetProgressBarTime( 0 );
	if ( clearFade )
		UTIL_ScreenFade( pObserver, Vector( 0, 0, 0 ), 0.001f, 0, 0, 0 );
}

CBaseEntity* CBasePlayer::Observer_IsValidTarget( int iPlayerIndex, bool bSameTeam )
{
	if ( iPlayerIndex > gpGlobals->maxClients || iPlayerIndex <= 0 )
		return NULL;

	CBasePlayer* target = (CBasePlayer*)UTIL_PlayerByIndex( iPlayerIndex );
	if ( !target || target == this || target->has_disconnected || target->pev->iuser1
		|| ( target->pev->effects & EF_NODRAW ) || target->m_iTeam == UNASSIGNED )
		return NULL;
	if ( bSameTeam && target->m_iTeam != m_iTeam )
		return NULL;
	return target;
}

void CBasePlayer::Observer_FindNextPlayer( bool bReverse, const char* name )
{
	if ( m_flNextFollowTime != 0 && m_flNextFollowTime > gpGlobals->time )
		return;
	m_flNextFollowTime = gpGlobals->time + 0.25;

	int start = m_hObserverTarget ? ENTINDEX( m_hObserverTarget->edict() ) : ENTINDEX( edict() );
	int current = start;
	int direction = bReverse ? -1 : 1;
	m_hObserverTarget = NULL;
	bool sameTeam = GetForceCamera() != 0 && m_iTeam != SPECTATOR;

	do
	{
		current += direction;
		if ( current > gpGlobals->maxClients )
			current = 1;
		if ( current <= 0 )
			current = gpGlobals->maxClients;

		m_hObserverTarget = Observer_IsValidTarget( current, sameTeam );
		if ( m_hObserverTarget )
		{
			if ( !name )
				break;

			CBaseEntity* target = UTIL_PlayerByIndex( ENTINDEX( m_hObserverTarget->edict() ) );
			if ( !strcmp( name, STRING( target->pev->netname ) ) )
				break;
		}
	}
	while ( current != start );

	// Did we find a target?
	if ( m_hObserverTarget )
	{
		UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );
		MESSAGE_BEGIN( MSG_ONE, gmsgSpecHealth2, NULL, pev );
		WRITE_BYTE( m_hObserverTarget->pev->health > 0 ? (int)m_hObserverTarget->pev->health : 0 );
		WRITE_BYTE( ENTINDEX( m_hObserverTarget->edict() ) );
		MESSAGE_END();

		// ALERT( at_console, "Now Tracking %s\n", STRING( m_hObserverTarget->pev->netname ) );

		// Store the target in pev so the physics DLL can get to it
		if ( pev->iuser1 != OBS_ROAMING )
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
		UpdateClientEffects( this, pev->iuser1 );
	}
}

// Handle buttons in observer mode
void CBasePlayer::Observer_HandleButtons()
{
	// Slow down mouse clicks
	if ( m_flNextObserverInput > gpGlobals->time )
		return;

	// Jump changes from modes: Chase to Roaming
	if ( m_afButtonPressed & IN_JUMP )
	{
		switch ( pev->iuser1 )
		{
		case OBS_CHASE_LOCKED:
		case OBS_CHASE_FREE:
			Observer_SetMode( OBS_IN_EYE );
			break;
		case OBS_IN_EYE:
			Observer_SetMode( OBS_ROAMING );
			break;
		case OBS_ROAMING:
			Observer_SetMode( OBS_MAP_FREE );
			break;
		case OBS_MAP_FREE:
			Observer_SetMode( OBS_MAP_CHASE );
			break;
		default:
			Observer_SetMode( m_bObserverAutoDirector ? OBS_CHASE_LOCKED : OBS_CHASE_FREE );
			break;
		}
		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack moves to the next player
	if ( m_afButtonPressed & IN_ATTACK ) //&& pev->iuser1 != OBS_ROAMING )
	{
		Observer_FindNextPlayer( false, NULL );
		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack2 moves to the prev player
	if ( m_afButtonPressed & IN_ATTACK2 ) // && pev->iuser1 != OBS_ROAMING )
	{
		Observer_FindNextPlayer( true, NULL );
		m_flNextObserverInput = gpGlobals->time + 0.2;
	}
}

void CBasePlayer::Observer_CheckTarget()
{
	if ( pev->iuser1 == OBS_ROAMING && !m_bWasFollowing )
		return;

	if ( m_bWasFollowing )
	{
		Observer_FindNextPlayer( false, NULL );
		if ( m_hObserverTarget && m_iObserverLastMode != pev->iuser1 )
			Observer_SetMode( m_iObserverLastMode );
		return;
	}

	if ( !m_hObserverTarget )
	{
		Observer_FindNextPlayer( false, NULL );
		if ( !m_hObserverTarget )
		{
			// no target found at all 

			int lastMode = pev->iuser1;
			Observer_SetMode( OBS_ROAMING );
			m_iObserverLastMode = lastMode;	// don't overwrite users lastmode
			return;	// we still have np target return
		}
	}

	CBasePlayer* target = (CBasePlayer*)UTIL_PlayerByIndex( ENTINDEX( m_hObserverTarget->edict() ) );
	if ( !target || target->pev->deadflag == DEAD_RESPAWNABLE || ( target->pev->effects & EF_NODRAW ) )
	{
		Observer_FindNextPlayer( false, NULL );
		return;
	}

	if ( target->pev->deadflag == DEAD_DEAD && gpGlobals->time > target->m_fDeadTime + 2.0 )
	{
		Observer_FindNextPlayer( false, NULL );
		if ( !m_hObserverTarget )
		{
			int lastMode = pev->iuser1;
			Observer_SetMode( OBS_ROAMING );
			m_iObserverLastMode = lastMode;
			m_bWasFollowing = true;
		}
	}
}

void CBasePlayer::Observer_CheckProperties()
{
	// try to find a traget if we have no current one
	if ( pev->iuser1 == OBS_IN_EYE && m_hObserverTarget )
	{
		CBasePlayer* target = (CBasePlayer*)UTIL_PlayerByIndex( ENTINDEX( m_hObserverTarget->edict() ) );
		if ( !target )
			return;

		int weapon = target->m_pActiveItem ? target->m_pActiveItem->m_iId : 0;
		// use fov of tracked client
		if ( m_iFOV != target->m_iFOV || m_iObserverWeapon != weapon )
		{
			m_iFOV = target->m_iFOV;
			m_iClientFOV = m_iFOV;
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

		if ( target->m_bHasC4 )
		{
			int state = ( target->m_signals.GetState() & SIGNAL_BOMB ) ? 2 : 1;
			if ( m_iObserverC4State != state )
			{
				m_iObserverC4State = state;
				MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
				WRITE_BYTE( m_iObserverC4State );
				WRITE_STRING( "c4" );
				WRITE_BYTE( 0 );
				WRITE_BYTE( 160 );
				WRITE_BYTE( 0 );
				MESSAGE_END();
			}
		}
		else if ( m_iObserverC4State )
		{
			m_iObserverC4State = 0;
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "c4" );
			MESSAGE_END();
		}

		if ( m_bObserverHasDefuser != target->m_bHasDefuser )
		{
			m_bObserverHasDefuser = target->m_bHasDefuser;
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( m_bObserverHasDefuser ? 1 : 0 );
			WRITE_STRING( "defuser" );
			if ( m_bObserverHasDefuser )
			{
				WRITE_BYTE( 0 );
				WRITE_BYTE( 160 );
				WRITE_BYTE( 0 );
			}
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
			WRITE_BYTE( 0 );
			MESSAGE_END();
		}
		if ( m_iObserverC4State )
		{
			m_iObserverC4State = 0;
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "c4" );
			MESSAGE_END();
		}
		if ( m_bObserverHasDefuser )
		{
			m_bObserverHasDefuser = false;
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( "defuser" );
			MESSAGE_END();
		}
	}
}

// Attempt to change the observer mode
void CBasePlayer::Observer_SetMode( int iMode )
{

	// Just abort if we're changing to the mode we're already in
	if ( iMode == pev->iuser1 )
		return;

	int forceCamera = GetForceCamera();
	int oldMode = pev->iuser1;

	// is valid mode ?
	if ( iMode < OBS_CHASE_LOCKED || iMode > OBS_MAP_CHASE )
		iMode = OBS_IN_EYE;

	if ( m_iTeam != SPECTATOR )
	{
		if ( forceCamera == 1 && iMode == OBS_ROAMING )
			iMode = OBS_MAP_FREE;
		else if ( forceCamera == 2 )
			iMode = OBS_IN_EYE;
	}

	if ( m_hObserverTarget )
	{
		CBasePlayer* target = (CBasePlayer*)(CBaseEntity*)m_hObserverTarget;
		if ( !target || target == this || target->has_disconnected || target->pev->iuser1
			|| ( target->pev->effects & EF_NODRAW ) || ( forceCamera && target->m_iTeam != m_iTeam ) )
			m_hObserverTarget = NULL;
	}

	// set spectator mode
	pev->iuser1 = iMode;
	if ( iMode != OBS_ROAMING && !m_hObserverTarget )
	{
		Observer_FindNextPlayer( false, NULL );
		if ( !m_hObserverTarget )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget" );
			pev->iuser1 = OBS_ROAMING;
		}
	}

	pev->iuser2 = pev->iuser1 == OBS_ROAMING ? 0 : ENTINDEX( m_hObserverTarget->edict() );
	pev->iuser3 = 0;	// clear second target from death cam
	if ( m_hObserverTarget )
		UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );

	MESSAGE_BEGIN( MSG_ONE, gmsgCrosshair, NULL, pev );
	WRITE_BYTE( iMode == OBS_ROAMING ? 1 : 0 );
	MESSAGE_END();
	UpdateClientEffects( this, oldMode );

	char message[16];
	sprintf( message, "#Spec_Mode%i", pev->iuser1 );
	ClientPrint( pev, HUD_PRINTCENTER, message );
	m_iObserverLastMode = iMode;
	m_bWasFollowing = false;
}
