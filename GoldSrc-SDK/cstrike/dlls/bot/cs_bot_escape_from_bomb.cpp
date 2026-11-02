#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"

void EscapeFromBombState::OnEnter( CCSBot* bot )
{
	bot->StandUp();
	bot->Run();
	bot->m_pathLength = 0;
	bot->m_ladder = NULL;
	bot->EquipKnife();
}

void EscapeFromBombState::OnUpdate( CCSBot* bot )
{
	const Vector* bomb = bot->m_gameState.GetBombPosition();
	if ( !bomb )
	{
		bot->Idle();
		return;
	}

	bot->EquipKnife();
	bot->UpdateLookAround( false );
	if ( bot->UpdatePathMovement( true ) != 0 )
	{
		FarAwayFromPositionFunctor cost( bomb );
		CNavArea* area = FindMinimumCostArea( bot->m_lastKnownArea, cost );
		bot->ComputePath( area, NULL, FASTEST_ROUTE );
	}
}

void EscapeFromBombState::OnExit( CCSBot* bot )
{
	bot->EquipBestWeapon( false );
}

const char* EscapeFromBombState::GetName() const
{
	return "EscapeFromBomb";
}
