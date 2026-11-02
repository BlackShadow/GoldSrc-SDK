#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"
#include "cs_bot_manager.h"
#include "hostage.h"

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CSGameState::CSGameState( CCSBot* owner )
{
	m_owner = owner;
	m_isRoundOver = false;
	m_bombState = BOMB_CARRIED;
	m_isPlantedBombPositionKnown = false;
	m_plantedBombsite = -1;
	m_bombsiteCount = 0;
	m_bombsiteSearchIndex = 0;
	for ( int i = 0; i < ARRAYSIZE( m_hostage ); ++i )
	{
		m_hostage[i].hostage = NULL;
		m_hostage[i].knownPosition = Vector( 0.0f, 0.0f, 0.0f );
		m_hostage[i].isAtKnownPosition = false;
		m_hostage[i].isAlive = false;
		m_hostage[i].isValid = true;
	}
}

void CSGameState::Reset()
{
	m_isRoundOver = false;
	SetBombState( BOMB_CARRIED );
	m_lastSawBomber.Invalidate();
	m_lastSawLooseBomb.Invalidate();
	m_isPlantedBombPositionKnown = false;
	m_plantedBombsite = -1;
	m_bombsiteCount = TheCSBots()->GetZoneCount();
	for ( int i = 0; i < m_bombsiteCount; ++i )
	{
		m_isBombsiteClear[i] = false;
		m_bombsiteSearchOrder[i] = i;
	}
	for ( int i = 0; i < m_bombsiteCount; ++i )
	{
		int index = RANDOM_LONG( i, m_bombsiteCount - 1 );
		int old = m_bombsiteSearchOrder[i];
		m_bombsiteSearchOrder[i] = m_bombsiteSearchOrder[index];
		m_bombsiteSearchOrder[index] = old;
	}
	m_bombsiteSearchIndex = 0;
	InitializeHostageInfo();
}

bool CSGameState::IsRoundOver() const
{
	return m_isRoundOver;
}

void CSGameState::SetBombState( BombState state )
{
	if ( m_bombState != state )
		m_bombState = state;
}

void CSGameState::UpdateBomber( const Vector* position )
{
	m_bomberPosition = *position;
	m_lastSawBomber.Start();
	SetBombState( BOMB_CARRIED );
}

void CSGameState::UpdateLooseBomb( const Vector* position )
{
	m_looseBombPosition = *position;
	m_lastSawLooseBomb.Start();
	SetBombState( BOMB_LOOSE );
}

float CSGameState::TimeSinceLastSawBomber() const
{
	return m_lastSawBomber.GetElapsedTime();
}

float CSGameState::TimeSinceLastSawLooseBomb() const
{
	return m_lastSawLooseBomb.GetElapsedTime();
}

bool CSGameState::IsLooseBombLocationKnown() const
{
	return m_bombState == BOMB_LOOSE && m_lastSawLooseBomb.HasStarted();
}

bool CSGameState::IsPlantedBombLocationKnown() const
{
	return m_bombState == BOMB_PLANTED && m_isPlantedBombPositionKnown;
}

const Vector* CSGameState::GetBombPosition() const
{
	switch ( m_bombState )
	{
	case BOMB_CARRIED:
		return m_lastSawBomber.HasStarted() ? &m_bomberPosition : NULL;
	case BOMB_LOOSE:
		return m_lastSawLooseBomb.HasStarted() ? &m_looseBombPosition : NULL;
	case BOMB_PLANTED:
		return m_isPlantedBombPositionKnown ? &m_plantedBombPosition : NULL;
	default:
		return NULL;
	}
}

void CSGameState::UpdatePlantedBomb( const Vector* position )
{
	const CCSBotManager::Zone* zone = TheCSBots()->GetClosestZone( position );
	if ( zone != NULL )
		m_plantedBombsite = zone->m_index;
	else
	{
		CONSOLE_ECHO( "ERROR: Bomb planted outside of a zone!\n" );
		m_plantedBombsite = -1;
	}
	m_plantedBombPosition = *position;
	m_isPlantedBombPositionKnown = true;
	SetBombState( BOMB_PLANTED );
}

void CSGameState::MarkBombsiteAsPlanted( int bombsite )
{
	m_plantedBombsite = bombsite;
	SetBombState( BOMB_PLANTED );
}

int CSGameState::GetPlantedBombsite() const
{
	return m_bombState == BOMB_PLANTED ? m_plantedBombsite : -1;
}

void CSGameState::ClearBombsite( int bombsite )
{
	if ( bombsite >= 0 && bombsite < m_bombsiteCount )
		m_isBombsiteClear[bombsite] = true;
}

bool CSGameState::IsBombsiteClear( int bombsite ) const
{
	return bombsite >= 0 && bombsite < m_bombsiteCount && m_isBombsiteClear[bombsite];
}

int CSGameState::GetNextBombsiteToSearch()
{
	if ( m_bombsiteCount <= 0 )
		return 0;
	for ( ;; )
	{
		for ( int i = m_bombsiteSearchIndex; i < m_bombsiteCount; ++i )
		{
			int bombsite = m_bombsiteSearchOrder[i];
			if ( !m_isBombsiteClear[bombsite] )
			{
				m_bombsiteSearchIndex = i;
				return bombsite;
			}
		}
		for ( int i = 0; i < m_bombsiteCount; ++i )
			m_isBombsiteClear[i] = false;
		m_bombsiteSearchIndex = 0;
	}
}

void CSGameState::InitializeHostageInfo()
{
	m_hostageCount = 0;
	m_areAllHostagesRescued = false;
	m_isHostageTaken = false;
	CBaseEntity* entity = NULL;
	while ( (entity = UTIL_FindEntityByClassname( entity, "hostage_entity" )) != NULL )
	{
		if ( m_hostageCount >= ARRAYSIZE( m_hostage ) )
			break;
		if ( entity->pev->takedamage == DAMAGE_YES )
		{
			HostageInfo& info = m_hostage[m_hostageCount++];
			info.hostage = static_cast<CHostage*>( entity );
			info.knownPosition = entity->pev->origin;
			info.isAtKnownPosition = true;
			info.isAlive = true;
			info.isValid = true;
		}
	}
}

void CSGameState::AllHostagesGone()
{
	for ( int i = 0; i < m_hostageCount; ++i )
		m_hostage[i].isAtKnownPosition = false;
}

void CSGameState::OnEvent( GameEventType event, CBaseEntity*, CBaseEntity* other )
{
	switch ( event )
	{
	case EVENT_BOMB_PLANTED:
		SetBombState( BOMB_PLANTED );
		if ( m_owner->m_iTeam == TERRORIST && other != NULL )
			UpdatePlantedBomb( &other->pev->origin );
		break;
	case EVENT_BOMB_DEFUSED:
		SetBombState( BOMB_DEFUSED );
		break;
	case EVENT_BOMB_EXPLODED:
		SetBombState( BOMB_EXPLODED );
		break;
	case EVENT_ALL_HOSTAGES_RESCUED:
		m_areAllHostagesRescued = true;
		break;
	case EVENT_TERRORISTS_WIN:
	case EVENT_CTS_WIN:
	case EVENT_ROUND_DRAW:
		m_isRoundOver = true;
		break;
	default:
		break;
	}
}

bool CSGameState::IsAtPlantedBombsite() const
{
	if ( m_bombState != BOMB_PLANTED )
		return false;
	const CCSBotManager::Zone* zone = TheCSBots()->GetClosestZone( &m_owner->pev->origin );
	return zone != NULL && m_plantedBombsite == zone->m_index;
}

bool CSGameState::AreAllHostagesGone() const
{
	if ( m_areAllHostagesRescued )
		return true;
	for ( int i = 0; i < m_hostageCount; ++i )
	{
		if ( m_owner->m_iTeam == CT )
		{
			if ( m_hostage[i].hostage->IsValid() )
				return false;
		}
		else if ( m_hostage[i].isAtKnownPosition && m_hostage[i].isAlive )
		{
			return false;
		}
	}
	return true;
}

bool CSGameState::AreAllHostagesBeingRescued() const
{
	if ( m_areAllHostagesRescued )
		return false;
	bool found = false;
	for ( int i = 0; i < m_hostageCount; ++i )
	{
		if ( m_owner->m_iTeam == CT )
		{
			if ( !m_hostage[i].hostage->IsValid() )
				continue;
			if ( !m_hostage[i].hostage->IsFollowingSomeone() )
				return false;
			found = true;
		}
		else
		{
			if ( m_hostage[i].isAtKnownPosition && m_hostage[i].isAlive )
				return false;
			if ( !m_hostage[i].isAtKnownPosition && m_hostage[i].isAlive )
				found = true;
		}
	}
	return found;
}

unsigned int CSGameState::ValidateHostagePositions()
{
	if ( !m_validateHostagePositionsTimer.IsElapsed() )
		return 0;
	m_validateHostagePositionsTimer.Start( 0.5f );
	int knownBefore = 0;
	for ( int i = 0; i < m_hostageCount; ++i )
		knownBefore += m_hostage[i].isAtKnownPosition;

	const unsigned int hostageDied = 1;
	const unsigned int hostageMoved = 2;
	const unsigned int allHostagesGone = 4;
	unsigned int changes = 0;
	for ( int i = 0; i < m_hostageCount; ++i )
	{
		HostageInfo& info = m_hostage[i];
		CHostage* hostage = info.hostage;
		if ( m_owner->IsVisible( &hostage->pev->origin, true ) )
		{
			if ( !hostage->IsValid() )
			{
				info.isAtKnownPosition = false;
				if ( info.isAlive )
					changes |= hostageDied;
				info.isAlive = false;
			}
			else if ( hostage->IsFollowingSomeone() )
			{
				info.isAtKnownPosition = false;
			}
			else
			{
				info.knownPosition = hostage->pev->origin;
				info.isAtKnownPosition = true;
			}
		}
		else if ( info.isAtKnownPosition && m_owner->IsVisible( &info.knownPosition, true ) )
		{
			if ( !hostage->IsValid() )
			{
				info.isAlive = false;
				info.isAtKnownPosition = false;
				changes |= hostageDied;
			}
			else if ( hostage->IsFollowingSomeone() || (hostage->pev->origin - info.knownPosition).LengthSquared() > 2500.0f )
			{
				info.isAtKnownPosition = false;
				changes |= hostageMoved;
			}
		}
	}
	int knownNow = 0;
	for ( int i = 0; i < m_hostageCount; ++i )
		knownNow += m_hostage[i].isAtKnownPosition;
	if ( knownBefore > 0 && knownNow == 0 )
		changes = (changes & ~(hostageMoved | allHostagesGone)) | allHostagesGone;
	return changes;
}

CHostage* CSGameState::GetNearestFreeHostage( Vector* position ) const
{
	if ( m_owner == NULL || m_owner->GetLastKnownArea() == NULL )
		return NULL;
	CNavArea* startArea = m_owner->GetLastKnownArea();
	CHostage* closest = NULL;
	const Vector* closestPosition = NULL;
	float closestDistance = 10000000000.0f;
	for ( int i = 0; i < m_hostageCount; ++i )
	{
		const HostageInfo& info = m_hostage[i];
		const Vector* knownPosition;
		if ( m_owner->m_iTeam == CT )
		{
			if ( !info.hostage->IsValid() || info.hostage->IsFollowingSomeone() )
				continue;
			knownPosition = &info.hostage->pev->origin;
		}
		else
		{
			if ( !info.isAtKnownPosition )
				continue;
			knownPosition = &info.knownPosition;
		}
		CNavArea* goalArea = TheNavAreaGrid.GetNearestNavArea( knownPosition );
		if ( goalArea == NULL )
			continue;
		ShortestPathCost cost;
		float distance = NavAreaTravelDistance( startArea, goalArea, cost );
		if ( distance < closestDistance )
		{
			closestDistance = distance;
			closest = info.hostage;
			closestPosition = knownPosition;
		}
	}
	if ( position != NULL && closestPosition != NULL )
		*position = *closestPosition;
	return closest;
}

CHostage* CSGameState::GetNearestVisibleFreeHostage() const
{
	CHostage* closest = NULL;
	float closestDistance = 1000000000.0f;
	for ( int i = 0; i < m_hostageCount; ++i )
	{
		CHostage* hostage = m_hostage[i].hostage;
		if ( !hostage->IsValid() || hostage->IsFollowingSomeone() )
			continue;
		Vector head = hostage->pev->origin + Vector( 0.0f, 0.0f, 54.0f );
		float distance = (head - m_owner->pev->origin).LengthSquared();
		if ( distance < closestDistance && m_owner->IsVisible( &head, false ) )
		{
			closestDistance = distance;
			closest = hostage;
		}
	}
	return closest;
}

const Vector* CSGameState::GetRandomFreeHostagePosition() const
{
	if ( m_owner == NULL )
		return NULL;
	const Vector* position[12];
	int count = 0;
	for ( int i = 0; i < m_hostageCount; ++i )
	{
		const HostageInfo& info = m_hostage[i];
		if ( m_owner->m_iTeam == CT )
		{
			if ( info.hostage->IsValid() && !info.hostage->IsFollowingSomeone() )
				position[count++] = &info.hostage->pev->origin;
		}
		else if ( info.isAtKnownPosition )
		{
			position[count++] = &info.knownPosition;
		}
	}
	if ( count == 0 )
		return NULL;
	return position[RANDOM_LONG( 0, count - 1 )];
}
