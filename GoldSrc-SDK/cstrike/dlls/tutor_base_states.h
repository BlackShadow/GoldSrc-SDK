#ifndef TUTOR_BASE_STATES_H
#define TUTOR_BASE_STATES_H

#include "GameEvent.h"

class CBaseEntity;

class CBaseTutorState
{
public:
	CBaseTutorState();
	virtual ~CBaseTutorState();
	virtual int CheckForStateTransition( GameEventType event, CBaseEntity* entity, CBaseEntity* other ) = 0;
	virtual char* GetStateString() = 0;
	int GetType();

protected:
	int m_type;
};

class CBaseTutorStateSystem
{
public:
	CBaseTutorStateSystem();
	virtual ~CBaseTutorStateSystem();
	virtual bool UpdateState( GameEventType event, CBaseEntity* entity, CBaseEntity* other ) = 0;
	virtual char* GetCurrentStateString() = 0;
	int GetCurrentStateType();

protected:
	virtual CBaseTutorState* ConstructNewState( int stateType ) = 0;
	CBaseTutorState* m_currentState;
};

#endif
