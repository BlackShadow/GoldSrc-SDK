#include "extdll.h"
#include "tutor_base_states.h"

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CBaseTutorState::CBaseTutorState()
{
}

CBaseTutorState::~CBaseTutorState()
{
}

int CBaseTutorState::GetType()
{
	return m_type;
}

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CBaseTutorStateSystem::CBaseTutorStateSystem()
{
}

CBaseTutorStateSystem::~CBaseTutorStateSystem()
{
}

int CBaseTutorStateSystem::GetCurrentStateType()
{
	return m_currentState != NULL ? m_currentState->GetType() : 0;
}
