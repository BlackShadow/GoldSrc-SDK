#include "extdll.h"
#include "util.h"
#include "cbase.h"

#define SF_GENERICITEM_DROP_TO_FLOOR	1

class CGenericItem : public CBaseAnimating
{
public:
	void Spawn();
	void Precache();
	void KeyValue( KeyValueData* pkvd );
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT StartItem();
	void EXPORT AnimateThink();

	float	m_lastTime;
	int		m_iSequence;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( item_generic, CGenericItem );

void CGenericItem::Precache()
{
	PRECACHE_MODEL( (char*)STRING( pev->model ) );
}

void CGenericItem::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	Precache();
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	if ( m_iSequence )
	{
		SetThink( &CGenericItem::StartItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	if ( ( pev->spawnflags & SF_GENERICITEM_DROP_TO_FLOOR ) && DROP_TO_FLOOR( ENT( pev ) ) == 0 )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ),
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
	}
}

void CGenericItem::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "sequencename" ) )
		m_iSequence = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "skin" ) )
		pev->skin = (int)atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "body" ) )
		pev->body = atoi( pkvd->szValue );
	else
	{
		CBaseDelay::KeyValue( pkvd );
		return;
	}
	pkvd->fHandled = TRUE;
}

void CGenericItem::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	SetThink( &CGenericItem::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CGenericItem::StartItem()
{
	pev->effects = 0;
	pev->sequence = LookupSequence( STRING( m_iSequence ) );
	pev->frame = 0;
	ResetSequenceInfo();
	SetThink( &CGenericItem::AnimateThink );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->frame = 0;
}

void CGenericItem::AnimateThink()
{
	DispatchAnimEvents( 0.1 );
	StudioFrameAdvance();
	if ( m_fSequenceFinished && !m_fSequenceLoops )
	{
		pev->frame = 0;
		ResetSequenceInfo();
	}
	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}
