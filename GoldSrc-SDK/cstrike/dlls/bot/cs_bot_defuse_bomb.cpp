#pragma warning( push )
#pragma warning( disable: 4100 )
#include "cs_bot.h"
#pragma warning( pop )
#include "cs_bot_manager.h"

void DefuseBombState::OnEnter( CCSBot *bot )
{
	bot->Crouch();
	bot->SetDisposition( CCSBot::SELF_DEFENSE );
	BotStatement *statement = new BotStatement( bot->GetChatter(), BOT_STATEMENT_SAY, 3 );
	statement->AppendPhrase( TheBotPhrases->GetPhrase( "DefusingBomb" ) );
	bot->GetChatter()->AddStatement( statement, false );
}

void DefuseBombState::OnUpdate( CCSBot *bot )
{
	const Vector *position = bot->GetGameState()->GetBombPosition();
	if ( !position )
		bot->PrintIfWatched( "In Defuse state, but don't know where the bomb is!\n" );
	else
	{
		bot->SetLookAt( "Defuse bomb", position, PRIORITY_HIGH, -1, false, 5 );
		bot->UseEnvironment();
		if ( gpGlobals->time - bot->m_stateTimestamp > 1 )
		{
			CBasePlayer *defuser = TheCSBots()->GetBombDefuser();
			if ( !defuser )
			{
				bot->PrintIfWatched( "Failed to start defuse, giving up\n" );
				bot->Idle();
				return;
			}
			if ( defuser != bot )
			{
				bot->PrintIfWatched( "Someone else started defusing, givin up\n" );
				bot->Idle();
				return;
			}
		}
		if ( TheCSBots()->IsBombPlanted() )
			return;
	}
	bot->Idle();
}

void DefuseBombState::OnExit( CCSBot *bot )
{
	bot->StandUp();
	bot->ResetStuckMonitor();
	bot->SetTask( CCSBot::TASK_SEEK_AND_DESTROY );
	bot->SetTaskEntity( NULL );
	bot->SetDisposition( CCSBot::ENGAGE_AND_INVESTIGATE );
	bot->m_lookAtState = 0;
	bot->m_lookAtDescription = NULL;
}

const char *DefuseBombState::GetName() const
{
	return "DefuseBomb";
}
