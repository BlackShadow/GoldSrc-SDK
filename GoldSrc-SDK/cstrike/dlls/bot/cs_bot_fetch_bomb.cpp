#pragma warning( push )
#pragma warning( disable: 4100 )
#include "cs_bot.h"
#pragma warning( pop )
#include "cs_bot_manager.h"

void FetchBombState::OnEnter( CCSBot *bot )
{
	bot->m_pathLength = 0;
	bot->m_ladder = NULL;
}

void FetchBombState::OnUpdate( CCSBot *bot )
{
	CBaseEntity *bomb = TheCSBots()->GetLooseBomb();
	if ( !bomb )
	{
		bot->PrintIfWatched( "Bomb not loose\n" );
		bot->Idle();
		return;
	}
	if ( !bot->m_pathLength )
	{
		const Vector *position = &bomb->pev->origin;
		CNavArea *area = TheNavAreaGrid.GetNavArea( position );
		if ( !bot->ComputePath( area, position, SAFEST_ROUTE ) )
		{
			bot->PrintIfWatched( "Fetch bomb pathfind failed\n" );
			bot->Idle();
		}
	}
	bot->UpdateLookAround( false );
	if ( bot->UpdatePathMovement( true ) != 0 )
		bot->Idle();
}

const char *FetchBombState::GetName() const
{
	return "FetchBomb";
}
