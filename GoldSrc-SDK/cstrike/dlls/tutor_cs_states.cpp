#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "bot_util.h"
#include "tutor_cs_states.h"

static char* g_TutorStateStrings[] =
{
	"#Cstrike_TutorState_Undefined",
	"#Cstrike_TutorState_Looking_For_Hostage",
	"#Cstrike_TutorState_Escorting_Hostage",
	"#Cstrike_TutorState_Following_Hostage_Escort",
	"#Cstrike_TutorState_Moving_To_Bombsite",
	"#Cstrike_TutorState_Looking_For_Bomb_Carrier",
	"#Cstrike_TutorState_Guarding_Loose_Bomb",
	"#Cstrike_TutorState_Defusing_Bomb",
	"#Cstrike_TutorState_Guarding_Hostage",
	"#Cstrike_TutorState_Moving_To_Intercept_Enemy",
	"#Cstrike_TutorState_Looking_For_Hostage_Escort",
	"#Cstrike_TutorState_Attacking_Hostage_Escort",
	"#Cstrike_TutorState_Escorting_Bomb_Carrier",
	"#Cstrike_TutorState_Moving_To_Bomb_Site",
	"#Cstrike_TutorState_Planting_Bomb",
	"#Cstrike_TutorState_Guarding_Bomb",
	"#Cstrike_TutorState_Looking_For_Loose_Bomb",
	"#Cstrike_TutorState_Running_Away_From_Ticking_Bomb",
	"#Cstrike_TutorState_Buy_Time",
	"#Cstrike_TutorState_Waiting_For_Start"
};

CCSTutorStateSystem::CCSTutorStateSystem()
{
	m_currentState = new CCSTutorUndefinedState;
}

CCSTutorStateSystem::~CCSTutorStateSystem()
{
	if ( m_currentState != NULL )
	{
		delete m_currentState;
		m_currentState = NULL;
	}
}

bool CCSTutorStateSystem::UpdateState( GameEventType event, CBaseEntity* entity, CBaseEntity* other )
{
	if ( m_currentState == NULL )
		m_currentState = new CCSTutorUndefinedState;
	if ( m_currentState == NULL )
		return false;
	int nextStateType = m_currentState->CheckForStateTransition( event, entity, other );
	if ( nextStateType == TUTORSTATE_UNDEFINED )
		return false;
	if ( m_currentState != NULL )
		delete m_currentState;
	m_currentState = NULL;
	m_currentState = ConstructNewState( nextStateType );
	return true;
}

char* CCSTutorStateSystem::GetCurrentStateString()
{
	return m_currentState != NULL ? m_currentState->GetStateString() : NULL;
}

CBaseTutorState* CCSTutorStateSystem::ConstructNewState( int stateType )
{
	switch ( stateType )
	{
	case TUTORSTATE_UNDEFINED:
		return new CCSTutorUndefinedState;
	case TUTORSTATE_BUYTIME:
		return new CCSTutorBuyMenuState;
	case TUTORSTATE_WAITING_FOR_START:
		return new CCSTutorWaitingForStartState;
	default:
		return NULL;
	}
}

CCSTutorUndefinedState::CCSTutorUndefinedState()
{
	m_type = TUTORSTATE_UNDEFINED;
}

CCSTutorUndefinedState::~CCSTutorUndefinedState()
{
}

int CCSTutorUndefinedState::CheckForStateTransition( GameEventType event, CBaseEntity* entity, CBaseEntity* other )
{
	if ( event == EVENT_PLAYER_SPAWNED )
		return HandlePlayerSpawned( entity, other );
	return TUTORSTATE_UNDEFINED;
}

char* CCSTutorUndefinedState::GetStateString()
{
	return NULL;
}

int CCSTutorUndefinedState::HandlePlayerSpawned( CBaseEntity* entity, CBaseEntity* )
{
	if ( entity == NULL || !entity->IsPlayer() )
		return TUTORSTATE_UNDEFINED;
	CBasePlayer* player = UTIL_GetLocalPlayer();
	if ( player == NULL || entity != player )
		return TUTORSTATE_UNDEFINED;
	return TUTORSTATE_WAITING_FOR_START;
}

CCSTutorWaitingForStartState::CCSTutorWaitingForStartState()
{
	m_type = TUTORSTATE_WAITING_FOR_START;
}

CCSTutorWaitingForStartState::~CCSTutorWaitingForStartState()
{
}

int CCSTutorWaitingForStartState::CheckForStateTransition( GameEventType event, CBaseEntity* entity, CBaseEntity* other )
{
	switch ( event )
	{
	case EVENT_PLAYER_SPAWNED:
		return HandlePlayerSpawned( entity, other );
	case EVENT_BUY_TIME_START:
		return HandleBuyTimeStart( entity, other );
	default:
		return TUTORSTATE_UNDEFINED;
	}
}

char* CCSTutorWaitingForStartState::GetStateString()
{
	return g_TutorStateStrings[m_type];
}

int CCSTutorWaitingForStartState::HandlePlayerSpawned( CBaseEntity* entity, CBaseEntity* )
{
	if ( entity == NULL || !entity->IsPlayer() )
		return TUTORSTATE_UNDEFINED;
	CBasePlayer* player = UTIL_GetLocalPlayer();
	if ( player == NULL || entity != player )
		return TUTORSTATE_UNDEFINED;
	return TUTORSTATE_WAITING_FOR_START;
}

int CCSTutorWaitingForStartState::HandleBuyTimeStart( CBaseEntity*, CBaseEntity* )
{
	return TUTORSTATE_BUYTIME;
}

CCSTutorBuyMenuState::CCSTutorBuyMenuState()
{
	m_type = TUTORSTATE_BUYTIME;
}

CCSTutorBuyMenuState::~CCSTutorBuyMenuState()
{
}

int CCSTutorBuyMenuState::CheckForStateTransition( GameEventType event, CBaseEntity* entity, CBaseEntity* other )
{
	if ( event == EVENT_ROUND_START )
		return HandleRoundStart( entity, other );
	return TUTORSTATE_UNDEFINED;
}

char* CCSTutorBuyMenuState::GetStateString()
{
	return g_TutorStateStrings[m_type];
}

int CCSTutorBuyMenuState::HandleRoundStart( CBaseEntity*, CBaseEntity* )
{
	return TUTORSTATE_WAITING_FOR_START;
}
