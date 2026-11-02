#include "extdll.h"
#include "util.h"
#include "cbase.h"

#define SF_ALIAS_OFF			1
#define SF_ALIAS_DEBUG		2
#define SF_CHANGEALIAS_RESOLVE	1
#define SF_CHANGEALIAS_DEBUG		2
#define MAX_ALIAS_MEMBERS	16

LINK_ENTITY_TO_CLASS( info_alias, CInfoAlias );
LINK_ENTITY_TO_CLASS( info_group, CInfoGroup );
LINK_ENTITY_TO_CLASS( multi_alias, CMultiAlias );

void CInfoAlias::Spawn()
{
	RoundStore();
	if ( pev->spawnflags & SF_ALIAS_OFF )
	{
		string_t target = pev->target;
		pev->target = pev->netname;
		pev->netname = target;
	}
}

void CInfoAlias::RoundRespawn()
{
	if ( pev->spawnflags & SF_ALIAS_OFF )
	{
		string_t target = pev->target;
		pev->target = pev->netname;
		pev->netname = target;
	}
}

void CInfoAlias::Think()
{
	if ( pev->spawnflags & SF_ALIAS_OFF )
	{
		if ( pev->spawnflags & SF_ALIAS_DEBUG )
			ALERT( at_console, "DEBUG: info_alias %s turns on", STRING( pev->targetname ) );
		pev->spawnflags &= ~SF_ALIAS_OFF;
	}
	else
	{
		if ( pev->spawnflags & SF_ALIAS_DEBUG )
			ALERT( at_console, "DEBUG: info_alias %s turns off", STRING( pev->targetname ) );
		pev->spawnflags |= SF_ALIAS_OFF;
	}
	string_t target = pev->target;
	pev->target = pev->netname;
	pev->netname = target;
	if ( pev->spawnflags & SF_ALIAS_DEBUG )
		ALERT( at_console, ": target becomes \"%s\"\n", STRING( pev->target ) );
}

void CInfoAlias::Use( CBaseEntity*, CBaseEntity*, USE_TYPE useType, float )
{
	if ( ShouldToggle( useType ) )
		pev->nextthink = gpGlobals->time;
}

STATE CInfoAlias::GetState()
{
	return pev->spawnflags & SF_ALIAS_OFF ? STATE_OFF : STATE_ON;
}

TYPEDESCRIPTION CInfoGroup::m_SaveData[] =
{
	DEFINE_FIELD( CInfoGroup, m_cMembers, FIELD_INTEGER ),
	DEFINE_ARRAY( CInfoGroup, m_iszMemberName, FIELD_STRING, MAX_ALIAS_MEMBERS ),
	DEFINE_ARRAY( CInfoGroup, m_iszMemberValue, FIELD_STRING, MAX_ALIAS_MEMBERS ),
};

IMPLEMENT_SAVERESTORE( CInfoGroup, CBaseEntity );

void CInfoGroup::KeyValue( KeyValueData* pkvd )
{
	if ( m_cMembers < MAX_ALIAS_MEMBERS )
	{
		char key[256];
		UTIL_StripToken( pkvd->szKeyName, key );
		m_iszMemberName[m_cMembers] = ALLOC_STRING( key );
		m_iszMemberValue[m_cMembers] = ALLOC_STRING( pkvd->szValue );
		m_cMembers++;
		pkvd->fHandled = TRUE;
	}
	else
		ALERT( at_error, "Too many members for info_group %s (limit is %d)\n", STRING( pev->targetname ), MAX_ALIAS_MEMBERS );
}

int CInfoGroup::GetMember( const char* szMemberName )
{
	if ( !szMemberName )
	{
		ALERT( at_console, "info_group: GetMember called with null szMemberName!?\n" );
		return 0;
	}
	for ( int i = 0; i < m_cMembers; i++ )
	{
		if ( FStrEq( szMemberName, STRING( m_iszMemberName[i] ) ) )
			return m_iszMemberValue[i];
	}
	ALERT( at_console, "info_group %s has no member called \"%s\".\n", STRING( pev->targetname ), szMemberName );
	return 0;
}

void CInfoGroup::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	pev->nextthink = gpGlobals->time;
}

void CInfoGroup::Think()
{
	CBaseEntity* pAlias = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	if ( pAlias && FClassnameIs( pAlias->pev, "info_alias" ) )
	{
		pAlias->pev->target = pev->targetname;
		if ( pev->spawnflags & SF_ALIAS_DEBUG )
			ALERT( at_console, "info_group %s changed the contents of alias %s to \"%s\"\n",
				STRING( pev->targetname ), STRING( pAlias->pev->targetname ), STRING( pAlias->pev->target ) );
	}
	else if ( pev->target )
		ALERT( at_console, "info_group %s: alias %s was not found or not an alias!", STRING( pev->targetname ), STRING( pev->target ) );
}

TYPEDESCRIPTION CMultiAlias::m_SaveData[] =
{
	DEFINE_FIELD( CMultiAlias, m_cTargets, FIELD_INTEGER ),
	DEFINE_ARRAY( CMultiAlias, m_iszTargets, FIELD_STRING, MAX_ALIAS_MEMBERS ),
};

IMPLEMENT_SAVERESTORE( CMultiAlias, CBaseEntity );

void CMultiAlias::KeyValue( KeyValueData* pkvd )
{
	if ( m_cTargets < MAX_ALIAS_MEMBERS )
	{
		char key[256];
		UTIL_StripToken( pkvd->szKeyName, key );
		m_iszTargets[m_cTargets] = ALLOC_STRING( key );
		m_cTargets++;
		pkvd->fHandled = TRUE;
	}
	else
		ALERT( at_error, "Too many targets for multi_alias %s (limit is %d)\n", STRING( pev->targetname ), MAX_ALIAS_MEMBERS );
}

class CTriggerChangeAlias : public CBaseEntity
{
public:
	void Spawn() override {}
	int ObjectCaps() override { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;
};

LINK_ENTITY_TO_CLASS( trigger_changealias, CTriggerChangeAlias );

void CTriggerChangeAlias::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	CBaseEntity* pAlias = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	if ( !pAlias || !FClassnameIs( pAlias->pev, "info_alias" ) )
	{
		ALERT( at_console, "trigger_changealias %s: alias \"%s\" was not found or not an alias!",
			STRING( pev->targetname ), STRING( pev->target ) );
		return;
	}
	string_t newValue = 0;
	if ( pev->spawnflags & SF_CHANGEALIAS_RESOLVE )
	{
		CBaseEntity* pEntity = UTIL_FollowReference( NULL, STRING( pev->netname ) );
		if ( pEntity )
			newValue = pEntity->pev->targetname;
	}
	if ( !newValue )
		newValue = pev->netname;
	pAlias->pev->target = newValue;
	if ( pev->spawnflags & SF_CHANGEALIAS_DEBUG )
		ALERT( at_console, "DEBUG: trigger_changealias %s activated: changed contents of alias \"%s\" to \"%s\"\n",
			STRING( pev->targetname ), STRING( pAlias->pev->targetname ), STRING( newValue ) );
}
