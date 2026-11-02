#include "extdll.h"
#include "util.h"
#include "areadef.h"
#include "tf_defs.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

CAreaDef* g_pActiveAreas;
CAreaDef g_DefaultArea( true );

LINK_ENTITY_TO_CLASS( info_areadef, CAreaDef );

TYPEDESCRIPTION CAreaDef::m_SaveData[] =
{
	DEFINE_FIELD( CAreaDef, m_AreaName, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CAreaDef, CBaseEntity );

DBAreaClassData::DBAreaClassData()
{
	Clear();
}

void DBAreaClassData::Clear()
{
	m_DB_PingTotal = 0;
	m_DB_nPingTests = 0;
	m_DB_TimeInArea = 0;
}

void DBAreaClassData::MergeData( DBAreaClassData* pSource )
{
	m_DB_PingTotal += pSource->m_DB_PingTotal;
	m_DB_nPingTests += pSource->m_DB_nPingTests;
	m_DB_TimeInArea += pSource->m_DB_TimeInArea;
	pSource->Clear();
}

CAreaDef::CAreaDef( bool bDefaultArea )
{
	++g_nActiveAreas;
	if ( bDefaultArea )
	{
		strcpy( m_AreaName, "DefaultArea" );
		m_pNext = NULL;
		g_pActiveAreas = this;
	}
	else
	{
		m_AreaName[0] = 0;
		m_pNext = g_pActiveAreas;
		g_pActiveAreas = this;
	}
}

CAreaDef::~CAreaDef()
{
	--g_nActiveAreas;
	CAreaDef** pLink = &g_pActiveAreas;
	while ( *pLink )
	{
		if ( *pLink == this )
		{
			*pLink = m_pNext;
			break;
		}
		pLink = &( *pLink )->m_pNext;
	}
}

void CAreaDef::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "areaname" ) )
	{
		strncpy( m_AreaName, pkvd->szValue, sizeof( m_AreaName ) - 1 );
		m_AreaName[sizeof( m_AreaName ) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

DBAreaClassData* CAreaDef::GetClassData( const char* pClassName )
{
	for ( int i = 0; i < DB_CLASS_COUNT; ++i )
	{
		if ( !strcasecmp( pClassName, sClassNames[i] ) )
			return &m_ClassData[i];
	}
	return NULL;
}

void CAreaDef::MergeData( CAreaDef* pSource )
{
	for ( int i = 0; i < DB_CLASS_COUNT; ++i )
		m_ClassData[i].MergeData( &pSource->m_ClassData[i] );
}

void CAreaDef::ClearClassData()
{
	for ( int i = 0; i < DB_CLASS_COUNT; ++i )
		m_ClassData[i].Clear();
}
