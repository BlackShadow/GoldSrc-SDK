#pragma warning( push )
#pragma warning( disable: 4100 )
#include "cs_bot.h"
#pragma warning( pop )
#include "cs_bot_manager.h"

extern int gmsgBotVoice;

void CCSBot::StartVoiceFeedback( float duration )
{
	m_voiceStartTime = gpGlobals->time;
	m_voiceEndTime = gpGlobals->time + duration;
	CBasePlayer *recipient = NULL;
	while ( (recipient = GetNextRadioRecipient( recipient )) != NULL )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgBotVoice, NULL, recipient->edict() );
		WRITE_BYTE( 1 );
		WRITE_BYTE( entindex() );
		MESSAGE_END();
	}
}

void CCSBot::EndVoiceFeedback( bool force )
{
	if ( force || m_voiceEndTime != 0 )
	{
		m_voiceEndTime = 0;
		MESSAGE_BEGIN( MSG_ALL, gmsgBotVoice );
		WRITE_BYTE( 0 );
		WRITE_BYTE( entindex() );
		MESSAGE_END();
	}
}

void CCSBot::SendRadioMessage( GameEventType event )
{
	if ( (unsigned int)(event - EVENT_RADIO_COVER_ME) > EVENT_RADIO_ENEMY_DOWN - EVENT_RADIO_COVER_ME )
		return;
	PrintIfWatched( "%3.1f: SendRadioMessage( %s )\n", gpGlobals->time, GameEventName[event] );
	TheCSBots()->SetRadioMessageTimestamp( event, m_iTeam );
	m_lastSentRadioTimestamp = gpGlobals->time;
	char selection[2];
	selection[1] = '\0';
	if ( (unsigned int)(event - EVENT_RADIO_COVER_ME) <= 5 )
	{
		selection[0] = char( '1' + event - EVENT_RADIO_COVER_ME );
		ClientCommand( "radio1" );
	}
	else if ( (unsigned int)(event - EVENT_RADIO_GO_GO_GO) <= 5 )
	{
		selection[0] = char( '1' + event - EVENT_RADIO_GO_GO_GO );
		ClientCommand( "radio2" );
	}
	else
	{
		selection[0] = char( '1' + event - EVENT_RADIO_AFFIRMATIVE );
		ClientCommand( "radio3" );
	}
	ClientCommand( "menuselect", selection );
	ClientCommand( "menuselect", "10" );
}

bool CCSBot::IsRadioCommand( GameEventType event ) const
{
	switch ( event )
	{
	case EVENT_RADIO_REPORT_IN_TEAM:
	case EVENT_RADIO_AFFIRMATIVE:
	case EVENT_RADIO_ENEMY_SPOTTED:
	case EVENT_RADIO_SECTOR_CLEAR:
	case EVENT_RADIO_REPORTING_IN:
	case EVENT_RADIO_NEGATIVE:
	case EVENT_RADIO_ENEMY_DOWN:
		return false;
	default:
		return true;
	}
}

bool CCSBot::RespondToHelpRequest( CBasePlayer *player, Place place, float range )
{
	if ( IsRogue() || IsBusy() )
		return false;
	if ( range > 0 )
	{
		PathCost cost( this, SAFEST_ROUTE );
		CNavArea *area = TheNavAreaGrid.GetNearestNavArea( &player->pev->origin );
		float distance = NavAreaTravelDistance( m_lastKnownArea, area, cost );
		if ( distance < 0 || distance > range )
			return false;
	}
	if ( place )
	{
		if ( place == GetPlace() )
			return true;
		const Vector *position = GetRandomSpotAtPlace( place );
		MoveTo( position ? position : &player->pev->origin, FASTEST_ROUTE );
	}
	else
	{
		if ( (player->pev->origin - pev->origin).LengthSquared() < 750 * 750 )
			return true;
		MoveTo( &player->pev->origin, FASTEST_ROUTE );
	}
	BotStatement *statement = new BotStatement( &m_chatter, BOT_STATEMENT_SAY, 3 );
	statement->AppendPhrase( TheBotPhrases->GetPhrase( "OnMyWay" ) );
	m_chatter.AddStatement( statement, false );
	return true;
}

void CCSBot::RespondToRadioCommands()
{
	CBaseEntity *senderEntity = m_radioSender;
	if ( senderEntity && senderEntity->IsPlayer() && ((CBasePlayer *)senderEntity)->IsBot() )
	{
		m_lastRadioCommand = EVENT_INVALID;
		return;
	}
	if ( m_lastRadioCommand == EVENT_INVALID )
		return;
	m_chatter.ResetRadioSilenceDuration();
	if ( m_lastRadioCommand != EVENT_RADIO_REPORT_IN_TEAM && IsBusy() )
	{
		m_lastRadioCommand = EVENT_INVALID;
		return;
	}
	float delay = GetProfile()->GetReactionTime() * 2 + 1;
	if ( IsRogue() )
		delay += 2;
	if ( delay > gpGlobals->time - m_radioCommandTimestamp )
		return;
	if ( !m_isFollowing && IsRogue() )
	{
		if ( IsRadioCommand( m_lastRadioCommand ) )
			m_chatter.Negative();
		m_lastRadioCommand = EVENT_INVALID;
		return;
	}
	CBasePlayer *sender = (CBasePlayer *)(CBaseEntity *)m_radioSender;
	if ( !sender )
		return;
	bool affirmative = false;
	switch ( m_lastRadioCommand )
	{
	case EVENT_RADIO_COVER_ME:
	case EVENT_RADIO_REGROUP_TEAM:
	case EVENT_RADIO_FOLLOW_ME:
	case EVENT_RADIO_STICK_TOGETHER_TEAM:
		if ( !m_isFollowing )
		{
			Follow( sender );
			sender->m_allowAutoFollowTime = 0;
			affirmative = true;
		}
		break;
	case EVENT_RADIO_HOLD_THIS_POSITION:
		m_task = TASK_HOLD_POSITION;
		m_taskEntity = NULL;
		StopFollowing();
		sender->m_allowAutoFollowTime = gpGlobals->time + 60;
		Hide( TheNavAreaGrid.GetNearestNavArea( &m_radioPosition ), -1, 750, false );
		affirmative = true;
		break;
	case EVENT_RADIO_TAKING_FIRE:
	case EVENT_RADIO_ENEMY_SPOTTED:
	case EVENT_RADIO_NEED_BACKUP:
		if ( !m_isFollowing )
		{
			Follow( sender );
			BotStatement *statement = new BotStatement( &m_chatter, BOT_STATEMENT_SAY, 3 );
			statement->AppendPhrase( TheBotPhrases->GetPhrase( "OnMyWay" ) );
			m_chatter.AddStatement( statement, false );
			sender->m_allowAutoFollowTime = 0;
		}
		break;
	case EVENT_RADIO_GO_GO_GO:
	case EVENT_RADIO_STORM_THE_FRONT:
		StopFollowing();
		Hunt();
		sender->m_allowAutoFollowTime = gpGlobals->time + 60;
		affirmative = true;
		break;
	case EVENT_RADIO_TEAM_FALL_BACK:
		affirmative = TryToRetreat();
		break;
	case EVENT_RADIO_REPORT_IN_TEAM:
		m_chatter.ReportingIn();
		break;
	case EVENT_RADIO_SECTOR_CLEAR:
		if ( TheCSBots()->GetScenario() == CCSBotManager::SCENARIO_DEFUSE_BOMB && m_iTeam == CT && TheCSBots()->IsBombPlanted() )
		{
			const CCSBotManager::Zone *zone = TheCSBots()->GetClosestZone( &sender->pev->origin );
			if ( zone )
			{
				m_gameState.ClearBombsite( zone->m_index );
				if ( m_task == TASK_FIND_TICKING_BOMB )
					Idle();
				affirmative = true;
			}
		}
		break;
	case EVENT_RADIO_GET_OUT_OF_THERE:
		if ( TheCSBots()->IsBombPlanted() )
		{
			EscapeFromBomb();
			sender->m_allowAutoFollowTime = gpGlobals->time + 60;
			affirmative = true;
		}
		break;
	default:
		return;
	}
	if ( affirmative )
	{
		m_chatter.Affirmative();
		if ( IsRadioCommand( m_lastRadioCommand ) && IsUsingGrenade() )
			EquipBestWeapon( false );
	}
	m_lastRadioCommand = EVENT_INVALID;
}
