#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"
#include "cs_bot_manager.h"

extern int gmsgBotProgress;

static int currentIndex;
static int navAreaCount;

static void startProgressMeter( const char* title )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgBotProgress );
	WRITE_BYTE( 1 );
	WRITE_STRING( title );
	MESSAGE_END();
}

static void drawProgressMeter( float progress, char* title )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgBotProgress );
	WRITE_BYTE( 0 );
	WRITE_BYTE( static_cast<int>( progress * 100.0f ) );
	WRITE_STRING( title );
	MESSAGE_END();
}

void CCSBot::StartNormalProcess()
{
	m_process = 0;
}

void CCSBot::StartSaveProcess()
{
	m_process = 4;
}

void CCSBot::UpdateSaveProcess()
{
	char filename[256];
	GET_GAME_DIR( filename );
	strcat( filename, "\\" );
	strcat( filename, TheBots->GetNavMapFilename() );
	HintMessageToAllPlayers( "Saving..." );
	SaveNavigationMap( filename );
	char message[256];
	sprintf( message, "Navigation file '%s' saved.", filename );
	HintMessageToAllPlayers( message );
	MESSAGE_BEGIN( MSG_ALL, gmsgBotProgress );
	WRITE_BYTE( 2 );
	MESSAGE_END();
	m_process = 0;
	char command[128];
	sprintf( command, "map %s\n", STRING( gpGlobals->mapname ) );
	SERVER_COMMAND( command );
}

void CCSBot::StartLearnProcess()
{
	startProgressMeter( "#CZero_LearningMap" );
	drawProgressMeter( 0.0f, const_cast<char*>( "#CZero_LearningMap" ) );
	BuildLadders();
	Vector position = pev->origin;
	position.x = static_cast<int>( position.x / 25.0f ) * 25.0f;
	position.y = static_cast<int>( position.y / 25.0f ) * 25.0f;
	Vector normal;
	if ( GetGroundHeight( &position, &position.z, &normal ) )
	{
		m_currentNode = new CNavNode( &position, &normal, NULL );
		m_goalPosition = pev->origin;
		m_process = 1;
	}
	else
	{
		CONSOLE_ECHO( "ERROR: Start position invalid\n\n" );
		m_process = 0;
	}
}

void CCSBot::UpdateLearnProcess()
{
	float start = g_engfuncs.pfnTime();
	while ( g_engfuncs.pfnTime() - start < 0.5f )
	{
		if ( !LearnStep() )
		{
			StartAnalyzeAlphaProcess();
			return;
		}
	}
}

void CCSBot::StartAnalyzeAlphaProcess()
{
	m_process = 2;
	m_currentAnalyzeArea = TheNavAreaList.begin();
	navAreaCount = 0;
	for ( NavAreaList::iterator area = TheNavAreaList.begin(); area != TheNavAreaList.end(); ++area )
		++navAreaCount;
	currentIndex = 0;
	ApproachAreaAnalysisPrep();
	DestroyHidingSpots();
	startProgressMeter( "#CZero_AnalyzingHidingSpots" );
	drawProgressMeter( 0.0f, const_cast<char*>( "#CZero_AnalyzingHidingSpots" ) );
}

void CCSBot::StartAnalyzeBetaProcess()
{
	m_process = 3;
	m_currentAnalyzeArea = TheNavAreaList.begin();
	navAreaCount = 0;
	for ( NavAreaList::iterator area = TheNavAreaList.begin(); area != TheNavAreaList.end(); ++area )
		++navAreaCount;
	currentIndex = 0;
}

void CCSBot::UpdateAnalyzeAlphaProcess()
{
	float start = g_engfuncs.pfnTime();
	while ( g_engfuncs.pfnTime() - start < 0.5f )
	{
		++currentIndex;
		if ( m_currentAnalyzeArea == TheNavAreaList.end() )
		{
			drawProgressMeter( 0.5f, const_cast<char*>( "#CZero_AnalyzingHidingSpots" ) );
			CleanupApproachAreaAnalysisPrep();
			StartAnalyzeBetaProcess();
			return;
		}
		CNavArea* area = *m_currentAnalyzeArea;
		area->ComputeHidingSpots();
		area->ComputeApproachAreas();
		++m_currentAnalyzeArea;
	}
	drawProgressMeter( 0.5f * (static_cast<float>( currentIndex ) / navAreaCount), const_cast<char*>( "#CZero_AnalyzingHidingSpots" ) );
}

void CCSBot::UpdateAnalyzeBetaProcess()
{
	float start = g_engfuncs.pfnTime();
	while ( g_engfuncs.pfnTime() - start < 0.5f )
	{
		++currentIndex;
		if ( m_currentAnalyzeArea == TheNavAreaList.end() )
		{
			drawProgressMeter( 1.0f, const_cast<char*>( "#CZero_AnalyzingApproachPoints" ) );
			m_process = 4;
			return;
		}
		CNavArea* area = *m_currentAnalyzeArea;
		area->ComputeSpotEncounters();
		area->ComputeSniperSpots();
		++m_currentAnalyzeArea;
	}
	drawProgressMeter( static_cast<float>( currentIndex ) / navAreaCount * 0.5f + 0.5f, const_cast<char*>( "#CZero_AnalyzingApproachPoints" ) );
}

CNavNode* CCSBot::AddNode( const Vector* position, const Vector* normal, NavDirType direction, CNavNode* parent )
{
	CNavNode* node = const_cast<CNavNode*>( CNavNode::GetNode( position ) );
	bool created = false;
	if ( node == NULL )
	{
		node = new CNavNode( position, normal, parent );
		created = true;
	}
	parent->ConnectTo( node, direction );
	float height = parent->GetPosition()->z - position->z;
	float sign = height >= 0.0f ? 1.0f : -1.0f;
	if ( height * sign < 10.0f )
	{
		NavDirType opposite = OppositeDirection( direction );
		node->ConnectTo( parent, opposite );
		node->MarkAsVisited( opposite );
	}
	if ( created )
		m_currentNode = node;
	for ( int y = 0; y < 3; ++y )
	{
		for ( int x = 0; x < 3; ++x )
		{
			Vector start( position->x - 16.0f + x * 16.0f, position->y - 16.0f + y * 16.0f, position->z + 5.0f );
			Vector end( start.x, start.y, position->z + 72.0f - 0.1f );
			TraceResult trace;
			UTIL_TraceLine( start, end, ignore_monsters, dont_ignore_glass, pev->pContainingEntity, &trace );
			if ( trace.flFraction != 1.0f )
			{
				node->SetAttributes( NAV_CROUCH );
				return node;
			}
		}
	}
	return node;
}

bool CCSBot::LearnStep()
{
	for ( ;; )
	{
		while ( m_currentNode )
		{
			int direction;
			for ( direction = 0; direction < NUM_DIRECTIONS; ++direction )
			{
				if ( !m_currentNode->HasVisited( static_cast<NavDirType>( direction ) ) )
					break;
			}
			if ( direction == NUM_DIRECTIONS )
			{
				m_currentNode = m_currentNode->GetParent();
				continue;
			}
			float halfHeight = IsCrouching() ? 18.0f : 36.0f;
			Vector position = *m_currentNode->GetPosition();
			int x = static_cast<int>( position.x / 25.0f );
			int y = static_cast<int>( position.y / 25.0f );
			switch ( direction )
			{
			case NORTH: --y; break;
			case EAST: ++x; break;
			case SOUTH: ++y; break;
			case WEST: --x; break;
			}
			m_currentLearnDirection = static_cast<NavDirType>( direction );
			m_currentNode->MarkAsVisited( m_currentLearnDirection );
			position.x = x * 25.0f;
			position.y = y * 25.0f;
			Vector ground( position.x, position.y, 0.0f );
			Vector normal;
			if ( !GetGroundHeight( &position, &ground.z, &normal ) )
				return true;
			UTIL_SetOrigin( pev, ground + Vector( 0.0f, 0.0f, halfHeight ) );
			Vector parentPosition = *m_currentNode->GetPosition();
			TraceResult trace;
			UTIL_TraceLine( parentPosition + Vector( 0.0f, 0.0f, halfHeight ), ground + Vector( 0.0f, 0.0f, halfHeight ),
				ignore_monsters, dont_ignore_glass, pev->pContainingEntity, &trace );
			if ( trace.flFraction != 1.0f || trace.fStartSolid )
			{
				entvars_t* obstacle = trace.pHit ? VARS( trace.pHit ) : NULL;
				if ( !obstacle )
					return true;
				const char* classname = STRING( obstacle->classname );
				if ( !strcmp( classname, "func_door" ) || !strcmp( classname, "func_door_rotating" ) ||
					(!strcmp( classname, "func_breakable" ) && obstacle->takedamage == DAMAGE_YES) )
					AddNode( &ground, &normal, m_currentLearnDirection, m_currentNode );
				return true;
			}
			float rise = ground.z - parentPosition.z;
			if ( rise <= 58.1f && parentPosition.z - ground.z <= 200.0f )
			{
				bool walkable = true;
				for ( float distance = 2.0f; ; distance += 2.0f )
				{
					bool last = distance >= 25.0f;
					Vector sample = last ? ground : parentPosition + (ground - parentPosition) * (distance / 25.0f);
					float height;
					Vector sampleNormal;
					if ( !GetGroundHeight( &sample, &height, &sampleNormal ) )
						break;
					if ( sampleNormal.z < 0.7f )
						walkable = false;
					if ( last )
					{
						if ( walkable )
							AddNode( &ground, &normal, m_currentLearnDirection, m_currentNode );
						break;
					}
				}
			}
			return true;
		}

		for ( NavLadderList::iterator ladder = TheNavLadderList.begin(); ladder != TheNavLadderList.end(); ++ladder )
		{
			for ( int end = 0; end < 2; ++end )
			{
				Vector base = end == 0 ? (*ladder)->m_bottom : (*ladder)->m_top;
				switch ( (*ladder)->m_dir )
				{
				case NORTH: base.y -= 16.0f; break;
				case EAST: base.x += 16.0f; break;
				case SOUTH: base.y += 16.0f; break;
				case WEST: base.x -= 16.0f; break;
				default: break;
				}
				for ( int point = -1; point < 8; ++point )
				{
					Vector position = base;
					if ( point >= 0 )
					{
						float distance = point > 3 ? 50.0f : 25.0f;
						switch ( point % 4 )
						{
						case NORTH: position.y -= distance; break;
						case EAST: position.x += distance; break;
						case SOUTH: position.y += distance; break;
						case WEST: position.x -= distance; break;
						}
					}
					position.x = static_cast<int>( position.x / 25.0f ) * 25.0f;
					position.y = static_cast<int>( position.y / 25.0f ) * 25.0f;
					position.z += 25.0f;
					Vector normal;
					if ( !GetGroundHeight( &position, &position.z, &normal ) )
						continue;
					TraceResult trace;
					UTIL_TraceLine( base + Vector( 0.0f, 0.0f, 2.0f ), position + Vector( 0.0f, 0.0f, 2.0f ),
						ignore_monsters, dont_ignore_glass, (*ladder)->m_entity->pev->pContainingEntity, &trace );
					if ( trace.flFraction == 1.0f && CNavNode::GetNode( &position ) == NULL )
					{
						m_currentNode = new CNavNode( &position, &normal, NULL );
						break;
					}
				}
				if ( m_currentNode )
					break;
			}
			if ( m_currentNode )
				break;
		}
		if ( m_currentNode == NULL )
		{
			GenerateNavigationAreaMesh();
			return false;
		}
	}
}
