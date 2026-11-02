#ifndef AREADEF_H
#define AREADEF_H

#include "cbase.h"

#define DB_CLASS_COUNT	12
#define DB_AREA_NAME_LEN	32

class DBAreaClassData
{
public:
	DBAreaClassData();
	void Clear();
	void MergeData( DBAreaClassData* pSource );

	int	m_DB_PingTotal;
	int	m_DB_nPingTests;
	int	m_DB_TimeInArea;
};

class CAreaDef : public CBaseEntity
{
public:
	CAreaDef( bool bDefaultArea = false );
	~CAreaDef();
	void KeyValue( KeyValueData* pkvd );
	int Save( CSave& save );
	int Restore( CRestore& restore );
	DBAreaClassData* GetClassData( const char* pClassName );
	void MergeData( CAreaDef* pSource );
	void ClearClassData();

	static TYPEDESCRIPTION m_SaveData[];

	DBAreaClassData	m_ClassData[DB_CLASS_COUNT];
	char			m_AreaName[DB_AREA_NAME_LEN];
	CAreaDef*		m_pNext;
};

extern CAreaDef	g_DefaultArea;
extern CAreaDef*	g_pActiveAreas;
extern int		g_nActiveAreas;

#endif
