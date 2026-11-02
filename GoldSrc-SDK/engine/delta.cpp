#include "delta.h"
#include "common.h"
#include "sys.h"
#include "mem.h"
#include "conprint.h"
#include "cmd.h"
#include "server.h"
#include "client.h"
#include "../common/entity_state.h"
#include "../common/weaponinfo.h"
#include "../common/usercmd.h"
#include "../common/event_args.h"

#include <cassert>
#include <cstdlib>
#include <cctype>

#include "buildslave_assert.h"
#define ASSERT_FILE BUILDSLAVE_PATH( L"engine\\delta.c" )

#define LOADFILE_HEAP		5

#define DT_BYTE				( 1 << 0 )
#define DT_SHORT			( 1 << 1 )
#define DT_FLOAT			( 1 << 2 )
#define DT_INTEGER			( 1 << 3 )
#define DT_ANGLE			( 1 << 4 )
#define DT_TIMEWINDOW_8		( 1 << 5 )
#define DT_TIMEWINDOW_BIG	( 1 << 6 )
#define DT_STRING			( 1 << 7 )
#define DT_SIGNED			( 1 << 31 )

#define FDT_MARK			( 1 << 0 )

static delta_definition_list_t* g_defs;
static delta_encoder_t* g_encoders;
static delta_registry_t* g_deltaregistry;

delta_description_t* DELTA_FindField( delta_t* pFields, const char* pszField )
{
	int i;

	for ( i = 0; i < pFields->fieldCount; i++ )
	{
		if ( !Q_strcasecmp( pFields->pdd[i].fieldName, pszField ) )
			return &pFields->pdd[i];
	}

	Con_Printf( "DELTA_FindField:  Warning, couldn't find %s\n", pszField );
	return NULL;
}

int DELTA_FindFieldIndex( delta_t* pFields, const char* pszField )
{
	int i;

	for ( i = 0; i < pFields->fieldCount; i++ )
	{
		if ( !Q_strcasecmp( pFields->pdd[i].fieldName, pszField ) )
			return i;
	}

	Con_Printf( "DELTA_FindFieldIndex:  Warning, couldn't find %s\n", pszField );
	return -1;
}

void DELTA_SetField( delta_t* pFields, const char* pszField )
{
	delta_description_t* pTest;

	pTest = DELTA_FindField( pFields, pszField );
	if ( pTest )
		pTest->flags |= FDT_MARK;
}

void DELTA_UnsetField( delta_t* pFields, const char* pszField )
{
	delta_description_t* pTest;

	pTest = DELTA_FindField( pFields, pszField );
	if ( pTest )
		pTest->flags &= ~FDT_MARK;
}

void DELTA_SetFieldByIndex( delta_t* pFields, int fieldNumber )
{
	pFields->pdd[fieldNumber].flags |= FDT_MARK;
}

void DELTA_UnsetFieldByIndex( delta_t* pFields, int fieldNumber )
{
	pFields->pdd[fieldNumber].flags &= ~FDT_MARK;
}

void DELTA_AddEncoder( char* name, encoder_t conditionalencode )
{
	delta_encoder_t* p;

	p = (delta_encoder_t*)Mem_ZeroMalloc( sizeof( delta_encoder_t ) );
	p->name = Mem_Strdup( name );
	p->conditionalencode = conditionalencode;
	p->next = g_encoders;
	g_encoders = p;
}

encoder_t DELTA_LookupEncoder( char* name )
{
	delta_encoder_t* p;

	for ( p = g_encoders; p; p = p->next )
	{
		if ( !Q_strcasecmp( p->name, name ) )
			return p->conditionalencode;
	}

	return NULL;
}

void DELTA_ClearEncoders()
{
	delta_encoder_t *p, *n;

	for ( p = g_encoders; p; p = n )
	{
		n = p->next;
		Mem_Free( p->name );
		Mem_Free( p );
	}

	g_encoders = NULL;
}

void DELTA_ClearDefinitions()
{
	delta_definition_list_t *p, *n;

	for ( p = g_defs; p; p = n )
	{
		n = p->next;
		Mem_Free( p->ptypename );
		Mem_Free( p );
	}

	g_defs = NULL;
}

void DELTA_FreeDescription( delta_t** ppdesc )
{
	if ( !ppdesc || !*ppdesc )
		return;

	if ( ( *ppdesc )->dynamic )
		Mem_Free( ( *ppdesc )->pdd );

	Mem_Free( *ppdesc );
	*ppdesc = NULL;
}

void DELTA_ClearRegistrations()
{
	delta_registry_t *p, *n;

	for ( p = g_deltaregistry; p; p = n )
	{
		n = p->next;
		Mem_Free( p->name );
		DELTA_FreeDescription( &p->pdesc );
		Mem_Free( p );
	}

	g_deltaregistry = NULL;
}

void DELTA_Shutdown()
{
	DELTA_ClearEncoders();
	DELTA_ClearDefinitions();
	DELTA_ClearRegistrations();
}

void DELTA_ClearStats( delta_t* p )
{
	int i;

	if ( !p )
		return;

	for ( i = 0; i < p->fieldCount; i++ )
	{
		p->pdd[i].stats.sendcount = 0;
		p->pdd[i].stats.receivedcount = 0;
	}
}

void DELTA_ClearStats_f()
{
	delta_registry_t* p;

	Con_Printf( "Clearing delta stats\n" );

	for ( p = g_deltaregistry; p; p = p->next )
		DELTA_ClearStats( p->pdesc );
}

void DELTA_PrintStats( char* name, delta_t* p )
{
	int i;
	delta_description_t* dt;

	if ( !p )
		return;

	Con_Printf( "Stats for '%s'\n", name );

	for ( i = 0; i < p->fieldCount; i++ )
	{
		dt = &p->pdd[i];
		Con_Printf( "  %02i % 10s:  s % 5i r % 5i\n", i + 1, dt->fieldName, dt->stats.sendcount,
			dt->stats.receivedcount );
	}

	Con_Printf( "\n" );
}

void DELTA_DumpStats_f()
{
	delta_registry_t* p;

	Con_Printf( "Delta Stats\n" );

	for ( p = g_deltaregistry; p; p = p->next )
		DELTA_PrintStats( p->name, p->pdesc );
}

void DELTA_AddDefinition( char* name, delta_definition_t* pdef, int numelements )
{
	delta_definition_list_t* p;

	for ( p = g_defs; p; p = p->next )
	{
		if ( !Q_strcasecmp( name, p->ptypename ) )
			break;
	}

	if ( !p )
	{
		p = (delta_definition_list_t*)Mem_ZeroMalloc( sizeof( delta_definition_list_t ) );
		p->next = g_defs;
		g_defs = p;
		p->ptypename = Mem_Strdup( name );
	}

	p->pdefinition = pdef;
	p->numelements = numelements;
}

delta_description_t g_MetaDescription[] =
{
	{ DT_INTEGER, "fieldType", offsetof( delta_description_t, fieldType ), 1, 32, 1.0f, 1.0f },
	{ DT_STRING, "fieldName", offsetof( delta_description_t, fieldName ), 1, 1, 1.0f, 1.0f },
	{ DT_INTEGER, "fieldOffset", offsetof( delta_description_t, fieldOffset ), 1, 16, 1.0f, 1.0f },
	{ DT_INTEGER, "fieldSize", offsetof( delta_description_t, fieldSize ), 1, 8, 1.0f, 1.0f },
	{ DT_INTEGER, "significant_bits", offsetof( delta_description_t, significant_bits ), 1, 8, 1.0f, 1.0f },
	{ DT_FLOAT, "premultiply", offsetof( delta_description_t, premultiply ), 1, 32, 4000.0f, 1.0f },
	{ DT_FLOAT, "postmultiply", offsetof( delta_description_t, postmultiply ), 1, 32, 4000.0f, 1.0f },
};

delta_t g_MetaDelta = { 0, sizeof( g_MetaDescription ) / sizeof( g_MetaDescription[0] ), "", NULL, g_MetaDescription };

int DELTA_FindOffset( int count, delta_definition_t* pdef, const char* fieldname )
{
	int i;

	for ( i = 0; i < count; i++ )
	{
		if ( !Q_strcasecmp( fieldname, pdef[i].fieldName ) )
			return pdef[i].fieldOffset;
	}

	Sys_Error( "Couldn't find offset for %s!!!\n", fieldname );
}

qboolean DELTA_ParseType( delta_description_t* pdelta, const char** pstream )
{
	while ( Q_strlen( com_token ) > 0 )
	{
		if ( !Q_strcasecmp( com_token, "," ) )
			return TRUE;

		if ( Q_strcasecmp( com_token, "|" ) )
		{
			if ( !Q_strcasecmp( com_token, "DT_SIGNED" ) )
				pdelta->fieldType |= DT_SIGNED;
			else if ( !Q_strcasecmp( com_token, "DT_BYTE" ) )
				pdelta->fieldType |= DT_BYTE;
			else if ( !Q_strcasecmp( com_token, "DT_SHORT" ) )
				pdelta->fieldType |= DT_SHORT;
			else if ( !Q_strcasecmp( com_token, "DT_FLOAT" ) )
				pdelta->fieldType |= DT_FLOAT;
			else if ( !Q_strcasecmp( com_token, "DT_INTEGER" ) )
				pdelta->fieldType |= DT_INTEGER;
			else if ( !Q_strcasecmp( com_token, "DT_ANGLE" ) )
				pdelta->fieldType |= DT_ANGLE;
			else if ( !Q_strcasecmp( com_token, "DT_TIMEWINDOW_8" ) )
				pdelta->fieldType |= DT_TIMEWINDOW_8;
			else if ( !Q_strcasecmp( com_token, "DT_TIMEWINDOW_BIG" ) )
				pdelta->fieldType |= DT_TIMEWINDOW_BIG;
			else if ( !Q_strcasecmp( com_token, "DT_STRING" ) )
				pdelta->fieldType |= DT_STRING;
			else
				Sys_Error( "DELTA_ParseField:  Unknown type or type flag %s\n", com_token );
		}

		*pstream = COM_Parse( *pstream );
	}

	Con_Printf( "DELTA_ParseField:  Expecting fieldtype info\n" );
	return FALSE;
}

qboolean DELTA_ParseField( int count, delta_definition_t* pdefinition, delta_link_t* pField, const char** pstream )
{
	qboolean readpost = FALSE;

	if ( Q_strcasecmp( com_token, "DEFINE_DELTA" ) )
	{
		if ( Q_strcasecmp( com_token, "DEFINE_DELTA_POST" ) )
			Sys_Error( "DELTA_ParseField:  Expecting DEFINE_*, got %s\n", com_token );
		readpost = TRUE;
	}

	*pstream = COM_Parse( *pstream );
	if ( Q_strcasecmp( com_token, "(" ) )
		Sys_Error( "DELTA_ParseField:  Expecting (, got %s\n", com_token );

	*pstream = COM_Parse( *pstream );
	if ( Q_strlen( com_token ) <= 0 )
		Sys_Error( "DELTA_ParseField:  Expecting fieldname\n" );

	Q_strncpy( pField->delta->fieldName, com_token, DELTA_FIELD_NAME_BYTES - 1 );
	pField->delta->fieldName[DELTA_FIELD_NAME_BYTES - 1] = 0;
	pField->delta->fieldOffset = DELTA_FindOffset( count, pdefinition, com_token );

	*pstream = COM_Parse( *pstream );
	*pstream = COM_Parse( *pstream );
	if ( !DELTA_ParseType( pField->delta, pstream ) )
		return FALSE;

	*pstream = COM_Parse( *pstream );
	pField->delta->fieldSize = 1;
#ifdef _WIN32
	ASSERT_AT( Q_atoi( com_token ) > 0 && Q_atoi( com_token ) <= 32, 1646 );
#endif
	pField->delta->significant_bits = Q_atoi( com_token );

	*pstream = COM_Parse( *pstream );
	*pstream = COM_Parse( *pstream );
	pField->delta->premultiply = Q_atof( com_token );

	if ( readpost )
	{
		*pstream = COM_Parse( *pstream );
		*pstream = COM_Parse( *pstream );
		pField->delta->postmultiply = (float)atof( com_token );
	}
	else
	{
		pField->delta->postmultiply = 1.0f;
	}

	*pstream = COM_Parse( *pstream );
	if ( Q_strcasecmp( com_token, ")" ) )
	{
		Con_Printf( "DELTA_ParseField:  Expecting ), got %s\n", com_token );
		return FALSE;
	}

	*pstream = COM_Parse( *pstream );
	if ( Q_strcasecmp( com_token, "," ) )
		COM_UngetToken();

	return TRUE;
}

delta_definition_t* DELTA_FindDefinition( char* name, int* count )
{
	delta_definition_list_t* p;

	for ( p = g_defs; p; p = p->next )
	{
		if ( !Q_strcasecmp( name, p->ptypename ) )
		{
			*count = p->numelements;
			return p->pdefinition;
		}
	}

	*count = 0;
	return NULL;
}

void DELTA_ReverseLinks( delta_link_t** plinks )
{
	delta_link_t *p, *n, *newlist;

	newlist = NULL;
	for ( p = *plinks; p; p = n )
	{
		n = p->next;
		p->next = newlist;
		newlist = p;
	}

	*plinks = newlist;
}

int DELTA_CountLinks( delta_link_t* plinks )
{
	delta_link_t* p;
	int c;

	c = 0;
	for ( p = plinks; p; p = p->next )
		c++;

	return c;
}

void DELTA_ClearLinks( delta_link_t** plinks )
{
	delta_link_t *p, *n;

	for ( p = *plinks; p; p = n )
	{
		n = p->next;
		free( p );
	}

	*plinks = NULL;
}

delta_t* DELTA_BuildFromLinks( delta_link_t** pplinks )
{
	delta_t* pdesc;
	delta_link_t* p;
	delta_description_t *pdd, *pcur;
	int count;

	pdesc = (delta_t*)Mem_ZeroMalloc( sizeof( delta_t ) );

	DELTA_ReverseLinks( pplinks );

	count = DELTA_CountLinks( *pplinks );
	pdd = (delta_description_t*)Mem_ZeroMalloc( count * sizeof( delta_description_t ) );

	for ( p = *pplinks, pcur = pdd; p; p = p->next, pcur++ )
	{
		Q_memcpy( pcur, p->delta, sizeof( delta_description_t ) );
		free( p->delta );
		p->delta = NULL;
	}

	DELTA_ClearLinks( pplinks );

	pdesc->fieldCount = count;
	pdesc->pdd = pdd;
	pdesc->dynamic = 1;
	return pdesc;
}

qboolean DELTA_ParseOneField( const char** ppstream, delta_link_t** pplist, int count, delta_definition_t* pdefinition )
{
	delta_link_t* newlink;
	delta_link_t link;

	Q_memset( &link, 0, sizeof( link ) );
	link.delta = (delta_description_t*)Mem_ZeroMalloc( sizeof( delta_description_t ) );
	if ( !DELTA_ParseField( count, pdefinition, &link, ppstream ) )
		return FALSE;

	newlink = (delta_link_t*)Mem_ZeroMalloc( sizeof( delta_link_t ) );
	newlink->delta = link.delta;
	newlink->next = *pplist;
	*pplist = newlink;
	return TRUE;
}

void DELTA_SkipDescription( const char** pstream )
{
	*pstream = COM_Parse( *pstream );

	do
	{
		*pstream = COM_Parse( *pstream );
		if ( Q_strlen( com_token ) <= 0 )
			Sys_Error( "Error during description skip" );
	} while ( Q_stricmp( com_token, "}" ) );
}

qboolean DELTA_ParseDescription( char* name, delta_t** ppdesc, char* pstream )
{
	delta_link_t* links;
	delta_definition_t* pdefinition;
	const char* p;
	char encoder[DELTA_ENCODER_NAME_BYTES];
	char source[DELTA_ENCODER_NAME_BYTES];
	int count;

	links = NULL;
	encoder[0] = 0;

	if ( !ppdesc )
		Sys_Error( "DELTA_ParseDescription with no delta_description_t\n" );

	*ppdesc = NULL;

	if ( !pstream )
		Sys_Error( "DELTA_ParseDescription with no data stream\n" );

	p = COM_Parse( pstream );
	while ( Q_strlen( com_token ) > 0 )
	{
		if ( Q_strcasecmp( com_token, name ) )
		{
			DELTA_SkipDescription( &p );
		}
		else
		{
			pdefinition = DELTA_FindDefinition( com_token, &count );
			if ( !count )
				Sys_Error( "DELTA_ParseDescription:  Unknown data type:  %s\n", com_token );

			p = COM_Parse( p );
			if ( Q_strlen( com_token ) <= 0 )
				Sys_Error(
					"DELTA_ParseDescription:  Unknown encoder :  %s\nValid values:\nnone\ngamedll funcname\nclientdll funcname\n",
					com_token );

			if ( Q_strcasecmp( com_token, "none" ) )
			{
				Q_strncpy( source, com_token, sizeof( source ) - 1 );
				source[sizeof( source ) - 1] = 0;

				p = COM_Parse( p );
				if ( Q_strlen( com_token ) <= 0 )
					Sys_Error( "DELTA_ParseDescription:  Expecting encoder\n" );

				Q_strncpy( encoder, com_token, sizeof( encoder ) - 1 );
				encoder[sizeof( encoder ) - 1] = 0;
			}

			p = COM_Parse( p );
			while ( Q_strlen( com_token ) > 0 )
			{
				if ( !Q_strcasecmp( com_token, "}" ) )
					break;

				if ( Q_strcasecmp( com_token, "{" ) )
				{
					Con_Printf( "DELTA_ParseDescription:  Expecting {, got %s\n", com_token );
					return FALSE;
				}

				for ( ;; )
				{
					p = COM_Parse( p );
					if ( Q_strlen( com_token ) <= 0 )
						break;

					if ( !DELTA_ParseOneField( &p, &links, count, pdefinition ) )
						return FALSE;

					if ( !Q_strcasecmp( com_token, "}" ) )
					{
						COM_UngetToken();
						break;
					}
				}

				p = COM_Parse( p );
			}
		}

		p = COM_Parse( p );
	}

	*ppdesc = DELTA_BuildFromLinks( &links );

	if ( Q_strlen( encoder ) > 0 )
	{
		Q_strncpy( ( *ppdesc )->conditionalencodename, encoder, DELTA_ENCODER_NAME_BYTES - 1 );
		( *ppdesc )->conditionalencodename[DELTA_ENCODER_NAME_BYTES - 1] = 0;
		( *ppdesc )->conditionalencode = NULL;
	}

	return TRUE;
}

qboolean DELTA_Load( char* name, delta_t** ppdesc, char* pszFile )
{
	char* pbuf;
	qboolean bret;

#ifdef _WIN32
	ASSERT_AT( name && name[0], 2008 );
	ASSERT_AT( ppdesc, 2009 );
	ASSERT_AT( pszFile && pszFile[0], 2010 );
#endif

	pbuf = (char*)COM_LoadFile( pszFile, LOADFILE_HEAP, NULL );
	if ( !pbuf )
		Sys_Error( "DELTA_Load:  Couldn't load file %s\n", pszFile );

	bret = DELTA_ParseDescription( name, ppdesc, pbuf );
	free( pbuf );
	return bret;
}

#define DELTA_DEF( structname, fieldname ) { #fieldname, offsetof( structname, fieldname ) }

delta_definition_t g_ClientDataDefinition[] =
{
	DELTA_DEF( clientdata_t, origin[0] ),
	DELTA_DEF( clientdata_t, origin[1] ),
	DELTA_DEF( clientdata_t, origin[2] ),
	DELTA_DEF( clientdata_t, velocity[0] ),
	DELTA_DEF( clientdata_t, velocity[1] ),
	DELTA_DEF( clientdata_t, velocity[2] ),
	DELTA_DEF( clientdata_t, viewmodel ),
	DELTA_DEF( clientdata_t, punchangle[0] ),
	DELTA_DEF( clientdata_t, punchangle[1] ),
	DELTA_DEF( clientdata_t, punchangle[2] ),
	DELTA_DEF( clientdata_t, flags ),
	DELTA_DEF( clientdata_t, waterlevel ),
	DELTA_DEF( clientdata_t, watertype ),
	DELTA_DEF( clientdata_t, view_ofs[0] ),
	DELTA_DEF( clientdata_t, view_ofs[1] ),
	DELTA_DEF( clientdata_t, view_ofs[2] ),
	DELTA_DEF( clientdata_t, health ),
	DELTA_DEF( clientdata_t, bInDuck ),
	DELTA_DEF( clientdata_t, weapons ),
	DELTA_DEF( clientdata_t, flTimeStepSound ),
	DELTA_DEF( clientdata_t, flDuckTime ),
	DELTA_DEF( clientdata_t, flSwimTime ),
	DELTA_DEF( clientdata_t, waterjumptime ),
	DELTA_DEF( clientdata_t, maxspeed ),
	DELTA_DEF( clientdata_t, m_iId ),
	DELTA_DEF( clientdata_t, ammo_nails ),
	DELTA_DEF( clientdata_t, ammo_shells ),
	DELTA_DEF( clientdata_t, ammo_cells ),
	DELTA_DEF( clientdata_t, ammo_rockets ),
	DELTA_DEF( clientdata_t, m_flNextAttack ),
	DELTA_DEF( clientdata_t, physinfo ),
	DELTA_DEF( clientdata_t, fov ),
	DELTA_DEF( clientdata_t, weaponanim ),
	DELTA_DEF( clientdata_t, tfstate ),
	DELTA_DEF( clientdata_t, pushmsec ),
	DELTA_DEF( clientdata_t, deadflag ),
	DELTA_DEF( clientdata_t, iuser1 ),
	DELTA_DEF( clientdata_t, iuser2 ),
	DELTA_DEF( clientdata_t, iuser3 ),
	DELTA_DEF( clientdata_t, iuser4 ),
	DELTA_DEF( clientdata_t, fuser1 ),
	DELTA_DEF( clientdata_t, fuser2 ),
	DELTA_DEF( clientdata_t, fuser3 ),
	DELTA_DEF( clientdata_t, fuser4 ),
	DELTA_DEF( clientdata_t, vuser1[0] ),
	DELTA_DEF( clientdata_t, vuser1[1] ),
	DELTA_DEF( clientdata_t, vuser1[2] ),
	DELTA_DEF( clientdata_t, vuser2[0] ),
	DELTA_DEF( clientdata_t, vuser2[1] ),
	DELTA_DEF( clientdata_t, vuser2[2] ),
	DELTA_DEF( clientdata_t, vuser3[0] ),
	DELTA_DEF( clientdata_t, vuser3[1] ),
	DELTA_DEF( clientdata_t, vuser3[2] ),
	DELTA_DEF( clientdata_t, vuser4[0] ),
	DELTA_DEF( clientdata_t, vuser4[1] ),
	DELTA_DEF( clientdata_t, vuser4[2] ),
};

delta_definition_t g_WeaponDataDefinition[] =
{
	DELTA_DEF( weapon_data_t, m_iId ),
	DELTA_DEF( weapon_data_t, m_iClip ),
	DELTA_DEF( weapon_data_t, m_flNextPrimaryAttack ),
	DELTA_DEF( weapon_data_t, m_flNextSecondaryAttack ),
	DELTA_DEF( weapon_data_t, m_flTimeWeaponIdle ),
	DELTA_DEF( weapon_data_t, m_fInReload ),
	DELTA_DEF( weapon_data_t, m_fInSpecialReload ),
	DELTA_DEF( weapon_data_t, m_flNextReload ),
	DELTA_DEF( weapon_data_t, m_flPumpTime ),
	DELTA_DEF( weapon_data_t, m_fReloadTime ),
	DELTA_DEF( weapon_data_t, m_fAimedDamage ),
	DELTA_DEF( weapon_data_t, m_fNextAimBonus ),
	DELTA_DEF( weapon_data_t, m_fInZoom ),
	DELTA_DEF( weapon_data_t, m_iWeaponState ),
	DELTA_DEF( weapon_data_t, iuser1 ),
	DELTA_DEF( weapon_data_t, iuser2 ),
	DELTA_DEF( weapon_data_t, iuser3 ),
	DELTA_DEF( weapon_data_t, iuser4 ),
	DELTA_DEF( weapon_data_t, fuser1 ),
	DELTA_DEF( weapon_data_t, fuser2 ),
	DELTA_DEF( weapon_data_t, fuser3 ),
	DELTA_DEF( weapon_data_t, fuser4 ),
};

delta_definition_t g_UsercmdDataDefinition[] =
{
	DELTA_DEF( usercmd_t, lerp_msec ),
	DELTA_DEF( usercmd_t, msec ),
	DELTA_DEF( usercmd_t, lightlevel ),
	DELTA_DEF( usercmd_t, viewangles[0] ),
	DELTA_DEF( usercmd_t, viewangles[1] ),
	DELTA_DEF( usercmd_t, viewangles[2] ),
	DELTA_DEF( usercmd_t, buttons ),
	DELTA_DEF( usercmd_t, forwardmove ),
	DELTA_DEF( usercmd_t, sidemove ),
	DELTA_DEF( usercmd_t, upmove ),
	DELTA_DEF( usercmd_t, impulse ),
	DELTA_DEF( usercmd_t, weaponselect ),
	DELTA_DEF( usercmd_t, impact_index ),
	DELTA_DEF( usercmd_t, impact_position[0] ),
	DELTA_DEF( usercmd_t, impact_position[1] ),
	DELTA_DEF( usercmd_t, impact_position[2] ),
};

delta_definition_t g_EntityDataDefinition[] =
{
	DELTA_DEF( entity_state_t, startpos[0] ),
	DELTA_DEF( entity_state_t, startpos[1] ),
	DELTA_DEF( entity_state_t, startpos[2] ),
	DELTA_DEF( entity_state_t, endpos[0] ),
	DELTA_DEF( entity_state_t, endpos[1] ),
	DELTA_DEF( entity_state_t, endpos[2] ),
	DELTA_DEF( entity_state_t, impacttime ),
	DELTA_DEF( entity_state_t, starttime ),
	DELTA_DEF( entity_state_t, origin[0] ),
	DELTA_DEF( entity_state_t, origin[1] ),
	DELTA_DEF( entity_state_t, origin[2] ),
	DELTA_DEF( entity_state_t, angles[0] ),
	DELTA_DEF( entity_state_t, angles[1] ),
	DELTA_DEF( entity_state_t, angles[2] ),
	DELTA_DEF( entity_state_t, modelindex ),
	DELTA_DEF( entity_state_t, frame ),
	DELTA_DEF( entity_state_t, movetype ),
	DELTA_DEF( entity_state_t, colormap ),
	DELTA_DEF( entity_state_t, skin ),
	DELTA_DEF( entity_state_t, solid ),
	DELTA_DEF( entity_state_t, scale ),
	DELTA_DEF( entity_state_t, effects ),
	DELTA_DEF( entity_state_t, sequence ),
	DELTA_DEF( entity_state_t, animtime ),
	DELTA_DEF( entity_state_t, framerate ),
	DELTA_DEF( entity_state_t, controller[0] ),
	DELTA_DEF( entity_state_t, controller[1] ),
	DELTA_DEF( entity_state_t, controller[2] ),
	DELTA_DEF( entity_state_t, controller[3] ),
	DELTA_DEF( entity_state_t, blending[0] ),
	DELTA_DEF( entity_state_t, blending[1] ),
	DELTA_DEF( entity_state_t, body ),
	DELTA_DEF( entity_state_t, owner ),
	DELTA_DEF( entity_state_t, rendermode ),
	DELTA_DEF( entity_state_t, renderamt ),
	DELTA_DEF( entity_state_t, renderfx ),
	DELTA_DEF( entity_state_t, rendercolor.r ),
	DELTA_DEF( entity_state_t, rendercolor.g ),
	DELTA_DEF( entity_state_t, rendercolor.b ),
	DELTA_DEF( entity_state_t, weaponmodel ),
	DELTA_DEF( entity_state_t, gaitsequence ),
	DELTA_DEF( entity_state_t, mins[0] ),
	DELTA_DEF( entity_state_t, mins[1] ),
	DELTA_DEF( entity_state_t, mins[2] ),
	DELTA_DEF( entity_state_t, maxs[0] ),
	DELTA_DEF( entity_state_t, maxs[1] ),
	DELTA_DEF( entity_state_t, maxs[2] ),
	DELTA_DEF( entity_state_t, aiment ),
	DELTA_DEF( entity_state_t, basevelocity[0] ),
	DELTA_DEF( entity_state_t, basevelocity[1] ),
	DELTA_DEF( entity_state_t, basevelocity[2] ),
	DELTA_DEF( entity_state_t, friction ),
	DELTA_DEF( entity_state_t, gravity ),
	DELTA_DEF( entity_state_t, spectator ),
	DELTA_DEF( entity_state_t, velocity[0] ),
	DELTA_DEF( entity_state_t, velocity[1] ),
	DELTA_DEF( entity_state_t, velocity[2] ),
	DELTA_DEF( entity_state_t, team ),
	DELTA_DEF( entity_state_t, playerclass ),
	DELTA_DEF( entity_state_t, health ),
	DELTA_DEF( entity_state_t, usehull ),
	DELTA_DEF( entity_state_t, oldbuttons ),
	DELTA_DEF( entity_state_t, onground ),
	DELTA_DEF( entity_state_t, iStepLeft ),
	DELTA_DEF( entity_state_t, flFallVelocity ),
	DELTA_DEF( entity_state_t, weaponanim ),
	DELTA_DEF( entity_state_t, eflags ),
	DELTA_DEF( entity_state_t, iuser1 ),
	DELTA_DEF( entity_state_t, iuser2 ),
	DELTA_DEF( entity_state_t, iuser3 ),
	DELTA_DEF( entity_state_t, iuser4 ),
	DELTA_DEF( entity_state_t, fuser1 ),
	DELTA_DEF( entity_state_t, fuser2 ),
	DELTA_DEF( entity_state_t, fuser3 ),
	DELTA_DEF( entity_state_t, fuser4 ),
	DELTA_DEF( entity_state_t, vuser1[0] ),
	DELTA_DEF( entity_state_t, vuser1[1] ),
	DELTA_DEF( entity_state_t, vuser1[2] ),
	DELTA_DEF( entity_state_t, vuser2[0] ),
	DELTA_DEF( entity_state_t, vuser2[1] ),
	DELTA_DEF( entity_state_t, vuser2[2] ),
	DELTA_DEF( entity_state_t, vuser3[0] ),
	DELTA_DEF( entity_state_t, vuser3[1] ),
	DELTA_DEF( entity_state_t, vuser3[2] ),
	DELTA_DEF( entity_state_t, vuser4[0] ),
	DELTA_DEF( entity_state_t, vuser4[1] ),
	DELTA_DEF( entity_state_t, vuser4[2] ),
};

delta_definition_t g_EventDataDefinition[] =
{
	DELTA_DEF( event_args_t, entindex ),
	DELTA_DEF( event_args_t, origin[0] ),
	DELTA_DEF( event_args_t, origin[1] ),
	DELTA_DEF( event_args_t, origin[2] ),
	DELTA_DEF( event_args_t, angles[0] ),
	DELTA_DEF( event_args_t, angles[1] ),
	DELTA_DEF( event_args_t, angles[2] ),
	DELTA_DEF( event_args_t, fparam1 ),
	DELTA_DEF( event_args_t, fparam2 ),
	DELTA_DEF( event_args_t, iparam1 ),
	DELTA_DEF( event_args_t, iparam2 ),
	DELTA_DEF( event_args_t, bparam1 ),
	DELTA_DEF( event_args_t, bparam2 ),
	DELTA_DEF( event_args_t, ducking ),
};
#undef DELTA_DEF

void DELTA_Init()
{
	Cmd_AddCommand( "delta_stats", DELTA_DumpStats_f );
	Cmd_AddCommand( "delta_clear", DELTA_ClearStats_f );

	DELTA_AddDefinition( "clientdata_t", g_ClientDataDefinition,
		sizeof( g_ClientDataDefinition ) / sizeof( g_ClientDataDefinition[0] ) );
	DELTA_AddDefinition( "weapon_data_t", g_WeaponDataDefinition,
		sizeof( g_WeaponDataDefinition ) / sizeof( g_WeaponDataDefinition[0] ) );
	DELTA_AddDefinition( "usercmd_t", g_UsercmdDataDefinition,
		sizeof( g_UsercmdDataDefinition ) / sizeof( g_UsercmdDataDefinition[0] ) );
	DELTA_AddDefinition( "entity_state_t", g_EntityDataDefinition,
		sizeof( g_EntityDataDefinition ) / sizeof( g_EntityDataDefinition[0] ) );
	DELTA_AddDefinition( "entity_state_player_t", g_EntityDataDefinition,
		sizeof( g_EntityDataDefinition ) / sizeof( g_EntityDataDefinition[0] ) );
	DELTA_AddDefinition( "custom_entity_state_t", g_EntityDataDefinition,
		sizeof( g_EntityDataDefinition ) / sizeof( g_EntityDataDefinition[0] ) );
	DELTA_AddDefinition( "event_t", g_EventDataDefinition,
		sizeof( g_EventDataDefinition ) / sizeof( g_EventDataDefinition[0] ) );
}

void DELTA_RegisterDescription( char* name )
{
	delta_registry_t* p;

	p = (delta_registry_t*)Mem_ZeroMalloc( sizeof( delta_registry_t ) );
	p->next = g_deltaregistry;
	g_deltaregistry = p;
	p->name = Mem_Strdup( name );
	p->pdesc = NULL;
}

delta_t** DELTA_LookupRegistration( char* name )
{
	delta_registry_t* p;

	for ( p = g_deltaregistry; p; p = p->next )
	{
		if ( !Q_strcasecmp( p->name, name ) )
			return &p->pdesc;
	}

	return NULL;
}

void DELTA_ClearFlags( delta_t* pFields )
{
	int i;

	for ( i = 0; i < pFields->fieldCount; i++ )
		pFields->pdd[i].flags = 0;
}

int DELTA_CountSendFields( delta_t* pFields )
{
	int i;
	int c;
	delta_description_t* pTest;

	c = 0;
	for ( i = 0; i < pFields->fieldCount; i++ )
	{
		pTest = &pFields->pdd[i];
		if ( pTest->flags & FDT_MARK )
		{
			pTest->stats.sendcount++;
			c++;
		}
	}

	return c;
}

void DELTA_MarkSendFields( unsigned char* from, unsigned char* to, delta_t* pFields )
{
	ASSERT_AT( pFields && pFields->pdd && from && to, 640 );

	int i;
	int fieldType;
	int fieldCount;
	qboolean different;
	unsigned char *f, *t;
	char *st1, *st2;
	delta_description_t* pTest;

	fieldCount = pFields->fieldCount;
	for ( i = 0; i < fieldCount; i++ )
	{
		pTest = &pFields->pdd[i];
		fieldType = pTest->fieldType & ~DT_SIGNED;
		f = from + pTest->fieldOffset;
		t = to + pTest->fieldOffset;
		different = FALSE;

		switch ( fieldType )
		{
		case DT_BYTE:
			different = *f != *t;
			break;
		case DT_SHORT:
			different = *(unsigned short*)f != *(unsigned short*)t;
			break;
		case DT_FLOAT:
		case DT_INTEGER:
		case DT_ANGLE:
			different = *(unsigned int*)f != *(unsigned int*)t;
			break;
		case DT_TIMEWINDOW_8:
			different = (int)( *(float*)f * 100.0 ) != (int)( *(float*)t * 100.0 );
			break;
		case DT_TIMEWINDOW_BIG:
			different = (int)( *(float*)f * 1000.0 ) != (int)( *(float*)t * 1000.0 );
			break;
		case DT_STRING:
			for ( st1 = (char*)f, st2 = (char*)t; *st1 && *st2; st1++, st2++ )
			{
				if ( tolower( *st1 ) != tolower( *st2 ) )
					break;
			}
			if ( *st1 || *st2 )
				different = TRUE;
			break;
		default:
			Con_Printf( "Bad field type %i\n", fieldType );
			break;
		}

		if ( different )
			pTest->flags |= FDT_MARK;
	}

	if ( pFields->conditionalencode )
		pFields->conditionalencode( pFields, from, to );
}

int DELTA_CheckDelta( unsigned char* from, unsigned char* to, delta_t* pFields )
{
#ifdef _WIN32
	ASSERT_AT( pFields && from && to, 930 );
#endif

	DELTA_ClearFlags( pFields );
	DELTA_MarkSendFields( from, to, pFields );
	return DELTA_CountSendFields( pFields );
}

int DELTA_TestDelta( unsigned char* from, unsigned char* to, delta_t* pFields )
{
	ASSERT_AT( pFields && pFields->pdd && from && to, 490 );

	int i;
	int fieldType;
	int fieldCount;
	int length;
	int neededBits;
	int highestBit;
	qboolean different;
	qboolean stringDifferent;
	unsigned char *f, *t;
	char *st1, *st2;
	delta_description_t* pTest;

	neededBits = 0;
	highestBit = -1;
	fieldCount = pFields->fieldCount;

	for ( i = 0; i < fieldCount; i++ )
	{
		pTest = &pFields->pdd[i];
		fieldType = pTest->fieldType & ~DT_SIGNED;
		f = from + pTest->fieldOffset;
		t = to + pTest->fieldOffset;
		different = FALSE;

		switch ( fieldType )
		{
		case DT_BYTE:
			different = *f != *t;
			break;
		case DT_SHORT:
			different = *(unsigned short*)f != *(unsigned short*)t;
			break;
		case DT_FLOAT:
		case DT_INTEGER:
		case DT_ANGLE:
			different = *(unsigned int*)f != *(unsigned int*)t;
			break;
		case DT_TIMEWINDOW_8:
			different = (int)( *(float*)f * 100.0 ) != (int)( *(float*)t * 100.0 );
			break;
		case DT_TIMEWINDOW_BIG:
			different = (int)( *(float*)f * 1000.0 ) != (int)( *(float*)t * 1000.0 );
			break;
		case DT_STRING:
			length = 0;
			stringDifferent = FALSE;
			for ( st1 = (char*)f, st2 = (char*)t; *st2; st1++, st2++ )
			{
				if ( !stringDifferent && tolower( *st1 ) != tolower( *st2 ) )
				{
					pTest->flags |= FDT_MARK;
					stringDifferent = TRUE;
				}
				length += 8;
			}
			if ( stringDifferent )
				neededBits += length + 8;
			break;
		default:
			Con_Printf( "Bad field type %i\n", fieldType );
			break;
		}

		if ( different )
		{
			neededBits += pTest->significant_bits;
			highestBit = i;
		}
	}

	if ( highestBit != -1 )
		neededBits += ( highestBit & ~7 ) + 8;

	return neededBits;
}

void DELTA_SetSendFlagBits( delta_t* pFields, int* bits, int* bytecount )
{
	int i;
	int lastbit;

	Q_memset( bits, 0, 8 );

	lastbit = -1;
	for ( i = pFields->fieldCount - 1; i >= 0; i-- )
	{
		if ( pFields->pdd[i].flags & FDT_MARK )
		{
			bits[i > 31 ? 1 : 0] |= 1 << ( i & 31 );
			if ( lastbit == -1 )
				lastbit = i;
		}
	}

	*bytecount = ( lastbit >> 3 ) + 1;
}

void DELTA_WriteMarkedFields( unsigned char*, unsigned char* to, delta_t* pFields )
{
	int i;
	int fieldType;
	int fieldCount;
	int value;
	unsigned int uvalue;
	qboolean fieldSign;
	unsigned char* data;
	char* st;
	delta_description_t* pTest;

	fieldCount = pFields->fieldCount;
	for ( i = 0; i < fieldCount; i++ )
	{
		pTest = &pFields->pdd[i];
		if ( !( pTest->flags & FDT_MARK ) )
			continue;

		fieldType = pTest->fieldType & ~DT_SIGNED;
		fieldSign = ( pTest->fieldType & DT_SIGNED ) ? TRUE : FALSE;
		data = to + pTest->fieldOffset;

		switch ( fieldType )
		{
		case DT_BYTE:
			if ( fieldSign )
				MSG_WriteSBits( (signed char)(int)( *(signed char*)data * pTest->premultiply ), pTest->significant_bits );
			else
				MSG_WriteBits( (unsigned char)(int)( *data * pTest->premultiply ), pTest->significant_bits );
			break;
		case DT_SHORT:
			if ( fieldSign )
				MSG_WriteSBits( (short)(int)( *(short*)data * pTest->premultiply ), pTest->significant_bits );
			else
				MSG_WriteBits(
					(unsigned short)(int)( *(unsigned short*)data * pTest->premultiply ), pTest->significant_bits );
			break;
		case DT_FLOAT:
			value = (int)( *(float*)data * pTest->premultiply );
			if ( fieldSign )
				MSG_WriteSBits( value, pTest->significant_bits );
			else
				MSG_WriteBits( value, pTest->significant_bits );
			break;
		case DT_INTEGER:
			if ( fieldSign )
			{
				value = *(int*)data;
				if ( pTest->premultiply < 0.9999f || pTest->premultiply > 1.0001f )
					value = (int)( value * pTest->premultiply );
				MSG_WriteSBits( value, pTest->significant_bits );
			}
			else
			{
				uvalue = *(unsigned int*)data;
				if ( pTest->premultiply < 0.9999f || pTest->premultiply > 1.0001f )
					uvalue = (unsigned int)( uvalue * pTest->premultiply );
				MSG_WriteBits( uvalue, pTest->significant_bits );
			}
			break;
		case DT_ANGLE:
			MSG_WriteBitAngle( *(float*)data, pTest->significant_bits );
			break;
		case DT_TIMEWINDOW_8:
			value = (int)( *(float*)data * 100.0f );
			MSG_WriteSBits( (int)( sv.time * 100.0 ) - value, 8 );
			break;
		case DT_TIMEWINDOW_BIG:
			value = (int)( pTest->premultiply * *(float*)data );
			MSG_WriteSBits( (int)( pTest->premultiply * sv.time ) - value, pTest->significant_bits );
			break;
		case DT_STRING:
			for ( st = (char*)data; st && *st; st++ )
				MSG_WriteBits( *st, 8 );
			MSG_WriteBits( 0, 8 );
			break;
		default:
			Con_Printf( "unknown send field type\n" );
			break;
		}
	}
}

int DELTA_WriteDelta( unsigned char* from, unsigned char* to, qboolean force, delta_t* pFields, void ( *callback )() )
{
	int sendfields;

	DELTA_ClearFlags( pFields );
	DELTA_MarkSendFields( from, to, pFields );
	sendfields = DELTA_CountSendFields( pFields );
	return _DELTA_WriteDelta( from, to, force, pFields, callback, sendfields );
}

int _DELTA_WriteDelta(
	unsigned char* from, unsigned char* to, qboolean force, delta_t* pFields, void ( *callback )(), int sendfields )
{
	int i;
	int bytecount;
	int bits[2];

#ifdef _WIN32
	ASSERT_AT( pFields && from && to, 972 );
#endif

	if ( !sendfields && !force )
		return 1;

	DELTA_SetSendFlagBits( pFields, bits, &bytecount );

	if ( callback )
		callback();

	MSG_WriteBits( bytecount, 3 );
	for ( i = 0; i < bytecount; i++ )
		MSG_WriteBits( ( (byte*)bits )[i], 8 );

	DELTA_WriteMarkedFields( from, to, pFields );
	return 1;
}

int DELTA_ParseDelta( unsigned char* from, unsigned char* to, delta_t* pFields, int bufferSize )
{
	int i;
	int startbit;
	int bytecount;
	int bits[2];
	int fieldType;
	int fieldCount;
	int offset;
	int copied;
	int difference;
	int t;
	int si;
	unsigned int ui;
	short s;
	unsigned short us;
	signed char c;
	unsigned char uc;
	float f;
	qboolean fieldSign;
	qboolean ended;
	unsigned char *src, *dst;
	delta_description_t* pTest;

	startbit = MSG_CurrentBit();
	Q_memset( bits, 0, sizeof( bits ) );

	bytecount = MSG_ReadBits( 3 );
	for ( i = 0; i < bytecount; i++ )
		( (byte*)bits )[i] = (byte)MSG_ReadBits( 8 );

	fieldCount = pFields->fieldCount;
	for ( i = 0; i < fieldCount; i++ )
	{
		pTest = &pFields->pdd[i];
		offset = pTest->fieldOffset;
		if ( offset < 0 || offset >= bufferSize )
			continue;

		fieldType = pTest->fieldType & ~DT_SIGNED;
		dst = to + offset;

		if ( !( bits[i > 31 ? 1 : 0] & ( 1 << ( i & 31 ) ) ) )
		{
			src = from + offset;
			switch ( fieldType )
			{
			case DT_BYTE:
				*dst = *src;
				break;
			case DT_SHORT:
				if ( offset + (int)sizeof( short ) <= bufferSize )
					*(unsigned short*)dst = *(unsigned short*)src;
				break;
			case DT_FLOAT:
			case DT_INTEGER:
			case DT_ANGLE:
			case DT_TIMEWINDOW_8:
			case DT_TIMEWINDOW_BIG:
				if ( offset + (int)sizeof( int ) <= bufferSize )
					*(unsigned int*)dst = *(unsigned int*)src;
				break;
			case DT_STRING:
				copied = 0;
				ended = FALSE;
				while ( copied + offset < bufferSize )
				{
					*dst++ = *src++;
					copied++;
					if ( !*src )
					{
						ended = TRUE;
						break;
					}
				}
				if ( !ended && copied > 0 )
					dst[-1] = 0;
				break;
			default:
				Con_Printf( "unparseable field type %i\n", fieldType );
				break;
			}
			continue;
		}

		pTest->stats.receivedcount++;
		fieldSign = ( pTest->fieldType & DT_SIGNED ) ? TRUE : FALSE;

		switch ( fieldType )
		{
		case DT_BYTE:
			if ( fieldSign )
			{
				c = (signed char)MSG_ReadSBits( pTest->significant_bits );
				if ( pTest->premultiply < 0.9999f || pTest->premultiply > 1.0001f )
					c = (signed char)(int)( c / pTest->premultiply );
				if ( pTest->postmultiply < 0.9999f || pTest->postmultiply > 1.0001f )
					c = (signed char)(int)( c * pTest->postmultiply );
				*(signed char*)dst = c;
			}
			else
			{
				uc = (unsigned char)MSG_ReadBits( pTest->significant_bits );
				if ( pTest->premultiply < 0.9999f || pTest->premultiply > 1.0001f )
					uc = (unsigned char)(int)( uc / pTest->premultiply );
				if ( pTest->postmultiply < 0.9999f || pTest->postmultiply > 1.0001f )
					uc = (unsigned char)(int)( uc * pTest->postmultiply );
				*dst = uc;
			}
			break;
		case DT_SHORT:
			if ( offset + (int)sizeof( short ) > bufferSize )
				break;
			if ( fieldSign )
			{
				s = (short)MSG_ReadSBits( pTest->significant_bits );
				if ( pTest->premultiply < 0.9999f || pTest->premultiply > 1.0001f )
					s = (short)(int)( s / pTest->premultiply );
				if ( pTest->postmultiply < 0.9999f || pTest->postmultiply > 1.0001f )
					s = (short)(int)( s * pTest->postmultiply );
				*(short*)dst = s;
			}
			else
			{
				us = (unsigned short)MSG_ReadBits( pTest->significant_bits );
				if ( pTest->premultiply < 0.9999f || pTest->premultiply > 1.0001f )
					us = (unsigned short)(int)( us / pTest->premultiply );
				if ( pTest->postmultiply < 0.9999f || pTest->postmultiply > 1.0001f )
					us = (unsigned short)(int)( us * pTest->postmultiply );
				*(unsigned short*)dst = us;
			}
			break;
		case DT_FLOAT:
			if ( offset + (int)sizeof( float ) > bufferSize )
				break;
			if ( fieldSign )
				f = (float)MSG_ReadSBits( pTest->significant_bits );
			else
				f = (float)MSG_ReadBits( pTest->significant_bits );
			if ( pTest->premultiply < 0.9999f || pTest->premultiply > 1.0001f )
				f /= pTest->premultiply;
			if ( pTest->postmultiply < 0.9999f || pTest->postmultiply > 1.0001f )
				f *= pTest->postmultiply;
			*(float*)dst = f;
			break;
		case DT_INTEGER:
			if ( offset + (int)sizeof( int ) > bufferSize )
				break;
			if ( fieldSign )
			{
				ui = (unsigned int)MSG_ReadSBits( pTest->significant_bits );
				if ( pTest->premultiply < 0.9999f || pTest->premultiply > 1.0001f )
					ui = (unsigned int)( ui / pTest->premultiply );
				if ( pTest->postmultiply < 0.9999f || pTest->postmultiply > 1.0001f )
					ui = (unsigned int)( ui * pTest->postmultiply );
				*(unsigned int*)dst = ui;
			}
			else
			{
				si = (int)MSG_ReadBits( pTest->significant_bits );
				if ( pTest->premultiply < 0.9999f || pTest->premultiply > 1.0001f )
					si = (int)( si / pTest->premultiply );
				if ( pTest->postmultiply < 0.9999f || pTest->postmultiply > 1.0001f )
					si = (int)( si * pTest->postmultiply );
				*(int*)dst = si;
			}
			break;
		case DT_ANGLE:
			if ( offset + (int)sizeof( float ) <= bufferSize )
				*(float*)dst = MSG_ReadBitAngle( pTest->significant_bits );
			break;
		case DT_TIMEWINDOW_8:
			if ( offset + (int)sizeof( float ) <= bufferSize )
			{
				difference = MSG_ReadSBits( 8 );
				t = (int)( cl.mtime[0] * 100.0 );
				*(float*)dst = ( (float)t - (float)difference ) / 100.0f;
			}
			break;
		case DT_TIMEWINDOW_BIG:
			if ( offset + (int)sizeof( float ) <= bufferSize )
			{
				difference = MSG_ReadSBits( pTest->significant_bits );
				f = (float)( pTest->premultiply * cl.mtime[0] ) - (float)difference;
				if ( pTest->premultiply < 0.9999f || pTest->premultiply > 1.0001f )
					f /= pTest->premultiply;
				*(float*)dst = f;
			}
			break;
		case DT_STRING:
			copied = 0;
			ended = FALSE;
			while ( copied + offset < bufferSize )
			{
				uc = (unsigned char)MSG_ReadBits( 8 );
				*dst++ = uc;
				copied++;
				if ( !uc )
				{
					ended = TRUE;
					break;
				}
			}
			if ( !ended && copied > 0 )
				dst[-1] = 0;
			break;
		default:
			Con_Printf( "unparseable field type %i\n", fieldType );
			break;
		}
	}

	return MSG_CurrentBit() - startbit;
}
