#ifndef DBHELPERS_H
#define DBHELPERS_H

#ifndef _WIN32

#include "common/gamedb/igamedb.h"

class CAreaDef;
class Vector;

extern IGameDB*	g_pDB;
extern bool		g_bDBSent;
extern bool		g_bFirstSend;
extern int		g_ClassPlayerCounts[12];
extern int		g_nClassPlayerSamples;
extern float	g_fLastPlayerCountSampleTime;
extern int		g_nTotalClients;
extern int		g_nClientSamples;
extern float	g_fLevelStartTime;

extern HTABLE	g_hLevel;
extern HCOLUMN	g_hLevel_LevelName;
extern HCOLUMN	g_hLevel_GameLength;
extern HCOLUMN	g_hLevel_WinningTeam;
extern HCOLUMN	g_hLevel_NumClients;

extern HTABLE	g_hLevelClass;
extern HCOLUMN	g_hLevelClass_Parent;
extern HCOLUMN	g_hLevelClass_ClassName;
extern HCOLUMN	g_hLevelClass_ClassPercent;

extern HTABLE	g_hLevelArea;
extern HCOLUMN	g_hLevelArea_AreaName;
extern HCOLUMN	g_hLevelArea_Parent;

extern HTABLE	g_hLevelAreaClass;
extern HCOLUMN	g_hLevelAreaClass_Parent;
extern HCOLUMN	g_hLevelAreaClass_ClassName;
extern HCOLUMN	g_hLevelAreaClass_DeathCount;
extern HCOLUMN	g_hLevelAreaClass_PingTime;
extern HCOLUMN	g_hLevelAreaClass_TimeInArea;

extern HTABLE	g_hLevelAreaClassWeapon;
extern HCOLUMN	g_hLevelAreaClassWeapon_Parent;
extern HCOLUMN	g_hLevelAreaClassWeapon_WeaponName;
extern HCOLUMN	g_hLevelAreaClassWeapon_NumShotsFired;

extern HTABLE	g_hLevelAreaClassWeaponDamage;
extern HCOLUMN	g_hLevelAreaClassWeaponDamage_Parent;
extern HCOLUMN	g_hLevelAreaClassWeaponDamage_DamagedClassName;
extern HCOLUMN	g_hLevelAreaClassWeaponDamage_DamageInflicted;
extern HCOLUMN	g_hLevelAreaClassWeaponDamage_Kills;

void		DB_GameDBShutdown();
void		DB_GameDBLoad();
void		DB_OnStartLevel();
void		DB_OnStartFrame();
bool		DB_AddFinalData();
void		DB_Clear();
bool		DB_Transmit( bool bClear );
HROW		DB_GetRow_Level( const char* pLevelName );
HROW		DB_GetRow_LevelClass( const char* pLevelName, const char* pClassName );
HROW		DB_GetRow_LevelArea( const char* pLevelName, const char* pAreaName );
HROW		DB_GetRow_LevelAreaClass( const char* pLevelName, const char* pAreaName, const char* pClassName );
HROW		DB_GetRow_LevelAreaClassWeapon( const char* pLevelName, const char* pAreaName, const char* pClassName,
				const char* pWeaponName );
HROW		DB_GetRow_LevelAreaClassWeaponDamage( const char* pLevelName, const char* pAreaName, const char* pClassName,
				const char* pWeaponName, const char* pDamagedClassName );
HFIELD		DB_GetField( HROW hRow, HCOLUMN hColumn );
bool		DB_SetFieldValueInt( HFIELD hField, int value );
bool		DB_AddToField( HFIELD hField, int amount );
const char*	DB_GetLevelName();
CAreaDef*	DB_GetArea( const Vector& pos );
const char*	DB_GetAreaName( const Vector& pos );

#else

#define DB_GameDBShutdown()	( (void)0 )
#define DB_GameDBLoad()		( (void)0 )
#define DB_OnStartLevel()		( (void)0 )
#define DB_OnStartFrame()		( (void)0 )
#define DB_Clear()			( (void)0 )

#endif

#endif
