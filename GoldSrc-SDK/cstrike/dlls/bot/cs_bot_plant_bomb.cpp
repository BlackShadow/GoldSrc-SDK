#pragma warning( push )
#pragma warning( disable: 4100 )
#include "cs_bot.h"
#pragma warning( pop )

void PlantBombState::OnEnter( CCSBot *bot )
{
	bot->Crouch();
	bot->SetDisposition( CCSBot::SELF_DEFENSE );
	float angle = bot->pev->v_angle.y;
	float sine = BotSIN( angle );
	float cosine = BotCOS( angle );
	Vector position = bot->pev->origin;
	position.x += cosine * 10;
	position.y += sine * 10;
	position.z -= bot->IsCrouching() ? 18.0f : 36.0f;
	bot->SetLookAt( "Plant bomb on floor", &position, PRIORITY_HIGH, -1, false, 5 );
}

void PlantBombState::OnUpdate( CCSBot *bot )
{
	if ( bot->m_pActiveItem && FStrEq( STRING( bot->m_pActiveItem->pev->classname ), "weapon_c4" ) )
		bot->PrimaryAttack();
	else
		bot->SelectItem( "weapon_c4" );
	if ( !bot->m_bHasC4 )
	{
		bot->SetTask( CCSBot::TASK_GUARD_TICKING_BOMB );
		bot->SetTaskEntity( NULL );
		bot->Hide( (CNavArea *)NULL, -1, 750, false );
	}
	if ( gpGlobals->time - bot->m_stateTimestamp > 5 )
		bot->Idle();
}

void PlantBombState::OnExit( CCSBot *bot )
{
	bot->EquipBestWeapon( false );
	bot->StandUp();
	bot->ResetStuckMonitor();
	bot->SetDisposition( CCSBot::ENGAGE_AND_INVESTIGATE );
	bot->m_lookAtState = 0;
	bot->m_lookAtDescription = NULL;
}

const char *PlantBombState::GetName() const
{
	return "PlantBomb";
}
