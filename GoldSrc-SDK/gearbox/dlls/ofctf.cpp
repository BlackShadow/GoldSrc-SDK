#include "extdll.h"
#include "util.h"
#include "triggers.h"
#include "player.h"
#include "gamerules.h"
#include "ofctf.h"
#include "ctf_gamerules.h"

extern int gmsgFlagIcon;
extern int gmsgCTFScore;
extern int gmsgScoreInfo;

const char* sTeamSpawnNames[3] = { "ctfs0", "ctfs1", "ctfs2" };
int teamscores[2];
int g_iMapScoreMax;
float g_flPowerupRespawnTime = 30;
float g_flCaptureAssistTime = 10;
float g_flDefendCarrierTime = 10;
float g_flBaseDefendDist = 192;
float g_flFlagReturnTime = 30;
static char g_szScoreIconNameBM[40] = "item_ctfflagh";
static char g_szScoreIconNameOF[40] = "item_ctfflagh";

LINK_ENTITY_TO_CLASS( info_ctfspawn, CTFSpawn );
LINK_ENTITY_TO_CLASS( item_ctfbase, CTFGoalBase );
LINK_ENTITY_TO_CLASS( item_ctfflag, CTFGoalFlag );
LINK_ENTITY_TO_CLASS( info_ctfdetect, CTFDetect );

void CTFGoal::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "goal_no" ) )
		m_iGoalNum = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "goal_min" ) || FStrEq( pkvd->szKeyName, "goal_max" ) )
	{
		Vector bounds;
		UTIL_StringToVector( bounds, pkvd->szValue );
		if ( bounds != g_vecZero )
		{
			if ( FStrEq( pkvd->szKeyName, "goal_min" ) )
				m_GoalMin = bounds;
			else
				m_GoalMax = bounds;
		}
	}
	else
	{
		pkvd->fHandled = FALSE;
		return;
	}
	pkvd->fHandled = TRUE;
}

void CTFGoal::Spawn()
{
	if ( pev->model )
	{
		if ( STRING( pev->model )[0] == '*' )
			pev->effects |= EF_NODRAW;
		PRECACHE_MODEL( (char*)STRING( pev->model ) );
		SET_MODEL( edict(), STRING( pev->model ) );
	}
	pev->solid = SOLID_TRIGGER;
	if ( !m_iGoalState )
		m_iGoalState = CTF_GOAL_HOME;
	UTIL_SetOrigin( pev, pev->origin );
	StartGoal();
}

void CTFGoal::StartGoal()
{
	SetThink( &CTFGoal::PlaceGoal );
	pev->nextthink = gpGlobals->time + 0.2;
}

void CTFGoal::PlaceGoal()
{
	pev->movetype = MOVETYPE_NONE;
	pev->velocity = g_vecZero;
	pev->oldorigin = pev->origin;
}

void CTFGoal::SetObjectCollisionBox()
{
	if ( STRING( pev->model )[0] != '*' )
	{
		pev->absmin = pev->origin + Vector( -24, -24, 0 );
		pev->absmax = pev->origin + Vector( 24, 24, 16 );
		return;
	}
	float radius = 0;
	for ( int i = 0; i < 3; ++i )
	{
		float extent = fabs( pev->mins[i] );
		if ( extent > radius )
			radius = extent;
		extent = fabs( pev->maxs[i] );
		if ( extent > radius )
			radius = extent;
	}
	pev->absmin = pev->origin - Vector( radius, radius, radius ) - Vector( 1, 1, 1 );
	pev->absmax = pev->origin + Vector( radius, radius, radius ) + Vector( 1, 1, 1 );
}

void CTFGoalBase::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_NOT;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	if ( !DROP_TO_FLOOR( edict() ) )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ),
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
		return;
	}
	if ( pev->model )
	{
		PRECACHE_MODEL( (char*)STRING( pev->model ) );
		SET_MODEL( edict(), STRING( pev->model ) );
		pev->sequence = LookupSequence( "on_ground" );
		if ( pev->sequence != -1 )
		{
			ResetSequenceInfo();
			pev->frame = 0;
		}
	}
	SetThink( &CTFGoalBase::BaseThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CTFGoalBase::BaseThink()
{
	Vector origin, angles;
	GetAttachment( 0, origin, angles );
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( entindex() + 0x1000 );
		WRITE_COORD( origin.x );
		WRITE_COORD( origin.y );
		WRITE_COORD( origin.z );
		WRITE_COORD( 128 );
		WRITE_BYTE( m_iGoalNum == 1 ? 255 : 128 );
		WRITE_BYTE( m_iGoalNum == 1 ? 128 : 255 );
		WRITE_BYTE( 128 );
		WRITE_BYTE( 255 );
		WRITE_COORD( 0 );
	MESSAGE_END();
	pev->nextthink = gpGlobals->time + 20;
}

void CTFGoalBase::TurnOnLight( CBasePlayer* pPlayer )
{
	Vector origin, angles;
	GetAttachment( 0, origin, angles );
	MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pPlayer->pev );
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( entindex() + 0x1000 );
		WRITE_COORD( origin.x );
		WRITE_COORD( origin.y );
		WRITE_COORD( origin.z );
		WRITE_COORD( 128 );
		WRITE_BYTE( m_iGoalNum == 1 ? 255 : 128 );
		WRITE_BYTE( m_iGoalNum == 1 ? 128 : 255 );
		WRITE_BYTE( 128 );
		WRITE_BYTE( 255 );
		WRITE_COORD( 0 );
	MESSAGE_END();
}

void CTFGoalFlag::Precache()
{
	if ( pev->model )
		PRECACHE_MODEL( (char*)STRING( pev->model ) );
	PRECACHE_SOUND( "ctf/bm_flagtaken.wav" );
	PRECACHE_SOUND( "ctf/soldier_flagtaken.wav" );
	PRECACHE_SOUND( "ctf/marine_flag_capture.wav" );
	PRECACHE_SOUND( "ctf/civ_flag_capture.wav" );
}

void CTFGoalFlag::Spawn()
{
	Precache();
	if ( !m_iGoalNum )
	{
		ALERT( at_error, "Invalid goal_no set for CTF flag\n" );
		return;
	}
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );
	if ( !DROP_TO_FLOOR( edict() ) )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ),
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
		return;
	}
	if ( pev->model )
	{
		SET_MODEL( edict(), STRING( pev->model ) );
		pev->sequence = LookupSequence( "flag_positioned" );
		if ( pev->sequence != -1 )
		{
			ResetSequenceInfo();
			pev->frame = 0;
		}
	}
	m_iGoalState = CTF_GOAL_HOME;
	pev->solid = SOLID_TRIGGER;
	if ( !pev->netname )
		pev->netname = MAKE_STRING( "goalflag" );
	UTIL_SetOrigin( pev, pev->origin );
	m_OriginalAngles = pev->angles;
	SetTouch( &CTFGoalFlag::goal_item_touch );
	StartItem();
}

void CTFGoalFlag::StartItem()
{
	SetThink( &CTFGoalFlag::PlaceItem );
	pev->nextthink = gpGlobals->time + 0.2;
}

void CTFGoalFlag::PlaceItem()
{
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->oldorigin = pev->origin;
}

void CTFGoalFlag::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector( -16, -16, 0 );
	pev->absmax = pev->origin + Vector( 16, 16, 72 );
}

void CTFGoalFlag::ReturnFlag()
{
	if ( pev->owner )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)GET_PRIVATE( pev->owner );
		pPlayer->m_pFlag = NULL;
		pPlayer->m_iItems &= ~CTF_FLAG_MASK;
	}
	SetTouch( NULL );
	SetThink( NULL );
	pev->nextthink = gpGlobals->time;
	m_iGoalState = CTF_GOAL_HOME;
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->aiment = NULL;
	pev->angles = m_OriginalAngles;
	pev->effects |= EF_NODRAW;
	pev->origin = pev->oldorigin;
	UTIL_SetOrigin( pev, pev->origin );
	if ( pev->model )
	{
		pev->sequence = LookupSequence( "flag_positioned" );
		if ( pev->sequence != -1 )
		{
			ResetSequenceInfo();
			pev->frame = 0;
		}
	}
	SetThink( &CTFGoalFlag::ReturnFlagThink );
	pev->nextthink = gpGlobals->time + 0.25;
	CTFGoalBase* pBase = NULL;
	while ( ( pBase = (CTFGoalBase*)UTIL_FindEntityByClassname( pBase, "item_ctfbase" ) ) != NULL )
	{
		if ( pBase->m_iGoalNum == m_iGoalNum )
		{
			pBase->pev->skin = 0;
			return;
		}
	}
}

void CTFGoalFlag::ReturnFlagThink()
{
	pev->effects &= ~EF_NODRAW;
	SetThink( NULL );
	SetTouch( &CTFGoalFlag::goal_item_touch );
	UTIL_LogPrintf( "World triggered \"%s\"\n", m_iGoalNum == 1 ? "ReturnedBlackMesaFlag" :
		m_iGoalNum == 2 ? "ReturnedOpposingForceFlag" : "" );
	DisplayTeamFlags();
}

void CTFGoalFlag::TurnOnLight( CBasePlayer* pPlayer )
{
	Vector origin, angles;
	GetAttachment( 0, origin, angles );
	MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pPlayer->pev );
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( entindex() + 0x1000 );
		WRITE_COORD( origin.x );
		WRITE_COORD( origin.y );
		WRITE_COORD( origin.z );
		WRITE_COORD( 128 );
		WRITE_BYTE( m_iGoalNum == 1 ? 255 : 128 );
		WRITE_BYTE( m_iGoalNum == 1 ? 128 : 255 );
		WRITE_BYTE( 128 );
		WRITE_BYTE( 255 );
		WRITE_COORD( 0 );
	MESSAGE_END();
}

void CTFGoalFlag::FlagCarryThink()
{
	Vector origin, angles;
	GetAttachment( 0, origin, angles );
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( entindex() + 0x1000 );
		WRITE_COORD( origin.x );
		WRITE_COORD( origin.y );
		WRITE_COORD( origin.z );
		WRITE_COORD( 128 );
		WRITE_BYTE( m_iGoalNum == 1 ? 255 : 128 );
		WRITE_BYTE( m_iGoalNum == 1 ? 128 : 255 );
		WRITE_BYTE( 128 );
		WRITE_BYTE( 255 );
		WRITE_COORD( 0 );
	MESSAGE_END();
	CBasePlayer* pPlayer = (CBasePlayer*)CBaseEntity::Instance( pev->owner );
	if ( !pPlayer || !pPlayer->IsPlayer() )
	{
		ReturnFlag();
		return;
	}
	if ( !( pPlayer->m_iItems & CTF_FLAG_MASK ) )
	{
		pPlayer->m_iClientItems = 3;
		ReturnFlag();
		return;
	}
	CTFGoalFlag* pFlag = NULL;
	while ( ( pFlag = (CTFGoalFlag*)UTIL_FindEntityByClassname( pFlag, "item_ctfflag" ) ) != NULL )
	{
		if ( pFlag->m_iGoalNum == pPlayer->m_iTeamNum )
			break;
	}
	ClientPrint( pPlayer->pev, HUD_PRINTCENTER, pFlag && pFlag->m_iGoalState == CTF_GOAL_HOME ?
		"#CTFPickUpFlagP" : "#CTFPickUpFlagG" );
	pev->nextthink = gpGlobals->time + 20;
}

void CTFGoalFlag::GiveFlagToPlayer( CBasePlayer* pPlayer )
{
	pPlayer->m_pFlag = this;
	if ( m_iGoalNum == 1 )
		pPlayer->m_iItems |= CTF_FLAG_BM;
	else if ( m_iGoalNum == 2 )
		pPlayer->m_iItems |= CTF_FLAG_OF;
	SetTouch( NULL );
	SetThink( NULL );
	pev->nextthink = gpGlobals->time;
	pev->owner = pPlayer->edict();
	pev->movetype = MOVETYPE_FOLLOW;
	pev->aiment = pPlayer->edict();
	if ( pev->model )
	{
		pev->sequence = LookupSequence( "carried" );
		if ( pev->sequence != -1 )
		{
			ResetSequenceInfo();
			pev->frame = 0;
		}
	}
	pev->effects &= ~EF_NODRAW;
	pev->solid = SOLID_NOT;
	m_iGoalState = CTF_GOAL_CARRIED;
	if ( pPlayer->m_iTeamNum == 1 )
		EMIT_SOUND_DYN( edict(), CHAN_STATIC, "ctf/bm_flagtaken.wav", 1, ATTN_NONE, 0, PITCH_NORM );
	else if ( pPlayer->m_iTeamNum == 2 )
		EMIT_SOUND_DYN( edict(), CHAN_STATIC, "ctf/soldier_flagtaken.wav", 1, ATTN_NONE, 0, PITCH_NORM );
	ScoreFlagTouch( pPlayer );
	SetThink( &CTFGoalFlag::FlagCarryThink );
	pev->nextthink = gpGlobals->time + 0.1;
	++pPlayer->m_iCTFScore;
	++pPlayer->m_iOffense;
	MESSAGE_BEGIN( MSG_ALL, gmsgCTFScore );
		WRITE_BYTE( pPlayer->entindex() );
		WRITE_BYTE( pPlayer->m_iCTFScore );
	MESSAGE_END();
	ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#CTFScorePoint" );
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( pPlayer->entindex() );
		WRITE_SHORT( (int)pPlayer->pev->frags );
		WRITE_SHORT( pPlayer->m_iDeaths );
	MESSAGE_END();
	DisplayTeamFlags();
	CTFGoalBase* pBase = NULL;
	while ( ( pBase = (CTFGoalBase*)UTIL_FindEntityByClassname( pBase, "item_ctfbase" ) ) != NULL )
	{
		if ( pBase->m_iGoalNum == m_iGoalNum )
		{
			pBase->pev->skin = 1;
			return;
		}
	}
}

void CTFGoalFlag::SetDropTouch()
{
	SetTouch( &CTFGoalFlag::goal_item_touch );
	SetThink( &CTFGoalFlag::goal_item_dropthink );
	pev->nextthink = gpGlobals->time + 4.5;
}

void CTFGoalFlag::DoDrop( Vector vecOrigin )
{
	pev->flags &= ~FL_ONGROUND;
	pev->origin = vecOrigin;
	UTIL_SetOrigin( pev, pev->origin );
	pev->angles = g_vecZero;
	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	pev->movetype = MOVETYPE_TOSS;
	pev->velocity = g_vecZero;
	pev->velocity.z = -100;
	SetTouch( &CTFGoalFlag::goal_item_touch );
	SetThink( &CTFGoalFlag::goal_item_dropthink );
	pev->nextthink = gpGlobals->time + 5;
}

void CTFGoalFlag::DropFlag( CBasePlayer* pPlayer )
{
	pPlayer->m_pFlag = NULL;
	pPlayer->m_iItems &= ~CTF_FLAG_MASK;
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->aiment = NULL;
	pev->owner = NULL;
	if ( pev->model )
	{
		pev->sequence = LookupSequence( "not_carried" );
		if ( pev->sequence != -1 )
		{
			ResetSequenceInfo();
			pev->frame = 0;
		}
	}
	DoDrop( pPlayer->pev->origin + Vector( 0, 0, ( pPlayer->pev->flags & FL_DUCKING ) ? 34 : 16 ) );
	m_iGoalState = CTF_GOAL_DROPPED;
	DisplayTeamFlags();
}

void CTFGoalFlag::goal_item_dropthink()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->aiment = NULL;
	int contents = UTIL_PointContents( pev->origin );
	if ( contents == CONTENTS_SLIME || contents == CONTENTS_LAVA ||
		contents == CONTENTS_SOLID || contents == CONTENTS_SKY )
	{
		ReturnFlag();
		return;
	}
	SetThink( &CTFGoalFlag::ReturnFlag );
	pev->nextthink = gpGlobals->time + max( 0.0, g_flFlagReturnTime - 5.0 );
}

void CTFGoalFlag::goal_item_touch( CBaseEntity* pOther )
{
	if ( pOther->Classify() != CLASS_PLAYER || pOther->pev->health <= 0 )
		return;
	CBasePlayer* pPlayer = (CBasePlayer*)pOther;
	if ( pPlayer->m_iTeamNum != m_iGoalNum )
	{
		GiveFlagToPlayer( pPlayer );
		return;
	}
	if ( m_iGoalState == CTF_GOAL_HOME )
	{
		if ( pPlayer->m_iItems & CTF_FLAG_MASK )
		{
			CTFGoalFlag* pFlag = (CTFGoalFlag*)(CBaseEntity*)pPlayer->m_pFlag;
			if ( pFlag )
				pFlag->ReturnFlag();
			if ( pPlayer->m_iTeamNum == 1 )
				EMIT_SOUND_DYN( edict(), CHAN_STATIC, "ctf/civ_flag_capture.wav", 1, ATTN_NONE, 0, PITCH_NORM );
			else if ( pPlayer->m_iTeamNum == 2 )
				EMIT_SOUND_DYN( edict(), CHAN_STATIC, "ctf/marine_flag_capture.wav", 1, ATTN_NONE, 0, PITCH_NORM );
			ScoreFlagTouch( pPlayer );
		}
		return;
	}
	if ( pPlayer->m_iTeamNum == 1 || pPlayer->m_iTeamNum == 2 )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, STRING( pPlayer->pev->netname ) );
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, pPlayer->m_iTeamNum == 1 ? "#CTFFlagReturnBM" : "#CTFFlagReturnOF" );
		UTIL_LogPrintf( "\"%s<%i><%u><%s>\" triggered \"ReturnedFlag\"\n", STRING( pPlayer->pev->netname ),
			GETPLAYERUSERID( pPlayer->edict() ), g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() ), GetTeamName( pPlayer->edict() ) );
	}
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer* pClient = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( !pClient || ( pClient->pev->flags & FL_DORMANT ) )
			continue;
		if ( ( pClient->m_iTeamNum == 1 && m_iGoalNum == 1 ) ||
			( pClient->m_iTeamNum == 2 && m_iGoalNum == 2 ) )
			ClientPrint( pClient->pev, HUD_PRINTCENTER, "#CTFFlagReturnedT" );
		else if ( m_iGoalNum == 1 || m_iGoalNum == 2 )
			ClientPrint( pClient->pev, HUD_PRINTCENTER, "#CTFFlagReturnedE" );
	}
	++pPlayer->m_iCTFScore;
	++pPlayer->m_iDefense;
	MESSAGE_BEGIN( MSG_ALL, gmsgCTFScore );
		WRITE_BYTE( pPlayer->entindex() );
		WRITE_BYTE( pPlayer->m_iCTFScore );
	MESSAGE_END();
	ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#CTFScorePoint" );
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( pPlayer->entindex() );
		WRITE_SHORT( (int)pPlayer->pev->frags );
		WRITE_SHORT( pPlayer->m_iDeaths );
	MESSAGE_END();
	ReturnFlag();
	m_nReturnPlayer = pPlayer->entindex();
	m_flReturnTime = gpGlobals->time;
}

void CTFGoalFlag::ScoreFlagTouch( CBasePlayer* pPlayer )
{
	if ( !pPlayer || m_iGoalNum < 1 || m_iGoalNum > 2 )
	{
		DisplayTeamFlags();
		return;
	}
	BOOL captured = pPlayer->m_iTeamNum == m_iGoalNum;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer* pClient = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( !pClient || ( pClient->pev->flags & FL_DORMANT ) )
			continue;
		const char* message;
		if ( pClient->m_iTeamNum != pPlayer->m_iTeamNum )
			message = captured ? "#CTFFlagCaptured" : "#CTFLoseFlag";
		else if ( captured )
			message = pClient == pPlayer ? "#CTFCaptureFlagP" : "#CTFCaptureFlagT";
		else if ( pClient != pPlayer )
			message = "#CTFPickUpFlagT";
		else
		{
			CTFGoalFlag* pFlag = NULL;
			while ( ( pFlag = (CTFGoalFlag*)UTIL_FindEntityByClassname( pFlag, "item_ctfflag" ) ) != NULL )
			{
				if ( pFlag->m_iGoalNum == pPlayer->m_iTeamNum )
					break;
			}
			message = pFlag && pFlag->m_iGoalState == CTF_GOAL_HOME ? "#CTFPickUpFlagP" : "#CTFPickUpFlagG";
		}
		ClientPrint( pClient->pev, HUD_PRINTCENTER, message );
	}
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, STRING( pPlayer->pev->netname ) );
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, captured ?
		( m_iGoalNum == 1 ? "#CTFFlagScorerOF" : "#CTFFlagScorerBM" ) :
		( m_iGoalNum == 1 ? "#CTFFlagGetBM" : "#CTFFlagGetOF" ) );
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "\n" );
	UTIL_LogPrintf( "\"%s<%i><%u><%s>\" triggered \"%s\"\n", STRING( pPlayer->pev->netname ),
		GETPLAYERUSERID( pPlayer->edict() ), g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() ), GetTeamName( pPlayer->edict() ),
		captured ? "CapturedFlag" : "TookFlag" );
	if ( captured )
	{
		++teamscores[pPlayer->m_iTeamNum - 1];
		++pPlayer->m_iFlagCaptures;
		pPlayer->m_iCTFScore += 10;
		pPlayer->m_iOffense += 10;
		MESSAGE_BEGIN( MSG_ALL, gmsgCTFScore );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_BYTE( pPlayer->m_iCTFScore );
		MESSAGE_END();
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#CTFScorePoints" );
		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_SHORT( (int)pPlayer->pev->frags );
			WRITE_SHORT( pPlayer->m_iDeaths );
		MESSAGE_END();
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#CTFTeamBM" );
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( ": %d\n", teamscores[0] ) );
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#CTFTeamOF" );
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( ": %d\n", teamscores[1] ) );
		if ( m_nReturnPlayer > 0 && g_flCaptureAssistTime > gpGlobals->time - m_flReturnTime )
		{
			CBasePlayer* pAssist = (CBasePlayer*)UTIL_PlayerByIndex( m_nReturnPlayer );
			++pPlayer->m_iCTFScore;
			++pPlayer->m_iOffense;
			MESSAGE_BEGIN( MSG_ALL, gmsgCTFScore );
				WRITE_BYTE( pPlayer->entindex() );
				WRITE_BYTE( pPlayer->m_iCTFScore );
			MESSAGE_END();
			ClientPrint( pAssist->pev, HUD_PRINTTALK, "#CTFScorePoint" );
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s", STRING( pAssist->pev->netname ) ) );
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, m_iGoalNum == 1 ? "#CTFFlagAssistBM" : "#CTFFlagAssistOF" );
		}
	}
	DisplayTeamFlags();
}

void CTFGoalFlag::DisplayFlagStatus( CBasePlayer* pPlayer )
{
	if ( m_iGoalState == CTF_GOAL_HOME )
		ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, m_iGoalNum == 1 ? "#CTFInfoFlagAtBaseBM" : "#CTFInfoFlagAtBaseOF" );
	else if ( m_iGoalState == CTF_GOAL_DROPPED )
		ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, m_iGoalNum == 1 ? "#CTFInfoFlagDroppedBM" : "#CTFInfoFlagDroppedOF" );
	else if ( m_iGoalState == CTF_GOAL_CARRIED )
	{
		CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
		if ( pOwner )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, "%s", STRING( pOwner->pev->netname ) );
			ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, m_iGoalNum == 1 ? "#CTFInfoFlagCarriedBM" : "#CTFInfoFlagCarriedOF" );
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, m_iGoalNum == 1 ? "#CTFInfoFlagCarryBM" : "#CTFInfoFlagCarryOF" );
	}
}

void CTFSpawn::Spawn()
{
	if ( (unsigned int)team_no > 2 )
	{
		ALERT( at_console, "Teamspawnpoint with an invalid team_no of %d\n", team_no );
		return;
	}
	pev->netname = pev->classname;
	pev->classname = MAKE_STRING( sTeamSpawnNames[team_no] );
	m_fState = true;
}

void CTFSpawn::Use( CBaseEntity*, CBaseEntity*, USE_TYPE useType, float )
{
	if ( useType == USE_ON )
		m_fState = true;
	else if ( useType == USE_OFF )
		m_fState = false;
	else
		m_fState = !m_fState;
}

BOOL CTFSpawn::IsTriggered( CBaseEntity* pEntity )
{
	if ( pev->targetname && STRING( pev->targetname ) )
		return m_fState;
	return UTIL_IsMasterTriggered( pev->netname, pEntity );
}

void CTFSpawn::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "team_no" ) )
		team_no = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "master" ) )
		pev->netname = ALLOC_STRING( pkvd->szValue );
	else
	{
		pkvd->fHandled = FALSE;
		return;
	}
	pkvd->fHandled = TRUE;
}

void CTFDetect::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "is_ctf" ) )
		is_ctf = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "flagreturntime" ) )
		g_flFlagReturnTime = atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "basedefenddist" ) )
		g_flBaseDefendDist = atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "defendcarriertime" ) )
		g_flDefendCarrierTime = atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "captureassisttime" ) )
		g_flCaptureAssistTime = atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "poweruprespawntime" ) )
		g_flPowerupRespawnTime = atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "score_icon_namebm" ) )
		strcpy( g_szScoreIconNameBM, pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "score_icon_nameof" ) )
		strcpy( g_szScoreIconNameOF, pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "map_score_max" ) )
		g_iMapScoreMax = atoi( pkvd->szValue );
	else
	{
		pkvd->fHandled = FALSE;
		return;
	}
	pkvd->fHandled = TRUE;
}

void DisplayTeamFlags( CBasePlayer* pPlayer )
{
	CTFGoalFlag* flags[2] = { NULL, NULL };
	CTFGoalFlag* pFlag = NULL;
	while ( ( pFlag = (CTFGoalFlag*)UTIL_FindEntityByClassname( pFlag, "item_ctfflag" ) ) != NULL )
	{
		if ( pFlag->m_iGoalNum >= 1 && pFlag->m_iGoalNum <= 2 && !flags[pFlag->m_iGoalNum - 1] )
			flags[pFlag->m_iGoalNum - 1] = pFlag;
	}
	int first = pPlayer ? pPlayer->entindex() : 1;
	int last = pPlayer ? first : gpGlobals->maxClients;
	for ( int i = first; i <= last; ++i )
	{
		CBasePlayer* pClient = pPlayer ? pPlayer : (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( !pClient || !gmsgFlagIcon )
			continue;
		CBaseEntity* pCarried = pClient->m_pFlag;
		for ( int team = 0; team < 2; ++team )
		{
			if ( !flags[team] && ( flags[0] || flags[1] ) )
				continue;
			MESSAGE_BEGIN( MSG_ONE, gmsgFlagIcon, NULL, pClient->pev );
				WRITE_BYTE( 1 );
				if ( !flags[0] && !flags[1] )
					WRITE_STRING( team ? g_szScoreIconNameOF : g_szScoreIconNameBM );
				else if ( flags[team]->m_iGoalState == CTF_GOAL_HOME )
					WRITE_STRING( "item_ctfflagh" );
				else if ( flags[team]->m_iGoalState == CTF_GOAL_DROPPED )
					WRITE_STRING( "item_ctfflagd" );
				else if ( flags[team]->m_iGoalState == CTF_GOAL_CARRIED )
					WRITE_STRING( pCarried == flags[team] && pCarried ? "item_ctfflagg" : "item_ctfflagc" );
				WRITE_BYTE( team );
				WRITE_BYTE( team ? 0 : 255 );
				WRITE_BYTE( 160 );
				WRITE_BYTE( 0 );
				WRITE_BYTE( teamscores[team] );
			MESSAGE_END();
		}
	}
}

void ResetTeamScores()
{
	teamscores[0] = teamscores[1] = 0;
}

int CheckForLevelEnd( int iMaxScore )
{
	if ( !iMaxScore )
		iMaxScore = 255;
	if ( g_iMapScoreMax > 0 && iMaxScore > g_iMapScoreMax )
		iMaxScore = g_iMapScoreMax;
	return teamscores[0] >= iMaxScore || teamscores[1] >= iMaxScore;
}

void GetLosingTeam( int& iTeamNum, int& iScoreDiff )
{
	iTeamNum = 0;
	iScoreDiff = 0;
	if ( teamscores[1] < teamscores[iTeamNum] )
	{
		iTeamNum = 1;
		if ( iScoreDiff < teamscores[0] - teamscores[1] )
			iScoreDiff = teamscores[0] - teamscores[1];
	}
}

int GetWinningTeam()
{
	if ( teamscores[0] == teamscores[1] )
		return 0;
	return teamscores[0] > teamscores[1] ? 1 : 2;
}

void DumpCTFFlagInfo( CBasePlayer* pPlayer )
{
	if ( !g_pGameRules->IsCTF() )
		return;
	CTFGoalFlag* pFlag = NULL;
	while ( ( pFlag = (CTFGoalFlag*)UTIL_FindEntityByClassname( pFlag, "item_ctfflag" ) ) != NULL )
		pFlag->DisplayFlagStatus( pPlayer );
}

void UpdateCTFCustomIcons( CBasePlayer* )
{
}

class CTriggerCTFGeneric : public CBaseTrigger
{
public:
	void Spawn();
	void Touch( CBaseEntity* pOther );
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData* pkvd );

	USE_TYPE	triggerType;
	int			team_no;
	float		trigger_delay;
	float		m_flTriggerDelayTime;
	int			score;
	int			team_score;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( trigger_ctfgeneric, CTriggerCTFGeneric );

void CTriggerCTFGeneric::Spawn()
{
	InitTrigger();
	m_flTriggerDelayTime = 0;
}

void CTriggerCTFGeneric::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	Touch( NULL );
}

void CTriggerCTFGeneric::Touch( CBaseEntity* pOther )
{
	if ( m_flTriggerDelayTime > gpGlobals->time ||
		( pOther && ( score || !pOther->IsPlayer() || ( team_no && team_no != ( (CBasePlayer*)pOther )->m_iTeamNum ) ) ) )
		return;
	SUB_UseTargets( this, triggerType, 0 );
	if ( team_score && (unsigned int)team_no - 1 < ARRAYSIZE( teamscores ) )
		teamscores[team_no - 1] += team_score;
	if ( pOther && score )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)pOther;
		pPlayer->m_iCTFScore += score;
		pPlayer->m_iOffense += score;
		MESSAGE_BEGIN( MSG_ALL, gmsgCTFScore );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_BYTE( pPlayer->m_iCTFScore );
		MESSAGE_END();
		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_SHORT( (int)pPlayer->pev->frags );
			WRITE_SHORT( pPlayer->m_iDeaths );
		MESSAGE_END();
		UTIL_LogPrintf( "\"%s<%i><%u><%s>\" triggered \"%s\"\n", STRING( pPlayer->pev->netname ),
			GETPLAYERUSERID( pPlayer->edict() ), g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() ), GetTeamName( pPlayer->edict() ),
			STRING( pev->targetname ) );
	}
	else if ( team_score && pev )
		UTIL_LogPrintf( "World triggered \"%s\"\n", STRING( pev->targetname ) );
	if ( team_score )
		DisplayTeamFlags();
	m_flTriggerDelayTime = gpGlobals->time + trigger_delay;
}

void CTriggerCTFGeneric::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "team_no" ) )
		team_no = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "trigger_delay" ) )
	{
		trigger_delay = atof( pkvd->szValue );
		if ( !trigger_delay )
			trigger_delay = 5;
	}
	else if ( FStrEq( pkvd->szKeyName, "score" ) )
		score = (int)atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "team_score" ) )
		team_score = (int)atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		int state = atoi( pkvd->szValue );
		triggerType = state == 0 ? USE_OFF : state == 2 ? USE_TOGGLE : USE_ON;
	}
	else
	{
		pkvd->fHandled = FALSE;
		return;
	}
	pkvd->fHandled = TRUE;
}
