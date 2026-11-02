#pragma warning( push )
#pragma warning( disable: 4100 )
#include "cs_bot.h"
#pragma warning( pop )
#include "bot_profile.h"

void InvestigateNoiseState::AttendCurrentNoise( CCSBot *bot )
{
	if ( bot->m_noiseTimestamp <= 0 || gpGlobals->time - bot->m_noiseTimestamp < bot->GetProfile()->GetReactionTime() )
		return;
	m_noisePosition = bot->m_noisePosition;
	if ( bot->IsWellPastSafe() && bot->HasNotSeenEnemyForLongTime() && bot->m_noisePriority != PRIORITY_HIGH )
		bot->GetChatter()->HeardNoise( bot->m_noiseTimestamp > 0 ? &bot->m_noisePosition : NULL );
	bot->PrintIfWatched( "Attending to noise...\n" );
	bot->ComputePath( bot->m_noiseArea, &m_noisePosition, SAFEST_ROUTE );
	bot->m_noiseTimestamp = 0;
}

void InvestigateNoiseState::OnEnter( CCSBot *bot )
{
	AttendCurrentNoise( bot );
}

void InvestigateNoiseState::OnExit( CCSBot *bot )
{
	bot->Run();
}

void InvestigateNoiseState::OnUpdate( CCSBot *bot )
{
	float newRange;
	if ( bot->ShouldInvestigateNoise( &newRange ) &&
		(m_noisePosition - bot->pev->origin).LengthSquared() > (newRange + 100.0f) * (newRange + 100.0f) )
		AttendCurrentNoise( bot );
	if ( bot->m_pathLength )
	{
		bot->UpdateLookAround( false );
		float range = (m_noisePosition - bot->pev->origin).Length();
		if ( (!bot->IsUsingKnife() && (range >= 1500 || bot->GetFriendsRemaining() > 2)) || bot->IsHurrying() )
			bot->Run();
		else
			bot->Walk();
		if ( range < 200 && bot->IsLookingAtPosition( &m_noisePosition, 20 ) && bot->IsVisible( &m_noisePosition, false ) )
		{
			bot->PrintIfWatched( "Noise location is clear.\n" );
			bot->Idle();
			return;
		}
		if ( bot->UpdatePathMovement( true ) == 0 )
			return;
	}
	bot->Idle();
}

const char *InvestigateNoiseState::GetName() const
{
	return "InvestigateNoise";
}
