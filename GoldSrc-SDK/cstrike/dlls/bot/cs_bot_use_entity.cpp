#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"
#include "cs_bot_manager.h"

void UseEntityState::OnEnter( CCSBot* )
{
}

void UseEntityState::OnUpdate( CCSBot* bot )
{
	if ( bot->m_stateTimestamp - gpGlobals->time > 5.0f )
	{
		bot->Idle();
		return;
	}

	Vector position = m_entity->pev->origin + Vector( 0.0f, 0.0f, 36.0f );
	bot->SetLookAt( "Use entity", &position, PRIORITY_HIGH, -1.0f, false, 5.0f );
	if ( bot->IsLookingAtPosition( &position, 20.0f ) )
	{
		if ( TheCSBots()->GetScenario() == CCSBotManager::SCENARIO_RESCUE_HOSTAGES
			&& bot->m_iTeam == CT && bot->m_task == CCSBot::TASK_COLLECT_HOSTAGES )
			++bot->m_hostageEscortCount;
		bot->UseEnvironment();
		bot->Idle();
	}
}

void UseEntityState::OnExit( CCSBot* bot )
{
	bot->m_lookAtState = 0;
	bot->m_lookAtDescription = NULL;
	bot->ResetStuckMonitor();
}

const char* UseEntityState::GetName() const
{
	return "UseEntity";
}
