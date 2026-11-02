#include "extdll.h"
#include "util.h"
#include "cbase.h"

class COFNuclearBombTimer : public CBaseEntity
{
public:
	void Spawn();
	void Precache();
	int ObjectCaps() { return FCAP_DONT_SAVE; }
	void EXPORT NuclearBombTimerThink();
	void SetNuclearBombTimer( BOOL fOn );

	BOOL	bPlayBombSound;
	BOOL	bBombSoundPlaying;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class COFNuclearBombButton : public CBaseEntity
{
public:
	void Spawn();
	void Precache();
	int ObjectCaps() { return FCAP_DONT_SAVE; }
	void SetNuclearBombButton( BOOL fOn );
};

class COFNuclearBomb : public CBaseToggle
{
public:
	void Spawn();
	void Precache();
	void KeyValue( KeyValueData* pkvd );
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	int ObjectCaps() { return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE; }
	int Save( CSave& save );
	int Restore( CRestore& restore );
	static TYPEDESCRIPTION m_SaveData[];

	COFNuclearBombTimer*		m_pTimer;
	COFNuclearBombButton*	m_pButton;
	BOOL					m_fOn;
	float					m_flLastPush;
	int						m_iPushCount;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( item_nuclearbombtimer, COFNuclearBombTimer );
LINK_ENTITY_TO_CLASS( item_nuclearbombbutton, COFNuclearBombButton );
LINK_ENTITY_TO_CLASS( item_nuclearbomb, COFNuclearBomb );

TYPEDESCRIPTION COFNuclearBomb::m_SaveData[] =
{
	DEFINE_FIELD( COFNuclearBomb, m_fOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFNuclearBomb, m_flLastPush, FIELD_TIME ),
	DEFINE_FIELD( COFNuclearBomb, m_iPushCount, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( COFNuclearBomb, CBaseToggle );

void COFNuclearBombTimer::Precache()
{
	PRECACHE_MODEL( "models/nuke_timer.mdl" );
	PRECACHE_SOUND( "common/nuke_ticking.wav" );
}

void COFNuclearBombButton::Precache()
{
	PRECACHE_MODEL( "models/nuke_button.mdl" );
}

void COFNuclearBombTimer::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/nuke_timer.mdl" );
	pev->solid = SOLID_NOT;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 32 ) );
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin( pev, pev->origin );
	if ( !DROP_TO_FLOOR( ENT( pev ) ) )
	{
		ALERT( at_error, "Nuclear Bomb Timer fell out of level at %f,%f,%f",
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
	}
	else
	{
		pev->skin = 0;
		bPlayBombSound = FALSE;
		bBombSoundPlaying = FALSE;
	}
}

void COFNuclearBombButton::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/nuke_button.mdl" );
	pev->solid = SOLID_NOT;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 32 ) );
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin( pev, pev->origin );
	if ( !DROP_TO_FLOOR( ENT( pev ) ) )
	{
		ALERT( at_error, "Nuclear Bomb Button fell out of level at %f,%f,%f",
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
	}
	else
		pev->skin = 0;
}

void COFNuclearBomb::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/nuke_case.mdl" );
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 32 ) );
	pev->movetype = MOVETYPE_NONE;
	if ( DROP_TO_FLOOR( ENT( pev ) ) )
	{
		m_iPushCount = 0;
		m_flLastPush = gpGlobals->time;
	}
	else
	{
		ALERT( at_error, "Nuclear Bomb fell out of level at %f,%f,%f",
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
	}
}

void COFNuclearBomb::Precache()
{
	PRECACHE_MODEL( "models/nuke_case.mdl" );
	UTIL_PrecacheOther( "item_nuclearbombtimer" );
	UTIL_PrecacheOther( "item_nuclearbombbutton" );
	PRECACHE_MODEL( "models/nuke_timer.mdl" );
	PRECACHE_MODEL( "models/nuke_button.mdl" );
	PRECACHE_SOUND( "buttons/button4.wav" );
	PRECACHE_SOUND( "buttons/button6.wav" );

	m_pTimer = GetClassPtr( (COFNuclearBombTimer*)NULL );
	if ( m_pTimer )
	{
		m_pTimer->pev->origin = pev->origin;
		m_pTimer->pev->angles = pev->angles;
		m_pTimer->Spawn();
		m_pTimer->SetNuclearBombTimer( m_fOn );
	}
	else
	{
		ALERT( at_error, "Unable to create Nuclear Bomb Timer\n" );
	}

	m_pButton = GetClassPtr( (COFNuclearBombButton*)NULL );
	if ( m_pButton )
	{
		m_pButton->pev->origin = pev->origin;
		m_pButton->pev->angles = pev->angles;
		m_pButton->Spawn();
		m_pButton->SetNuclearBombButton( m_fOn );
	}
	else
	{
		ALERT( at_error, "Unable to create Nuclear Bomb Button\n" );
	}
}

void COFNuclearBomb::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "initialstate" ) )
	{
		m_fOn = atoi( pkvd->szValue ) != 0;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void COFNuclearBomb::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if ( m_flWait == -1 && m_iPushCount > 0 )
		return;
	if ( !( m_flWait <= gpGlobals->time - m_flLastPush ) || !ShouldToggle( useType, m_fOn ) )
		return;

	if ( m_fOn )
	{
		m_fOn = FALSE;
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "buttons/button4.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	}
	else
	{
		m_fOn = TRUE;
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "buttons/button6.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	}

	SUB_UseTargets( pActivator, USE_TOGGLE, 0 );
	if ( m_pButton )
		m_pButton->SetNuclearBombButton( m_fOn );
	if ( m_pTimer )
		m_pTimer->SetNuclearBombTimer( m_fOn );
	m_iPushCount++;
	m_flLastPush = gpGlobals->time;
}

void COFNuclearBombTimer::NuclearBombTimerThink()
{
	if ( pev->skin <= 1 )
		pev->skin++;
	else
		pev->skin = 0;

	if ( bPlayBombSound )
	{
		EMIT_SOUND_DYN( edict(), CHAN_BODY, "common/nuke_ticking.wav", 0.75, ATTN_IDLE, 0, PITCH_NORM );
		bBombSoundPlaying = TRUE;
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void COFNuclearBombTimer::SetNuclearBombTimer( BOOL fOn )
{
	if ( fOn )
	{
		SetThink( &COFNuclearBombTimer::NuclearBombTimerThink );
		pev->nextthink = gpGlobals->time + 0.5;
		bPlayBombSound = TRUE;
	}
	else
	{
		SetThink( NULL );
		pev->nextthink = gpGlobals->time;
		pev->skin = 3;
		if ( bBombSoundPlaying )
		{
			EMIT_SOUND_DYN( edict(), CHAN_BODY, "common/nuke_ticking.wav", 0, ATTN_NONE, SND_STOP, PITCH_NORM );
			bBombSoundPlaying = FALSE;
		}
	}
}

void COFNuclearBombButton::SetNuclearBombButton( BOOL fOn )
{
	pev->skin = fOn;
}
