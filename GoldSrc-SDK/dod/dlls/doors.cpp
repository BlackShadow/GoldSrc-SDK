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
/*

===== doors.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "doors.h"


extern void SetMovedir(entvars_t* ev);

#define noiseMoving noise1
#define noiseArrived noise2
#define SF_DOOR_TOUCH_ONLY 1024
#define SF_DOOR_ONOFF_MODE 2048

class CBaseDoor : public CBaseToggle
{
public:
	void RoundRespawn() override;
	void Activate() override { CBaseEntity::Activate(); }
	void Spawn( void );
	void Precache( void );
	virtual void KeyValue( KeyValueData *pkvd );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void Blocked( CBaseEntity *pOther );


	virtual int	ObjectCaps( void )
	{
		if (pev->spawnflags & SF_ITEM_USE_ONLY)
			return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE;
		else
			return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
	};
	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	void SetToggleState( TOGGLE_STATE state ) override;

	// used to selectivly override defaults
	void EXPORT DoorTouch( CBaseEntity *pOther );

	// local functions
	int DoorActivate( );
	void EXPORT DoorGoUp( void );
	void EXPORT DoorGoDown( void );
	void EXPORT DoorHitTop( void );
	void EXPORT DoorHitBottom( void );

	BYTE	m_bHealthValue;// some doors are medi-kit doors, they give players health

	BYTE	m_bMoveSnd;			// sound a door makes while moving
	BYTE	m_bStopSnd;			// sound a door makes when it stops

	locksound_t m_ls;			// door lock sounds

	BYTE	m_bLockedSound;		// ordinals from entity selection
	BYTE	m_bLockedSentence;
	BYTE	m_bUnlockedSound;
	BYTE	m_bUnlockedSentence;
	int TeamSpecific;
	float m_lastBlockedTimestamp;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};


TYPEDESCRIPTION	CBaseDoor::m_SaveData[] =
{
	DEFINE_FIELD( CBaseDoor, m_bHealthValue, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bMoveSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bStopSnd, FIELD_CHARACTER ),

	DEFINE_FIELD( CBaseDoor, m_bLockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bLockedSentence, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bUnlockedSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CBaseDoor, m_bUnlockedSentence, FIELD_CHARACTER ),

};

IMPLEMENT_SAVERESTORE( CBaseDoor, CBaseToggle );


#define DOOR_SENTENCEWAIT	6
#define DOOR_SOUNDWAIT		3
#define BUTTON_SOUNDWAIT	0.5

// play door or button locked or unlocked sounds.
// pass in pointer to valid locksound struct.
// if flocked is true, play 'door is locked' sound,
// otherwise play 'door is unlocked' sound
// NOTE: this routine is shared by doors and buttons

void PlayLockSounds(entvars_t *pev, locksound_t *pls, int flocked, int fbutton)
{
	// LOCKED SOUND

	// CONSIDER: consolidate the locksound_t struct (all entries are duplicates for lock/unlock)
	// CONSIDER: and condense this code.
	float flsoundwait;

	if (fbutton)
		flsoundwait = BUTTON_SOUNDWAIT;
	else
		flsoundwait = DOOR_SOUNDWAIT;

	if (flocked)
	{
		int fplaysound = (pls->sLockedSound && gpGlobals->time > pls->flwaitSound);
		int fplaysentence = (pls->sLockedSentence && !pls->bEOFLocked && gpGlobals->time > pls->flwaitSentence);
		float fvol;

		if (fplaysound && fplaysentence)
			fvol = 0.25;
		else
			fvol = 1.0;

		// if there is a locked sound, and we've debounced, play sound
		if (fplaysound)
		{
			// play 'door locked' sound
			EMIT_SOUND(ENT(pev), CHAN_ITEM, (char*)STRING(pls->sLockedSound), fvol, ATTN_NORM);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// if there is a sentence, we've not played all in list, and we've debounced, play sound
		if (fplaysentence)
		{
			// play next 'door locked' sentence in group
			int iprev = pls->iLockedSentence;

			pls->iLockedSentence = SENTENCEG_PlaySequentialSz(ENT(pev), STRING(pls->sLockedSentence),
					  0.85, ATTN_NORM, 0, 100, pls->iLockedSentence, FALSE);
			pls->iUnlockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFLocked = (iprev == pls->iLockedSentence);

			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
	else
	{
		// UNLOCKED SOUND

		int fplaysound = (pls->sUnlockedSound && gpGlobals->time > pls->flwaitSound);
		int fplaysentence = (pls->sUnlockedSentence && !pls->bEOFUnlocked && gpGlobals->time > pls->flwaitSentence);
		float fvol;

		// if playing both sentence and sound, lower sound volume so we hear sentence
		if (fplaysound && fplaysentence)
			fvol = 0.25;
		else
			fvol = 1.0;

		// play 'door unlocked' sound if set
		if (fplaysound)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, (char*)STRING(pls->sUnlockedSound), fvol, ATTN_NORM);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// play next 'door unlocked' sentence in group
		if (fplaysentence)
		{
			int iprev = pls->iUnlockedSentence;

			pls->iUnlockedSentence = SENTENCEG_PlaySequentialSz(ENT(pev), STRING(pls->sUnlockedSentence),
					  0.85, ATTN_NORM, 0, 100, pls->iUnlockedSentence, FALSE);
			pls->iLockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFUnlocked = (iprev == pls->iUnlockedSentence);
			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
}

//
// Cache user-entity-field values until spawn is called.
//

void CBaseDoor::KeyValue( KeyValueData *pkvd )
{

	if (FStrEq(pkvd->szKeyName, "skin"))//skin is used for content type
	{
		pev->skin = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "movesnd"))
	{
		m_bMoveSnd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "stopsnd"))
	{
		m_bStopSnd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "healthvalue"))
	{
		m_bHealthValue = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_sound"))
	{
		m_bLockedSound = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_sentence"))
	{
		m_bLockedSentence = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "unlocked_sound"))
	{
		m_bUnlockedSound = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "unlocked_sentence"))
	{
		m_bUnlockedSentence = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "WaveHeight"))
	{
		pev->scale = atof(pkvd->szValue) * (1.0/8.0);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "TeamDoors" ) )
	{
		TeamSpecific = (int)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

/*QUAKED func_door (0 .5 .8) ? START_OPEN x DOOR_DONT_LINK TOGGLE
if two doors touch, they are assumed to be connected and operate as a unit.

TOGGLE causes the door to wait in both the start and end states for a trigger event.

START_OPEN causes the door to move to its destination when spawned, and operate in reverse.
It is used to temporarily or permanently close off an area when triggered (not usefull for
touch or takedamage doors).

"angle"         determines the opening direction
"targetname"	if set, no touch field will be spawned and a remote button or trigger
				field activates the door.
"health"        if set, door must be shot open
"speed"         movement speed (100 default)
"wait"          wait before returning (3 default, -1 = never return)
"lip"           lip remaining at end of move (8 default)
"dmg"           damage to inflict when blocked (2 default)
"sounds"
0)      no sound
1)      stone
2)      base
3)      stone chain
4)      screechy metal
*/

LINK_ENTITY_TO_CLASS( func_door, CBaseDoor );
//
// func_water - same as a door.
//
LINK_ENTITY_TO_CLASS( func_water, CBaseDoor );


void CBaseDoor::RoundRespawn()
{
	STOP_SOUND( ENT( pev ), CHAN_STATIC, (char *)STRING( pev->noiseMoving ) );

	m_vecPosition1	= pev->origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2	= m_vecPosition1 + (pev->movedir * (fabs( pev->movedir.x * (pev->size.x-2) ) + fabs( pev->movedir.y * (pev->size.y-2) ) + fabs( pev->movedir.z * (pev->size.z-2) ) - m_flLip));
	ASSERTSZ(m_vecPosition1 != m_vecPosition2, "door start/end positions are equal");
	if ( FBitSet (pev->spawnflags, SF_DOOR_START_OPEN) )
	{	// swap pos1 and pos2, put door at pos2
		UTIL_SetOrigin( this, m_vecPosition2 );
		Vector vecTemp = m_vecPosition2;
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = vecTemp;
	}

	m_toggle_state = TS_AT_BOTTOM;

	// if the door is flagged for USE button activation only, use NULL touch function
	if ( FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) && !FBitSet( pev->spawnflags, SF_DOOR_TOUCH_ONLY ) )
	{
		SetTouch( NULL );
	}
	else // touchable button
		SetTouch( &CBaseDoor::DoorTouch );
}

void CBaseDoor::Spawn( )
{
	Precache();
	SetMovedir (pev);

	if ( pev->skin == 0 )
	{//normal door
		if ( FBitSet (pev->spawnflags, SF_DOOR_PASSABLE) )
			pev->solid		= SOLID_NOT;
		else
			pev->solid		= SOLID_BSP;
	}
	else
	{// special contents
		pev->solid		= SOLID_NOT;
		SetBits( pev->spawnflags, SF_DOOR_SILENT );	// water is silent for now
	}

	pev->movetype	= MOVETYPE_PUSH;
	UTIL_SetOrigin( this, pev->origin );
	SET_MODEL( ENT(pev), STRING(pev->model) );

	if (pev->speed == 0)
		pev->speed = 100;

	m_vecPosition1	= pev->origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2	= m_vecPosition1 + (pev->movedir * (fabs( pev->movedir.x * (pev->size.x-2) ) + fabs( pev->movedir.y * (pev->size.y-2) ) + fabs( pev->movedir.z * (pev->size.z-2) ) - m_flLip));
	ASSERTSZ(m_vecPosition1 != m_vecPosition2, "door start/end positions are equal");
	if ( FBitSet (pev->spawnflags, SF_DOOR_START_OPEN) )
	{	// swap pos1 and pos2, put door at pos2
		UTIL_SetOrigin( this, m_vecPosition2 );
		Vector vecTemp = m_vecPosition2;
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = vecTemp;
	}

	m_toggle_state = TS_AT_BOTTOM;

	// if the door is flagged for USE button activation only, use NULL touch function
	if ( FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) && !FBitSet( pev->spawnflags, SF_DOOR_TOUCH_ONLY ) )
	{
		SetTouch( NULL );
	}
	else // touchable button
		SetTouch( &CBaseDoor::DoorTouch );

	RoundStore();
	m_lastBlockedTimestamp = 0;
}


void CBaseDoor :: SetToggleState( TOGGLE_STATE state )
{
	if ( state == TS_AT_TOP )
		UTIL_SetOrigin( this, m_vecPosition2 );
	else
		UTIL_SetOrigin( this, m_vecPosition1 );
}


void CBaseDoor::Precache( void )
{
	char *pszSound;

	PRECACHE_SOUND( "doors/doormove.wav" );
	PRECACHE_SOUND( "doors/doorstop.wav" );

// set the door's "in-motion" sound
	switch (m_bMoveSnd)
	{
	case	0:
		pev->noiseMoving = ALLOC_STRING("common/null.wav");
		break;
	default:
		pev->noiseMoving = ALLOC_STRING("doors/doormove.wav");
		break;
	}

// set the door's 'reached destination' stop sound
	switch (m_bStopSnd)
	{
	case	0:
		pev->noiseArrived = ALLOC_STRING("common/null.wav");
		break;
	default:
		pev->noiseArrived = ALLOC_STRING("doors/doorstop.wav");
		break;
	}

	// get door button sounds, for doors which are directly 'touched' to open

	if (m_bLockedSound)
	{
		pszSound = ButtonSound( (int)m_bLockedSound );
		PRECACHE_SOUND(pszSound);
		m_ls.sLockedSound = ALLOC_STRING(pszSound);
	}

	if (m_bUnlockedSound)
	{
		pszSound = ButtonSound( (int)m_bUnlockedSound );
		PRECACHE_SOUND(pszSound);
		m_ls.sUnlockedSound = ALLOC_STRING(pszSound);
	}

	PRECACHE_SOUND( "buttons/latchlocked1.wav" );

	// get sentence group names, for doors which are directly 'touched' to open

	switch (m_bLockedSentence)
	{
		case 1: m_ls.sLockedSentence = ALLOC_STRING("NA"); break; // access denied
		case 2: m_ls.sLockedSentence = ALLOC_STRING("ND"); break; // security lockout
		case 3: m_ls.sLockedSentence = ALLOC_STRING("NF"); break; // blast door
		case 4: m_ls.sLockedSentence = ALLOC_STRING("NFIRE"); break; // fire door
		case 5: m_ls.sLockedSentence = ALLOC_STRING("NCHEM"); break; // chemical door
		case 6: m_ls.sLockedSentence = ALLOC_STRING("NRAD"); break; // radiation door
		case 7: m_ls.sLockedSentence = ALLOC_STRING("NCON"); break; // gen containment
		case 8: m_ls.sLockedSentence = ALLOC_STRING("NH"); break; // maintenance door
		case 9: m_ls.sLockedSentence = ALLOC_STRING("NG"); break; // broken door

		default: m_ls.sLockedSentence = 0; break;
	}

	switch (m_bUnlockedSentence)
	{
		case 1: m_ls.sUnlockedSentence = ALLOC_STRING("EA"); break; // access granted
		case 2: m_ls.sUnlockedSentence = ALLOC_STRING("ED"); break; // security door
		case 3: m_ls.sUnlockedSentence = ALLOC_STRING("EF"); break; // blast door
		case 4: m_ls.sUnlockedSentence = ALLOC_STRING("EFIRE"); break; // fire door
		case 5: m_ls.sUnlockedSentence = ALLOC_STRING("ECHEM"); break; // chemical door
		case 6: m_ls.sUnlockedSentence = ALLOC_STRING("ERAD"); break; // radiation door
		case 7: m_ls.sUnlockedSentence = ALLOC_STRING("ECON"); break; // gen containment
		case 8: m_ls.sUnlockedSentence = ALLOC_STRING("EH"); break; // maintenance door

		default: m_ls.sUnlockedSentence = 0; break;
	}
}

//
// Doors not tied to anything (e.g. button, another door) can be touched, to make them activate.
//
void CBaseDoor::DoorTouch( CBaseEntity *pOther )
{
	entvars_t*	pevToucher = pOther->pev;

	// Ignore touches by anything but players
	if (!FClassnameIs(pevToucher, "player"))
		return;

	// If door has master, and it's not ready to trigger,
	// play 'locked' sound

	if (m_sMaster && !UTIL_IsMasterTriggered(m_sMaster, pOther))
		PlayLockSounds(pev, &m_ls, TRUE, FALSE);

	// If door is somebody's target, then touching does nothing.
	// You have to activate the owner (e.g. button).

	if ( !FStringNull( pev->targetname ) && !FBitSet( pev->spawnflags, SF_DOOR_TOUCH_ONLY ) )
	{
		// play locked sound
		PlayLockSounds(pev, &m_ls, TRUE, FALSE);
		return;
	}

	m_hActivator = pOther;// remember who activated the door

	if ( ( TeamSpecific == 1 || TeamSpecific == 2 ) && m_hActivator->pev->team != TeamSpecific )
	{
		PlayLockSounds( pev, &m_ls, TRUE, FALSE );
		return;
	}

	if (DoorActivate( ))
		SetTouch( NULL ); // Temporarily disable the touch function, until movement is finished.
}


//
// Used by SUB_UseTargets, when a door is the target of a button.
//
void CBaseDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;
	if ( ( pev->spawnflags & SF_DOOR_ONOFF_MODE ) && UTIL_IsMasterTriggered( m_sMaster, pActivator ) )
	{
		if ( useType == USE_ON )
		{
			if ( m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN )
			{
				PlayLockSounds( pev, &m_ls, FALSE, FALSE );
				DoorGoUp();
			}
			return;
		}
		if ( useType == USE_OFF )
		{
			if ( m_toggle_state == TS_AT_TOP || m_toggle_state == TS_GOING_UP )
				DoorGoDown();
			return;
		}
	}
	// if not ready to be used, ignore "use" command.
	if (m_toggle_state == TS_AT_BOTTOM || FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
		DoorActivate();
}

//
// Causes the door to "do its thing", i.e. start moving, and cascade activation.
//
int CBaseDoor::DoorActivate( )
{
	if (!UTIL_IsMasterTriggered(m_sMaster, m_hActivator))
		return 0;

	if (FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
	{// door should close
		DoorGoDown();
	}
	else
	{// door should open

		if ( m_hActivator != NULL && m_hActivator->IsPlayer() )
		{// give health if player opened the door (medikit)
		// VARS( m_eoActivator )->health += m_bHealthValue;

			m_hActivator->TakeHealth( m_bHealthValue, DMG_GENERIC );

		}

		// play door unlock sounds
		PlayLockSounds(pev, &m_ls, FALSE, FALSE);

		DoorGoUp();
	}

	return 1;
}

extern Vector VecBModelOrigin( entvars_t* pevBModel );

//
// Starts the door going to its "up" position (simply ToggleData->vecPosition2).
//
void CBaseDoor::DoorGoUp( void )
{
	entvars_t	*pevActivator;

	// It could be going-down, if blocked.
	ASSERT(m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN);

	bool isReversing = ( m_toggle_state == TS_GOING_DOWN );

	// emit door moving and stop sounds on CHAN_STATIC so that the multicast doesn't
	// filter them out and leave a client stuck with looping door sounds!
	if ( !isReversing && !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
		EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMoving), 1, ATTN_NORM);

	m_toggle_state = TS_GOING_UP;

	SetMoveDone( &CBaseDoor::DoorHitTop );

	if ( FBitSet( pev->spawnflags, SF_DOOR_ONOFF_MODE ) )
		SUB_UseTargets( m_hActivator, USE_ON, 0 );

	if ( FClassnameIs(pev, "func_door_rotating"))		// !!! BUGBUG Triggered doors don't work with this yet
	{
		float	sign = 1.0;

		if ( m_hActivator != NULL )
		{
			pevActivator = m_hActivator->pev;

			if ( !FBitSet( pev->spawnflags, SF_DOOR_ONEWAY ) && pev->movedir.y ) 		// Y axis rotation, move away from the player
			{
				Vector vec = pevActivator->origin - pev->origin;
	//			Vector vnext = (pevToucher->origin + (pevToucher->velocity * 10)) - pev->origin;
				UTIL_MakeVectors ( pevActivator->angles );
				Vector vnext = (pevActivator->origin + (gpGlobals->v_forward * 10)) - pev->origin;
				if ( (vec.x*vnext.y - vec.y*vnext.x) < 0 )
					sign = -1.0;

				if ( isReversing && pev->angles.y < 0 )
					sign = -sign;
			}
		}
		AngularMove(m_vecAngle2*sign, pev->speed);
	}
	else
		LinearMove(m_vecPosition2, pev->speed);
}


//
// The door has reached the "up" position.  Either go back down, or wait for another activation.
//
void CBaseDoor::DoorHitTop( void )
{
	if ( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
	{
		STOP_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMoving) );
		EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseArrived), 1, ATTN_NORM);
	}

	ASSERT(m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_AT_TOP;

	// toggle-doors don't come down automatically, they wait for refire.
	if (FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN))
	{
		// Re-instate touch method, movement is complete
		if ( !FBitSet ( pev->spawnflags, SF_DOOR_USE_ONLY ) || FBitSet( pev->spawnflags, SF_DOOR_TOUCH_ONLY ) )
			SetTouch( &CBaseDoor::DoorTouch );
	}
	else
	{
		// In flWait seconds, DoorGoDown will fire, unless wait is -1, then door stays open
		pev->nextthink = pev->ltime + m_flWait;
		SetThink( &CBaseDoor::DoorGoDown );

		if ( m_flWait == -1 )
		{
			pev->nextthink = -1;
		}
	}

	if ( FBitSet( pev->spawnflags, SF_DOOR_START_OPEN ) )
	{
		if ( pev->netname )
			FireTargets( STRING(pev->netname), m_hActivator, this, USE_TOGGLE, 0 );
	}
	else if ( pev->message )
		FireTargets( STRING(pev->message), m_hActivator, this, USE_TOGGLE, 0 );

	if ( !FBitSet( pev->spawnflags, SF_DOOR_ONOFF_MODE ) )
		SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 ); // this isn't finished
}


//
// Starts the door going to its "down" position (simply ToggleData->vecPosition1).
//
void CBaseDoor::DoorGoDown( void )
{
	bool isReversing = ( m_toggle_state == TS_GOING_UP );

	if ( !isReversing && !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
		EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMoving), 1, ATTN_NORM);

#ifdef DOOR_ASSERT
	ASSERT(m_toggle_state == TS_AT_TOP);
#endif // DOOR_ASSERT
	m_toggle_state = TS_GOING_DOWN;

	SetMoveDone( &CBaseDoor::DoorHitBottom );
	if ( FClassnameIs(pev, "func_door_rotating"))//rotating door
	{
		if ( FBitSet( pev->spawnflags, SF_DOOR_ONOFF_MODE ) )
			SUB_UseTargets( m_hActivator, USE_OFF, 0 );

		AngularMove( m_vecAngle1, pev->speed);
	}
	else
	{
		if ( FBitSet( pev->spawnflags, SF_DOOR_ONOFF_MODE ) )
			SUB_UseTargets( m_hActivator, USE_OFF, 0 );

		LinearMove( m_vecPosition1, pev->speed);
	}
}

//
// The door has reached the "down" position.  Back to quiescence.
//
void CBaseDoor::DoorHitBottom( void )
{
	if ( !FBitSet( pev->spawnflags, SF_DOOR_SILENT ) )
	{
		STOP_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseMoving) );
		EMIT_SOUND(ENT(pev), CHAN_STATIC, (char*)STRING(pev->noiseArrived), 1, ATTN_NORM);
	}

	ASSERT(m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_AT_BOTTOM;

	// Re-instate touch method, cycle is complete
	if ( FBitSet ( pev->spawnflags, SF_DOOR_USE_ONLY ) && !FBitSet( pev->spawnflags, SF_DOOR_TOUCH_ONLY ) )
	{// use only door
		SetTouch ( NULL );
	}
	else // touchable door
		SetTouch( &CBaseDoor::DoorTouch );

	if ( FBitSet( pev->spawnflags, SF_DOOR_START_OPEN ) )
	{
		if ( pev->message )
			FireTargets( STRING(pev->message), m_hActivator, this, USE_TOGGLE, 0 );
	}
	else if ( pev->netname )
		FireTargets( STRING(pev->netname), m_hActivator, this, USE_TOGGLE, 0 );

	if ( !FBitSet( pev->spawnflags, SF_DOOR_ONOFF_MODE ) )
		SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 ); // this isn't finished
}

void CBaseDoor::Blocked( CBaseEntity *pOther )
{
	CBaseEntity* pTarget = NULL;
	CBaseDoor* pDoor = NULL;

	// Hurt the blocker a little.
	if ( pev->dmg )
		pOther->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH );

	const float checkBlockedInterval = 0.25;

	if ( gpGlobals->time - m_lastBlockedTimestamp < checkBlockedInterval )
		return;

	m_lastBlockedTimestamp = gpGlobals->time;

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast
	if ( m_flWait >= 0 )
	{
		if ( m_toggle_state == TS_GOING_DOWN )
			DoorGoUp();
		else
			DoorGoDown();
	}

	// Block all door pieces with the same targetname here.
	if ( !FStringNull( pev->targetname ) )
	{
		for ( ;; )
		{
			pTarget = UTIL_FindEntityByTargetname( pTarget, STRING( pev->targetname ) );

			if ( !pTarget )
				break;

			if ( VARS( pTarget->pev ) != pev && FClassnameIs( pTarget->pev, "func_door" ) || FClassnameIs( pTarget->pev, "func_door_rotating" ) )
			{
				pDoor = GetClassPtr( (CBaseDoor *)VARS( pTarget->pev ) );

				if ( pDoor->m_flWait >= 0 )
				{
					if ( pDoor->pev->velocity == pev->velocity && pDoor->pev->avelocity == pev->velocity )
					{
						// this is the most hacked, evil, bastardized thing I've ever seen. kjb
						if ( FClassnameIs( pTarget->pev, "func_door" ) )
						{// set origin to realign normal doors
							pDoor->pev->origin = pev->origin;
							pDoor->pev->velocity = g_vecZero;// stop!
						}
						else
						{// set angles to realign rotating doors
							pDoor->pev->angles = pev->angles;
							pDoor->pev->velocity = g_vecZero;
						}
					}

					if ( pDoor->m_toggle_state == TS_GOING_DOWN )
						pDoor->DoorGoUp();
					else
						pDoor->DoorGoDown();
				}
			}
		}
	}
}


/*QUAKED FuncRotDoorSpawn (0 .5 .8) ? START_OPEN REVERSE
DOOR_DONT_LINK TOGGLE X_AXIS Y_AXIS
if two doors touch, they are assumed to be connected and operate as
a unit.

TOGGLE causes the door to wait in both the start and end states for
a trigger event.

START_OPEN causes the door to move to its destination when spawned,
and operate in reverse.  It is used to temporarily or permanently
close off an area when triggered (not usefull for touch or
takedamage doors).

You need to have an origin brush as part of this entity.  The
center of that brush will be
the point around which it is rotated. It will rotate around the Z
axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"distance" is how many degrees the door will be rotated.
"speed" determines how fast the door moves; default value is 100.

REVERSE will cause the door to rotate in the opposite direction.

"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote
button or trigger field activates the door.
"health"	if set, door must be shot open
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"dmg"		damage to inflict when blocked (2 default)
"sounds"
0)	no sound
1)	stone
2)	base
3)	stone chain
4)	screechy metal
*/
class CRotDoor : public CBaseDoor
{
public:
	void RoundRespawn() override;
	void Activate() override { CBaseEntity::Activate(); }
	void Spawn( void );
	void SetToggleState( TOGGLE_STATE state ) override;
};

LINK_ENTITY_TO_CLASS( func_door_rotating, CRotDoor );


void CRotDoor::RoundRespawn()
{
	m_vecAngle1	= pev->angles;
	m_vecAngle2	= pev->angles + pev->movedir * m_flMoveDistance;

	if ( FBitSet (pev->spawnflags, SF_DOOR_START_OPEN) )
	{	// swap pos1 and pos2, put door at pos2, invert movement direction
		pev->angles = m_vecAngle2;
		Vector vecSav = m_vecAngle1;
		m_vecAngle2 = m_vecAngle1;
		m_vecAngle1 = vecSav;
		pev->movedir = pev->movedir * -1;
	}
	else
		pev->angles = m_vecAngle1;

	m_toggle_state = TS_AT_BOTTOM;

	// if the door is flagged for USE button activation only, use NULL touch function
	if ( FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) && !FBitSet( pev->spawnflags, SF_DOOR_TOUCH_ONLY ) )
	{
		SetTouch( NULL );
	}
	else // touchable button
		SetTouch( &CBaseDoor::DoorTouch );
}

void CRotDoor::Spawn( void )
{
	Precache();
	// set the axis of rotation
	CBaseToggle::AxisDir( pev );

	// check for clockwise rotation
	if ( FBitSet (pev->spawnflags, SF_DOOR_ROTATE_BACKWARDS) )
		pev->movedir = pev->movedir * -1;

	//m_flWait			= 2; who the hell did this? (sjb)
	m_vecAngle1	= pev->angles;
	m_vecAngle2	= pev->angles + pev->movedir * m_flMoveDistance;

	ASSERTSZ(m_vecAngle1 != m_vecAngle2, "rotating door start/end positions are equal");

	if ( FBitSet (pev->spawnflags, SF_DOOR_PASSABLE) )
		pev->solid		= SOLID_NOT;
	else
		pev->solid		= SOLID_BSP;

	pev->movetype	= MOVETYPE_PUSH;
	UTIL_SetOrigin( this, pev->origin );
	SET_MODEL(ENT(pev), STRING(pev->model) );

	if (pev->speed == 0)
		pev->speed = 100;

// DOOR_START_OPEN is to allow an entity to be lighted in the closed position
// but spawn in the open position
	if ( FBitSet (pev->spawnflags, SF_DOOR_START_OPEN) )
	{	// swap pos1 and pos2, put door at pos2, invert movement direction
		pev->angles = m_vecAngle2;
		Vector vecSav = m_vecAngle1;
		m_vecAngle2 = m_vecAngle1;
		m_vecAngle1 = vecSav;
		pev->movedir = pev->movedir * -1;
	}

	m_toggle_state = TS_AT_BOTTOM;

	// if the door is flagged for USE button activation only, use NULL touch function
	if ( FBitSet( pev->spawnflags, SF_DOOR_USE_ONLY ) && !FBitSet( pev->spawnflags, SF_DOOR_TOUCH_ONLY ) )
	{
		SetTouch( NULL );
	}
	else // touchable button
		SetTouch( &CBaseDoor::DoorTouch );
	RoundStore();
}


void CRotDoor :: SetToggleState( TOGGLE_STATE state )
{
	if ( state == TS_AT_TOP )
		pev->angles = m_vecAngle2;
	else
		pev->angles = m_vecAngle1;

	m_toggle_state = state;
	UTIL_SetOrigin( this, pev->origin );
}


class CMomentaryDoor : public CBaseToggle
{
public:
	void Activate() override { CBaseEntity::Activate(); }
	STATE GetState() override { return m_iState; }
	void	Spawn( void );
	void Precache( void );

	void	KeyValue( KeyValueData *pkvd );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return CBaseToggle :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void MomentaryMove();
	void MomentaryMoveDone();

	BYTE	m_bMoveSnd;			// sound a door makes while moving
	STATE m_iState;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( momentary_door, CMomentaryDoor );

TYPEDESCRIPTION	CMomentaryDoor::m_SaveData[] =
{
	DEFINE_FIELD( CMomentaryDoor, m_bMoveSnd, FIELD_CHARACTER ),
	DEFINE_FIELD( CMomentaryDoor, m_iState, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMomentaryDoor, CBaseToggle );

void CMomentaryDoor::Spawn( void )
{
	SetMovedir (pev);

	pev->solid		= SOLID_BSP;
	pev->movetype	= MOVETYPE_PUSH;

	UTIL_SetOrigin( this, pev->origin );
	SET_MODEL( ENT(pev), STRING(pev->model) );

	if (pev->dmg == 0)
		pev->dmg = 2;
	m_iState = STATE_OFF;

	m_vecPosition1	= pev->origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2	= m_vecPosition1 + (pev->movedir * (fabs( pev->movedir.x * (pev->size.x-2) ) + fabs( pev->movedir.y * (pev->size.y-2) ) + fabs( pev->movedir.z * (pev->size.z-2) ) - m_flLip));
	ASSERTSZ(m_vecPosition1 != m_vecPosition2, "door start/end positions are equal");

	if ( FBitSet (pev->spawnflags, SF_DOOR_START_OPEN) )
	{	// swap pos1 and pos2, put door at pos2
		UTIL_SetOrigin( this, m_vecPosition2 );
		Vector vecTemp = m_vecPosition2;
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = vecTemp;
	}
	SetTouch( NULL );

	Precache();
}

void CMomentaryDoor::Precache( void )
{

// set the door's "in-motion" sound
	switch (m_bMoveSnd)
	{
	case	0:
		pev->noiseMoving = ALLOC_STRING("common/null.wav");
		break;
	default:
		PRECACHE_SOUND( "doors/doormove.wav" );
		pev->noiseMoving = ALLOC_STRING("doors/doormove.wav");
		break;
	}
}

void CMomentaryDoor::KeyValue( KeyValueData *pkvd )
{

	if (FStrEq(pkvd->szKeyName, "movesnd"))
	{
		m_bMoveSnd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "stopsnd"))
	{
//		m_bStopSnd = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "healthvalue"))
	{
//		m_bHealthValue = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CMomentaryDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( useType != USE_SET )		// Momentary buttons will pass down a float in here
		return;

	if ( value > 1.0 )
		value = 1.0;

	Vector move = m_vecPosition1 + ( value * ( m_vecPosition2 - m_vecPosition1 ) );

	float speed = 0;

	if ( pev->speed )
	{
		speed = pev->speed;
	}
	else
	{
		Vector delta;
		delta = move - pev->origin;
		speed = delta.Length() * 10;
	}

	if ( m_iState == STATE_OFF )
	{
		m_iState = STATE_ON;
		EMIT_SOUND( ENT( pev ), CHAN_STATIC, (char *)STRING( pev->noiseMoving ), 1, ATTN_NORM );
		pev->nextthink = pev->ltime + 0.01;
	}
	else if ( pev->nextthink > pev->ltime + 1 )
		pev->nextthink = pev->ltime + 1;

	m_vecFinalDest = move;
	m_flLinearMoveSpeed = speed;
	SetThink( &CMomentaryDoor::MomentaryMove );
}

void CMomentaryDoor::MomentaryMove()
{
	Vector vecDest = m_vecFinalDest;

	if ( vecDest == pev->origin )
	{
		MomentaryMoveDone();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDest - pev->origin;

	float flTravelTime = vecDestDelta.Length() / m_flLinearMoveSpeed;

	pev->nextthink = flTravelTime + pev->ltime;
	SetThink( &CMomentaryDoor::MomentaryMoveDone );

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->velocity = vecDestDelta / flTravelTime;
}

void CMomentaryDoor::MomentaryMoveDone()
{
	ALERT( at_console, "THINK: MomentaryMove DONE\n" );

	Vector vecDest = m_vecFinalDest;
	pev->velocity = g_vecZero;
	UTIL_SetOrigin( this, vecDest );

	SetThink( NULL );
	pev->nextthink = -1;

	if ( m_pfnCallWhenMoveDone )
		( this->*m_pfnCallWhenMoveDone )();

	m_iState = STATE_OFF;
	STOP_SOUND( ENT( pev ), CHAN_STATIC, (char *)STRING( pev->noiseMoving ) );
}
