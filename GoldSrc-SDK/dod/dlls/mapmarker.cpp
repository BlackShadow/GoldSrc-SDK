#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "mapmarker.h"

extern cvar_t* pcvar_mp_markerstaytime;

LINK_ENTITY_TO_CLASS( dod_mapmarker, CMapMarker );

void CMapMarker::Die()
{
	UTIL_Remove( this );
}

void CMapMarker::Spawn( void )
{
	pev->solid = SOLID_NOT;
	SET_MODEL( ENT( pev ), "sprites/markernull.spr" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	MAKE_STRING_CLASS( "dod_mapmarker", pev );

	float staytime = 10;

	if ( pcvar_mp_markerstaytime )
		staytime = pcvar_mp_markerstaytime->value;

	if ( staytime < 2 )
		staytime = 2;

	pev->nextthink = staytime + gpGlobals->time;
	SetThink( &CMapMarker::Die );
	pev->flags |= FL_CUSTOMENTITY;
}

CMapMarker *CMapMarker::CreateMapMarker( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, int markerIcon )
{
	CMapMarker *pMarker = GetClassPtr( (CMapMarker *)NULL );
	static int i;

	if ( pMarker )
	{
		UTIL_SetOrigin( pMarker, vecOrigin );
		pMarker->Spawn();
		pMarker->pev->owner = pOwner->edict();
		pMarker->pev->team = pOwner->pev->team;
		pMarker->SetIcon( markerIcon );
		pMarker->pev->angles.x = 0;
		pMarker->pev->angles.y = 90;
		pMarker->pev->angles.z = 0;
		pMarker->pev->fuser1 = gpGlobals->time;
		MAKE_STRING_CLASS( "dod_mapmarker", pMarker->pev );
	}

	i++;

	return pMarker;
}
