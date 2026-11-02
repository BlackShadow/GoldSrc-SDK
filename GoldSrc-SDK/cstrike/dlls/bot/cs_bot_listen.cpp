#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"
#include "bot_profile.h"

bool CCSBot::CanSeeNoisePosition() const
{
	Vector position = m_noisePosition + Vector( 0.0f, 0.0f, 36.0f );
	m_eyePosition = pev->origin + pev->view_ofs;
	TraceResult trace;
	UTIL_TraceLine( m_eyePosition, position, ignore_monsters, ignore_glass, pev->pContainingEntity, &trace );
	return trace.flFraction == 1.0f;
}

bool CCSBot::CanHearNearbyEnemyGunfire( float range ) const
{
	if ( gpGlobals->time - m_noiseTimestamp > 0.5f || m_noisePriority != PRIORITY_HIGH )
		return false;
	float distanceSquared = (pev->origin - m_noisePosition).LengthSquared();
	if ( range > 0.0f && distanceSquared > range * range )
		return false;
	if ( !CanSeeNoisePosition() )
		return false;
	EHANDLE& enemy = const_cast<EHANDLE&>( m_enemy );
	if ( IsAttacking() && static_cast<int>( enemy ) != 0 )
		return distanceSquared <= (enemy->pev->origin - pev->origin).LengthSquared() - 10000.0f;
	return true;
}

bool CCSBot::UpdateLookAtNoise()
{
	if ( m_noiseTimestamp <= 0.0f )
		return false;
	float elapsed = gpGlobals->time - m_noiseTimestamp;
	if ( elapsed < m_profile->GetReactionTime() || elapsed > 0.5f )
		return false;
	bool nearbyRecentEnemy = false;
	if ( gpGlobals->time - m_lastSawEnemyTime < 5.0f )
	{
		bool nearby = (pev->origin - m_lastEnemyPosition).LengthSquared() < 750.0f * 750.0f;
		nearbyRecentEnemy = IsAtHidingSpot() < nearby;
	}
	else
		IsAtHidingSpot();
	Vector target;
	if ( nearbyRecentEnemy || CanSeeNoisePosition() )
		target = m_noisePosition;
	else
	{
		if ( m_approachPointCount == 0 )
			return false;
		int closest = -1;
		float closestDistance = 99999997952.0f;
		for ( int i = 0; i < m_approachPointCount; ++i )
		{
			float distance = (m_approachPoint[i] - m_noisePosition).LengthSquared();
			if ( closestDistance > distance )
			{
				closestDistance = distance;
				closest = i;
			}
		}
		if ( closest == -1 || !BendLineOfSight( &pev->origin, &m_approachPoint[closest], &target ) )
			return false;
	}
	PriorityType priority = m_noisePriority == PRIORITY_HIGH ? PRIORITY_HIGH : PRIORITY_MEDIUM;
	target.z += 36.0f;
	float duration;
	if ( IsAtHidingSpot() )
		duration = GetEnemiesRemaining() == 1 ? RANDOM_FLOAT( 5.0f, 15.0f ) : RANDOM_FLOAT( 2.0f, 5.0f );
	else
		duration = RANDOM_FLOAT( 1.0f, 2.0f );
	SetLookAt( "Noise", &target, priority, duration, true, 5.0f );
	return true;
}

bool CCSBot::ShouldInvestigateNoise( float* range )
{
	if ( m_noiseTravelDistanceChecked || !IsWellPastSafe() || GetDisposition() != ENGAGE_AND_INVESTIGATE )
		return false;
	if ( m_noiseTimestamp <= 0.0f || gpGlobals->time - m_noiseTimestamp < m_profile->GetReactionTime() )
		return false;
	if ( gpGlobals->time - m_noiseCheckTimestamp < 0.25f )
		return false;
	m_noiseCheckTimestamp = gpGlobals->time;
	Vector delta = m_noisePosition - pev->origin;
	float distance = delta.Length();
	if ( abs( static_cast<int>( delta.z ) ) > 120 )
	{
		PathCost cost( this, SAFEST_ROUTE );
		float travel = NavAreaTravelDistance( m_lastKnownArea, m_noiseArea, cost );
		m_noiseTravelDistanceChecked = true;
		if ( travel < 0.0f || travel > 1500.0f )
			return false;
		if ( travel > distance )
			distance = travel;
	}
	if ( IsAtHidingSpot() && distance > 400.0f * m_profile->GetAggression() + 100.0f )
		return false;
	float chance = 1.0f - distance / 2000.0f;
	if ( GetFriendsRemaining() > 2 )
	{
		float penalty = GetFriendsRemaining() * 0.05f;
		chance -= penalty <= 0.5f ? penalty : 0.5f;
	}
	if ( chance >= RANDOM_FLOAT( 0.0f, 1.0f ) )
	{
		if ( range )
			*range = distance;
		return true;
	}
	return false;
}
