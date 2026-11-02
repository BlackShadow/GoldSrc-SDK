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

===== triggers.cpp ========================================================

  spawn and use functions for editor-placed triggers              

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "trains.h"			// trigger_camera has train functionality
#include "gamerules.h"
#include "dod_gamerules.h"
#include "objective.h"
#include "monsters.h"
#include "talkmonster.h"
#include "weapons.h"
#include "shake.h"

#define	SF_TRIGGER_PUSH_START_OFF	2//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_TARGETONCE	1// Only fire hurt target once
#define	SF_TRIGGER_HURT_START_OFF	2//spawnflag that makes trigger_push spawn turned OFF
#define	SF_TRIGGER_HURT_NO_CLIENTS	8//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_CLIENTONLYFIRE	16// trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32// only clients may touch this trigger.
#define SF_TRIGGER_ROUND_RESTART		512
#define SF_TRIGGER_NO_ALLIES			8
#define SF_TRIGGER_NO_AXIS			16
#define SF_TRIGGER_HURT_NO_ALLIES		64
#define SF_TRIGGER_HURT_NO_AXIS		128

extern DLL_GLOBAL BOOL		g_fGameOver;
extern int gmsgStartProgressBar;
extern int gmsgCancelProgressBar;
extern int gmsgTimerStatus;
extern int gmsgSetObjOwner;
extern int gmsgCapMsg;
extern int gmsgClientAreas;
extern int gmsgPStatus;
extern int gmsgStartProgressBarF;
extern int gmsgPlayersInArea;
extern int gmsgProgBarUpdate;

extern void SetMovedir(entvars_t* pev);
extern Vector VecBModelOrigin( entvars_t* pevBModel );

class CFrictionModifier : public CBaseEntity
{
public:
	void		Spawn( void );
	void		KeyValue( KeyValueData *pkvd );
	void EXPORT	ChangeFriction( CBaseEntity *pOther );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static	TYPEDESCRIPTION m_SaveData[];

	float		m_frictionFraction;		// Sorry, couldn't resist this name :)
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( func_friction, CFrictionModifier );

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CFrictionModifier::m_SaveData[] = 
{
	DEFINE_FIELD( CFrictionModifier, m_frictionFraction, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE(CFrictionModifier,CBaseEntity);


// Modify an entity's friction
void CFrictionModifier :: Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetTouch( &CFrictionModifier::ChangeFriction );
}


// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier :: ChangeFriction( CBaseEntity *pOther )
{
	if ( pOther->pev->movetype != MOVETYPE_BOUNCEMISSILE && pOther->pev->movetype != MOVETYPE_BOUNCE )
		pOther->pev->friction = m_frictionFraction;
}



// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "modifier"))
	{
		m_frictionFraction = atof(pkvd->szValue) / 100.0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


// This trigger will fire when the level spawns (or respawns if not fire once)
// It will check a global state before firing.  It supports delay and killtargets

#define SF_AUTO_FIREONCE		0x0001

class CAutoTrigger : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
	void Think( void );
	void RoundRespawn( void );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int m_iEnabled;
	int			m_globalstate;
	USE_TYPE	triggerType;
	bool b_isRoundRestart;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( trigger_auto, CAutoTrigger );

TYPEDESCRIPTION	CAutoTrigger::m_SaveData[] = 
{
	DEFINE_FIELD( CAutoTrigger, m_globalstate, FIELD_STRING ),
	DEFINE_FIELD( CAutoTrigger, triggerType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CAutoTrigger,CBaseDelay);

void CAutoTrigger::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "globalstate"))
	{
		m_globalstate = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi( pkvd->szValue );
		switch( type )
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}


void CAutoTrigger::Spawn( void )
{
	m_iEnabled = 1;
	b_isRoundRestart = false;
	Precache();
	RoundStore();
}

void CAutoTrigger::RoundRespawn( void )
{
	m_iEnabled = 1;
	if ( pev->spawnflags & 512 )
	{
		pev->nextthink = gpGlobals->time + 0.1;
		b_isRoundRestart = true;
	}
}


void CAutoTrigger::Precache( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
}


void CAutoTrigger::Think( void )
{
	if ( m_iEnabled )
	{
		if ( ( pev->spawnflags & 512 ) && g_pGameRules->IsMultiplayer() == 1 )
		{
			if ( b_isRoundRestart && ( !m_globalstate || gGlobalState.EntityGetState( m_globalstate ) == GLOBAL_ON ) )
			{
				SUB_UseTargets( this, triggerType, 0 );
				if ( pev->spawnflags & SF_AUTO_FIREONCE )
					m_iEnabled = 0;
			}
		}
		else if ( !m_globalstate || gGlobalState.EntityGetState( m_globalstate ) == GLOBAL_ON )
		{
			SUB_UseTargets( this, triggerType, 0 );
			if ( pev->spawnflags & SF_AUTO_FIREONCE )
				m_iEnabled = 0;
		}
	}
}



#define SF_RELAY_FIREONCE		0x0001

class CTriggerRelay : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void RoundRespawn( void ) { m_iEnabled = 1; }
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int m_iEnabled;
	USE_TYPE m_triggerType;
	string_t m_sMaster;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( trigger_relay, CTriggerRelay );

TYPEDESCRIPTION	CTriggerRelay::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerRelay, m_triggerType, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerRelay, m_sMaster, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CTriggerRelay,CBaseDelay);

void CTriggerRelay::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
		return;
	}
	if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi( pkvd->szValue );
		switch( type )
		{
		case 0:
			m_triggerType = USE_OFF;
			break;
		case 2:
			m_triggerType = USE_TOGGLE;
			break;
		case 4:
			m_triggerType = USE_KILL;
			break;
		case 5:
			pev->spawnflags |= 0x80000000;
			break;
		default:
			m_triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}


void CTriggerRelay::Spawn( void )
{
	m_iEnabled = 1;
	RoundStore();
}




void CTriggerRelay::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !m_iEnabled )
		return;

	if ( !UTIL_IsMasterTriggered( m_sMaster, pActivator ) )
	{
		if ( pev->spawnflags & 2 )
		{
			if ( pev->targetname )
				ALERT( at_console, "DEBUG: trigger_relay %s wasn't activated: locked by master %s\n",
					STRING(pev->targetname), STRING(m_sMaster) );
			else
				ALERT( at_console, "DEBUG: unnamed trigger_relay wasn't activated: locked by master %s\n", STRING(m_sMaster) );
		}
		return;
	}

	if ( pev->spawnflags & 2 )
	{
		if ( pev->targetname )
			ALERT( at_console, "DEBUG: trigger_relay %s was activated: ", STRING(pev->targetname) );
		else
			ALERT( at_console, "DEBUG: unnamed trigger_relay was activated: " );
	}

	if ( pev->spawnflags & 0x80000000 )
	{
		if ( pev->spawnflags & 2 )
		{
			if ( m_flDelay != 0 )
				ALERT( at_console, "will send %s in %f seconds.\n", GetStringForUseType( useType ), m_flDelay );
			else
				ALERT( at_console, "sending %s now.\n", GetStringForUseType( useType ) );
		}

		SUB_UseTargets( this, useType, value );
	}
	else
	{
		if ( pev->spawnflags & 2 )
		{
			if ( m_flDelay != 0 )
				ALERT( at_console, "will send %s in %f seconds.\n", GetStringForUseType( m_triggerType ), m_flDelay );
			else
				ALERT( at_console, "sending %s now.\n", GetStringForUseType( m_triggerType ) );
		}

		SUB_UseTargets( this, m_triggerType, 0 );
	}

	if ( pev->spawnflags & SF_RELAY_FIREONCE )
		m_iEnabled = 0;
}


//**********************************************************
// The Multimanager Entity - when fired, will fire up to 16 targets 
// at specified times.
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define SF_MULTIMAN_CLONE		0x80000000
#define SF_MULTIMAN_THREAD		0x00000001
#define SF_MULTIMAN_LOOP			0x00000002
#define SF_MULTIMAN_ONCE			0x00000004
#define SF_MULTIMAN_SETSTATE		0x20000000
#define SF_MULTIMAN_SAMESTATE		0x40000000

class CMultiManager : public CBaseEntity
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn ( void );
	void RoundRespawn( void );
	STATE GetState( void ) { return m_iState; }
	void EXPORT ManagerThink ( void );
	void EXPORT ManagerUse   ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );


	BOOL		HasTarget( string_t targetname );
	
	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	STATE m_iState;
	int		m_cTargets;	// the total number of targets in this manager's fire list.
	int		m_index;	// Current target
	float	m_startTime;// Time we started firing
	int		m_iTargetName	[ MAX_MULTI_TARGETS ];// list if indexes into global string array
	float	m_flTargetDelay [ MAX_MULTI_TARGETS ];// delay (in seconds) from time of manager fire to target fire
	float m_flWait;
	float m_flMaxWait;
	string_t m_sMaster;
	int m_iMode;
	int m_rscTargets;
	int m_rsindex;
	float m_rsstartTime;
	int m_rsiTargetName[MAX_MULTI_TARGETS];
	float m_rsflTargetDelay[MAX_MULTI_TARGETS];
	float m_rsflWait;
	float m_rsflMaxWait;
	string_t m_rssMaster;
	int m_rsiMode;
	EHANDLE m_hActivator;
	USE_TYPE m_triggerType;
private:
	inline BOOL IsClone( void ) { return (pev->spawnflags & SF_MULTIMAN_CLONE) ? TRUE : FALSE; }
	inline BOOL ShouldClone( void ) 
	{ 
		if ( IsClone() )
			return FALSE;

		return (pev->spawnflags & SF_MULTIMAN_THREAD) ? TRUE : FALSE; 
	}

	CMultiManager *Clone( void );
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( multi_manager, CMultiManager );

// Global Savedata for multi_manager
TYPEDESCRIPTION	CMultiManager::m_SaveData[] = 
{
	DEFINE_FIELD( CMultiManager, m_cTargets, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_index, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_iMode, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_startTime, FIELD_TIME ),
	DEFINE_FIELD( CMultiManager, m_triggerType, FIELD_INTEGER ),
	DEFINE_ARRAY( CMultiManager, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS ),
	DEFINE_ARRAY( CMultiManager, m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS ),
	DEFINE_FIELD( CMultiManager, m_sMaster, FIELD_STRING ),
	DEFINE_FIELD( CMultiManager, m_hActivator, FIELD_EHANDLE ),
	DEFINE_FIELD( CMultiManager, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CMultiManager, m_flMaxWait, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE(CMultiManager,CBaseEntity);

void CMultiManager :: KeyValue( KeyValueData *pkvd )
{
	// UNDONE: Maybe this should do something like this:
	//CBaseToggle::KeyValue( pkvd );
	// if ( !pkvd->fHandled )
	// ... etc.

	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "maxwait" ) )
	{
		m_flMaxWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "mode" ) )
	{
		m_iMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		switch ( atoi(pkvd->szValue) )
		{
		case 4: pev->spawnflags |= SF_MULTIMAN_SAMESTATE; break;
		case 1: m_triggerType = USE_ON; break;
		case 2: m_triggerType = USE_OFF; break;
		case 3: m_triggerType = USE_KILL; break;
		default: m_triggerType = USE_TOGGLE; break;
		}
		pev->spawnflags |= SF_MULTIMAN_SETSTATE;
		pkvd->fHandled = TRUE;
	}
	else // add this field to the target list
	{
		// this assumes that additional fields are targetnames and their values are delay values.
		if ( m_cTargets < MAX_MULTI_TARGETS )
		{
			char tmp[256];

			UTIL_StripToken( pkvd->szKeyName, tmp );
			m_iTargetName [ m_cTargets ] = ALLOC_STRING( tmp );
			m_flTargetDelay [ m_cTargets ] = atof (pkvd->szValue);
			m_cTargets++;
			pkvd->fHandled = TRUE;
		}
		else
			ALERT( at_error, "Too many targets on multi_manager %s\n", STRING(pev->targetname) );
	}
}


void CMultiManager :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	SetUse ( &CMultiManager::ManagerUse );
	SetThink ( &CMultiManager::ManagerThink);
	m_iState = STATE_OFF;

	if ( !(pev->spawnflags & SF_MULTIMAN_SETSTATE) )
		m_triggerType = USE_TOGGLE;

	// Sort targets
	// Quick and dirty bubble sort
	int swapped = 1;
	int i;

	m_rscTargets = m_cTargets;
	m_rsindex = m_index;
	m_rsstartTime = m_startTime;
	m_rsflWait = m_flWait;
	m_rsflMaxWait = m_flMaxWait;
	m_rssMaster = m_sMaster;
	m_rsiMode = m_iMode;

	for ( i = 0; i < MAX_MULTI_TARGETS; i++ )
	{
		m_rsiTargetName[i] = m_iTargetName[i];
		m_rsflTargetDelay[i] = m_flTargetDelay[i];
	}

	while ( swapped )
	{
		swapped = 0;
		for ( i = 1; i < m_cTargets; i++ )
		{
			if ( m_flTargetDelay[i] < m_flTargetDelay[i-1] )
			{
				// Swap out of order elements
				int name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				m_iTargetName[i] = m_iTargetName[i-1];
				m_flTargetDelay[i] = m_flTargetDelay[i-1];
				m_iTargetName[i-1] = name;
				m_flTargetDelay[i-1] = delay;
				swapped = 1;
			}
		}
	}

	RoundStore();
}


BOOL CMultiManager::HasTarget( string_t targetname )
{ 
	for ( int i = 0; i < m_cTargets; i++ )
		if ( FStrEq(STRING(targetname), STRING(m_iTargetName[i])) )
			return TRUE;
	
	return FALSE;
}


// Designers were using this to fire targets that may or may not exist -- 
// so I changed it to use the standard target fire code, made it a little simpler.
void CMultiManager :: ManagerThink ( void )
{
	if ( m_iMode )
	{
		int i = 0;

		if ( m_iMode == 1 )
		{
			float total = 0;

			for ( i = 0; i < m_cTargets; i++ )
				total += m_flTargetDelay[i];

			if ( total == 0 )
			{
				FireTargets( STRING( m_iTargetName[ RANDOM_LONG( 0, m_cTargets - 1 ) ] ), m_hActivator, this, m_triggerType, 0 );
			}
			else
			{
				float choice = RANDOM_FLOAT( 0, total );
				float weight = 0;

				for ( i = 0; i < m_cTargets; i++ )
				{
					weight += m_flTargetDelay[i];

					if ( weight >= choice )
					{
						FireTargets( STRING( m_iTargetName[i] ), m_hActivator, this, m_triggerType, 0 );
						break;
					}
				}
			}
		}
		else
		{
			for ( i = 0; i < m_cTargets; i++ )
			{
				if ( RANDOM_LONG( 0, 100 ) <= m_flTargetDelay[i] )
					FireTargets( STRING( m_iTargetName[i] ), m_hActivator, this, m_triggerType, 0 );
			}
		}

		m_iState = STATE_OFF;
		SetThink( NULL );

		if ( IsClone() || ( pev->spawnflags & SF_MULTIMAN_ONCE ) )
		{
			UTIL_Remove( this );
			return;
		}

		SetUse( &CMultiManager::ManagerUse );
	}
	else
	{
		float time = gpGlobals->time - m_startTime;

		while ( m_index < m_cTargets && m_flTargetDelay[ m_index ] <= time )
		{
			FireTargets( STRING( m_iTargetName[ m_index ] ), m_hActivator, this, m_triggerType, 0 );
			m_index++;
		}

		if ( m_index >= m_cTargets )// have we fired all targets?
		{
			if ( pev->spawnflags & SF_MULTIMAN_LOOP )
			{
				m_index = 0;

				if ( m_flMaxWait != 0 )
				{
					m_startTime = gpGlobals->time + RANDOM_FLOAT( m_flWait, m_flMaxWait );
					m_iState = STATE_TURN_ON;
				}
				else if ( m_flWait != 0 )
				{
					m_startTime = gpGlobals->time + m_flWait;
					m_iState = STATE_TURN_ON;
				}
				else
				{
					m_startTime = gpGlobals->time;
					m_iState = STATE_ON;
				}

				pev->nextthink = m_startTime;
			}
			else
			{
				m_iState = STATE_OFF;
				SetThink( NULL );

				if ( IsClone() || ( pev->spawnflags & SF_MULTIMAN_ONCE ) )
				{
					UTIL_Remove( this );
					return;
				}

				SetUse( &CMultiManager::ManagerUse );// allow manager re-use
			}
		}
		else
		{
			m_iState = STATE_ON;
			pev->nextthink = m_startTime + m_flTargetDelay[ m_index ];
		}
	}
}

CMultiManager *CMultiManager::Clone( void )
{
	CMultiManager *pMulti = GetClassPtr( (CMultiManager *)NULL );

	edict_t *pEdict = pMulti->pev->pContainingEntity;
	memcpy( pMulti->pev, pev, sizeof(*pev) );
	pMulti->pev->pContainingEntity = pEdict;

	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE;
	pMulti->m_cTargets = m_cTargets;
	pMulti->m_triggerType = m_triggerType;
	pMulti->m_iMode = m_iMode;
	memcpy( pMulti->m_iTargetName, m_iTargetName, sizeof( m_iTargetName ) );
	memcpy( pMulti->m_flTargetDelay, m_flTargetDelay, sizeof( m_flTargetDelay ) );

	return pMulti;
}


// The USE function builds the time table and starts the entity thinking.
void CMultiManager :: ManagerUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( pev->spawnflags & SF_MULTIMAN_LOOP )
	{
		if ( m_iState != STATE_OFF )
		{
			if ( useType != USE_ON )
			{
				m_iState = STATE_OFF;
				SetThink( NULL );
			}

			return;
		}

		if ( useType == USE_OFF )
			return;
	}

	if ( !UTIL_IsMasterTriggered( m_sMaster, pActivator ) )
		return;

	// In multiplayer games, clone the MM and execute in the clone (like a thread)
	// to allow multiple players to trigger the same multimanager
	if ( ShouldClone() )
	{
		CMultiManager *pClone = Clone();
		pClone->ManagerUse( pActivator, pCaller, useType, value );
		return;
	}

	m_hActivator = pActivator;
	m_index = 0;

	if ( m_flMaxWait != 0 )
	{
		m_startTime = gpGlobals->time + RANDOM_FLOAT( m_flWait, m_flMaxWait );
		m_iState = STATE_TURN_ON;
	}
	else if ( m_flWait != 0 )
	{
		m_startTime = gpGlobals->time + m_flWait;
		m_iState = STATE_TURN_ON;
	}
	else
	{
		m_startTime = gpGlobals->time;
		m_iState = STATE_ON;
	}

	if ( m_cTargets > 0 && m_flTargetDelay[0] < 0 )
		m_startTime += m_flTargetDelay[0];

	if ( pev->spawnflags & SF_MULTIMAN_SAMESTATE )
		m_triggerType = useType;

	if ( !(pev->spawnflags & SF_MULTIMAN_LOOP) )
		SetUse( NULL );

	SetThink ( &CMultiManager::ManagerThink );
	pev->nextthink = m_startTime;
}

void CMultiManager::RoundRespawn( void )
{
	int i;

	m_cTargets = m_rscTargets;
	m_index = m_rsindex;
	m_startTime = m_rsstartTime;
	m_flWait = m_rsflWait;
	m_flMaxWait = m_rsflMaxWait;
	m_sMaster = m_rssMaster;
	m_iMode = m_rsiMode;

	for ( i = 0; i < MAX_MULTI_TARGETS; i++ )
	{
		m_iTargetName[i] = m_rsiTargetName[i];
		m_flTargetDelay[i] = m_rsflTargetDelay[i];
	}

	SetUse( &CMultiManager::ManagerUse );
	SetThink( &CMultiManager::ManagerThink );
	m_iState = STATE_OFF;

	if ( !(pev->spawnflags & SF_MULTIMAN_SETSTATE) )
		m_triggerType = USE_TOGGLE;

	int swapped = 1;

	while ( swapped )
	{
		swapped = 0;
		for ( i = 1; i < m_cTargets; i++ )
		{
			if ( m_flTargetDelay[i] < m_flTargetDelay[i-1] )
			{
				int name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				m_iTargetName[i] = m_iTargetName[i-1];
				m_flTargetDelay[i] = m_flTargetDelay[i-1];
				m_iTargetName[i-1] = name;
				m_flTargetDelay[i-1] = delay;
				swapped = 1;
			}
		}
	}
}

//***********************************************************


//
// Render parameters trigger
//
// This entity will copy its render parameters (renderfx, rendermode, rendercolor, renderamt)
// to its targets when triggered.
//


// Flags to indicate masking off various render parameters that are normally copied to the targets
#define SF_RENDER_MASKFX	(1<<0)
#define SF_RENDER_MASKAMT	(1<<1)
#define SF_RENDER_MASKMODE	(1<<2)
#define SF_RENDER_MASKCOLOR	(1<<3)

class CRenderFxManager : public CBaseEntity
{
public:
	void Spawn( void );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( env_render, CRenderFxManager );


void CRenderFxManager :: Spawn ( void )
{
	pev->solid = SOLID_NOT;
	RoundStore();
}

void CRenderFxManager :: Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!FStringNull(pev->target))
	{
		CBaseEntity *pTarget = NULL;
		while ( 1 )
		{
			pTarget = UTIL_FindEntityByTargetname( pTarget, STRING(pev->target) );
			if ( !pTarget )
				break;

			entvars_t *pevTarget = pTarget->pev;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKFX ) )
				pevTarget->renderfx = pev->renderfx;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKAMT ) )
				pevTarget->renderamt = pev->renderamt;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKMODE ) )
				pevTarget->rendermode = pev->rendermode;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKCOLOR ) )
				pevTarget->rendercolor = pev->rendercolor;
		}
	}
}



#define SF_WATCHER_TOGGLE		1
#define SF_WATCHER_NO_ON			2
#define SF_WATCHER_NO_OFF			4
#define SF_WATCHER_NOT_ON			8
#define SF_WATCHER_OFF			16
#define SF_WATCHER_TRANSITION		32
#define SF_WATCHER_IN_USE			128
#define SF_WATCHER_ACTIVE			512

#define LOGIC_AND				0
#define LOGIC_OR				1
#define LOGIC_NAND				2
#define LOGIC_NOR				3
#define LOGIC_XOR				4
#define LOGIC_XNOR				5

class CStateWatcher : public CBaseToggle
{
public:
	void Spawn( void );
	void RoundRespawn( void ) { pev->nextthink = gpGlobals->time + 0.5; }
	void Think( void );
	void KeyValue( KeyValueData *pkvd );
	BOOL EvalLogic( void );
	STATE GetState( void ) { return (pev->spawnflags & SF_WATCHER_ACTIVE) ? STATE_ON : STATE_OFF; }
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int m_fLogic;
	int m_cTargets;
	int m_iTargetName[MAX_MULTI_TARGETS];
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( multi_watcher, CStateWatcher );
LINK_ENTITY_TO_CLASS( watcher, CStateWatcher );

TYPEDESCRIPTION CStateWatcher::m_SaveData[] =
{
	DEFINE_FIELD( CStateWatcher, m_fLogic, FIELD_INTEGER ),
	DEFINE_FIELD( CStateWatcher, m_cTargets, FIELD_INTEGER ),
	DEFINE_ARRAY( CStateWatcher, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS ),
};

IMPLEMENT_SAVERESTORE( CStateWatcher, CBaseToggle );

void CStateWatcher::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->nextthink = gpGlobals->time + 0.5;
	RoundStore();
}

void CStateWatcher::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "m_fLogic" ) )
	{
		m_fLogic = (int)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iszWatch" ) )
	{
		if ( m_cTargets < MAX_MULTI_TARGETS )
		{
			m_iTargetName[ m_cTargets ] = ALLOC_STRING( pkvd->szValue );
			m_cTargets++;
			pkvd->fHandled = TRUE;
		}
		else
		{
			if ( pev->targetname )
				ALERT( at_console, "%s: Too many targets for %s %s (limit is %d)\n", pkvd->szKeyName,
					STRING(pev->classname), STRING(pev->targetname), MAX_MULTI_TARGETS );
			else
				ALERT( at_console, "%s: Too many targets for unnamed %s (limit is %d)\n", pkvd->szKeyName,
					STRING(pev->classname), MAX_MULTI_TARGETS );
		}
	}
	else // add this field to the target list
	{
		// this assumes that additional fields are targetnames and their values are delay values.
		if ( m_cTargets < MAX_MULTI_TARGETS )
		{
			char tmp[256];
			UTIL_StripToken( pkvd->szKeyName, tmp );
			m_iTargetName[ m_cTargets ] = ALLOC_STRING( tmp );
			m_cTargets++;
			pkvd->fHandled = TRUE;
		}
		else
		{
			if ( pev->targetname )
				ALERT( at_console, "%s: Too many targets for %s %s (limit is %d)\n", pkvd->szKeyName,
					STRING(pev->classname), STRING(pev->targetname), MAX_MULTI_TARGETS );
			else
				ALERT( at_console, "%s: Too many targets for unnamed %s (limit is %d)\n", pkvd->szKeyName,
					STRING(pev->classname), MAX_MULTI_TARGETS );
		}
	}
}

BOOL CStateWatcher::EvalLogic( void )
{
	int i;
	BOOL b;
	BOOL xorgot = FALSE;

	CBaseEntity *pEntity;

	for ( i = 0; i < m_cTargets; i++ )
	{
		pEntity = UTIL_FindEntityByTargetname( NULL, STRING( m_iTargetName[i] ) );
		if ( pEntity != NULL )
		{
			b = FALSE;
			switch ( pEntity->GetState() )
			{
			case STATE_ON:		 if ( !( pev->spawnflags & SF_WATCHER_NOT_ON ) )	b = TRUE; break;
			case STATE_OFF:		 if ( pev->spawnflags & SF_WATCHER_OFF )		b = TRUE; break;
			case STATE_TURN_ON:	 if ( pev->spawnflags & SF_WATCHER_TRANSITION )	b = TRUE; break;
			case STATE_TURN_OFF: if ( pev->spawnflags & SF_WATCHER_TRANSITION )	b = TRUE; break;
			case STATE_IN_USE:	 if ( pev->spawnflags & SF_WATCHER_IN_USE )	b = TRUE; break;
			}
			if ( b )
			{
				switch ( m_fLogic )
				{
				case LOGIC_OR:
					return TRUE;
				case LOGIC_NOR:
					return FALSE;
				case LOGIC_XOR:
					if ( xorgot )
						return FALSE;
					xorgot = TRUE;
					break;
				case LOGIC_XNOR:
					if ( xorgot )
						return TRUE;
					xorgot = TRUE;
					break;
				}
			}
			else
			{
				switch ( m_fLogic )
				{
				case LOGIC_AND:
					return FALSE;
				case LOGIC_NAND:
					return TRUE;
				}
			}
		}
	}
	switch ( m_fLogic )
	{
	case LOGIC_AND:
	case LOGIC_NOR:
		return TRUE;
	case LOGIC_XOR:
		return xorgot;
	case LOGIC_XNOR:
		return !xorgot;
	default:
		return FALSE;
	}
}

void CStateWatcher::Think( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	int oldflag = pev->spawnflags & SF_WATCHER_ACTIVE;

	if ( EvalLogic() )
		pev->spawnflags |= SF_WATCHER_ACTIVE;
	else
		pev->spawnflags &= ~SF_WATCHER_ACTIVE;

	if ( ( pev->spawnflags & SF_WATCHER_ACTIVE ) != oldflag )
	{
		if ( oldflag )
		{
			if ( !( pev->spawnflags & SF_WATCHER_NO_OFF ) )
			{
				if ( pev->spawnflags & SF_WATCHER_TOGGLE )
					SUB_UseTargets( this, USE_TOGGLE, 0 );
				else
					SUB_UseTargets( this, USE_OFF, 0 );
			}
		}
		else
		{
			if ( !( pev->spawnflags & SF_WATCHER_NO_ON ) )
			{
				if ( pev->spawnflags & SF_WATCHER_TOGGLE )
					SUB_UseTargets( this, USE_TOGGLE, 0 );
				else
					SUB_UseTargets( this, USE_ON, 0 );
			}
		}
	}
}

class CEnvCustomize : public CBaseEntity
{
public:
	void Spawn( void );
	void Think( void ) { Use( this, this, USE_TOGGLE, 0 ); }
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float m_flRadius;
	int m_iszModel;
	int m_iClass;
	int m_iPrisoner;
	int m_iPlayerAlly;
	int m_iVisible;
	int m_iSolid;
	int m_iProvoked;
	int m_voicePitch;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( env_customize, CEnvCustomize );

TYPEDESCRIPTION CEnvCustomize::m_SaveData[] =
{
	DEFINE_FIELD( CEnvCustomize, m_flRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvCustomize, m_iszModel, FIELD_STRING ),
	DEFINE_FIELD( CEnvCustomize, m_iClass, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_iPrisoner, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_iPlayerAlly, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_iVisible, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_iSolid, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_iProvoked, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvCustomize, m_voicePitch, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CEnvCustomize, CBaseEntity );

void CEnvCustomize::Spawn( void )
{
	pev->solid = SOLID_NOT;
	if ( m_iszModel )
		PRECACHE_MODEL( (char *)STRING(m_iszModel) );
	if ( !pev->targetname )
		pev->nextthink = gpGlobals->time + 0.1;
}

void CEnvCustomize::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "m_iVisible" ) )
	{
		m_iVisible = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iSolid" ) )
	{
		m_iSolid = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iszModel" ) )
	{
		m_iszModel = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_voicePitch" ) )
	{
		m_voicePitch = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iPlayerAlly" ) )
	{
		m_iPlayerAlly = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iPrisoner" ) )
	{
		m_iPrisoner = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iClass" ) )
	{
		m_iClass = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_flRadius" ) )
	{
		m_flRadius = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iProvoked" ) )
	{
		m_iProvoked = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvCustomize::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pEntity = UTIL_FindEntityGeneric( STRING(pev->target), pev->origin, m_flRadius );
	if ( !pEntity )
		return;

	CBaseMonster *pMonster = pEntity->MyMonsterPointer();
	if ( !(pev->spawnflags & 1) && pMonster && pMonster->m_MonsterState == MONSTERSTATE_DEAD )
		return;

	if ( m_iszModel )
	{
		Vector mins;
		Vector maxs;
		mins = pEntity->pev->mins;
		maxs = pEntity->pev->maxs;
		SET_MODEL( pEntity->edict(), STRING(m_iszModel) );
		UTIL_SetSize( pEntity->pev, mins, maxs );
	}

	if ( pev->body != -1 )
		pEntity->pev->body = pev->body;

	if ( pev->skin != -1 )
	{
		if ( pev->skin == -2 )
		{
			if ( pEntity->pev->skin )
				pEntity->pev->skin = 0;
			else
				pEntity->pev->skin = 1;
		}
		else if ( pev->skin == -99 )
			pEntity->pev->skin = -1;
		else
			pEntity->pev->skin = pev->skin;
	}

	if ( m_iVisible )
	{
		if ( ( pEntity->pev->effects & EF_NODRAW ) && ( m_iVisible == 3 || m_iVisible == 2 ) )
			pEntity->pev->effects &= ~EF_NODRAW;
		else if ( m_iVisible != 2 )
			pEntity->pev->effects |= EF_NODRAW;
	}

	if ( m_iSolid )
	{
		if ( pEntity->pev->solid == SOLID_NOT && ( m_iSolid == 3 || m_iSolid == 2 ) )
		{
			if ( pMonster )
				pEntity->pev->solid = SOLID_SLIDEBOX;
			else
				pEntity->pev->solid = SOLID_BSP;
		}
		else if ( m_iSolid != 2 )
			pEntity->pev->solid = SOLID_NOT;
	}

	if ( !pMonster )
	{
		if ( pev->spawnflags & 2 )
			UTIL_Remove( this );
		return;
	}

	if ( m_voicePitch > 0 && ( FClassnameIs( pEntity->pev, "monster_barney" ) ||
		FClassnameIs( pEntity->pev, "monster_scientist" ) || FClassnameIs( pEntity->pev, "monster_sitting_scientist" ) ) )
		((CTalkMonster *)pEntity)->m_voicePitch = m_voicePitch;

	if ( m_iClass )
	{
		pMonster->m_iClass = m_iClass;
		if ( pMonster->m_hEnemy )
		{
			pMonster->m_hEnemy = NULL;
			pMonster->SetConditions( bits_COND_NEW_ENEMY );
		}
	}

	if ( m_iPrisoner )
	{
		if ( ( pMonster->pev->spawnflags & SF_MONSTER_PRISONER ) && ( m_iPrisoner == 3 || m_iPrisoner == 2 ) )
			pMonster->pev->spawnflags &= ~SF_MONSTER_PRISONER;
		else if ( m_iPrisoner != 2 )
		{
			pMonster->pev->spawnflags |= SF_MONSTER_PRISONER;
			if ( pMonster->m_hEnemy )
			{
				pMonster->m_hEnemy = NULL;
				pMonster->SetConditions( bits_COND_NEW_ENEMY );
			}
		}
	}

	if ( m_iPlayerAlly )
	{
		if ( ( pMonster->pev->spawnflags & SF_MONSTER_WAIT_UNTIL_PROVOKED ) && ( m_iPlayerAlly == 3 || m_iPlayerAlly == 2 ) )
			pMonster->pev->spawnflags &= ~SF_MONSTER_WAIT_UNTIL_PROVOKED;
		else if ( m_iPlayerAlly != 2 )
			pMonster->pev->spawnflags |= SF_MONSTER_WAIT_UNTIL_PROVOKED;
	}

	if ( m_iProvoked )
	{
		if ( ( pMonster->m_afMemory & bits_MEMORY_PROVOKED ) && ( m_iProvoked == 3 || m_iProvoked == 2 ) )
			pMonster->Forget( bits_MEMORY_PROVOKED );
		else if ( m_iProvoked != 2 )
			pMonster->Remember( bits_MEMORY_PROVOKED );
	}

	if ( pev->spawnflags & 2 )
		UTIL_Remove( this );
}

class CBaseTrigger : public CBaseToggle
{
public:
	void EXPORT TeleportTouch ( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT MultiTouch( CBaseEntity *pOther );
	void EXPORT HurtTouch ( CBaseEntity *pOther );
	void ActivateMultiTrigger( CBaseEntity *pActivator );
	void EXPORT MultiWaitOver( void );
	void EXPORT CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void InitTrigger( void );

	virtual int	ObjectCaps( void ) { return CBaseToggle :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS( trigger, CBaseTrigger );

/*
================
InitTrigger
================
*/
void CBaseTrigger::InitTrigger( )
{
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if (pev->angles != g_vecZero)
		SetMovedir(pev);
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	if ( CVAR_GET_FLOAT("showtriggers") == 0 )
		SetBits( pev->effects, EF_NODRAW );
}


//
// Cache user-entity-field values until spawn is called.
//

void CBaseTrigger :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "count"))
	{
		m_cTriggersLeft = (int) atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		m_bitsDamageInflict = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "teamonly" ) )
	{
		m_team = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

class CTriggerHurt : public CBaseTrigger
{
public:
	void Spawn( void );
	void RoundRespawn( void );
	void EXPORT RadiationThink( void );
};

LINK_ENTITY_TO_CLASS( trigger_hurt, CTriggerHurt );

//
// trigger_monsterjump
//
class CTriggerMonsterJump : public CBaseTrigger
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void Think( void );
};

LINK_ENTITY_TO_CLASS( trigger_monsterjump, CTriggerMonsterJump );


void CTriggerMonsterJump :: Spawn ( void )
{
	SetMovedir ( pev );
	
	InitTrigger ();

	pev->nextthink = 0;
	pev->speed = 200;
	m_flHeight = 150;

	if ( !FStringNull ( pev->targetname ) )
	{// if targetted, spawn turned off
		pev->solid = SOLID_NOT;
		UTIL_SetOrigin( this, pev->origin ); // Unlink from trigger list
		SetUse( &CTriggerMonsterJump::ToggleUse );
	}
}


void CTriggerMonsterJump :: Think( void )
{
	pev->solid = SOLID_NOT;// kill the trigger for now !!!UNDONE
	UTIL_SetOrigin( this, pev->origin ); // Unlink from trigger list
	SetThink( NULL );
}

void CTriggerMonsterJump :: Touch( CBaseEntity *pOther )
{
	entvars_t *pevOther = pOther->pev;

	if ( !FBitSet ( pevOther->flags , FL_MONSTER ) ) 
	{// touched by a non-monster.
		return;
	}

	pevOther->origin.z += 1;
	
	if ( FBitSet ( pevOther->flags, FL_ONGROUND ) ) 
	{// clear the onground so physics don't bitch
		pevOther->flags &= ~FL_ONGROUND;
	}

	// toss the monster!
	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += m_flHeight;
	pev->nextthink = gpGlobals->time;
}


//=====================================
//
// trigger_cdaudio - starts/stops cd audio tracks
//
class CTriggerCDAudio : public CBaseTrigger
{
public:
	void Spawn( void );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void PlayTrack( void );
	void Touch ( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_cdaudio, CTriggerCDAudio );

//
// Changes tracks or stops CD when player touches
//
// !!!HACK - overloaded HEALTH to avoid adding new field
void CTriggerCDAudio :: Touch ( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
	{// only clients may trigger these events
		return;
	}

	PlayTrack();
}

void CTriggerCDAudio :: Spawn( void )
{
	InitTrigger();
}

void CTriggerCDAudio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	PlayTrack();
}

void PlayCDTrack( int iTrack )
{
	edict_t *pClient;
	
	// manually find the single player. 
	pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	
	// Can't play if the client is not connected!
	if ( !pClient )
		return;

	if ( iTrack < -1 || iTrack > 30 )
	{
		ALERT ( at_console, "TriggerCDAudio - Track %d out of range\n" );
		return;
	}

	if ( iTrack == -1 )
	{
		CLIENT_COMMAND ( pClient, "cd stop\n");
	}
	else
	{
		char string [ 64 ];

		sprintf( string, "cd play %3d\n", iTrack );
		CLIENT_COMMAND ( pClient, string);
	}
}


// only plays for ONE client, so only use in single play!
void CTriggerCDAudio :: PlayTrack( void )
{
	PlayCDTrack( (int)pev->health );
	
	SetTouch( NULL );
	UTIL_Remove( this );
}


// This plays a CD track when fired or when the player enters it's radius
class CTargetCDAudio : public CPointEntity
{
public:
	void			Spawn( void );
	void			KeyValue( KeyValueData *pkvd );

	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void			Think( void );
	void			Play( void );
};

LINK_ENTITY_TO_CLASS( target_cdaudio, CTargetCDAudio );

void CTargetCDAudio :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		pev->scale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTargetCDAudio :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if ( pev->scale > 0 )
		pev->nextthink = gpGlobals->time + 1.0;
}

void CTargetCDAudio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Play();
}

// only plays for ONE client, so only use in single play!
void CTargetCDAudio::Think( void )
{
	edict_t *pClient;
	
	// manually find the single player. 
	pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	
	// Can't play if the client is not connected!
	if ( !pClient )
		return;
	
	pev->nextthink = gpGlobals->time + 0.5;

	if ( (pClient->v.origin - pev->origin).Length() <= pev->scale )
		Play();

}

void CTargetCDAudio::Play( void ) 
{ 
	PlayCDTrack( (int)pev->health );
	UTIL_Remove(this); 
}

//=====================================

//
// trigger_hurt - hurts anything that touches it. if the trigger has a targetname, firing it will toggle state
//
//int gfToggleState = 0; // used to determine when all radiation trigger hurts have called 'RadiationThink'

void CTriggerHurt :: Spawn( void )
{
	InitTrigger();
	SetTouch ( &CTriggerHurt::HurtTouch );

	if ( !FStringNull ( pev->targetname ) )
	{
		SetUse ( &CTriggerHurt::ToggleUse );
	}
	else
	{
		SetUse ( NULL );
	}

	if (m_bitsDamageInflict & DMG_RADIATION)
	{
		SetThink ( &CTriggerHurt::RadiationThink );
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.0, 0.5); 
	}

	if ( FBitSet (pev->spawnflags, SF_TRIGGER_HURT_START_OFF) )// if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin( this, pev->origin );
	RoundStore();
}

void CTriggerHurt::RoundRespawn( void )
{
	InitTrigger();
	SetTouch( &CBaseTrigger::HurtTouch );
	if ( !FStringNull( pev->targetname ) )
		SetUse( &CBaseTrigger::ToggleUse );
	else
		SetUse( NULL );
	if ( m_bitsDamageInflict & DMG_RADIATION )
	{
		SetThink( &CTriggerHurt::RadiationThink );
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0, 0.5 );
	}
	if ( pev->spawnflags & SF_TRIGGER_HURT_START_OFF )
		pev->solid = SOLID_NOT;
}

// trigger hurt that causes radiation will do a radius
// check and set the player's geiger counter level
// according to distance from center of trigger

void CTriggerHurt :: RadiationThink( void )
{
	edict_t *pentPlayer;
	CBasePlayer *pPlayer;
	pPlayer = NULL;
	Vector vecSpot1;
	Vector vecSpot2;
	Vector vecRange;
	Vector origin;
	Vector view_ofs;

	// check to see if a player is in pvs
	// if not, continue

	// set origin to center of trigger so that this check works
	origin = pev->origin;
	view_ofs = pev->view_ofs;

	pev->origin = (pev->absmin + pev->absmax) * 0.5;
	pev->view_ofs = pev->view_ofs * 0.0;

	pentPlayer = FIND_CLIENT_IN_PVS( edict() );

	pev->origin = origin;
	pev->view_ofs = view_ofs;

	pev->nextthink = gpGlobals->time + 0.25;
}

//
// ToggleUse - If this is the USE function for a trigger, its state will toggle every time it's fired
//
void CBaseTrigger :: ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (pev->solid == SOLID_NOT)
	{// if the trigger is off, turn it on
		pev->solid = SOLID_TRIGGER;
		
		// Force retouch
		gpGlobals->force_retouch++;
	}
	else
	{// turn the trigger off
		pev->solid = SOLID_NOT;
	}
	UTIL_SetOrigin( this, pev->origin );
}

// When touched, a hurt trigger does DMG points of damage each half-second
void CBaseTrigger :: HurtTouch ( CBaseEntity *pOther )
{
	if ( !UTIL_IsMasterTriggered( m_sMaster, this ) )
		return;
	float fldmg;

	if ( !pOther->pev->takedamage )
		return;

	if ( (pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH) && !pOther->IsPlayer() )
	{
		// this trigger is only allowed to touch clients, and this ain't a client.
		return;
	}

	if ( (pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS) && pOther->IsPlayer() )
		return;
	if ( (pOther->pev->team == 1 && (pev->spawnflags & SF_TRIGGER_HURT_NO_ALLIES)) ||
		(pOther->pev->team == 2 && (pev->spawnflags & SF_TRIGGER_HURT_NO_AXIS)) )
		return;

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if ( g_pGameRules->IsMultiplayer() )
	{
		if ( pev->dmgtime > gpGlobals->time )
		{
			if ( gpGlobals->time != pev->pain_finished )
			{// too early to hurt again, and not same frame with a different entity
				if ( pOther->IsPlayer() )
				{
					int playerMask = 1 << (pOther->entindex() - 1);

					// If I've already touched this player (this time), then bail out
					if ( pev->impulse & playerMask )
						return;

					// Mark this player as touched
					// BUGBUG - There can be only 32 players!
					pev->impulse |= playerMask;
				}
				else
				{
					return;
				}
			}
		}
		else
		{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if ( pOther->IsPlayer() )
			{
				int playerMask = 1 << (pOther->entindex() - 1);

				// Mark this player as touched
				// BUGBUG - There can be only 32 players!
				pev->impulse |= playerMask;
			}
		}
	}
	else	// Original code -- single player
	{
		if ( pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished )
		{// too early to hurt again, and not same frame with a different entity
			return;
		}
	}



	// If this is time_based damage (poison, radiation), override the pev->dmg with a 
	// default for the given damage type.  Monsters only take time-based damage
	// while touching the trigger.  Player continues taking damage for a while after
	// leaving the trigger

	fldmg = pev->dmg * 0.5;	// 0.5 seconds worth of damage, pev->dmg is damage/second


	if ( fldmg < 0 )
	{
		pOther->TakeHealth( -fldmg, m_bitsDamageInflict );
	}
	else
		pOther->TakeDamage( pev, pev, fldmg, m_bitsDamageInflict );

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5;// half second delay until this trigger can hurt toucher again

  
	
	if ( pev->target && pOther->pev->health > 0 )
	{
		// trigger has a target it wants to fire. 
		if ( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE )
		{
			// if the toucher isn't a client, don't fire the target!
			if ( !pOther->IsPlayer() )
			{
				return;
			}
		}

		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		if ( pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE )
			pev->target = 0;
	}
}


/*QUAKED trigger_multiple (.5 .5 .5) ? notouch
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "health" is set, the trigger must be killed to activate each time.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
If notouch is set, the trigger is only fired by other entities, not by touching.
NOTOUCH has been obsoleted by trigger_relay!
sounds
1)      secret
2)      beep beep
3)      large switch
4)
NEW
if a trigger has a NETNAME, that NETNAME will become the TARGET of the triggered object.
*/
class CTriggerMultiple : public CBaseTrigger
{
public:
	void Spawn( void );
	void RoundRespawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_multiple, CTriggerMultiple );


void CTriggerMultiple :: Spawn( void )
{
	if (m_flWait == 0)
		m_flWait = 0.2;

	InitTrigger();

	SetTouch( &CBaseTrigger::MultiTouch );
	RoundStore();
}

void CTriggerMultiple::RoundRespawn( void )
{
	if ( m_flWait == 0 )
		m_flWait = 0.2f;
	InitTrigger();
	SetTouch( &CBaseTrigger::MultiTouch );
	if ( pev->spawnflags & SF_TRIGGER_ROUND_RESTART )
		pev->nextthink = gpGlobals->time;
}


/*QUAKED trigger_once (.5 .5 .5) ? notouch
Variable sized trigger. Triggers once, then removes itself.  You must set the key "target" to the name of another object in the level that has a matching
"targetname".  If "health" is set, the trigger must be killed to activate.
If notouch is set, the trigger is only fired by other entities, not by touching.
if "killtarget" is set, any objects that have a matching "target" will be removed when the trigger is fired.
if "angle" is set, the trigger will only fire when someone is facing the direction of the angle.  Use "360" for an angle of 0.
sounds
1)      secret
2)      beep beep
3)      large switch
4)
*/
class CTriggerOnce : public CTriggerMultiple
{
public:
	void Spawn( void );
	void RoundRespawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_once, CTriggerOnce );
void CTriggerOnce::Spawn( void )
{
	m_flWait = -1;
	
	CTriggerMultiple :: Spawn();
}

void CTriggerOnce::RoundRespawn( void )
{
	if ( pev->spawnflags & SF_TRIGGER_ROUND_RESTART )
		SetTouch( &CBaseTrigger::MultiTouch );
}



void CBaseTrigger :: MultiTouch( CBaseEntity *pOther )
{
	entvars_t	*pevToucher;

	pevToucher = pOther->pev;

	if ( ( pev->spawnflags & SF_TRIGGER_NO_ALLIES ) && pOther->IsPlayer() && pOther->pev->team == 1 )
		return;

	if ( ( pev->spawnflags & SF_TRIGGER_NO_AXIS ) && pOther->IsPlayer() && pOther->pev->team == 2 )
		return;

	if ( m_team )
	{
		if ( m_team == 1 )
		{
			if ( pevToucher->team == 2 )
				return;
		}
		else if ( m_team == 2 )
		{
			if ( pevToucher->team == 1 )
				return;
		}
	}

	// Only touch clients, monsters, or pushables (depending on flags)
	if ( ((pevToucher->flags & FL_CLIENT) && !(pev->spawnflags & SF_TRIGGER_NOCLIENTS)) ||
		 ((pevToucher->flags & FL_MONSTER) && (pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS)) ||
		 (pev->spawnflags & SF_TRIGGER_PUSHABLES) && FClassnameIs(pevToucher,"func_pushable") )
	{

		ActivateMultiTrigger( pOther );
	}
}


//
// the trigger was just touched/killed/used
// self.enemy should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
//
void CBaseTrigger :: ActivateMultiTrigger( CBaseEntity *pActivator )
{
	if (pev->nextthink > gpGlobals->time)
		return;         // still waiting for reset time

	if (!UTIL_IsMasterTriggered(m_sMaster,pActivator))
		return;
	if ( (pev->spawnflags & SF_TRIGGER_ROUND_RESTART) && g_pGameRules->RoundState() != 1 )
		return;

	if (FClassnameIs(pev, "trigger_secret"))
	{
		if ( pev->enemy == NULL || !FClassnameIs(pev->enemy, "player"))
			return;
		gpGlobals->found_secrets++;
	}

	if (!FStringNull(pev->noise))
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise), 1, ATTN_NORM);

// don't trigger again until reset
// pev->takedamage = DAMAGE_NO;

	m_hActivator = pActivator;
	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 );

	if ( pev->message && pActivator->IsPlayer() )
	{
		UTIL_ShowMessage( STRING(pev->message), pActivator, false );
//		CLIENT_PRINTF( ENT( pActivator->pev ), print_center, STRING(pev->message) );
	}

	if (m_flWait > 0)
	{
		SetThink( &CBaseTrigger::MultiWaitOver );
		pev->nextthink = gpGlobals->time + m_flWait;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		if ( pev->spawnflags & SF_TRIGGER_ROUND_RESTART )
		{
			SetTouch( NULL );
			SetThink( NULL );
			pev->nextthink = 0;
		}
		else
		{
			SetTouch( NULL );
			pev->nextthink = gpGlobals->time + 0.1;
			SetThink( &CBaseTrigger::SUB_Remove );
		}
	}
}


// the wait time has passed, so set back up for another activation
void CBaseTrigger :: MultiWaitOver( void )
{
//	if (pev->max_health)
//		{
//		pev->health		= pev->max_health;
//		pev->takedamage	= DAMAGE_YES;
//		pev->solid		= SOLID_BBOX;
//		}
	SetThink( NULL );
}


// ========================= COUNTING TRIGGER =====================================

//
// GLOBALS ASSUMED SET:  g_eoActivator
//
void CBaseTrigger::CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_cTriggersLeft--;
	m_hActivator = pActivator;

	if (m_cTriggersLeft < 0)
		return;
	
	BOOL fTellActivator =
		FClassnameIs(m_hActivator->pev, "player") &&
		!FBitSet(pev->spawnflags, SPAWNFLAG_NOMESSAGE);
	if (m_cTriggersLeft != 0)
	{
		if (fTellActivator)
		{
			// UNDONE: I don't think we want these Quakesque messages
			switch (m_cTriggersLeft)
			{
			case 1:		ALERT(at_console, "Only 1 more to go...");		break;
			case 2:		ALERT(at_console, "Only 2 more to go...");		break;
			case 3:		ALERT(at_console, "Only 3 more to go...");		break;
			default:	ALERT(at_console, "There are more to go...");	break;
			}
		}
		return;
	}

	// !!!UNDONE: I don't think we want these Quakesque messages
	if (fTellActivator)
		ALERT(at_console, "Sequence completed!");
	
	ActivateMultiTrigger( m_hActivator );
}


/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.
If nomessage is not set, it will print "1 more.. " etc when triggered and
"sequence complete" when finished.  After the counter has been triggered "cTriggersLeft"
times (default 2), it will fire all of it's targets and remove itself.
*/
class CTriggerCounter : public CBaseTrigger
{
public:
	void Spawn( void );
};
LINK_ENTITY_TO_CLASS( trigger_counter, CTriggerCounter );

void CTriggerCounter :: Spawn( void )
{
	// By making the flWait be -1, this counter-trigger will disappear after it's activated
	// (but of course it needs cTriggersLeft "uses" before that happens).
	m_flWait = -1;

	if (m_cTriggersLeft == 0)
		m_cTriggersLeft = 2;
	SetUse( &CTriggerCounter::CounterUse );
}

// ====================== TRIGGER_CHANGELEVEL ================================

class CTriggerVolume : public CPointEntity	// Derive from point entity so this doesn't move across levels
{
public:
	void		Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_transition, CTriggerVolume );

// Define space that travels across a level transition
void CTriggerVolume :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	pev->model = NULL;
	pev->modelindex = 0;
}


// Fires a target after level transition and then dies
class CFireAndDie : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void Think( void );
	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() | FCAP_FORCE_TRANSITION; }	// Always go across transitions
};
LINK_ENTITY_TO_CLASS( fireanddie, CFireAndDie );

void CFireAndDie::Spawn( void )
{
	MAKE_STRING_CLASS( "fireanddie", pev );
	// Don't call Precache() - it should be called on restore
}


void CFireAndDie::Precache( void )
{
	// This gets called on restore
	pev->nextthink = gpGlobals->time + m_flDelay;
}


void CFireAndDie::Think( void )
{
	SUB_UseTargets( this, USE_TOGGLE, 0 );
	UTIL_Remove( this );
}


#define SF_CHANGELEVEL_USEONLY		0x0002
class CChangeLevel : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT UseChangeLevel ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TriggerChangeLevel( void );
	void EXPORT ExecuteChangeLevel( void );
	void EXPORT TouchChangeLevel( CBaseEntity *pOther );
	void ChangeLevelNow( CBaseEntity *pActivator );

	static edict_t *FindLandmark( const char *pLandmarkName );
	static int ChangeList( LEVELLIST *pLevelList, int maxList );
	static int AddTransitionToList( LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark );
	static int InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	char m_szMapName[cchMapNameMost];		// trigger_changelevel only:  next map
	char m_szLandmarkName[cchMapNameMost];		// trigger_changelevel only:  landmark on next map
	int		m_changeTarget;
	float	m_changeTargetDelay;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( trigger_changelevel, CChangeLevel );

// Global Savedata for changelevel trigger
TYPEDESCRIPTION	CChangeLevel::m_SaveData[] = 
{
	DEFINE_ARRAY( CChangeLevel, m_szMapName, FIELD_CHARACTER, cchMapNameMost ),
	DEFINE_ARRAY( CChangeLevel, m_szLandmarkName, FIELD_CHARACTER, cchMapNameMost ),
	DEFINE_FIELD( CChangeLevel, m_changeTarget, FIELD_STRING ),
	DEFINE_FIELD( CChangeLevel, m_changeTargetDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE(CChangeLevel,CBaseTrigger);

//
// Cache user-entity-field values until spawn is called.
//

void CChangeLevel :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "map"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			ALERT( at_error, "Map name '%s' too long (32 chars)\n", pkvd->szValue );
		strncpy( m_szMapName, pkvd->szValue, sizeof( m_szMapName ) - 1 );
		m_szMapName[sizeof( m_szMapName ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "landmark"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			ALERT( at_error, "Landmark name '%s' too long (32 chars)\n", pkvd->szValue );
		strncpy( m_szLandmarkName, pkvd->szValue, sizeof( m_szLandmarkName ) - 1 );
		m_szLandmarkName[sizeof( m_szLandmarkName ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "changetarget"))
	{
		m_changeTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "changedelay"))
	{
		m_changeTargetDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}


/*QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
When the player touches this, he gets sent to the map listed in the "map" variable.  Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
*/

void CChangeLevel :: Spawn( void )
{
	if ( FStrEq( m_szMapName, "" ) )
		ALERT( at_console, "a trigger_changelevel doesn't have a map" );

	if ( FStrEq( m_szLandmarkName, "" ) )
		ALERT( at_console, "trigger_changelevel to %s doesn't have a landmark", m_szMapName );

	if (!FStringNull ( pev->targetname ) )
	{
		SetUse ( &CChangeLevel::UseChangeLevel );
	}
	InitTrigger();
	if ( !(pev->spawnflags & SF_CHANGELEVEL_USEONLY) )
		SetTouch( &CChangeLevel::TouchChangeLevel );
//	ALERT( at_console, "TRANSITION: %s (%s)\n", m_szMapName, m_szLandmarkName );
}


void CChangeLevel :: ExecuteChangeLevel( void )
{
	MESSAGE_BEGIN( MSG_ALL, SVC_CDTRACK );
		WRITE_BYTE( 3 );
		WRITE_BYTE( 3 );
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();
}


FILE_GLOBAL char st_szNextMap[cchMapNameMost];
FILE_GLOBAL char st_szNextSpot[cchMapNameMost];

edict_t *CChangeLevel :: FindLandmark( const char *pLandmarkName )
{
	CBaseEntity *pLandmark;

	pLandmark = UTIL_FindEntityByTargetname( NULL, pLandmarkName );
	while ( pLandmark )
	{
		// Found the landmark
		if ( FClassnameIs( pLandmark->pev, "info_landmark" ) )
			return ENT( pLandmark->pev );
		else
			pLandmark = UTIL_FindEntityByTargetname( pLandmark, pLandmarkName );
	}
	ALERT( at_error, "Can't find landmark %s\n", pLandmarkName );
	return NULL;
}


//=========================================================
// CChangeLevel :: Use - allows level transitions to be 
// triggered by buttons, etc.
//
//=========================================================
void CChangeLevel :: UseChangeLevel ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	ChangeLevelNow( pActivator );
}

void CChangeLevel :: ChangeLevelNow( CBaseEntity *pActivator )
{
	edict_t	*pentLandmark;
	LEVELLIST	levels[16];

	// Don't work in deathmatch
	if ( g_pGameRules->IsDeathmatch() )
		return;

	// Some people are firing these multiple times in a frame, disable
	if ( gpGlobals->time == pev->dmgtime )
		return;

	pev->dmgtime = gpGlobals->time;


	CBaseEntity *pPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	if ( !InTransitionVolume( pPlayer, m_szLandmarkName ) )
	{
		ALERT( at_aiconsole, "Player isn't in the transition volume %s, aborting\n", m_szLandmarkName );
		return;
	}

	// Create an entity to fire the changetarget
	if ( m_changeTarget )
	{
		CFireAndDie *pFireAndDie = GetClassPtr( (CFireAndDie *)NULL );
		if ( pFireAndDie )
		{
			// Set target and delay
			pFireAndDie->pev->target = m_changeTarget;
			pFireAndDie->m_flDelay = m_changeTargetDelay;
			pFireAndDie->pev->origin = pPlayer->pev->origin;
			// Call spawn
			DispatchSpawn( pFireAndDie->edict() );
		}
	}
	// This object will get removed in the call to CHANGE_LEVEL, copy the params into "safe" memory
	strcpy(st_szNextMap, m_szMapName);

	m_hActivator = pActivator;
	SUB_UseTargets( pActivator, USE_TOGGLE, 0 );
	st_szNextSpot[0] = 0;	// Init landmark to NULL

	// look for a landmark entity		
	pentLandmark = FindLandmark( m_szLandmarkName );
	if ( !FNullEnt( pentLandmark ) )
	{
		strcpy(st_szNextSpot, m_szLandmarkName);
		gpGlobals->vecLandmarkOffset = VARS(pentLandmark)->origin;
	}
//	ALERT( at_console, "Level touches %d levels\n", ChangeList( levels, 16 ) );
	ALERT( at_console, "CHANGE LEVEL: %s %s\n", st_szNextMap, st_szNextSpot );
	CHANGE_LEVEL( st_szNextMap, st_szNextSpot );
}

//
// GLOBALS ASSUMED SET:  st_szNextMap
//
void CChangeLevel :: TouchChangeLevel( CBaseEntity *pOther )
{
	if (!FClassnameIs(pOther->pev, "player"))
		return;

	ChangeLevelNow( pOther );
}


// Add a transition to the list, but ignore duplicates 
// (a designer may have placed multiple trigger_changelevels with the same landmark)
int CChangeLevel::AddTransitionToList( LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark )
{
	int i;

	if ( !pLevelList || !pMapName || !pLandmarkName || !pentLandmark )
		return 0;

	for ( i = 0; i < listCount; i++ )
	{
		if ( pLevelList[i].pentLandmark == pentLandmark && strcmp( pLevelList[i].mapName, pMapName ) == 0 )
			return 0;
	}
	strcpy( pLevelList[listCount].mapName, pMapName );
	strcpy( pLevelList[listCount].landmarkName, pLandmarkName );
	pLevelList[listCount].pentLandmark = pentLandmark;
	pLevelList[listCount].vecLandmarkOrigin = VARS(pentLandmark)->origin;

	return 1;
}

int BuildChangeList( LEVELLIST *pLevelList, int maxList )
{
	return CChangeLevel::ChangeList( pLevelList, maxList );
}


int CChangeLevel::InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName )
{
	CBaseEntity *pVolume;


	if ( pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION )
		return 1;

	// If you're following another entity, follow it through the transition (weapons follow the player)
	if ( pEntity->pev->movetype == MOVETYPE_FOLLOW )
	{
		if ( pEntity->pev->aiment != NULL )
			pEntity = CBaseEntity::Instance( pEntity->pev->aiment );
	}

	int inVolume = 1;	// Unless we find a trigger_transition, everything is in the volume

	pVolume = UTIL_FindEntityByTargetname( NULL, pVolumeName );
	while ( pVolume )
	{
		if ( FClassnameIs( pVolume->pev, "trigger_transition" ) )
		{
			if ( pVolume->Intersects( pEntity ) )	// It touches one, it's in the volume
				return 1;
			else
				inVolume = 0;	// Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
		}
		pVolume = UTIL_FindEntityByTargetname( pVolume, pVolumeName );
	}

	return inVolume;
}


// We can only ever move 512 entities across a transition
#define MAX_ENTITY 512

// This has grown into a complicated beast
// Can we make this more elegant?
// This builds the list of all transitions on this level and which entities are in their PVS's and can / should
// be moved across.
int CChangeLevel::ChangeList( LEVELLIST *pLevelList, int maxList )
{
	edict_t	*pentLandmark;
	int			i, count;

	count = 0;

	// Find all of the possible level changes on this BSP
	CBaseEntity *pChangelevel = UTIL_FindEntityByClassname( NULL, "trigger_changelevel" );
	if ( !pChangelevel )
		return 0;
	while ( pChangelevel )
	{
		CChangeLevel *pTrigger;
		
		pTrigger = GetClassPtr((CChangeLevel *)pChangelevel->pev);
		if ( pTrigger )
		{
			// Find the corresponding landmark
			pentLandmark = FindLandmark( pTrigger->m_szLandmarkName );
			if ( pentLandmark )
			{
				// Build a list of unique transitions
				if ( AddTransitionToList( pLevelList, count, pTrigger->m_szMapName, pTrigger->m_szLandmarkName, pentLandmark ) )
				{
					count++;
					if ( count >= maxList )		// FULL!!
						break;
				}
			}
		}
		pChangelevel = UTIL_FindEntityByClassname( pChangelevel, "trigger_changelevel" );
	}

	if ( gpGlobals->pSaveData && ((SAVERESTOREDATA *)gpGlobals->pSaveData)->pTable )
	{
		CSave saveHelper( (SAVERESTOREDATA *)gpGlobals->pSaveData );

		for ( i = 0; i < count; i++ )
		{
			int j, entityCount = 0;
			CBaseEntity *pEntList[ MAX_ENTITY ];
			int			 entityFlags[ MAX_ENTITY ];

			// Follow the linked list of entities in the PVS of the transition landmark
			edict_t *pent = UTIL_EntitiesInPVS( pLevelList[i].pentLandmark );

			// Build a list of valid entities in this linked list (we're going to use pent->v.chain again)
			while ( !FNullEnt( pent ) )
			{
				CBaseEntity *pEntity = CBaseEntity::Instance(pent);
				if ( pEntity )
				{
//					ALERT( at_console, "Trying %s\n", STRING(pEntity->pev->classname) );
					int caps = pEntity->ObjectCaps();
					if ( !(caps & FCAP_DONT_SAVE) )
					{
						int flags = 0;

						// If this entity can be moved or is global, mark it
						if ( caps & FCAP_ACROSS_TRANSITION )
							flags |= FENTTABLE_MOVEABLE;
						if ( pEntity->pev->globalname && !pEntity->IsDormant() )
							flags |= FENTTABLE_GLOBAL;
						if ( flags )
						{
							if ( entityCount < MAX_ENTITY )
							{
								pEntList[ entityCount ] = pEntity;
								entityFlags[ entityCount ] = flags;
								entityCount++;
							}
							else
								ALERT( at_error, "Too many entities across a transition!" );
						}
//						else
//							ALERT( at_console, "Failed %s\n", STRING(pEntity->pev->classname) );
					}
//					else
//						ALERT( at_console, "DON'T SAVE %s\n", STRING(pEntity->pev->classname) );
				}
				pent = pent->v.chain;
			}

			for ( j = 0; j < entityCount; j++ )
			{
				// Check to make sure the entity isn't screened out by a trigger_transition
				if ( entityFlags[j] && InTransitionVolume( pEntList[j], pLevelList[i].landmarkName ) )
				{
					// Mark entity table with 1<<i
					int index = saveHelper.EntityIndex( pEntList[j] );
					// Flag it with the level number
					saveHelper.EntityFlagsSet( index, entityFlags[j] | (1<<i) );
				}
//				else
//					ALERT( at_console, "Screened out %s\n", STRING(pEntList[j]->pev->classname) );

			}
		}
	}

	return count;
}

/*
go to the next level for deathmatch
only called if a time or frag limit has expired
*/
void NextLevel( void )
{
	CBaseEntity *pEnt;
	CChangeLevel *pChange;

	// find a trigger_changelevel
	pEnt = UTIL_FindEntityByClassname( NULL, "trigger_changelevel" );

	// go back to start if no trigger_changelevel
	if ( !pEnt )
	{
		gpGlobals->mapname = ALLOC_STRING( "start" );
		pChange = GetClassPtr( (CChangeLevel *)NULL );
		strcpy( pChange->m_szMapName, "start" );
	}
	else
		pChange = GetClassPtr( (CChangeLevel *)pEnt->pev );

	strcpy( st_szNextMap, pChange->m_szMapName );
	g_fGameOver = TRUE;

	if ( pChange->pev->nextthink < gpGlobals->time )
	{
		pChange->SetThink( &CChangeLevel::ExecuteChangeLevel );
		pChange->pev->nextthink = gpGlobals->time + 0.1;
	}
}


// ============================== LADDER =======================================

class CLadder : public CBaseTrigger
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
};
LINK_ENTITY_TO_CLASS( func_ladder, CLadder );


void CLadder :: KeyValue( KeyValueData *pkvd )
{
	CBaseTrigger::KeyValue( pkvd );
}


//=========================================================
// func_ladder - makes an area vertically negotiable
//=========================================================
void CLadder :: Precache( void )
{
	// Do all of this in here because we need to 'convert' old saved games
	pev->solid = SOLID_NOT;
	pev->skin = CONTENTS_LADDER;
	if ( CVAR_GET_FLOAT("showtriggers") == 0 )
	{
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 0;
	}
	pev->effects &= ~EF_NODRAW;
}


void CLadder :: Spawn( void )
{
	Precache();

	if ( pev->spawnflags & 1 )
		pev->iuser1 = 1;
	else
		pev->iuser1 = 0;

	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	pev->movetype = MOVETYPE_PUSH;
	RoundStore();
}


// ========================== A TRIGGER THAT PUSHES YOU ===============================

class CTriggerPush : public CBaseTrigger
{
public:
	void Spawn( void );
	void RoundRespawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Touch( CBaseEntity *pOther );
	int m_iEnabled;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( trigger_push, CTriggerPush );


void CTriggerPush :: KeyValue( KeyValueData *pkvd )
{
	CBaseTrigger::KeyValue( pkvd );
}


/*QUAKED trigger_push (.5 .5 .5) ? TRIG_PUSH_ONCE
Pushes the player
*/

void CTriggerPush :: Spawn( )
{
	if ( pev->angles == g_vecZero )
		pev->angles.y = 360;
	InitTrigger();

	if (pev->speed == 0)
		pev->speed = 100;

	if ( FBitSet (pev->spawnflags, SF_TRIGGER_PUSH_START_OFF) )// if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	SetUse( &CTriggerPush::ToggleUse );

	UTIL_SetOrigin( this, pev->origin );
	m_iEnabled = 1;
	RoundStore();
}

void CTriggerPush::RoundRespawn( void )
{
	InitTrigger();
	SetUse( &CBaseTrigger::ToggleUse );
	m_iEnabled = 1;
}


void CTriggerPush :: Touch( CBaseEntity *pOther )
{
	if ( !m_iEnabled )
		return;
	entvars_t* pevToucher = pOther->pev;

	// UNDONE: Is there a better way than health to detect things that have physics? (clients/monsters)
	switch( pevToucher->movetype )
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
	case MOVETYPE_FOLLOW:
		return;
	}

	if ( pevToucher->solid != SOLID_NOT && pevToucher->solid != SOLID_BSP )
	{
		// Instant trigger, just transfer velocity and remove
		if (FBitSet(pev->spawnflags, SF_TRIG_PUSH_ONCE))
		{
			pevToucher->velocity = pevToucher->velocity + (pev->speed * pev->movedir);
			if ( pevToucher->velocity.z > 0 )
				pevToucher->flags &= ~FL_ONGROUND;
			m_iEnabled = 0;
		}
		else
		{	// Push field, transfer to base velocity
			Vector vecPush = (pev->speed * pev->movedir);
			if ( pevToucher->flags & FL_BASEVELOCITY )
				vecPush = vecPush +  pevToucher->basevelocity;

			pevToucher->basevelocity = vecPush;

			pevToucher->flags |= FL_BASEVELOCITY;
//			ALERT( at_console, "Vel %f, base %f\n", pevToucher->velocity.z, pevToucher->basevelocity.z );
		}
	}
}


//======================================
// teleport trigger
//
//

void CBaseTrigger :: TeleportTouch( CBaseEntity *pOther )
{
	entvars_t* pevToucher = pOther->pev;
	CBaseEntity *pTarget = NULL;

	// Only teleport monsters or clients
	if ( !FBitSet( pevToucher->flags, FL_CLIENT|FL_MONSTER ) )
		return;
    
	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;
 	
	if ( !( pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS ) )
	{// no monsters allowed!
		if ( FBitSet( pevToucher->flags, FL_MONSTER ) )
		{
			return;
		}
	}

	if ( ( pev->spawnflags & SF_TRIGGER_NOCLIENTS ) )
	{// no clients allowed
		if ( pOther->IsPlayer() )
		{
			return;
		}
	}
	
	if ( ( pev->spawnflags & SF_TRIGGER_NO_ALLIES ) && pOther->IsPlayer() && pOther->pev->team == 1 )
		return;

	if ( ( pev->spawnflags & SF_TRIGGER_NO_AXIS ) && pOther->IsPlayer() && pOther->pev->team == 2 )
		return;

	pTarget = UTIL_FindEntityByTargetname( pTarget, STRING(pev->target) );
	if (!pTarget)
	   return;	
	
	Vector tmp = pTarget->pev->origin;

	if ( pOther->IsPlayer() )
	{
		tmp.z -= pOther->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;
	
	UTIL_SetOrigin( pOther, tmp );

	pevToucher->angles = pTarget->pev->angles;

	if ( pOther->IsPlayer() )
	{
		pevToucher->v_angle = pTarget->pev->angles;
	}

	pevToucher->fixangle = TRUE;
	pevToucher->velocity = pevToucher->basevelocity = g_vecZero;
}


class CTriggerTeleport : public CBaseTrigger
{
public:
	void Spawn( void );
	void RoundRespawn( void );
};
LINK_ENTITY_TO_CLASS( trigger_teleport, CTriggerTeleport );

void CTriggerTeleport :: Spawn( void )
{
	InitTrigger();

	SetTouch( &CTriggerTeleport::TeleportTouch );
	RoundStore();
}

void CTriggerTeleport::RoundRespawn( void )
{
	InitTrigger();
	SetTouch( &CBaseTrigger::TeleportTouch );
}


LINK_ENTITY_TO_CLASS( info_teleport_destination, CPointEntity );



class CTriggerSave : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT SaveTouch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_autosave, CTriggerSave );

void CTriggerSave::Spawn( void )
{
	if ( g_pGameRules->IsDeathmatch() )
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	InitTrigger();
	SetTouch( &CTriggerSave::SaveTouch );
}

void CTriggerSave::SaveTouch( CBaseEntity *pOther )
{
	if ( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;

	// Only save on clients
	if ( !pOther->IsPlayer() )
		return;
    
	SetTouch( NULL );
	UTIL_Remove( this );
	SERVER_COMMAND( "autosave\n" );
}

#define SF_ENDSECTION_USEONLY		0x0001

class CTriggerEndSection : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT EndSectionTouch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT EndSectionUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};
LINK_ENTITY_TO_CLASS( trigger_endsection, CTriggerEndSection );


void CTriggerEndSection::EndSectionUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Only save on clients
	if ( pActivator && !pActivator->IsNetClient() )
		return;
    
	SetUse( NULL );

	if ( pev->message )
	{
		g_engfuncs.pfnEndSection(STRING(pev->message));
	}
	UTIL_Remove( this );
}

void CTriggerEndSection::Spawn( void )
{
	if ( g_pGameRules->IsDeathmatch() )
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	InitTrigger();

	SetUse ( &CTriggerEndSection::EndSectionUse );
	// If it is a "use only" trigger, then don't set the touch function.
	if ( ! (pev->spawnflags & SF_ENDSECTION_USEONLY) )
		SetTouch( &CTriggerEndSection::EndSectionTouch );
}

void CTriggerEndSection::EndSectionTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if ( !pOther->IsNetClient() )
		return;
    
	SetTouch( NULL );

	if (pev->message)
	{
		g_engfuncs.pfnEndSection(STRING(pev->message));
	}
	UTIL_Remove( this );
}

void CTriggerEndSection :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "section"))
	{
//		m_iszSectionName = ALLOC_STRING( pkvd->szValue );
		// Store this in message so we don't have to write save/restore for this ent
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}


class CTriggerGravity : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT GravityTouch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_gravity, CTriggerGravity );

void CTriggerGravity::Spawn( void )
{
	InitTrigger();
	SetTouch( &CTriggerGravity::GravityTouch );
}

void CTriggerGravity::GravityTouch( CBaseEntity *pOther )
{
	pOther->pev->gravity = pev->gravity;
}







// this is a really bad idea.
class CTriggerSetPatrol : public CBaseDelay
{
public:
	void Spawn();
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps() { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save(CSave &save);
	int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	int m_iszPath;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(trigger_startpatrol, CTriggerSetPatrol);

TYPEDESCRIPTION CTriggerSetPatrol::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerSetPatrol, m_iszPath, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CTriggerSetPatrol, CBaseDelay);

void CTriggerSetPatrol::Spawn()
{
}

void CTriggerSetPatrol::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszPath"))
	{
		m_iszPath = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CTriggerSetPatrol::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseMonster *pMonster = (CBaseMonster *)UTIL_FindEntityByTargetname(NULL, STRING(pev->target));
	CBaseEntity *pPath = UTIL_FindEntityByTargetname(NULL, STRING(m_iszPath));
	if (pPath && pMonster && !memcmp(STRING(pMonster->pev->classname), "monster_", 8))
		pMonster->StartPatrol(pPath);
}

class CTriggerChangeTarget : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int		m_iszNewTarget;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( trigger_changetarget, CTriggerChangeTarget );

TYPEDESCRIPTION	CTriggerChangeTarget::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerChangeTarget, m_iszNewTarget, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CTriggerChangeTarget,CBaseDelay);

void CTriggerChangeTarget::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszNewTarget"))
	{
		m_iszNewTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerChangeTarget::Spawn( void )
{
}


void CTriggerChangeTarget::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );

	if (pTarget)
	{
		pTarget->pev->target = m_iszNewTarget;
		CBaseMonster *pMonster = pTarget->MyMonsterPointer( );
		if (pMonster)
		{
			pMonster->m_pGoalEnt = NULL;
		}
	}
}




#define SF_CAMERA_PLAYER_POSITION	1
#define SF_CAMERA_PLAYER_TARGET		2
#define SF_CAMERA_PLAYER_TAKECONTROL 4

class CTriggerCamera : public CBaseDelay
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT FollowTarget( void );
	void Move(void);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;
	CBaseEntity *m_pentPath;
	int	  m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	int	  m_state;
	
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( trigger_camera, CTriggerCamera );

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CTriggerCamera::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerCamera, m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_pentPath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTriggerCamera, m_sPath, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCamera, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_flReturnTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_flStopTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_moveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_targetSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_initialSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_acceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_deceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_state, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CTriggerCamera,CBaseDelay);

void CTriggerCamera::Spawn( void )
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;							// Remove model & collisions
	pev->renderamt = 0;								// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;

	m_initialSpeed = pev->speed;
	if ( m_acceleration == 0 )
		m_acceleration = 500;
	if ( m_deceleration == 0 )
		m_deceleration = 500;
}


void CTriggerCamera :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "moveto"))
	{
		m_sPath = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "acceleration"))
	{
		m_acceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "deceleration"))
	{
		m_deceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}



void CTriggerCamera::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_state ) )
		return;

	// Toggle state
	m_state = !m_state;
	if (m_state == 0)
	{
		m_flReturnTime = gpGlobals->time;
		return;
	}
	if ( !pActivator || !pActivator->IsPlayer() )
	{
		pActivator = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex( 1 ));
	}
		
	m_hPlayer = pActivator;

	m_flReturnTime = gpGlobals->time + m_flWait;
	pev->speed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TARGET ) )
	{
		m_hTarget = m_hPlayer;
	}
	else
	{
		m_hTarget = GetNextTarget();
	}

	// Nothing to look at!
	if ( m_hTarget == NULL )
	{
		return;
	}


	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL ) )
	{
		((CBasePlayer *)pActivator)->EnableControl(FALSE);
	}

	if ( m_sPath )
	{
		m_pentPath = UTIL_FindEntityByTargetname( NULL, STRING( m_sPath ) );
	}
	else
	{
		m_pentPath = NULL;
	}

	m_flStopTime = gpGlobals->time;
	if ( m_pentPath )
	{
		if ( m_pentPath->pev->speed != 0 )
			m_targetSpeed = m_pentPath->pev->speed;
		
		m_flStopTime += m_pentPath->GetDelay();
	}

	// copy over player information
	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_POSITION ) )
	{
		UTIL_SetOrigin( this, pActivator->pev->origin + pActivator->pev->view_ofs );
		pev->angles.x = -pActivator->pev->angles.x;
		pev->angles.y = pActivator->pev->angles.y;
		pev->angles.z = 0;
		pev->velocity = pActivator->pev->velocity;
	}
	else
	{
		pev->velocity = Vector( 0, 0, 0 );
	}

	SET_VIEW( pActivator->edict(), edict() );

	SET_MODEL(ENT(pev), STRING(pActivator->pev->model) );

	// follow the player down
	SetThink( &CTriggerCamera::FollowTarget );
	pev->nextthink = gpGlobals->time;

	m_moveDistance = 0;
	Move();
}


void CTriggerCamera::FollowTarget( )
{
	if (m_hPlayer == NULL)
		return;

	if (m_hTarget == NULL || m_flReturnTime < gpGlobals->time)
	{
		if (m_hPlayer->IsAlive( ))
		{
			SET_VIEW( m_hPlayer->edict(), m_hPlayer->edict() );
			((CBasePlayer *)((CBaseEntity *)m_hPlayer))->EnableControl(TRUE);
		}
		SUB_UseTargets( this, USE_TOGGLE, 0 );
		pev->avelocity = Vector( 0, 0, 0 );
		m_state = 0;
		return;
	}

	Vector vecGoal = UTIL_VecToAngles( m_hTarget->pev->origin - pev->origin );
	vecGoal.x = -vecGoal.x;

	if (pev->angles.y > 360)
		pev->angles.y -= 360;

	if (pev->angles.y < 0)
		pev->angles.y += 360;

	float dx = vecGoal.x - pev->angles.x;
	float dy = vecGoal.y - pev->angles.y;

	if (dx < -180) 
		dx += 360;
	if (dx > 180) 
		dx = dx - 360;
	
	if (dy < -180) 
		dy += 360;
	if (dy > 180) 
		dy = dy - 360;

	pev->avelocity.x = dx * 40 * gpGlobals->frametime;
	pev->avelocity.y = dy * 40 * gpGlobals->frametime;


	if (!(FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL)))	
	{
		pev->velocity = pev->velocity * 0.8;
		if (pev->velocity.Length( ) < 10.0)
			pev->velocity = g_vecZero;
	}

	pev->nextthink = gpGlobals->time;

	Move();
}

void CTriggerCamera::Move()
{
	// Not moving on a path, return
	if (!m_pentPath)
		return;

	// Subtract movement from the previous frame
	m_moveDistance -= pev->speed * gpGlobals->frametime;

	// Have we moved enough to reach the target?
	if ( m_moveDistance <= 0 )
	{
		// Fire the passtarget if there is one
		if ( m_pentPath->pev->message )
		{
			FireTargets( STRING(m_pentPath->pev->message), this, this, USE_TOGGLE, 0 );
			if ( FBitSet( m_pentPath->pev->spawnflags, SF_CORNER_FIREONCE ) )
				m_pentPath->pev->message = 0;
		}
		// Time to go to the next target
		m_pentPath = m_pentPath->GetNextTarget();

		// Set up next corner
		if ( !m_pentPath )
		{
			pev->velocity = g_vecZero;
		}
		else 
		{
			if ( m_pentPath->pev->speed != 0 )
				m_targetSpeed = m_pentPath->pev->speed;

			Vector delta = m_pentPath->pev->origin - pev->origin;
			m_moveDistance = delta.Length();
			pev->movedir = delta.Normalize();
			m_flStopTime = gpGlobals->time + m_pentPath->GetDelay();
		}
	}

	if ( m_flStopTime > gpGlobals->time )
		pev->speed = UTIL_Approach( 0, pev->speed, m_deceleration * gpGlobals->frametime );
	else
		pev->speed = UTIL_Approach( m_targetSpeed, pev->speed, m_acceleration * gpGlobals->frametime );

	float fraction = 2 * gpGlobals->frametime;
	pev->velocity = ((pev->movedir * pev->speed) * fraction) + (pev->velocity * (1-fraction));
}

LINK_ENTITY_TO_CLASS(dod_location, CLocation);

void CLocation::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(this, pev->origin);
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	SetTouch(NULL);
	pev->movedir = g_vecZero;
	RoundStore();
}

void CLocation::RoundRespawn()
{
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	SetTouch(NULL);
}

void CLocation::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "location_name" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

class CDodPointRelay : public CBaseEntity
{
public:
	void Spawn();
	void KeyValue(KeyValueData *pkvd);
	int m_nTeam;
private:
	void EXPORT DodRelayUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(dod_point_relay, CDodPointRelay);

void CDodPointRelay::Spawn()
{
	pev->solid = SOLID_NOT;
	SetUse(&CDodPointRelay::DodRelayUse);
}

void CDodPointRelay::KeyValue( KeyValueData *pkvd )
{
	m_nTeam = -1;

	if ( FStrEq( pkvd->szKeyName, "dod_relay_team" ) )
	{
		m_nTeam = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CDodPointRelay::DodRelayUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );

	if ( !pEnt )
		return;

	CControlPoint *pPoint = (CControlPoint *)pEnt;

	if ( !pPoint )
		return;

	if ( useType == USE_START )
	{
		if ( pPoint->PointIsVisible() )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgStartProgressBar );
			WRITE_BYTE( pPoint->GetIndex() );
			WRITE_BYTE( m_nTeam );
			WRITE_SHORT( (int)value );
			MESSAGE_END();
		}
	}
	else if ( useType == USE_BREAK )
	{
		if ( pPoint->PointIsVisible() )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgCancelProgressBar );
			WRITE_BYTE( pPoint->GetIndex() );
			WRITE_BYTE( pPoint->GetOwner() );
			MESSAGE_END();
		}
	}
	else
	{
		if ( pActivator && pActivator->IsPlayer() )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
			pPoint->SetOwner( m_nTeam, TRUE, pPlayer );
		}
		else
			pPoint->SetOwner( m_nTeam, TRUE, NULL );
	}
}

class CSandBag : public CBaseTrigger
{
public:
	bool IsMasteredOn()
	{
		if ( m_sMaster )
			return UTIL_IsMasterTriggered( m_sMaster, this ) > 0;

		return true;
	}

	CSandBag();
	void Spawn();
	void RoundRespawn();
	void KeyValue(KeyValueData *pkvd);
	int m_nYaw;
	int m_nRange;
	string_t m_sMaster;
private:
	void EXPORT SandBagTouch(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(dod_trigger_sandbag, CSandBag);

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
CSandBag::CSandBag()
{
	m_nRange = 5;
	m_sMaster = 0;
}

void CSandBag::Spawn()
{
	m_nYaw = (int)pev->angles.y;
	if (m_nYaw > 180)
		m_nYaw -= 360;
	InitTrigger();
	Precache();
	pev->sequence = 0;
	pev->framerate = 1;
	UTIL_SetOrigin(this, pev->origin);
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SetTouch(&CSandBag::SandBagTouch);
	RoundStore();
}

void CSandBag::RoundRespawn()
{
	InitTrigger();
	pev->sequence = 0;
	pev->framerate = 1;
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SetTouch(&CSandBag::SandBagTouch);
}

void CSandBag::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "sandbag_range" ) )
	{
		m_nRange = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CSandBag::SandBagTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
		return;

	if ( !pOther->IsAlive() )
		return;

	if ( !IsMasteredOn() )
		return;

	CBasePlayer *player = (CBasePlayer *)pOther;

	int delta = abs( (int)( player->pev->angles.y - m_nYaw ) );

	if ( delta > 180 )
		delta = 360 - delta;

	bool bAdded = false;

	if ( !player->IsProne() && !( player->pev->button & IN_DUCK ) )
	{
		if ( delta <= m_nRange && player->m_pActiveItem && player->m_pActiveItem->Classify() == 256 )
		{
			player->m_signals.Signal( 1 );
			bAdded = true;
		}
		else if ( player->IsSandbagDeployed() )
		{
			player->m_signals.Signal( 1 );
			bAdded = true;
		}
	}

	if ( bAdded && !( player->m_flDisplayHistory & 0x400 ) )
	{
		player->m_flDisplayHistory |= 0x400;
		player->HintMessage( "#Hint_sandbag_area_touch", FALSE );
	}
}

LINK_ENTITY_TO_CLASS(dod_round_timer, CDodRoundTimer);

void CDodRoundTimer::Spawn()
{
	m_bTimer = false;
	pev->solid = SOLID_NOT;
	m_fLastThinkTime = 0;
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CDodRoundTimer::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "round_timer_length" ) )
	{
		m_fTimerLength = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CDodRoundTimer::SendStatus()
{
	if ( ( pev->spawnflags & 1 ) && UTIL_IsMasterTriggered( m_sMaster, this ) && m_bTimer == true )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( m_fRoundTime );
		MESSAGE_END();
	}
}

void CDodRoundTimer::SendStatus( CBasePlayer *pPlayer )
{
	if ( ( pev->spawnflags & 1 ) && UTIL_IsMasterTriggered( m_sMaster, this ) && m_bTimer == true )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgTimerStatus, NULL, pPlayer->edict() );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( m_fRoundTime );
		MESSAGE_END();
	}
}

void CDodRoundTimer::StartTimer()
{
	if (UTIL_IsMasterTriggered(m_sMaster, this))
	{
		m_bTimer = true;
		SendStatus();
	}
}

void CDodRoundTimer::StopTimer()
{
	m_bTimer = false;
	if ((pev->spawnflags & 1) && UTIL_IsMasterTriggered(m_sMaster, this))
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgTimerStatus);
			WRITE_BYTE(m_bTimer);
			WRITE_COORD(0);
		MESSAGE_END();
	}
}

void CDodRoundTimer::ResetTimer()
{
	m_bTimer = false;
	m_fRoundTime = m_fTimerLength;
	m_fUpdateTimer = 30;

	if ( ( pev->spawnflags & 1 ) && UTIL_IsMasterTriggered( m_sMaster, this ) )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgTimerStatus );
		WRITE_BYTE( m_bTimer );
		WRITE_COORD( m_fRoundTime );
		MESSAGE_END();
	}
}

void CDodRoundTimer::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !m_bTimer )
		StartTimer();
	else
		StopTimer();
}

void CDodRoundTimer::Think()
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if ( m_bTimer == true )
	{
		m_fRoundTime -= 0.1f;
		m_fUpdateTimer -= 0.1f;

		if ( m_fUpdateTimer <= 0 )
		{
			SendStatus();
			m_fUpdateTimer = 30;
		}

		if ( m_fRoundTime <= 0 )
		{
			StopTimer();

			if ( !FStringNull( pev->target ) )
				FireTargets( STRING( pev->target ), this, NULL, USE_TOGGLE, 0 );
		}
	}
}

LINK_ENTITY_TO_CLASS(dod_control_point, CControlPoint);

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
CControlPoint::CControlPoint()
{
	m_iPointIndex = -1;
	m_bAlliesCantTouch = false;
	m_bAxisCantTouch = false;
	m_bPointVisible = 0;
	m_szGroup[0] = 0;
	m_iAlliesIcon = 1;
	m_iAxisIcon = 2;
	m_iNeutralIcon = 0;
	m_iAlliesModelBodygroup = 0;
	m_iAxisModelBodygroup = 0;
	m_iResetModelBodygroup = 0;
}

void CControlPoint::Spawn()
{
	Precache();
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->sequence = 0;
	pev->framerate = 1;
	m_fCapDelayTime = 1;
	UTIL_SetOrigin( this, pev->origin );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SetTouch( NULL );
	pev->movedir = g_vecZero;
	UTIL_MakeVectors( pev->angles );
	SetOwner( m_iDefaultOwner, FALSE, NULL );
	RoundStore();
}

void CControlPoint::Precache()
{
	if ( pev->noise )
		PRECACHE_SOUND( (char *)STRING( pev->noise ) );

	if ( pev->noise1 )
		PRECACHE_SOUND( (char *)STRING( pev->noise1 ) );

	if ( pev->noise2 )
		PRECACHE_SOUND( (char *)STRING( pev->noise2 ) );

	if ( sz_AlliesModel )
		PRECACHE_MODEL( sz_AlliesModel );

	if ( sz_AxisModel )
		PRECACHE_MODEL( sz_AxisModel );

	if ( sz_ResetModel )
		PRECACHE_MODEL( sz_ResetModel );
}

void CControlPoint::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "point_name" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_pointvalue" ) )
	{
		m_iPointValue = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_team_points" ) )
	{
		m_iTeamPoints = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_points_for_cap" ) )
	{
		m_iCapPoints = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_reset_time" ) )
	{
		m_iResetTime = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_default_owner" ) )
	{
		m_iDefaultOwner = atoi( pkvd->szValue );

		if ( m_iDefaultOwner > 2 )
			m_iDefaultOwner = 0;

		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_axis_capsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise2 = ALLOC_STRING( pkvd->szValue );

		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_allies_capsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise1 = ALLOC_STRING( pkvd->szValue );

		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_resetsound" ) )
	{
		if ( pkvd->szValue )
			pev->noise = ALLOC_STRING( pkvd->szValue );

		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_allies_model" ) )
	{
		strncpy( sz_AlliesModel, pkvd->szValue, sizeof( sz_AlliesModel ) - 1 );
		sz_AlliesModel[sizeof( sz_AlliesModel ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_axis_model" ) )
	{
		strncpy( sz_AxisModel, pkvd->szValue, sizeof( sz_AxisModel ) - 1 );
		sz_AxisModel[sizeof( sz_AxisModel ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_reset_model" ) )
	{
		strncpy( sz_ResetModel, pkvd->szValue, sizeof( sz_ResetModel ) - 1 );
		sz_ResetModel[sizeof( sz_ResetModel ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_allies_model_bodygroup" ) )
	{
		m_iAlliesModelBodygroup = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_axis_model_bodygroup" ) )
	{
		m_iAxisModelBodygroup = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_reset_model_bodygroup" ) )
	{
		m_iResetModelBodygroup = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_allies_target" ) )
	{
		strncpy( sz_AlliesTarget, pkvd->szValue, sizeof( sz_AlliesTarget ) - 1 );
		sz_AlliesTarget[sizeof( sz_AlliesTarget ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_axis_target" ) )
	{
		strncpy( sz_AxisTarget, pkvd->szValue, sizeof( sz_AxisTarget ) - 1 );
		sz_AxisTarget[sizeof( sz_AxisTarget ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_reset_target" ) )
	{
		strncpy( sz_ResetTarget, pkvd->szValue, sizeof( sz_ResetTarget ) - 1 );
		sz_ResetTarget[sizeof( sz_ResetTarget ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_can_allies_touch" ) )
	{
		if ( atoi( pkvd->szValue ) > 0 )
			m_bAlliesCantTouch = true;

		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_can_axis_touch" ) )
	{
		if ( atoi( pkvd->szValue ) > 0 )
			m_bAxisCantTouch = true;

		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "visible" ) )
	{
		m_bPointVisible = atoi( pkvd->szValue );
	}
	else if ( FStrEq( pkvd->szKeyName, "point_win_string" ) )
	{
		strncpy( sz_WinString, pkvd->szValue, sizeof( sz_WinString ) - 1 );
		sz_WinString[sizeof( sz_WinString ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_index" ) )
	{
		m_iPointIndex = atoi( pkvd->szValue ) - 1;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_group" ) )
	{
		strncpy( m_szGroup, pkvd->szValue, sizeof( m_szGroup ) - 1 );
		m_szGroup[sizeof( m_szGroup ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_hud_icon_allies" ) )
	{
		m_iAlliesIcon = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_hud_icon_axis" ) )
	{
		m_iAxisIcon = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_hud_icon_neutral" ) )
	{
		m_iNeutralIcon = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_allies_win_string" ) )
	{
		m_sAlliesCapString = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "point_axis_win_string" ) )
	{
		m_sAxisCapString = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CControlPoint::RoundRespawn()
{
	SetOwner( m_iDefaultOwner, FALSE, NULL );
}

void CControlPoint::Reset()
{
	SetOwner( m_iDefaultOwner, FALSE, NULL );
}

void CControlPoint::SetIndex( int i_index )
{
	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;

	if ( pRules && m_iAlliesIcon == 1 && pRules->IsAlliesBrit() )
		m_iAlliesIcon = 27;

	m_iIndex = i_index;
}

void CControlPoint::SetActive(bool bActive)
{
	m_bActive = bActive;
	if (bActive)
		pev->effects &= ~EF_NODRAW;
	else
		pev->effects |= EF_NODRAW;
}

int CControlPoint::PointValue()
{
	return m_iPointValue;
}

int CControlPoint::GetOwner()
{
	return m_iTeam;
}

char *CControlPoint::GetGroup()
{
	return m_szGroup;
}

void CControlPoint::SendCapString( int team, CBasePlayer *pPlayer )
{
	if ( strlen( sz_WinString ) > 0 )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgCapMsg );

		if ( pPlayer )
			WRITE_BYTE( pPlayer->entindex() );
		else
			WRITE_BYTE( 0 );

		WRITE_STRING( GetName() );
		WRITE_BYTE( team );
		MESSAGE_END();
	}
}

void CControlPoint::SetOwner( int owner, BOOL bMakeSound, CBasePlayer *pPlayer )
{
	if ( !m_bActive )
	{
		SET_MODEL( ENT( pev ), sz_ResetModel );
		SetBodygroup( 0, m_iResetModelBodygroup );
		return;
	}

	if ( g_pGameRules->IsClanMatch() && !g_pGameRules->IsClanMatchActive() )
		return;

	if ( ( g_pGameRules->RoundState() != 1 && g_pGameRules->RoundState() != 0 ) || g_fGameOver )
		return;

	m_iTeam = owner;

	switch ( owner )
	{
	case 0:
		if ( pev->noise && bMakeSound )
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noise ), 1.0, ATTN_NONE );

		SET_MODEL( ENT( pev ), sz_ResetModel );
		SetBodygroup( 0, m_iResetModelBodygroup );

		if ( sz_ResetTarget )
			FireTargets( sz_ResetTarget, this, this, USE_TOGGLE, 0 );
		break;

	case 1:
		if ( pev->noise1 && bMakeSound )
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noise1 ), 1.0, ATTN_NONE );

		SET_MODEL( ENT( pev ), sz_AlliesModel );
		SetBodygroup( 0, m_iAlliesModelBodygroup );

		if ( sz_AlliesTarget )
			FireTargets( sz_AlliesTarget, this, this, USE_TOGGLE, 0 );
		break;

	case 2:
		if ( pev->noise2 && bMakeSound )
			EMIT_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noise2 ), 1.0, ATTN_NONE );

		SET_MODEL( ENT( pev ), sz_AxisModel );
		SetBodygroup( 0, m_iAxisModelBodygroup );

		if ( sz_AxisTarget )
			FireTargets( sz_AxisTarget, this, this, USE_TOGGLE, 0 );
		break;
	}

	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;

	if ( bMakeSound && pRules && pRules->m_RoundDoesCount == 1 )
	{
		if ( pPlayer )
			pPlayer->AddObjScore( m_iCapPoints, TRUE );

		if ( owner == 1 || owner == 2 )
			g_pGameRules->AddToTeamScores( owner, m_iTeamPoints );
	}

	if ( PointIsVisible() )
	{
		if ( !gmsgSetObjOwner )
			return;

		MESSAGE_BEGIN( MSG_ALL, gmsgSetObjOwner );
		WRITE_BYTE( m_iIndex );
		WRITE_BYTE( owner );
		WRITE_BYTE( 0 );
		MESSAGE_END();

		UTIL_SendDirectorMessage( this, NULL, 11 );
	}

	if ( bMakeSound && owner )
		SendCapString( owner, pPlayer );

	m_fNextCapTime = gpGlobals->time + m_fCapDelayTime;
}

void CControlPoint::Touch( CBaseEntity *pOther )
{
	CDoDTeamPlay *pRules;
	pRules = (CDoDTeamPlay *)g_pGameRules;

	if ( g_pGameRules->RoundState() != 1 )
		return;

	if ( !m_bActive )
		return;

	if ( m_fNextCapTime > gpGlobals->time )
		return;

	if ( !pOther->IsPlayer() )
		return;

	if ( !pOther->IsAlive() )
		return;

	CBasePlayer *player = (CBasePlayer *)pOther;

	if ( player->pev->team == 1 && m_bAlliesCantTouch )
		return;

	if ( player->pev->team == 2 && m_bAxisCantTouch )
		return;

	if ( player->pev->team == m_iTeam )
		return;

	SetOwner( player->pev->team, TRUE, player );

	if ( !( player->m_flDisplayHistory & 0x80 ) )
	{
		player->m_flDisplayHistory |= 0x80;
		player->HintMessage( "#Hint_touched_control_point", FALSE );
	}

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_control_point\" - \"%s\"\n",
		STRING( player->pev->netname ),
		GETPLAYERUSERID( player->edict() ),
		GETPLAYERAUTHID( player->edict() ),
		( player->pev->team == 1 ) ? "Allies" : "Axis",
		GetName() );

	m_fNextCapTime = gpGlobals->time + m_fCapDelayTime;
}

class CObjectCapture : public CBaseTrigger
{
public:
	void Spawn();
	void Precache();
	void RoundRespawn();
	void KeyValue(KeyValueData *pkvd);
	void Touch(CBaseEntity *pOther);
	void area_SetIndex(int index);
	void area_SendStatus(CBasePlayer *pPlayer);
	bool m_bCapped;
	char sz_Group[256];
	char sz_DoneTarget[256];
	string_t m_sMaster;
	int m_iAreaIndex;
	char sz_HudIcon[256];
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(dod_object_goal, CObjectCapture);

void CObjectCapture::Spawn()
{
	UTIL_SetOrigin(this, pev->origin);
	InitTrigger();
	RoundStore();
	m_bCapped = false;
}

void CObjectCapture::RoundRespawn()
{
	UTIL_SetOrigin(this, pev->origin);
	InitTrigger();
	m_bCapped = false;
}

void CObjectCapture::Precache()
{
	if ( strlen( sz_HudIcon ) > 0 )
		PRECACHE_MODEL( sz_HudIcon );
}

void CObjectCapture::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "capobj_group"))
	{
		strncpy( sz_Group, pkvd->szValue, sizeof( sz_Group ) - 1 );
		sz_Group[sizeof( sz_Group ) - 1] = 0;
	}
	else if (FStrEq(pkvd->szKeyName, "capobj_donetarget"))
	{
		strncpy( sz_DoneTarget, pkvd->szValue, sizeof( sz_DoneTarget ) - 1 );
		sz_DoneTarget[sizeof( sz_DoneTarget ) - 1] = 0;
	}
	else if (FStrEq(pkvd->szKeyName, "capobj_hud_sprite"))
	{
		strncpy( sz_HudIcon, pkvd->szValue, sizeof( sz_HudIcon ) - 1 );
		sz_HudIcon[sizeof( sz_HudIcon ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue(pkvd);
}

void CObjectCapture::area_SetIndex( int index )
{
	m_iAreaIndex = index;

	if ( m_iAreaIndex > 128 )
		return;

	MESSAGE_BEGIN( MSG_ALL, gmsgClientAreas );
	WRITE_BYTE( m_iAreaIndex );

	if ( sz_HudIcon )
	{
		WRITE_BYTE( -1 );
		WRITE_STRING( sz_HudIcon );
	}
	else
		WRITE_BYTE( 0 );

	MESSAGE_END();
}

void CObjectCapture::area_SendStatus( CBasePlayer *p )
{
	if ( !sz_HudIcon )
		return;

	if ( m_iAreaIndex > 128 )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgClientAreas, NULL, p->pev );
	WRITE_BYTE( m_iAreaIndex );

	if ( sz_HudIcon )
	{
		WRITE_BYTE( -1 );
		WRITE_STRING( sz_HudIcon );
	}
	else
		WRITE_BYTE( 0 );

	MESSAGE_END();
}

void CObjectCapture::Touch( CBaseEntity *pOther )
{
	if ( g_pGameRules->RoundState() != 1 )
		return;

	if ( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;

	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;

	if ( pRules && pRules->RoundState() != 1 )
		return;

	int NumObjects = 0;

	if ( !pOther->IsPlayer() )
		return;

	if ( !pOther->IsAlive() )
		return;

	if ( m_bCapped )
		return;

	CBasePlayer *player = (CBasePlayer *)pOther;

	player->m_signals.Signal( 4 );
	player->m_iObjectAreaIndex = m_iAreaIndex;
	player->SetClientAreaIcon( m_iAreaIndex, true );

	if ( !player->HasObject )
	{
		if ( !( player->m_flDisplayHistory & 0x8000 ) )
		{
			player->m_flDisplayHistory |= 0x8000;
			player->HintMessage( "#Hint_area_requires_object", FALSE );
		}

		return;
	}

	if ( sz_Group && strcmp( sz_Group, player->m_pObject->sz_Group ) )
		return;

	CBaseEntity *ent = NULL;
	CObject *obj = NULL;

	ent = UTIL_FindEntityByClassname( NULL, "dod_object" );

	while ( ent )
	{
		obj = (CObject *)ent;

		if ( obj == player->m_pObject )
			UTIL_SendDirectorMessage( this, player, 11 );
		else if ( !strcmp( obj->sz_Group, player->m_pObject->sz_Group ) && !( obj->pev->effects & EF_NODRAW ) )
			NumObjects++;

		ent = UTIL_FindEntityByClassname( ent, "dod_object" );
	}

	player->HasObject = FALSE;

#pragma warning( suppress : 6011 )
	if ( pRules->m_RoundDoesCount == 1 )
		player->AddObjScore( player->m_pObject->m_iCapPoints, TRUE );

	if ( NumObjects > 0 && !( pev->spawnflags & 1 ) )
	{
		if ( NumObjects == 1 )
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_left_object", UTIL_dtos1( NumObjects ) );
		else
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_left_plural_object", UTIL_dtos1( NumObjects ) );

		if ( !FStringNull( pev->target ) )
			FireTargets( STRING( pev->target ), pOther, this, USE_TOGGLE, 0 );

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer )
			{
				if ( pPlayer->pev->team != pOther->pev->team )
					continue;

				CBasePlayer *plr = (CBasePlayer *)pPlayer;

				if ( pPlayer != pOther && pRules->m_RoundDoesCount == 1 )
					plr->AddObjScore( player->m_pObject->m_iOtherPoints, TRUE );
			}
		}
	}
	else
	{
		if ( sz_DoneTarget )
			FireTargets( sz_DoneTarget, pOther, this, USE_ON, 0 );

		if ( player->m_pObject->sz_DoneTarget )
			FireTargets( player->m_pObject->sz_DoneTarget, pOther, this, USE_ON, 0 );

		if ( pev->spawnflags & 2 )
			m_bCapped = true;

		int entindex = ENTINDEX( player->edict() );

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && pPlayer->pev->team == player->pev->team )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgPStatus, NULL, pPlayer->pev );
				WRITE_BYTE( entindex );
				WRITE_BYTE( player->PlayerStatus() );
				MESSAGE_END();
			}
		}
	}

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_object_goal\"\n",
		STRING( player->pev->netname ),
		GETPLAYERUSERID( player->edict() ),
		GETPLAYERAUTHID( player->edict() ),
		( player->pev->team == 1 ) ? "Allies" : "Axis" );

#pragma warning( suppress : 6011 )
	if ( obj->pev->noise1 )
		EMIT_SOUND( ENT( pOther->pev ), CHAN_STATIC, STRING( obj->pev->noise1 ), 1.0, ATTN_NONE );

	player->HasObject = FALSE;
	player->RemoveObjectIcon();
}

class CAreaCapture : public CBaseTrigger
{
public:
	void Spawn();
	void Precache();
	void RoundRespawn();
	void KeyValue(KeyValueData *pkvd);
	void Think();
	void area_SetIndex(int index);
	void area_SendStatus(CBasePlayer *pPlayer);
	void GetNumRequiredPlayers(int *reqAllies, int *reqAxis);
	int GetNumAlive(int team);
	bool CanUseCatchup();
	void SendNumPlayers(CBasePlayer *pPlayer);
	void ReSendSwipeInfo();
	void StartCapture(int team, int capmode);
	void BreakCapture(int team);
	void EndCapture(int team);
	void SetOwner(int team);
	int m_iCapMode;
	int m_bCapturing;
	int m_nCapturingTeam;
	int m_nOwningTeam;
	int m_nCapTime;
	float m_fTimeRemaining;
	int m_nAlliesNumCap;
	int m_nAxisNumCap;
	int m_nNumAllies;
	int m_nNumAxis;
	bool m_bAlliesCanCap;
	bool m_bAxisCanCap;
	int m_iCappingRequired;
	int m_iCappingPlayers;
	char sz_AlliesCap[256];
	char sz_AxisCap[256];
	char sz_AlliesStart[256];
	char sz_AxisStart[256];
	char sz_AlliesBreak[256];
	char sz_AxisBreak[256];
	int m_iAreaIndex;
	char sz_HudIcon[256];
	CControlPoint *m_pPoint;
	char sz_CapPointName[256];
	char sz_RequiredObjectGroup[256];
	bool m_bRequiresObject;
private:
	void EXPORT AreaTouch(CBaseEntity *pOther);
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(dod_capture_area, CAreaCapture);

void CAreaCapture::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "area_allies_cancap" ) )
	{
		m_bAlliesCanCap = atoi( pkvd->szValue ) > 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_axis_cancap" ) )
	{
		m_bAxisCanCap = atoi( pkvd->szValue ) > 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_allies_numcap" ) )
	{
		m_nAlliesNumCap = max( 1, atoi( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_axis_numcap" ) )
	{
		m_nAxisNumCap = max( 1, atoi( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_allies_startcap" ) )
	{
		strncpy( sz_AlliesStart, pkvd->szValue, sizeof( sz_AlliesStart ) - 1 );
		sz_AlliesStart[sizeof( sz_AlliesStart ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_axis_startcap" ) )
	{
		strncpy( sz_AxisStart, pkvd->szValue, sizeof( sz_AxisStart ) - 1 );
		sz_AxisStart[sizeof( sz_AxisStart ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_allies_endcap" ) )
	{
		strncpy( sz_AlliesCap, pkvd->szValue, sizeof( sz_AlliesCap ) - 1 );
		sz_AlliesCap[sizeof( sz_AlliesCap ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_axis_endcap" ) )
	{
		strncpy( sz_AxisCap, pkvd->szValue, sizeof( sz_AxisCap ) - 1 );
		sz_AxisCap[sizeof( sz_AxisCap ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_allies_breakcap" ) )
	{
		strncpy( sz_AlliesBreak, pkvd->szValue, sizeof( sz_AlliesBreak ) - 1 );
		sz_AlliesBreak[sizeof( sz_AlliesBreak ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_axis_breakcap" ) )
	{
		strncpy( sz_AxisBreak, pkvd->szValue, sizeof( sz_AxisBreak ) - 1 );
		sz_AxisBreak[sizeof( sz_AxisBreak ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_hud_sprite" ) )
	{
		strncpy( sz_HudIcon, pkvd->szValue, sizeof( sz_HudIcon ) - 1 );
		sz_HudIcon[sizeof( sz_HudIcon ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_time_to_cap" ) )
	{
		m_nCapTime = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_cap_point" ) )
	{
		strncpy( sz_CapPointName, pkvd->szValue, sizeof( sz_CapPointName ) - 1 );
		sz_CapPointName[sizeof( sz_CapPointName ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "area_object_group" ) )
	{
		strncpy( sz_RequiredObjectGroup, pkvd->szValue, sizeof( sz_RequiredObjectGroup ) - 1 );
		sz_RequiredObjectGroup[sizeof( sz_RequiredObjectGroup ) - 1] = 0;
		m_bRequiresObject = strlen( sz_RequiredObjectGroup ) > 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

void CAreaCapture::Precache()
{
	if ( strlen( sz_HudIcon ) > 0 )
		PRECACHE_MODEL( sz_HudIcon );
}

void CAreaCapture::Spawn()
{
	InitTrigger();
	Precache();
	m_iAreaIndex = -1;
	pev->sequence = 0;
	pev->framerate = 1;
	UTIL_SetOrigin(this, pev->origin);
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SetTouch(&CAreaCapture::AreaTouch);
	SetThink(NULL);
	m_bCapturing = 0;
	m_nCapturingTeam = -1;
	m_nOwningTeam = -1;
	m_fTimeRemaining = 0;
	pev->nextthink = gpGlobals->time + 0.1f;
	RoundStore();
}

void CAreaCapture::RoundRespawn()
{
	InitTrigger();
	m_iAreaIndex = -1;
	pev->sequence = 0;
	pev->framerate = 1;
	UTIL_SetOrigin( this, pev->origin );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	SetTouch( &CAreaCapture::AreaTouch );
	pev->nextthink = gpGlobals->time + 0.1f;

	if ( m_bCapturing )
		BreakCapture( m_nCapturingTeam );

	m_bCapturing = 0;
	m_nCapturingTeam = -1;
	m_nOwningTeam = -1;
	m_fTimeRemaining = 0;
}

void CAreaCapture::area_SetIndex( int index )
{
	m_iAreaIndex = index;

	if ( m_iAreaIndex > 128 )
		return;

	MESSAGE_BEGIN( MSG_ALL, gmsgClientAreas );
	WRITE_BYTE( m_iAreaIndex );

	if ( sz_HudIcon )
	{
		WRITE_BYTE( -1 );
		WRITE_STRING( sz_HudIcon );
	}
	else
		WRITE_BYTE( 0 );

	MESSAGE_END();
}

void CAreaCapture::area_SendStatus( CBasePlayer *p )
{
	if ( !sz_HudIcon )
		return;

	if ( m_iAreaIndex > 128 )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgClientAreas, NULL, p->pev );
	WRITE_BYTE( m_iAreaIndex );

	if ( sz_HudIcon )
	{
		WRITE_BYTE( -1 );
		WRITE_STRING( sz_HudIcon );
	}
	else
		WRITE_BYTE( 0 );

	MESSAGE_END();

	SendNumPlayers( p );
}

int CAreaCapture::GetNumAlive(int team)
{
	int count = 0;
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex(i);
		if (pPlayer && pPlayer->IsPlayer() && pPlayer->pev->team == team && pPlayer->pev->deadflag == DEAD_NO)
			++count;
	}
	return count;
}

bool CAreaCapture::CanUseCatchup()
{
	bool pararules = false;
	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;

	if ( pRules )
		pararules = !pRules->IsAlliesInfinite() || !pRules->IsAxisInfinite();

	return ( pev->spawnflags & 1 ) && pararules;
}

void CAreaCapture::GetNumRequiredPlayers( int *reqAllies, int *reqAxis )
{
	int countAllies = 0, countAxis = 0;
	int i;

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if ( plr )
		{
			if ( plr->pev->team == 1 )
				countAllies++;
			else if ( plr->pev->team == 2 )
				countAxis++;

			if ( countAllies > m_nAlliesNumCap && countAxis > m_nAxisNumCap )
				i = gpGlobals->maxClients + 1;
		}
	}

	*reqAllies = m_nAlliesNumCap;
	*reqAxis = m_nAxisNumCap;

	*reqAllies = max( 1, *reqAllies );
	*reqAxis = max( 1, *reqAxis );
}

void CAreaCapture::SendNumPlayers( CBasePlayer *pPlayer )
{
	if ( !m_pPoint )
		return;

	int team = 0;
	int num = 0;

	if ( m_nNumAllies > 0 && m_nNumAxis <= 0 && m_nOwningTeam != 1 )
	{
		team = 1;
		num = m_nNumAllies;
	}
	else if ( m_nNumAxis > 0 && m_nNumAllies <= 0 && m_nOwningTeam != 2 )
	{
		team = 2;
		num = m_nNumAxis;
	}

	int reqAllies, reqAxis;
	GetNumRequiredPlayers( &reqAllies, &reqAxis );

	int numrequired;

	if ( team == 1 )
		numrequired = reqAllies;
	else if ( team == 2 )
		numrequired = reqAxis;
	else
		numrequired = -1;

	if ( num > numrequired && numrequired != -1 )
		num = numrequired;

	if ( numrequired > 1 || numrequired == -1 )
	{
		if ( pPlayer )
			MESSAGE_BEGIN( MSG_ONE, gmsgPlayersInArea, NULL, pPlayer->pev );
		else
			MESSAGE_BEGIN( MSG_ALL, gmsgPlayersInArea );

		WRITE_BYTE( m_pPoint->GetIndex() );
		WRITE_BYTE( team );
		WRITE_BYTE( num );
		WRITE_BYTE( numrequired );
		MESSAGE_END();
	}
}

void CAreaCapture::SetOwner(int team)
{
	if (m_bCapturing)
		BreakCapture(m_nCapturingTeam);
	m_nOwningTeam = team;
}

void CAreaCapture::ReSendSwipeInfo()
{
	if ( m_pPoint && m_pPoint->PointIsVisible() )
	{
		float captime = (float)m_nCapTime;
		captime *= ( m_iCappingRequired / m_iCappingPlayers );

		MESSAGE_BEGIN( MSG_ALL, gmsgProgBarUpdate );
		WRITE_BYTE( m_pPoint->GetIndex() );
		WRITE_COORD( captime );
		MESSAGE_END();
	}
}

void CAreaCapture::StartCapture( int team, int capmode )
{
	if ( team == 1 )
		FireTargets( sz_AlliesStart, NULL, this, USE_TOGGLE, 0 );
	else if ( team == 2 )
		FireTargets( sz_AxisStart, NULL, this, USE_TOGGLE, 0 );

	m_nCapturingTeam = team;
	m_fTimeRemaining = (float)m_nCapTime;
	m_bCapturing = TRUE;
	m_iCapMode = capmode;

	int captime = m_nCapTime;

	if ( m_iCapMode == 1 )
	{
		captime *= ( m_iCappingRequired / m_iCappingPlayers );

		if ( m_pPoint && m_pPoint->PointIsVisible() )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgStartProgressBarF );
			WRITE_BYTE( m_pPoint->GetIndex() );
			WRITE_BYTE( team );
			WRITE_COORD( (float)captime );
			MESSAGE_END();
		}
	}
	else
	{
		if ( m_pPoint && m_pPoint->PointIsVisible() )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgStartProgressBar );
			WRITE_BYTE( m_pPoint->GetIndex() );
			WRITE_BYTE( team );
			WRITE_SHORT( captime );
			MESSAGE_END();
		}
	}

	UTIL_SendDirectorMessage( this, NULL, 12 );
}

void CAreaCapture::AreaTouch( CBaseEntity *pOther )
{
	if ( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;

	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;

	if ( pRules && pRules->RoundState() != 1 )
		return;

	if ( g_fGameOver )
		return;

	if ( !m_pPoint )
	{
		CBaseEntity *pEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		m_pPoint = (CControlPoint *)pEnt;
	}

	if ( m_pPoint )
		m_nOwningTeam = m_pPoint->GetOwner();

	if ( !pOther->IsPlayer() )
		return;

	if ( !pOther->IsAlive() )
		return;

	CBasePlayer *player = (CBasePlayer *)pOther;

	bool bAbleToCap = ( player->pev->team == 1 && m_bAlliesCanCap ) || ( player->pev->team == 2 && m_bAxisCanCap );

	if ( !( player->m_flDisplayHistory & 0x40 ) && m_nOwningTeam != player->pev->team && bAbleToCap )
	{
		player->m_flDisplayHistory |= 0x40;
		player->HintMessage( "#Hint_touched_area_capture", FALSE );
	}

	player->m_iCapAreaIconIndex = m_iAreaIndex;

	if ( bAbleToCap )
		player->SetClientAreaIcon( player->m_iCapAreaIconIndex, true );

	player->m_signals.Signal( 2 );

	if ( m_bRequiresObject && bAbleToCap )
	{
		if ( !player->HasObject || !player->m_pObject )
		{
			if ( !( player->m_flDisplayHistory & 0x8000 ) && m_nOwningTeam != player->pev->team )
			{
				player->m_flDisplayHistory |= 0x8000;
				player->HintMessage( "#Hint_area_requires_object", FALSE );
			}

			return;
		}

		if ( !FStrEq( sz_RequiredObjectGroup, player->m_pObject->sz_Group ) )
			return;
	}

	player->m_iCapAreaIndex = m_iAreaIndex;
}

void CAreaCapture::BreakCapture( int team )
{
	if ( !m_bCapturing )
		return;

	m_iCappingRequired = 0;
	m_iCappingPlayers = 0;

	if ( m_nCapturingTeam == 1 )
		FireTargets( sz_AlliesBreak, NULL, this, USE_BREAK, 0 );
	else if ( m_nCapturingTeam == 2 )
		FireTargets( sz_AxisBreak, NULL, this, USE_BREAK, 0 );

	UTIL_ClientPrintAll( HUD_PRINTTALK, ( team == 1 ) ? "#game_capture_broken_allie" : "#game_capture_broken_axis" );

	m_bCapturing = 0;

	if ( m_pPoint )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgCancelProgressBar );
		WRITE_BYTE( m_pPoint->GetIndex() );
		WRITE_BYTE( m_pPoint->GetOwner() );
		MESSAGE_END();
	}

	UTIL_SendDirectorMessage( this, NULL, 12 );
}

void CAreaCapture::EndCapture( int team )
{
	if ( team == 1 )
	{
		FireTargets( sz_AlliesCap, NULL, this, USE_TOGGLE, 0 );

		if ( m_pPoint )
			UTIL_LogPrintf( "Team \"Allies\" triggered a \"dod_capture_area\" - \"%s\"\n", m_pPoint->GetName() );
		else
			UTIL_LogPrintf( "Team \"Allies\" triggered a \"dod_capture_area\"\n" );
	}
	else if ( team == 2 )
	{
		FireTargets( sz_AxisCap, NULL, this, USE_TOGGLE, 0 );

		if ( m_pPoint )
			UTIL_LogPrintf( "Team \"Axis\" triggered a \"dod_capture_area\" - \"%s\"\n", m_pPoint->GetName() );
		else
			UTIL_LogPrintf( "Team \"Axis\" triggered a \"dod_capture_area\"\n" );
	}

	int numObjects;
	numObjects = m_iCappingRequired;

	m_iCappingRequired = 0;
	m_iCappingPlayers = 0;

	CBasePlayer *pCappingPlayer = NULL;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *ent = UTIL_PlayerByIndex( i );

		if ( ent )
		{
			CBasePlayer *player = (CBasePlayer *)ent;

			if ( ( player->m_signals.GetState() & 2 ) && player->m_iCapAreaIndex == m_iAreaIndex )
			{
				if ( m_pPoint )
				{
					UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_capture_area\" - \"%s\"\n",
						STRING( player->pev->netname ),
						GETPLAYERUSERID( player->edict() ),
						GETPLAYERAUTHID( player->edict() ),
						( player->pev->team == 1 ) ? "Allies" : "Axis",
						m_pPoint->GetName() );
				}
				else
				{
					UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered a \"dod_capture_area\"\n",
						STRING( player->pev->netname ),
						GETPLAYERUSERID( player->edict() ),
						GETPLAYERAUTHID( player->edict() ),
						( player->pev->team == 1 ) ? "Allies" : "Axis" );
				}

				if ( !pCappingPlayer )
					pCappingPlayer = player;

				if ( m_pPoint && pCappingPlayer != player )
					player->AddObjScore( m_pPoint->GetPointValue(), TRUE );

#pragma warning( suppress : 28182 )
				if ( m_bRequiresObject && player->HasObject && player->m_pObject )
				{
					if ( player->m_pObject->pev->noise1 )
						EMIT_SOUND( ENT( player->pev ), CHAN_STATIC, STRING( player->m_pObject->pev->noise1 ), 1.0, ATTN_NONE );

					player->HasObject = FALSE;
					player->m_pObject = NULL;
					player->RemoveObjectIcon();

					int entindex = ENTINDEX( player->edict() );

					for ( int playerIndex = 1; playerIndex <= gpGlobals->maxClients; playerIndex++ )
					{
						CBaseEntity *pPlayer = UTIL_PlayerByIndex( playerIndex );

						if ( pPlayer && pPlayer->pev->team == player->pev->team )
						{
							MESSAGE_BEGIN( MSG_ONE, gmsgPStatus, NULL, pPlayer->pev );
							WRITE_BYTE( entindex );
							WRITE_BYTE( player->PlayerStatus() );
							MESSAGE_END();
						}
					}
				}
			}
		}
	}

	m_nOwningTeam = team;
	m_bCapturing = 0;
	m_fTimeRemaining = 0;

	if ( m_pPoint )
		m_pPoint->SetOwner( m_nOwningTeam, TRUE, pCappingPlayer );

	UTIL_SendDirectorMessage( this, NULL, 12 );
}

void CAreaCapture::Think()
{
	if ( g_fGameOver )
		return;

	pev->nextthink = gpGlobals->time + 0.1f;

	int numAllies = 0;
	int numAxis = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *ent = UTIL_PlayerByIndex( i );

		if ( ent )
		{
			CBasePlayer *player = (CBasePlayer *)ent;

			if ( ( player->m_signals.GetState() & 2 ) && player->m_iCapAreaIndex == m_iAreaIndex )
			{
				if ( m_bRequiresObject )
				{
					if ( !player->HasObject || !player->m_pObject || !FStrEq( sz_RequiredObjectGroup, player->m_pObject->sz_Group ) )
					{
						player->m_iCapAreaIndex = -1;
						continue;
					}
				}

				if ( player->pev->team == 1 )
					numAllies++;
				else if ( player->pev->team == 2 )
					numAxis++;
			}
		}
	}

	if ( numAllies != m_nNumAllies || numAxis != m_nNumAxis )
	{
		m_nNumAllies = numAllies;
		m_nNumAxis = numAxis;
		SendNumPlayers( NULL );
	}

	int requiredAllies, requiredAxis;
	GetNumRequiredPlayers( &requiredAllies, &requiredAxis );

	if ( m_bCapturing )
	{
		float capdelta = 0.1f;

		if ( m_iCapMode == 1 )
		{
			float numPlayers;
			float reqPlayers;

			if ( m_nCapturingTeam == 1 )
			{
				reqPlayers = (float)requiredAllies;
				numPlayers = (float)numAllies;
			}
			else
			{
				reqPlayers = (float)requiredAxis;
				numPlayers = (float)numAxis;
			}

			if ( numPlayers >= reqPlayers )
				m_iCapMode = 0;

			capdelta *= ( numPlayers / reqPlayers );
			m_fTimeRemaining -= capdelta;

			if ( numAllies == 0 && numAxis == 0 )
			{
				BreakCapture( m_nCapturingTeam );
				return;
			}

			if ( numAllies > 0 && numAxis > 0 )
			{
				BreakCapture( m_nCapturingTeam );
				return;
			}

			if ( m_fTimeRemaining <= 0 )
			{
				EndCapture( m_nCapturingTeam );
				return;
			}

			if ( m_iCappingPlayers != numPlayers )
			{
				m_iCappingPlayers = (int)numPlayers;
				ReSendSwipeInfo();
			}
		}
		else
		{
			m_fTimeRemaining -= 0.1f;

			if ( numAllies > 0 && numAxis > 0 )
			{
				BreakCapture( m_nCapturingTeam );
				return;
			}

			if ( numAllies == 0 && numAxis == 0 )
			{
				BreakCapture( m_nCapturingTeam );
				return;
			}

			if ( m_nCapturingTeam == 1 )
			{
				if ( numAllies < requiredAllies )
				{
					if ( CanUseCatchup() && numAllies == GetNumAlive( 1 ) )
						m_iCapMode = 1;
					else
						BreakCapture( 1 );
				}
			}
			else if ( m_nCapturingTeam == 2 )
			{
				if ( numAxis < requiredAxis )
				{
					if ( CanUseCatchup() && numAxis == GetNumAlive( 2 ) )
						m_iCapMode = 1;
					else
						BreakCapture( 2 );
				}
			}

			if ( m_fTimeRemaining <= 0 )
			{
				EndCapture( m_nCapturingTeam );
				return;
			}
		}
	}
	else
	{
		if ( numAllies <= 0 && numAxis <= 0 )
			return;

		bool bStarted = false;

		if ( numAllies > 0 && numAxis <= 0 && m_bAlliesCanCap && m_nOwningTeam != 1 )
		{
			if ( numAllies >= requiredAllies )
			{
				m_iCappingRequired = requiredAllies;
				m_iCappingPlayers = numAllies;
				StartCapture( 1, 0 );
				bStarted = true;
			}
		}
		else if ( numAxis > 0 && numAllies <= 0 && m_bAxisCanCap && m_nOwningTeam != 2 )
		{
			if ( numAxis >= requiredAxis )
			{
				m_iCappingRequired = requiredAxis;
				m_iCappingPlayers = numAxis;
				StartCapture( 2, 0 );
				bStarted = true;
			}
		}

		if ( !bStarted && CanUseCatchup() )
		{
			if ( numAllies > 0 && numAxis <= 0 && m_bAlliesCanCap && m_nOwningTeam != 1 && GetNumAlive( 1 ) == numAllies )
			{
				m_iCappingRequired = requiredAllies;
				m_iCappingPlayers = numAllies;
				StartCapture( 1, 1 );
			}
			else if ( numAxis > 0 && numAllies <= 0 && m_bAxisCanCap && m_nOwningTeam != 2 && GetNumAlive( 2 ) == numAxis )
			{
				m_iCappingRequired = requiredAxis;
				m_iCappingPlayers = numAxis;
				StartCapture( 2, 1 );
			}
		}
	}
}

extern int gmsgCameraView;

LINK_ENTITY_TO_CLASS( dod_camera, CDoDCamera );

TYPEDESCRIPTION CDoDCamera::m_SaveData[] =
{
	DEFINE_FIELD( CDoDCamera, m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( CDoDCamera, m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CDoDCamera, m_pentPath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CDoDCamera, m_sPath, FIELD_STRING ),
	DEFINE_FIELD( CDoDCamera, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_flReturnTime, FIELD_TIME ),
	DEFINE_FIELD( CDoDCamera, m_flStopTime, FIELD_TIME ),
	DEFINE_FIELD( CDoDCamera, m_moveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_targetSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_initialSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_acceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_deceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CDoDCamera, m_state, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( CDoDCamera, CBaseDelay );

int CDoDCamera::ObjectCaps()
{
	return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
}

void CDoDCamera::Precache()
{
	PRECACHE_MODEL( "models/mapmodels/null.mdl" );
}

void CDoDCamera::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/mapmodels/null.mdl" );
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->renderamt = 0;
	pev->rendermode = kRenderTransTexture;

	m_angles.x = pev->angles.x;
	m_angles.y = pev->angles.y;
	m_angles.z = pev->angles.z;
	m_origin.x = pev->origin.x;
	m_origin.y = pev->origin.y;
	m_origin.z = pev->origin.z;

	for ( int i = 1; i <= 32; i++ )
		PlayerSlot( i ) = NULL;

	m_initialSpeed = pev->speed;

	if ( m_acceleration == 0 )
		m_acceleration = 500;

	if ( m_deceleration == 0 )
		m_deceleration = 500;
}

void CDoDCamera::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "moveto" ) )
	{
		m_sPath = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "acceleration" ) )
	{
		m_acceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "deceleration" ) )
	{
		m_deceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "teambound" ) )
	{
		m_teamBound = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "zoomlevel" ) )
	{
		m_iFOV = atoi( pkvd->szValue );

		if ( m_iFOV > 90 )
			m_iFOV = 90;
		else if ( m_iFOV < 0 )
			m_iFOV = 0;

		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "overlay" ) )
	{
		m_iOverlay = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "subtitle" ) )
	{
		m_iSubtitle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "triggerstart" ) )
	{
		m_sTrigrStart = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "triggerend" ) )
	{
		m_sTrigrEnd = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "cam_fade" ) )
	{
		m_iCamFade = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

float CDoDCamera::ReturnTimeLeft()
{
	return m_flReturnTime;
}

void CDoDCamera::SetCameraMode( const char *Targetname, int fade )
{
	for ( int i = 1; i <= 32; i++ )
	{
		if ( PlayerSlot( i ) && PlayerSlot( i )->pev->team > 0 )
		{
			if ( fade )
			{
				float fadetime = 2;
				float holdtime = 1;
				int flags = FFADE_OUT | FFADE_STAYOUT;

				switch ( fade )
				{
				case 1:
					fadetime = 0.6;
					flags = FFADE_OUT;
					break;
				case 2:
					fadetime = 2;
					flags = FFADE_OUT;
					break;
				case 3:
					fadetime = 2;
					flags = FFADE_IN;
					break;
				case 4:
					fadetime = 3;
					flags = FFADE_OUT;
					break;
				case 5:
					fadetime = 4;
					flags = FFADE_IN;
					break;
				}

				UTIL_ScreenFade( PlayerSlot( i ), Vector( 0, 0, 0 ), fadetime, holdtime, 255, flags );
			}

			MESSAGE_BEGIN( MSG_ONE, gmsgCameraView, NULL, PlayerSlot( i )->edict() );
			WRITE_STRING( Targetname );
			MESSAGE_END();
		}
	}
}

void CDoDCamera::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_state ) )
		return;

	int i_iFoundPlayer = 0;
	int i;

	for ( i = 1; i <= 32; i++ )
	{
		PlayerSlot( i ) = UTIL_PlayerByIndex( i );

		if ( PlayerSlot( i ) && PlayerSlot( i )->IsPlayer() )
		{
			CBasePlayer *tPlayer = (CBasePlayer *)(CBaseEntity *)PlayerSlot( i );

			if ( m_teamBound != 3 && PlayerSlot( i )->pev->team != m_teamBound )
				PlayerSlot( i ) = NULL;
			else if ( tPlayer->m_iNextClass == -1 )
				PlayerSlot( i ) = NULL;
			else if ( pev->iuser2 == 1 && tPlayer->i_seenAxisSpawnScene )
				PlayerSlot( i ) = NULL;
			else if ( pev->iuser2 == 2 && tPlayer->i_seenAllieSpawnScene )
				PlayerSlot( i ) = NULL;
			else if ( pev->iuser2 == 3 && tPlayer->i_seenAxisWinScene )
				PlayerSlot( i ) = NULL;
			else if ( pev->iuser2 == 4 && tPlayer->i_seenAllieWinScene )
				PlayerSlot( i ) = NULL;
			else if ( pev->iuser2 == 5 && tPlayer->i_seenAxisLooseScene )
				PlayerSlot( i ) = NULL;
			else if ( pev->iuser2 == 6 && tPlayer->i_seenAllieLooseScene )
				PlayerSlot( i ) = NULL;
			else if ( pev->iuser2 == 7 && tPlayer->i_seenAxisDrawScene )
				PlayerSlot( i ) = NULL;
			else if ( pev->iuser2 == 8 && tPlayer->i_seenAllieDrawScene )
				PlayerSlot( i ) = NULL;
		}
		else
			PlayerSlot( i ) = NULL;

		if ( PlayerSlot( i ) )
			i_iFoundPlayer = 1;
	}

	if ( i_iFoundPlayer < 1 )
		return;

	m_state = !m_state;

	if ( m_state == 0 )
	{
		m_flReturnTime = gpGlobals->time;
		return;
	}

	m_flReturnTime = gpGlobals->time + m_flWait;
	pev->speed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	m_hTarget = GetNextTarget();

	if ( !m_hTarget )
		return;

	if ( m_sPath )
		m_pentPath = UTIL_FindEntityByTargetname( NULL, STRING( m_sPath ) );
	else
		m_pentPath = NULL;

	m_flStopTime = gpGlobals->time;

	if ( m_pentPath )
	{
		if ( m_pentPath->pev->speed != 0 )
			m_targetSpeed = m_pentPath->pev->speed;

		m_flStopTime += m_pentPath->GetDelay();
	}

	pev->velocity = Vector( 0, 0, 0 );

	pev->angles.x = m_angles.x;
	pev->angles.y = m_angles.y;
	pev->angles.z = m_angles.z;
	pev->origin.x = m_origin.x;
	pev->origin.y = m_origin.y;
	pev->origin.z = m_origin.z;

	int i_ScopeBin;
	i_ScopeBin = 0;

	for ( i = 1; i <= 32; i++ )
	{
		if ( PlayerSlot( i ) && PlayerSlot( i )->pev->team > 0 )
		{
			CBasePlayer *tPlayer = (CBasePlayer *)(CBaseEntity *)PlayerSlot( i );

			PlayerSlot( i )->pev->vuser1.z = 1;
			tPlayer->UTIL_EdictScreenFade( PlayerSlot( i )->edict(), Vector( 0, 0, 0 ), 1, 15, 0, FFADE_OUT | FFADE_STAYOUT );
			SET_VIEW( PlayerSlot( i )->edict(), edict() );
			tPlayer->EnableControl( FALSE );

			MESSAGE_BEGIN( MSG_ONE, gmsgCameraView, NULL, PlayerSlot( i )->edict() );
			WRITE_STRING( STRING( pev->targetname ) );
			MESSAGE_END();

			if ( pev->iuser2 == 1 )
				tPlayer->i_seenAxisSpawnScene = 1;
			else if ( pev->iuser2 == 2 )
				tPlayer->i_seenAllieSpawnScene = 1;
			else if ( pev->iuser2 == 3 )
				tPlayer->i_seenAxisWinScene = 1;
			else if ( pev->iuser2 == 4 )
				tPlayer->i_seenAllieWinScene = 1;
			else if ( pev->iuser2 == 5 )
				tPlayer->i_seenAxisLooseScene = 1;
			else if ( pev->iuser2 == 6 )
				tPlayer->i_seenAllieLooseScene = 1;
			else if ( pev->iuser2 == 7 )
				tPlayer->i_seenAxisDrawScene = 1;
			else if ( pev->iuser2 == 8 )
				tPlayer->i_seenAllieDrawScene = 1;
		}
	}

	FireTargets( STRING( m_sTrigrStart ), this, this, USE_TOGGLE, 0 );

	CDoDTeamPlay *Dodgamerules = (CDoDTeamPlay *)g_pGameRules;
	Dodgamerules->m_DoDCameraState++;

	SetThink( &CDoDCamera::FollowTarget );
	pev->nextthink = gpGlobals->time;

	m_moveDistance = 0;
	Move();
}

void CDoDCamera::FollowTarget()
{
	if ( !m_hTarget || m_flReturnTime < gpGlobals->time )
	{
		CDoDTeamPlay *Dodgamerules = (CDoDTeamPlay *)g_pGameRules;
		Dodgamerules->m_DoDCameraState--;

		for ( int i = 1; i <= 32; i++ )
		{
			if ( PlayerSlot( i ) && PlayerSlot( i )->pev->team > 0 )
			{
				( (CBasePlayer *)(CBaseEntity *)PlayerSlot( i ) )->EnableControl( TRUE );
				SET_VIEW( PlayerSlot( i )->edict(), PlayerSlot( i )->edict() );
				PlayerSlot( i )->pev->vuser1.z = 0;
				PlayerSlot( i ) = NULL;
			}
		}

		FireTargets( STRING( m_sTrigrEnd ), this, this, USE_TOGGLE, 0 );
		SUB_UseTargets( this, USE_TOGGLE, 0 );
		pev->avelocity = Vector( 0, 0, 0 );
		m_state = 0;

		pev->angles.x = m_angles.x;
		pev->angles.y = m_angles.y;
		pev->angles.z = m_angles.z;
		pev->origin.x = m_origin.x;
		pev->origin.y = m_origin.y;
		pev->origin.z = m_origin.z;
		return;
	}

	Vector vecGoal = UTIL_VecToAngles( m_hTarget->pev->origin - pev->origin );
	vecGoal.x = -vecGoal.x;

	if ( pev->angles.y > 360 )
		pev->angles.y -= 360;

	if ( pev->angles.y < 0 )
		pev->angles.y += 360;

	float dx = vecGoal.x - pev->angles.x;
	float dy = vecGoal.y - pev->angles.y;

	if ( dx < -180 )
		dx += 360;
	if ( dx > 180 )
		dx = dx - 360;

	if ( dy < -180 )
		dy += 360;
	if ( dy > 180 )
		dy = dy - 360;

	pev->avelocity.x = dx * 40 * gpGlobals->frametime;
	pev->avelocity.y = dy * 40 * gpGlobals->frametime;

	if ( !( FBitSet( pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL ) ) )
	{
		pev->velocity = pev->velocity * 0.8;

		if ( pev->velocity.Length() < 10.0 )
			pev->velocity = g_vecZero;
	}

	pev->nextthink = gpGlobals->time;

	Move();
}

void CDoDCamera::Move()
{
	if ( !m_pentPath )
	{
		ALERT( at_aiconsole, "**dod_camera** No Path_corner FOUND !!\n" );
		return;
	}

	m_moveDistance -= pev->speed * gpGlobals->frametime;

	if ( m_moveDistance <= 0 )
	{
		if ( pev->iuser2 > 0 )
		{
			char tmpI[32];

			tmpI[0] = '\0';

			switch ( pev->iuser2 )
			{
			case 1:
				strcpy( tmpI, "axis_spawn_cut" );
				break;
			case 2:
				strcpy( tmpI, "allie_spawn_cut" );
				break;
			case 3:
				strcpy( tmpI, "axis_win_cut" );
				break;
			case 4:
				strcpy( tmpI, "allie_win_cut" );
				break;
			case 5:
				strcpy( tmpI, "axis_loose_cut" );
				break;
			case 6:
				strcpy( tmpI, "allie_loose_cut" );
				break;
			case 7:
				strcpy( tmpI, "axis_draw_cut" );
				break;
			case 8:
				strcpy( tmpI, "allie_draw_cut" );
				break;
			}

			ALERT( at_aiconsole, "**dod_camera** type: %s  Reached %s: %s\n", tmpI, STRING( m_pentPath->pev->classname ), STRING( m_pentPath->pev->targetname ) );
		}
		else
			ALERT( at_aiconsole, "**dod_camera** Reached %s: %s\n", STRING( m_pentPath->pev->classname ), STRING( m_pentPath->pev->targetname ) );

		if ( m_pentPath->pev->message )
		{
			FireTargets( STRING( m_pentPath->pev->message ), this, this, USE_TOGGLE, 0 );

			if ( FBitSet( m_pentPath->pev->spawnflags, SF_CORNER_FIREONCE ) )
				m_pentPath->pev->message = 0;
		}

		if ( FClassnameIs( m_pentPath->pev, "path_corner" ) )
			SetCameraMode( STRING( m_pentPath->pev->targetname ), ( (CPathCorner *)m_pentPath )->m_iCamFade );

		m_pentPath = m_pentPath->GetNextTarget();

		if ( !m_pentPath )
		{
			pev->velocity = g_vecZero;
		}
		else
		{
			if ( m_pentPath->pev->speed != 0 )
				m_targetSpeed = m_pentPath->pev->speed;

			Vector delta = m_pentPath->pev->origin - pev->origin;
			m_moveDistance = delta.Length();
			pev->movedir = delta.Normalize();
			m_flStopTime = gpGlobals->time + m_pentPath->GetDelay();
		}
	}

	if ( m_flStopTime > gpGlobals->time )
		pev->speed = UTIL_Approach( 0, pev->speed, m_deceleration * gpGlobals->frametime );
	else
		pev->speed = UTIL_Approach( m_targetSpeed, pev->speed, m_acceleration * gpGlobals->frametime );

	float fraction = 2 * gpGlobals->frametime;
	pev->velocity = ( ( pev->movedir * pev->speed ) * fraction ) + ( pev->velocity * ( 1 - fraction ) );
}

extern int gmsgInitObjDisplay;
extern int gmsgTimeLeft;
extern cvar_t* pcvar_mp_clan_match;
extern cvar_t* pcvar_mp_clan_scoring;
extern cvar_t* pcvar_mp_clan_scoring_delay;
extern int get_maptime_left();

bool IsThisIndexUnique( CControlPoint *pThis )
{
	CBaseEntity *pEnt = NULL;

	pEnt = UTIL_FindEntityByClassname( NULL, "dod_control_point" );

	while ( pEnt )
	{
		CControlPoint *pPoint = (CControlPoint *)pEnt;

		if ( pPoint->GetPointIndex() == pThis->GetPointIndex() && pPoint != pThis )
			return false;

		pEnt = UTIL_FindEntityByClassname( pEnt, "dod_control_point" );
	}

	return true;
}

int GetClanScoringValues( int team, int cp )
{
	const char *szValues;

	switch ( team )
	{
	case 1:
		szValues = CVAR_GET_STRING( "mp_clan_scoring_values_allies" );
		break;
	case 2:
		szValues = CVAR_GET_STRING( "mp_clan_scoring_values_axis" );
		break;
	default:
		return 0;
	}

	if ( (size_t)cp > strlen( szValues ) - 1 )
	{
		ALERT( at_console, "Tried to get a clan scoring value for a point not defined in mp_clan_scoring_values_x\n" );
		return 0;
	}

	return szValues[cp] - '0';
}

LINK_ENTITY_TO_CLASS( dod_control_point_master, CControlPointMaster );

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
CControlPointMaster::CControlPointMaster()
{
	m_szGroup[0] = 0;
}

void CControlPointMaster::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	UTIL_SetOrigin( this, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	SetTouch( NULL );
	SetThink( &CControlPointMaster::CPMThink );
	FoundPoints = FALSE;
	m_bActive = FALSE;
	pev->nextthink = gpGlobals->time + 2.0;
	m_flNextSpectatorUpdateTime = gpGlobals->time + 10.0;
	RoundStore();
}

void CControlPointMaster::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "point_give_delay_time" ) )
	{
		m_iGivePointsDelay = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "allies_capture_target" ) )
	{
		strncpy( sz_AlliesTarget, pkvd->szValue, sizeof( sz_AlliesTarget ) - 1 );
		sz_AlliesTarget[sizeof( sz_AlliesTarget ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "axis_capture_target" ) )
	{
		strncpy( sz_AxisTarget, pkvd->szValue, sizeof( sz_AxisTarget ) - 1 );
		sz_AxisTarget[sizeof( sz_AxisTarget ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "main" ) )
	{
		m_iMainObjective = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "cpm_group" ) )
	{
		strncpy( m_szGroup, pkvd->szValue, sizeof( m_szGroup ) - 1 );
		m_szGroup[sizeof( m_szGroup ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CControlPointMaster::RoundRespawn()
{
	if ( pcvar_mp_clan_match && pcvar_mp_clan_match->value > 0 &&
		pcvar_mp_clan_scoring && pcvar_mp_clan_scoring->value > 0 )
	{
		m_fGivePointsTime = gpGlobals->time +
			(pcvar_mp_clan_scoring_delay ? pcvar_mp_clan_scoring_delay->value : 30);
	}
	else
		m_fGivePointsTime = gpGlobals->time + m_iGivePointsDelay;
}

bool CControlPointMaster::IsMasteredOn()
{
	if ( m_sMaster )
		return UTIL_IsMasterTriggered( m_sMaster, this ) > 0;

	return true;
}

void CControlPointMaster::BecomeActive()
{
	int i;

	if ( m_bActive )
		return;

	for ( i = 0; i < m_iNumPoints; i++ )
		ControlPointArray[i]->SetActive( true );

	MESSAGE_BEGIN( MSG_ALL, gmsgInitObjDisplay );
	WRITE_BYTE( m_iNumPoints );

	for ( i = 0; i < m_iNumPoints; i++ )
	{
		WRITE_SHORT( ENTINDEX( ENT( ControlPointArray[i]->pev ) ) );
		WRITE_BYTE( ControlPointArray[i]->GetIndex() );
		WRITE_BYTE( GetPointOwner( i ) );
		WRITE_BYTE( ControlPointArray[i]->PointIsVisible() );
		WRITE_BYTE( ControlPointArray[i]->GetNeutralIcon() );
		WRITE_BYTE( ControlPointArray[i]->GetAlliesIcon() );
		WRITE_BYTE( ControlPointArray[i]->GetAxisIcon() );
		WRITE_COORD( ControlPointArray[i]->pev->origin.x );
		WRITE_COORD( ControlPointArray[i]->pev->origin.y );
	}

	MESSAGE_END();

	m_bActive = TRUE;
}

void CControlPointMaster::BecomeInactive()
{
	for ( int i = 0; i < m_iNumPoints; i++ )
		ControlPointArray[i]->SetActive( false );

	m_bActive = FALSE;
}

int CControlPointMaster::GetPointOwner( int point )
{
	if ( point < 20 )
	{
		CControlPoint *pPoint = ControlPointArray[point];

		if ( pPoint )
			return pPoint->GetOwner();
	}

	return 0;
}

void CControlPointMaster::SendObjectiveStatus( CBasePlayer* pPlayer )
{
	if ( !pPlayer )
		MESSAGE_BEGIN( MSG_ALL, gmsgInitObjDisplay );
	else
		MESSAGE_BEGIN( MSG_ONE, gmsgInitObjDisplay, NULL, pPlayer->pev );

	WRITE_BYTE( m_iNumPoints );

	for ( int i = 0; i < m_iNumPoints; i++ )
	{
		WRITE_SHORT( ENTINDEX( ENT( ControlPointArray[i]->pev ) ) );
		WRITE_BYTE( ControlPointArray[i]->GetIndex() );
		WRITE_BYTE( GetPointOwner( i ) );
		WRITE_BYTE( ControlPointArray[i]->PointIsVisible() );
		WRITE_BYTE( ControlPointArray[i]->GetNeutralIcon() );
		WRITE_BYTE( ControlPointArray[i]->GetAlliesIcon() );
		WRITE_BYTE( ControlPointArray[i]->GetAxisIcon() );
		WRITE_COORD( ControlPointArray[i]->pev->origin.x );
		WRITE_COORD( ControlPointArray[i]->pev->origin.y );
	}

	MESSAGE_END();
}

void CControlPointMaster::Reset()
{
	m_bTeamHasWon = false;
}

int CControlPointMaster::TeamOwnsAllPoints()
{
	int team = ControlPointArray[0]->GetOwner();

	for ( int i = 1; i < m_iNumPoints; i++ )
	{
		if ( ControlPointArray[i]->GetOwner() != team )
		{
			team = 0;
			break;
		}
	}

	return team;
}

#pragma warning(push)
#pragma warning(disable: 4701)
void CControlPointMaster::CPMThink()
{
	if ( m_sMaster )
	{
		if ( !FoundPoints || m_bMasteredOn != UTIL_IsMasterTriggered( m_sMaster, this ) )
		{
			m_bMasteredOn = UTIL_IsMasterTriggered( m_sMaster, this );

			if ( m_bMasteredOn )
				BecomeActive();
			else
				BecomeInactive();
		}
	}
	else
		BecomeActive();

	if ( !FoundPoints )
	{
		int arrayIndex = 0;
		BOOL bSuccess = TRUE;
		CBaseEntity *pEnt = NULL;

		pEnt = UTIL_FindEntityByClassname( NULL, "dod_control_point" );

		while ( pEnt && arrayIndex < 20 )
		{
			CControlPoint *pPoint = (CControlPoint *)pEnt;

			if ( !strcmp( m_szGroup, pPoint->GetGroup() ) )
			{
				if ( pPoint->GetPointIndex() < 0 )
					bSuccess = FALSE;

				ControlPointArray[arrayIndex] = pPoint;

				for ( int i = arrayIndex - 1; i >= 0; i-- )
				{
					if ( pPoint->GetPointIndex() == ControlPointArray[i]->GetPointIndex() )
						bSuccess = FALSE;
				}

				pPoint->SetIndex( pPoint->GetPointIndex() );
				pPoint->SetActive( m_bActive > 0 );
				arrayIndex++;
			}

			pEnt = UTIL_FindEntityByClassname( pEnt, "dod_control_point" );
		}

		if ( !bSuccess )
		{
			pEnt = UTIL_FindEntityByClassname( NULL, "info_player_allies" );

			if ( !pEnt )
			{
				for ( int i = 0; i < arrayIndex; i++ )
				{
					if ( ControlPointArray[i] )
						ControlPointArray[i]->SetIndex( i );
				}
			}
			else
			{
				int handled[20];
				Vector spawnpos = pEnt->pev->origin;
				int i;

				for ( i = 0; i < arrayIndex; i++ )
					handled[i] = 0;

				int temp;

				for ( i = 0; i < arrayIndex; i++ )
				{
					float leastdist = 99999;
					float dist;

					for ( int j = 0; j < arrayIndex; j++ )
					{
						if ( !handled[j] )
						{
							dist = ( spawnpos - ControlPointArray[j]->pev->origin ).Length();

							if ( dist < leastdist )
							{
								leastdist = dist;
								ControlPointArray[j]->SetIndex( i );
								temp = j;
							}
						}
					}

					handled[temp] = 1;
				}
			}
		}

		if ( !arrayIndex )
		{
			ALERT( at_aiconsole, "Error! No control points found in map!\n" );
			return;
		}

		m_iNumPoints = arrayIndex;
		FoundPoints = TRUE;
		SendObjectiveStatus( NULL );
	}

	int iRoundState = g_pGameRules->RoundState();

	if ( !m_bActive || iRoundState == 5 || iRoundState == 3 || iRoundState == 4 )
	{
		pev->nextthink = gpGlobals->time + 0.5;
		return;
	}

	bool bClanMatch = g_pGameRules->IsClanMatch() && g_pGameRules->IsClanMatchActive();
	bool bClanScoring = pcvar_mp_clan_scoring && pcvar_mp_clan_scoring->value > 0;

	if ( gpGlobals->time > m_fGivePointsTime )
	{
		int AlliesPoints = 0;
		int AxisPoints = 0;

		if ( bClanMatch && bClanScoring )
		{
			for ( int i = 0; i < m_iNumPoints; i++ )
			{
				switch ( ControlPointArray[i]->GetOwner() )
				{
				case 1:
					AlliesPoints += GetClanScoringValues( 1, i );
					break;
				case 2:
					AxisPoints += GetClanScoringValues( 2, i );
					break;
				}
			}
		}
		else
		{
			for ( int i = 0; i < m_iNumPoints; i++ )
			{
				switch ( ControlPointArray[i]->GetOwner() )
				{
				case 1:
					AlliesPoints += ControlPointArray[i]->PointValue();
					break;
				case 2:
					AxisPoints += ControlPointArray[i]->PointValue();
					break;
				}
			}
		}

		BOOL bFoundAllies = FALSE;
		BOOL bFoundAxis = FALSE;

		for ( int i = 1; i <= gpGlobals->maxClients && ( !bFoundAllies || !bFoundAxis ); i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer )
			{
				if ( pPlayer->pev->team == 1 )
					bFoundAllies = TRUE;

				if ( pPlayer->pev->team == 2 )
					bFoundAxis = TRUE;
			}
		}

		bool bReportScore = bClanMatch;

		if ( AlliesPoints > 0 && bFoundAllies && g_pGameRules->RoundState() == 1 && !g_fGameOver )
		{
			if ( bReportScore )
			{
				if ( AlliesPoints == 1 )
					UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_score_allie_point" );
				else
					UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_score_allie_points", UTIL_dtos1( AlliesPoints ) );
			}

			g_pGameRules->AddToTeamScores( 1, AlliesPoints );
		}

		if ( AxisPoints > 0 && bFoundAxis && g_pGameRules->RoundState() == 1 && !g_fGameOver )
		{
			if ( bReportScore )
			{
				if ( AxisPoints == 1 )
					UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_score_axis_point" );
				else
					UTIL_ClientPrintAll( HUD_PRINTTALK, "#game_score_axis_points", UTIL_dtos1( AxisPoints ) );
			}

			g_pGameRules->AddToTeamScores( 2, AxisPoints );
		}

		if ( bClanMatch && bClanScoring )
			m_fGivePointsTime = gpGlobals->time + ( pcvar_mp_clan_scoring_delay ? pcvar_mp_clan_scoring_delay->value : 30 );
		else
			m_fGivePointsTime = gpGlobals->time + m_iGivePointsDelay;
	}

	int team = TeamOwnsAllPoints();

	if ( team > 0 )
	{
		switch ( team )
		{
		case 1:
			if ( sz_AlliesTarget )
			{
				CDoDTeamPlay *Dodgamerules;
				Dodgamerules = (CDoDTeamPlay *)g_pGameRules;
				FireTargets( sz_AlliesTarget, NULL, this, USE_TOGGLE, 0 );
				m_bTeamHasWon = true;
			}
			break;
		case 2:
			if ( sz_AxisTarget )
			{
				FireTargets( sz_AxisTarget, NULL, this, USE_TOGGLE, 0 );
				m_bTeamHasWon = true;
			}
			break;
		}
	}

	if ( gpGlobals->time > m_flNextSpectatorUpdateTime )
	{
		MESSAGE_BEGIN( MSG_SPEC, gmsgInitObjDisplay );
		WRITE_BYTE( m_iNumPoints );

		for ( int i = 0; i < m_iNumPoints; i++ )
		{
			WRITE_SHORT( ENTINDEX( ENT( ControlPointArray[i]->pev ) ) );
			WRITE_BYTE( ControlPointArray[i]->GetIndex() );
			WRITE_BYTE( GetPointOwner( i ) );
			WRITE_BYTE( ControlPointArray[i]->PointIsVisible() );
			WRITE_BYTE( ControlPointArray[i]->GetNeutralIcon() );
			WRITE_BYTE( ControlPointArray[i]->GetAlliesIcon() );
			WRITE_BYTE( ControlPointArray[i]->GetAxisIcon() );
			WRITE_COORD( ControlPointArray[i]->pev->origin.x );
			WRITE_COORD( ControlPointArray[i]->pev->origin.y );
		}

		MESSAGE_END();

		MESSAGE_BEGIN( MSG_SPEC, gmsgTimeLeft );
		WRITE_SHORT( get_maptime_left() );
		MESSAGE_END();

		m_flNextSpectatorUpdateTime = gpGlobals->time + 20;
	}

	pev->nextthink = gpGlobals->time + 0.5;
}
#pragma warning(pop)
