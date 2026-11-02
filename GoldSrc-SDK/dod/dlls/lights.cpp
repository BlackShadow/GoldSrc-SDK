/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== lights.cpp ========================================================

  spawn and think functions for editor-placed lights

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#define SF_LIGHT_START_OFF	1

char* GetStdLightStyle( int iStyle )
{
	switch ( iStyle )
	{
	case 1: return "mmnmmommommnonmmonqnmmo";
	case 2: return "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba";
	case 3:
	case 4: return "mamamamamama";
	case 5: return "jklmnopqrstuvwxyzyxwvutsrqponmlkj";
	case 6: return "nmonqnmomnmomomno";
	case 7: return "mmmaaaabcdefgmmmmaaaammmaamm";
	case 8: return "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa";
	case 9: return "aaaaaaaazzzzzzzz";
	case 10: return "mmamammmmammamamaaamammma";
	case 11: return "abcdefghijklmnopqrrqponmlkjihgfedcba";
	case 12: return "mmnnmmnnnmmnn";
	case 13: return "a";
	case 14: return "aabbccddeeffgghhiijjkkllmmmmmmmmmmmmmm";
	case 15: return "abcdefghijklmmmmmmmmmmmmmmmmmmmmmmmmmm";
	case 16: return "acegikmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm";
	case 17: return "llkkjjiihhggffeeddccbbaaaaaaaaaaaaaaaa";
	case 18: return "lkjihgfedcbaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	case 19: return "kigecaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	default: return "m";
	}
}

class CLight : public CPointEntity
{
public:
	void RoundRespawn() override;
	void Spawn() override;
	void KeyValue( KeyValueData* pkvd ) override;
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;
	void Think() override;
	STATE GetState() override { return m_iState; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	void SetCorrectStyle();
	static TYPEDESCRIPTION m_SaveData[];

	int		m_iStyle;
	BOOL	m_bDefaultOn;
	STATE	m_iState;
	int		m_iOnStyle;
	int		m_iOffStyle;
	int		m_iTurnOnStyle;
	int		m_iTurnOffStyle;
	int		m_iTurnOnTime;
	int		m_iTurnOffTime;
	int		m_iszPattern;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( light, CLight );

//
// shut up spawn functions for new spotlights
//
LINK_ENTITY_TO_CLASS( light_spot, CLight );

TYPEDESCRIPTION CLight::m_SaveData[] =
{
	DEFINE_FIELD( CLight, m_iState, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iszPattern, FIELD_STRING ),
	DEFINE_FIELD( CLight, m_iOnStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iOffStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iTurnOnStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iTurnOffStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iTurnOnTime, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iTurnOffTime, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CLight, CPointEntity );


//
// Cache user-entity-field values until spawn is called.
//
void CLight::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "style" ) )
		m_iStyle = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "m_iOnStyle" ) )
		m_iOnStyle = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "m_iOffStyle" ) )
		m_iOffStyle = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "m_iTurnOnStyle" ) )
		m_iTurnOnStyle = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "m_iTurnOffStyle" ) )
		m_iTurnOffStyle = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "m_iTurnOnTime" ) )
		m_iTurnOnTime = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "m_iTurnOffTime" ) )
		m_iTurnOffTime = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "pitch" ) )
		pev->angles.x = (float)atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "pattern" ) )
		m_iszPattern = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "firetarget" ) )
		pev->target = ALLOC_STRING( pkvd->szValue );
	else
	{
		CPointEntity::KeyValue( pkvd );
		return;
	}
	pkvd->fHandled = TRUE;
}

void CLight::SetCorrectStyle()
{
	switch ( m_iState )
	{
	case STATE_OFF:
		LIGHT_STYLE( m_iStyle, m_iOffStyle ? GetStdLightStyle( m_iOffStyle ) : "a" );
		break;
	case STATE_TURN_ON:
		LIGHT_STYLE( m_iStyle, m_iTurnOnStyle ? GetStdLightStyle( m_iTurnOnStyle ) : "a" );
		break;
	case STATE_ON:
		if ( m_iszPattern )
			LIGHT_STYLE( m_iStyle, (char*)STRING( m_iszPattern ) );
		else
			LIGHT_STYLE( m_iStyle, m_iOnStyle ? GetStdLightStyle( m_iOnStyle ) : "m" );
		break;
	case STATE_TURN_OFF:
		LIGHT_STYLE( m_iStyle, m_iTurnOffStyle ? GetStdLightStyle( m_iTurnOffStyle ) : "m" );
		break;
	}
}

void CLight::Spawn()
{
	if ( !pev->targetname )
	{
		REMOVE_ENTITY( edict() );
		return;
	}
	m_bDefaultOn = !( pev->spawnflags & SF_LIGHT_START_OFF );
	m_iState = pev->spawnflags & SF_LIGHT_START_OFF ? STATE_OFF : STATE_ON;
	if ( m_iStyle >= 32 )
		SetCorrectStyle();
	RoundStore();
}

void CLight::RoundRespawn()
{
	if ( m_bDefaultOn )
		LIGHT_STYLE( m_iStyle, m_iszPattern ? STRING( m_iszPattern ) : "m" );
	else
		LIGHT_STYLE( m_iStyle, "a" );
}

void CLight::Use( CBaseEntity*, CBaseEntity*, USE_TYPE useType, float )
{
	if ( m_iStyle < 32 || !ShouldToggle( useType ) )
		return;
	switch ( GetState() )
	{
	case STATE_ON:
	case STATE_TURN_ON:
		if ( m_iTurnOffTime )
		{
			m_iState = STATE_TURN_OFF;
			pev->nextthink = gpGlobals->time + (float)m_iTurnOffTime;
		}
		else
			m_iState = STATE_OFF;
		break;
	case STATE_OFF:
	case STATE_TURN_OFF:
		if ( m_iTurnOnTime )
		{
			m_iState = STATE_TURN_ON;
			pev->nextthink = gpGlobals->time + (float)m_iTurnOnTime;
		}
		else
			m_iState = STATE_ON;
		break;
	}
	if ( m_iStyle >= 32 )
		SetCorrectStyle();
}

void CLight::Think()
{
	STATE state = GetState();
	if ( state == STATE_TURN_ON )
	{
		m_iState = STATE_ON;
		FireTargets( STRING( pev->target ), this, this, USE_ON, 0 );
	}
	else if ( state == STATE_TURN_OFF )
	{
		m_iState = STATE_OFF;
		FireTargets( STRING( pev->target ), this, this, USE_OFF, 0 );
	}
	if ( m_iStyle >= 32 )
		SetCorrectStyle();
}

class CEnvLight : public CLight
{
public:
	void Spawn() override;
	void KeyValue( KeyValueData* pkvd ) override;
};

LINK_ENTITY_TO_CLASS( light_environment, CEnvLight );

void CEnvLight::Spawn()
{
	char value[76];
	UTIL_MakeAimVectors( pev->angles );
	sprintf( value, "%f", gpGlobals->v_forward.x );
	CVAR_SET_STRING( "sv_skyvec_x", value );
	sprintf( value, "%f", gpGlobals->v_forward.y );
	CVAR_SET_STRING( "sv_skyvec_y", value );
	sprintf( value, "%f", gpGlobals->v_forward.z );
	CVAR_SET_STRING( "sv_skyvec_z", value );
	CLight::Spawn();
}

#pragma warning( push )
#pragma warning( disable: 4700 4701 4703 )
void CEnvLight::KeyValue( KeyValueData* pkvd )
{
	if ( !FStrEq( pkvd->szKeyName, "_light" ) )
	{
		CLight::KeyValue( pkvd );
		return;
	}
	int red, green, blue, brightness;
	int count = sscanf( pkvd->szValue, "%d %d %d %d", &red, &green, &blue, &brightness );
	if ( count == 1 )
		blue = green = red;
	else if ( count == 4 )
	{
		red = (int)( red * ( brightness / 255.0 ) );
		green = (int)( green * ( brightness / 255.0 ) );
		blue = (int)( blue * ( brightness / 255.0 ) );
	}
	red = (int)( pow( red / 114.0, 0.6 ) * 264 );
	green = (int)( pow( green / 114.0, 0.6 ) * 264 );
	blue = (int)( pow( blue / 114.0, 0.6 ) * 264 );
	pkvd->fHandled = TRUE;
	char value[64];
	sprintf( value, "%d", red );
	CVAR_SET_STRING( "sv_skycolor_r", value );
	sprintf( value, "%d", green );
	CVAR_SET_STRING( "sv_skycolor_g", value );
	sprintf( value, "%d", blue );
	CVAR_SET_STRING( "sv_skycolor_b", value );
}
#pragma warning( pop )

class CTriggerLightstyle : public CBaseEntity
{
public:
	void Spawn() override {}
	int ObjectCaps() override { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void KeyValue( KeyValueData* pkvd ) override;
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;
	void Think() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int	m_iStyle;
	int	m_iszPattern;
	int	m_iWait;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( trigger_lightstyle, CTriggerLightstyle );

TYPEDESCRIPTION CTriggerLightstyle::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerLightstyle, m_iStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerLightstyle, m_iszPattern, FIELD_STRING ),
	DEFINE_FIELD( CTriggerLightstyle, m_iWait, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTriggerLightstyle, CBaseEntity );

void CTriggerLightstyle::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "style" ) )
	{
		m_iStyle = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "pattern" ) )
	{
		m_iszPattern = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iWait" ) )
	{
		m_iWait = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CTriggerLightstyle::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	if ( !pev->target )
		return;
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityByTargetname( pEntity, STRING( pev->target ) ) ) != NULL )
	{
		if ( FNullEnt( pEntity->edict() ) )
			return;
		if ( !FClassnameIs( pEntity->pev, "light" ) && !FClassnameIs( pEntity->pev, "light_spot" ) &&
			!FClassnameIs( pEntity->pev, "env_light" ) )
			continue;
		CLight* pLight = (CLight*)pEntity;
		if ( pLight->m_iStyle < 32 )
			continue;
		if ( m_iszPattern )
			LIGHT_STYLE( pLight->m_iStyle, (char*)STRING( m_iszPattern ) );
		else if ( m_iStyle )
			LIGHT_STYLE( pLight->m_iStyle, GetStdLightStyle( m_iStyle ) );
		if ( m_iWait != -1 )
			pev->nextthink = gpGlobals->time + (float)m_iWait;
	}
}

void CTriggerLightstyle::Think()
{
	if ( !pev->target )
		return;
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityByTargetname( pEntity, STRING( pev->target ) ) ) != NULL )
	{
		if ( FNullEnt( pEntity->edict() ) )
			return;
		if ( ( FClassnameIs( pEntity->pev, "light" ) || FClassnameIs( pEntity->pev, "light_spot" ) ||
			FClassnameIs( pEntity->pev, "env_light" ) ) && ( (CLight*)pEntity )->m_iStyle >= 32 )
			( (CLight*)pEntity )->SetCorrectStyle();
	}
}
