#include "hostage_localnav.h"
#include <math.h>
#include <string.h>

EHANDLE CLocalNav::queue[20];
int CLocalNav::qptr;
int CLocalNav::tot_inqueue;
float CLocalNav::nodeval;
float CLocalNav::flNextCvarCheck;
float CLocalNav::flLastThinkTime;
EHANDLE CLocalNav::hostages[20];
int CLocalNav::tot_hostages;
float CLocalNav::s_flStepSize;

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CLocalNav::CLocalNav( CHostage *owner )
{
	m_pOwner = owner;
	m_pTargetEnt = NULL;
	m_nodeArr = new localnode_t[100];
	if ( tot_hostages <= 19 )
		hostages[tot_hostages++] = owner;
}

CLocalNav::~CLocalNav()
{
	delete[] m_nodeArr;
}

void CLocalNav::Reset()
{
	flNextCvarCheck = flLastThinkTime = 0;
	tot_inqueue = 0;
	nodeval = 0;
	qptr = 0;
	tot_hostages = 0;
}

void CLocalNav::HostagePrethink()
{
	for ( int i = 0; i < tot_hostages; ++i )
	{
		if ( hostages[i] )
			GetClassPtr( (CHostage *)hostages[i]->pev )->PreThink();
	}
}

void CLocalNav::Think()
{
	static cvar_t *sv_stepsize;
	if ( gpGlobals->time >= flNextCvarCheck )
	{
		if ( !sv_stepsize )
			sv_stepsize = CVAR_GET_POINTER( "sv_stepsize" );
		flNextCvarCheck = gpGlobals->time + 1;
		s_flStepSize = sv_stepsize ? sv_stepsize->value : 18;
	}
	float elapsed = gpGlobals->time - flLastThinkTime;
	flLastThinkTime = gpGlobals->time;
	HostagePrethink();
	nodeval -= elapsed * 250;
	if ( nodeval < 0 )
		nodeval = 0;
	else if ( nodeval > 17 )
		return;
	if ( !tot_inqueue )
		return;
	EHANDLE callback;
	callback = NULL;
	for ( ;; )
	{
		callback = queue[qptr];
		if ( callback || tot_inqueue <= 0 )
			break;
		if ( ++qptr == 20 )
			qptr = 0;
		if ( --tot_inqueue == 0 )
		{
			callback = NULL;
			break;
		}
	}
	if ( callback )
	{
		CHostage *hostage = GetClassPtr( (CHostage *)callback->pev );
		if ( ++qptr == 20 )
			qptr = 0;
		--tot_inqueue;
		hostage->NavReady();
	}
}

void CLocalNav::RequestNav( CHostage *caller )
{
	if ( nodeval > 17 || tot_inqueue != 0 )
	{
		if ( tot_inqueue <= 19 )
		{
			int index = qptr;
			for ( int i = 0; i < tot_inqueue; ++i )
			{
				if ( GetClassPtr( (CHostage *)queue[index]->pev ) == caller )
					return;
				if ( ++index == 20 )
					index = 0;
			}
			queue[index] = caller;
			++tot_inqueue;
		}
	}
	else
		caller->NavReady();
}

node_index_t CLocalNav::AddNode( node_index_t parent, Vector &position, int offsetX, int offsetY, byte depth )
{
	if ( m_nindexAvailableNode == 100 )
		return -1;
	localnode_t *node = &m_nodeArr[m_nindexAvailableNode];
	node->vecLoc = position;
	node->bDepth = depth;
	node->fSearched = FALSE;
	node->nindexParent = parent;
	node->offsetX = offsetX;
	node->offsetY = offsetY;
	return m_nindexAvailableNode++;
}

node_index_t CLocalNav::NodeExists( int offsetX, int offsetY )
{
	for ( int i = m_nindexAvailableNode - 1; i != -1; --i )
	{
		if ( m_nodeArr[i].offsetX == offsetX && m_nodeArr[i].offsetY == offsetY )
			return i;
	}
	return -1;
}

void CLocalNav::AddPathNodes( node_index_t source, int noMonsters )
{
	AddPathNode( source, 1, 0, noMonsters );
	AddPathNode( source, -1, 0, noMonsters );
	AddPathNode( source, 0, 1, noMonsters );
	AddPathNode( source, 0, -1, noMonsters );
	AddPathNode( source, 1, 1, noMonsters );
	AddPathNode( source, 1, -1, noMonsters );
	AddPathNode( source, -1, 1, noMonsters );
	AddPathNode( source, -1, -1, noMonsters );
}

void CLocalNav::AddPathNode( node_index_t source, int offsetX, int offsetY, int noMonsters )
{
	Vector start, destination;
	byte depth;
	if ( source == -1 )
	{
		start = m_vecStartingLoc;
		destination = start + Vector( offsetX * 26.0f, offsetY * 26.0f, 0 );
		depth = 1;
	}
	else
	{
		localnode_t *node = &m_nodeArr[source];
		int x = node->offsetX + offsetX;
		int y = node->offsetY + offsetY;
		if ( NodeExists( x, y ) != -1 )
			return;
		start = node->vecLoc;
		destination = start + Vector( offsetX * 26.0f, offsetY * 26.0f, 0 );
		for ( int i = m_nindexAvailableNode - 1; i != -1; --i )
		{
			localnode_t *nearby = &m_nodeArr[i];
			if ( abs( nearby->offsetX - x ) <= 1 && abs( nearby->offsetY - y ) <= 1 )
			{
				if ( PathTraversable( nearby->vecLoc, destination, noMonsters ) )
				{
					source = i;
					node = nearby;
				}
			}
		}
		start = node->vecLoc;
		depth = node->bDepth + 1;
		offsetX = x;
		offsetY = y;
	}
	if ( PathTraversable( start, destination, noMonsters ) )
		AddNode( source, destination, offsetX, offsetY, depth );
}

node_index_t CLocalNav::GetBestNode( Vector &, Vector &destination )
{
	float bestCost = 1000000.0f;
	node_index_t best = -1;
	for ( int i = 0; i < m_nindexAvailableNode; ++i )
	{
		localnode_t *node = &m_nodeArr[i];
		if ( node->fSearched )
			continue;
		float distFromStart = node->bDepth * 26.0;
		float zDiff = node->vecLoc.z - destination.z;
		float distToDest = (node->vecLoc - destination).Length();
		float scale = fabs( zDiff ) <= s_flStepSize ? 1.0f : 1.25f;
		float cost = (distToDest + distFromStart) * scale;
		if ( bestCost > cost )
		{
			bestCost = cost;
			best = i;
		}
	}
	return best;
}

node_index_t CLocalNav::FindDirectPath( Vector &start, Vector &destination, float targetRadius, int noMonsters )
{
	Vector direction = (destination - start).Normalize();
	Vector actualDestination = destination - direction * targetRadius;
	if ( !PathTraversable( start, actualDestination, noMonsters ) )
		return -1;
	m_nindexAvailableNode = 0;
	Vector position = start;
	node_index_t parent = -1;
	while ( (position - actualDestination).Length2D() >= 26 && m_nindexAvailableNode != 100 )
	{
		position = position + direction * 26;
		parent = AddNode( parent, position, 0, 0, 0 );
		if ( parent == -1 )
			break;
	}
	return parent;
}

node_index_t CLocalNav::FindPath( Vector &start, Vector &destination, float targetRadius, int noMonsters )
{
	node_index_t result = FindDirectPath( start, destination, targetRadius, noMonsters );
	if ( result != -1 )
		return result;
	m_vecStartingLoc = start;
	m_nindexAvailableNode = 0;
	AddPathNodes( -1, noMonsters );
	for ( node_index_t best = GetBestNode( start, destination ); ; best = GetBestNode( start, destination ) )
	{
		if ( best == -1 )
		{
			result = -1;
			break;
		}
		localnode_t *node = &m_nodeArr[best];
		node->fSearched = TRUE;
		float distance = (destination - node->vecLoc).Length2D();
		if ( distance <= targetRadius || distance <= 26 )
		{
			result = best;
			break;
		}
		if ( distance - targetRadius > (100 - m_nindexAvailableNode) * 26.0 )
			break;
		if ( m_nindexAvailableNode == 100 )
			break;
		AddPathNodes( best, noMonsters );
		if ( m_nindexAvailableNode <= 0 )
			break;
	}
	if ( m_nindexAvailableNode <= 10 ) nodeval += 2;
	else if ( m_nindexAvailableNode <= 20 ) nodeval += 4;
	else if ( m_nindexAvailableNode <= 30 ) nodeval += 8;
	else if ( m_nindexAvailableNode <= 40 ) nodeval += 13;
	else if ( m_nindexAvailableNode <= 50 ) nodeval += 19;
	else if ( m_nindexAvailableNode <= 60 ) nodeval += 26;
	else if ( m_nindexAvailableNode <= 70 ) nodeval += 34;
	else if ( m_nindexAvailableNode <= 80 ) nodeval += 43;
	else if ( m_nindexAvailableNode <= 90 ) nodeval += 53;
	else if ( m_nindexAvailableNode <= 100 ) nodeval += 64;
	else if ( m_nindexAvailableNode <= 110 ) nodeval += 76;
	else if ( m_nindexAvailableNode <= 120 ) nodeval += 89;
	else if ( m_nindexAvailableNode <= 130 ) nodeval += 103;
	else if ( m_nindexAvailableNode <= 140 ) nodeval += 118;
	else if ( m_nindexAvailableNode <= 150 ) nodeval += 134;
	else if ( m_nindexAvailableNode <= 160 ) nodeval += 151;
	else nodeval += 169;
	return result;
}

int CLocalNav::SetupPathNodes( node_index_t index, Vector *nodes, int )
{
	int count = 0;
	for ( ; index != -1; index = m_nodeArr[index].nindexParent )
		nodes[count++] = m_nodeArr[index].vecLoc;
	return count;
}

int CLocalNav::GetFurthestTraversableNode( Vector &start, Vector *nodes, int count, int noMonsters )
{
	for ( int i = 0; i < count; ++i )
	{
		if ( PathTraversable( start, nodes[i], noMonsters ) )
			return i;
	}
	return -1;
}

BOOL CLocalNav::PathClear( Vector &source, Vector &destination, int noMonsters, TraceResult &trace )
{
	TRACE_MONSTER_HULL( m_pOwner->edict(), source, destination, noMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		return FALSE;
	if ( trace.flFraction == 1 )
		return TRUE;
	if ( trace.pHit == m_pTargetEnt )
	{
		m_fTargetEntHit = TRUE;
		return TRUE;
	}
	return FALSE;
}

BOOL CLocalNav::StepTraversable( Vector &source, Vector &destination, int noMonsters, TraceResult &trace )
{
	Vector start = source;
	start.z += s_flStepSize;
	Vector end( destination.x, destination.y, start.z );
	TRACE_MONSTER_HULL( m_pOwner->edict(), start, end, noMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		return FALSE;
	if ( trace.flFraction != 1 )
	{
		if ( trace.pHit == m_pTargetEnt )
			m_fTargetEntHit = TRUE;
		else if ( (trace.vecEndPos - start).Length() < 1 )
			return FALSE;
	}
	start = end = trace.vecEndPos;
	end.z -= s_flStepSize;
	TRACE_MONSTER_HULL( m_pOwner->edict(), start, end, noMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		destination = start;
	else
	{
		if ( trace.flFraction != 1 && trace.pHit == m_pTargetEnt )
			m_fTargetEntHit = TRUE;
		destination = trace.vecEndPos;
	}
	return TRUE;
}

BOOL CLocalNav::StepJumpable( Vector &source, Vector &destination, int noMonsters, TraceResult &trace )
{
	Vector start = source;
	start.z += s_flStepSize + 1;
	for ( double height = s_flStepSize + 1.0; height < 40; height += 10, start.z += 10 )
	{
		Vector end( destination.x, destination.y, start.z );
		TRACE_MONSTER_HULL( m_pOwner->edict(), start, end, noMonsters, m_pOwner->edict(), &trace );
		if ( trace.fStartSolid )
			return FALSE;
		if ( trace.flFraction != 1 )
		{
			if ( trace.pHit == m_pTargetEnt )
				m_fTargetEntHit = TRUE;
			else if ( (trace.vecEndPos - start).Length2D() < 1 )
				continue;
		}
		start = end = trace.vecEndPos;
		end.z -= s_flStepSize;
		TRACE_MONSTER_HULL( m_pOwner->edict(), start, end, noMonsters, m_pOwner->edict(), &trace );
		if ( trace.fStartSolid )
			destination = start;
		else
		{
			if ( trace.flFraction != 1 && trace.pHit == m_pTargetEnt )
				m_fTargetEntHit = TRUE;
			destination = trace.vecEndPos;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CLocalNav::SlopeTraversable( Vector &source, Vector &destination, int noMonsters, TraceResult &trace )
{
	Vector end = destination;
	Vector angles = UTIL_VecToAngles( trace.vecPlaneNormal );
	end.z = (float)(source.z + tan( (90.0 - angles.x) * M_PI / 180.0 ) * (destination - source).Length2D());
	TRACE_MONSTER_HULL( m_pOwner->edict(), source, end, noMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		return FALSE;
	if ( trace.flFraction != 1 )
	{
		if ( trace.pHit == m_pTargetEnt )
			m_fTargetEntHit = TRUE;
		else if ( (trace.vecEndPos - source).Length2D() < 1 )
			return FALSE;
	}
	end = trace.vecEndPos;
	Vector down = end;
	down.z -= s_flStepSize;
	TRACE_MONSTER_HULL( m_pOwner->edict(), end, down, noMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		destination = end;
	else
	{
		if ( trace.flFraction != 1 && trace.pHit == m_pTargetEnt )
			m_fTargetEntHit = TRUE;
		destination = trace.vecEndPos;
	}
	return TRUE;
}

BOOL CLocalNav::LadderTraversable( Vector &, Vector &destination, int noMonsters, TraceResult &trace )
{
	Vector start = trace.vecEndPos;
	Vector end = start;
	end.z += 26;
	TRACE_MONSTER_HULL( m_pOwner->edict(), start, end, noMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		return FALSE;
	if ( trace.flFraction != 1 )
	{
		if ( trace.pHit == m_pTargetEnt )
			m_fTargetEntHit = TRUE;
		else if ( (trace.vecEndPos - start).Length() < 1 )
			return FALSE;
	}
	start = trace.vecEndPos;
	destination.z = start.z;
	return PathTraversable( start, destination, noMonsters );
}

BOOL CLocalNav::LadderHit( Vector &, Vector &, TraceResult &trace )
{
	Vector angles = UTIL_VecToAngles( -trace.vecPlaneNormal );
	Vector forward, right, up;
	g_engfuncs.pfnAngleVectors( angles, forward, right, up );
	Vector center = trace.vecEndPos + forward * 15;
	if ( UTIL_PointContents( center + up * 36 ) == CONTENTS_LADDER )
		return TRUE;
	if ( UTIL_PointContents( center - up * 36 ) == CONTENTS_LADDER )
		return TRUE;
	if ( UTIL_PointContents( center + right * 16 + up * 36 ) == CONTENTS_LADDER )
		return TRUE;
	if ( UTIL_PointContents( center - right * 16 + up * 36 ) == CONTENTS_LADDER )
		return TRUE;
	if ( UTIL_PointContents( center + right * 16 - up * 36 ) == CONTENTS_LADDER )
		return TRUE;
	return UTIL_PointContents( center - right * 16 - up * 36 ) == CONTENTS_LADDER;
}

int CLocalNav::PathTraversable( Vector &source, Vector &destination, int noMonsters )
{
	Vector direction = (destination - source).Normalize();
	direction.z = 0;
	Vector start = source;
	Vector end;
	int traversable = 0;
	float distance = (destination - source).Length2D();
	while ( distance > 1 )
	{
		end = s_flStepSize <= distance ? start + direction * s_flStepSize : destination;
		m_fTargetEntHit = FALSE;
		TraceResult trace;
		TRACE_MONSTER_HULL( m_pOwner->edict(), start, end, noMonsters, m_pOwner->edict(), &trace );
		if ( trace.fStartSolid )
			return 0;
		if ( trace.flFraction == 1 || trace.pHit == m_pTargetEnt )
		{
			if ( trace.flFraction != 1 )
				m_fTargetEntHit = TRUE;
			end = trace.vecEndPos;
			if ( !traversable )
				traversable = 1;
		}
		else
		{
			if ( !noMonsters && trace.pHit && trace.pHit->v.classname &&
				!strcmp( STRING( trace.pHit->v.classname ), "hostage_entity" ) )
				return 0;
			start = trace.vecEndPos;
			if ( trace.vecPlaneNormal.z > 0.7 )
			{
				if ( !SlopeTraversable( start, end, noMonsters, trace ) )
					return 0;
				if ( !traversable )
					traversable = 1;
			}
			else if ( StepTraversable( start, end, noMonsters, trace ) )
			{
				if ( !traversable )
					traversable = 2;
			}
			else
			{
				if ( !StepJumpable( start, end, noMonsters, trace ) )
					return 0;
				if ( !traversable )
					traversable = 3;
			}
		}
		Vector down = end;
		down.z -= 300;
		TRACE_MONSTER_HULL( m_pOwner->edict(), end, down, noMonsters, m_pOwner->edict(), &trace );
		if ( !trace.fStartSolid )
		{
			if ( trace.flFraction == 1 )
				return 0;
			if ( trace.pHit == m_pTargetEnt )
			{
				m_fTargetEntHit = TRUE;
				return 0;
			}
			end = trace.vecEndPos;
		}
		start = end;
		if ( m_fTargetEntHit )
			break;
		distance = (destination - start).Length2D();
	}
	destination = end;
	return traversable;
}
