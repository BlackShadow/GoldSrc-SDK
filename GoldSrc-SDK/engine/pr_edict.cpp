#include "cvar.h"
#include "engine_edict.h"
#include "server.h"
#include "sys.h"
#include "gl_model.h"
#include "common.h"
#include "mem.h"
#include "zone.h"
#include "host.h"
#include "conprint.h"
#include "pr_dlls.h"
#include "eiface.h"

#include <cstdint>
#include <cstdlib>
#include <cstdio>

#if !defined( _WIN32 )
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#endif // _DEBUG

void ED_Free( edict_t* entity )
{
	if ( entity->free )
		return;
	SV_UnlinkEdict( entity );
	if ( entity->pvPrivateData )
	{
		if ( gNewDLLFunctions.pfnOnFreeEntPrivateData )
			gNewDLLFunctions.pfnOnFreeEntPrivateData( entity );
		Mem_Free( entity->pvPrivateData );
	}
	++entity->serialnumber;
	entity->pvPrivateData = nullptr;
	entity->free = true;
	entity->v.flags = 0;
	entity->v.colormap = 0;
	entity->v.takedamage = 0.0f;
	entity->v.modelindex = 0;
	entity->v.skin = 0;
	entity->v.frame = 0.0f;
	entity->v.scale = 0.0f;
	entity->v.gravity = 0.0f;
	entity->v.model = 0;
	for ( int axis = 0; axis < 3; ++axis )
		entity->v.origin[axis] = vec3_origin[axis];
	for ( int axis = 0; axis < 3; ++axis )
		entity->v.angles[axis] = vec3_origin[axis];
	entity->v.nextthink = -1.0f;
	entity->v.solid = 0;
	entity->freetime = static_cast<float>( sv.time );
}

void FreeEntPrivateData( edict_t* entity )
{
	if ( entity->pvPrivateData )
	{
		if ( gNewDLLFunctions.pfnOnFreeEntPrivateData )
			gNewDLLFunctions.pfnOnFreeEntPrivateData( entity );
		Mem_Free( entity->pvPrivateData );
	}
	entity->pvPrivateData = nullptr;
}

void* PvAllocEntPrivateData( edict_t* entity, std::int32_t size )
{
	FreeEntPrivateData( entity );
	if ( size <= 0 )
		return nullptr;
	entity->pvPrivateData = Mem_Calloc( 1, size );
	return entity->pvPrivateData;
}

edict_t* PEntityOfEntOffset( int offset )
{
	return reinterpret_cast<edict_t*>( reinterpret_cast<char*>( sv.edicts ) + offset );
}

int EntOffsetOfPEntity( const edict_t* entity )
{
	return static_cast<int>( reinterpret_cast<const char*>( entity ) - reinterpret_cast<const char*>( sv.edicts ) );
}

int IndexOfEdict( const edict_t* entity )
{
	if ( !entity )
		return 0;
	int offset = EntOffsetOfPEntity( entity );
	int index = offset / static_cast<int>( sizeof( edict_t ) );
	if ( index < 0 || index > sv.max_edicts )
		Sys_Error( "Bad entity in IndexOfEdict()" );
	return index;
}

edict_t* PEntityOfEntIndex( int index )
{
	if ( index < 0 || index >= sv.max_edicts )
		return nullptr;
	edict_t* entity = &sv.edicts[index];
	if ( !entity || entity->free || !entity->pvPrivateData )
	{
		if ( index >= svs.maxclients || entity->free )
			return nullptr;
	}
	return entity;
}

edict_t* PEntityOfEntIndexAllEntities( int index )
{
	int clientLimit = svs.maxclients + 1;
	if ( index < 0 || index >= sv.max_edicts )
		return nullptr;
	edict_t* entity = &sv.edicts[index];
	if ( !entity || entity->free || !entity->pvPrivateData )
	{
		if ( index >= clientLimit || entity->free )
			return nullptr;
	}
	return entity;
}

const char* SzFromIndex( int index )
{
	return pr_strings + index;
}

edict_t* FindEntityByVars( entvars_t* variables )
{
	for ( int index = 0; index < sv.num_edicts; ++index )
	{
		edict_t* entity = &sv.edicts[index];
		if ( &entity->v == variables )
			return entity;
	}
	return nullptr;
}

char* ED_NewString( const char* text )
{
	int length = Q_strlen( text ) + 1;
	char* string = static_cast<char*>( Hunk_Alloc( length ) );
	char* output = string;
	for ( int index = 0; index < length; ++index )
	{
		if ( text[index] == '\\' && index < length - 1 )
		{
			++index;
			*output++ = text[index] == 'n' ? '\n' : '\\';
		}
		else
			*output++ = text[index];
	}
	return string;
}

int AllocEngineString( const char* value )
{
	return static_cast<int>( ED_NewString( value ) - pr_strings );
}

void* GetModelPtr( edict_t* entity )
{
	if ( !entity )
		return nullptr;
	return Mod_Extradata( sv.models[entity->v.modelindex] );
}

float CVarGetFloat( const char* name )
{
	return Cvar_VariableValue( name );
}

const char* CVarGetString( const char* name )
{
	return Cvar_VariableString( name );
}

cvar_t* CVarGetPointer( const char* name )
{
	return Cvar_FindVar( name );
}

void CVarSetFloat( const char* name, float value )
{
	Cvar_SetValue( const_cast<char*>( name ), value );
}

void CVarSetString( const char* name, const char* value )
{
	Cvar_Set( name, value );
}

void CVarRegister( cvar_t* variable )
{
	if ( variable )
	{
		variable->flags |= FCVAR_EXTDLL;
		Cvar_RegisterVariable( variable );
	}
}

void* PvEntPrivateData( edict_t* entity )
{
	return entity ? entity->pvPrivateData : nullptr;
}

entvars_t* GetVarsOfEnt( edict_t* entity )
{
	return &entity->v;
}

int NUM_FOR_EDICT( const edict_t* entity )
{
	std::uintptr_t offset = reinterpret_cast<std::uintptr_t>( entity ) - reinterpret_cast<std::uintptr_t>( sv.edicts );
	int index = static_cast<int>( offset / sizeof( edict_t ) );
	if ( index >= sv.num_edicts )
		Sys_Error( "NUM_FOR_EDICT: bad pointer" );
	return index;
}

static qboolean SuckOutClassname( const char* data, edict_t* entity )
{
	char keyname[256];
	KeyValueData key = { nullptr, keyname, const_cast<char*>( com_empty_string ), 0 };
	const char* next = COM_Parse( data );
	while ( next && com_token[0] != '}' )
	{
		Q_strncpy( keyname, com_token, sizeof( keyname ) - 1 );
		keyname[sizeof( keyname ) - 1] = 0;
		next = COM_Parse( next );
		if ( !next || Q_strcmp( keyname, "classname" ) == 0 )
		{
			key.szValue = com_token;
			gEntityInterface.pfnKeyValue( entity, &key );
			if ( !key.fHandled )
				Host_Error( "SuckOutClassname: parse error" );
			return true;
		}
		next = COM_Parse( next );
	}
	return false;
}

char* ED_ParseEdict( char* data, edict_t* entity )
{
	if ( entity != sv.edicts ) // hack
		Q_memset( &entity->v, 0, sizeof( entity->v ) );
	entity->v.pContainingEntity = entity;
	if ( !SuckOutClassname( data, entity ) )
	{
		++entity->serialnumber;
		entity->free = true;
		return data;
	}

	qboolean initialized = false;
	ENTITYINIT initialize = GetEntityInit( pr_strings + entity->v.classname );
	if ( initialize )
	{
		initialize( &entity->v );
		initialized = true;
	}
	else
	{
		initialize = GetEntityInit( "custom" );
		if ( initialize )
		{
			initialize( &entity->v );
			KeyValueData key = { const_cast<char*>( "custom" ), const_cast<char*>( "customclass" ),
				pr_strings + entity->v.classname, 0 };
			gEntityInterface.pfnKeyValue( entity, &key );
			initialized = true;
		}
		else
		{
			Con_DPrintf( "Can't init %s\n", pr_strings + entity->v.classname );
		}
	}

	char keyname[256];
	const char* cursor = COM_Parse( data );
	while ( com_token[0] != '}' )
	{
		if ( !cursor )
			Sys_Error( "ED_ParseEntity: EOF without closing brace" );
		Q_strncpy( keyname, com_token, sizeof( keyname ) - 1 );
		keyname[sizeof( keyname ) - 1] = 0;
		int length = Q_strlen( keyname );
		while ( length && keyname[length - 1] == ' ' )
			keyname[--length] = 0;

		cursor = COM_Parse( cursor );
		if ( !cursor )
			Host_Error( "ED_ParseEntity: EOF without closing brace" );
		if ( com_token[0] == '}' )
			Host_Error( "ED_ParseEntity: closing brace without data" );

		char* classname = pr_strings + entity->v.classname;
		if ( !classname || Q_strcmp( classname, com_token ) != 0 )
		{
			if ( Q_strcmp( keyname, "angle" ) == 0 )
			{
				float angle = Q_atof( com_token );
				if ( !( angle >= 0.0f ) )
				{
					if ( static_cast<int>( angle ) == -1 )
						_snprintf( com_token, sizeof( com_token ), "-90 0 0" );
					else
						_snprintf( com_token, sizeof( com_token ), "90 0 0" );
				}
				else
				{
					_snprintf( 
						com_token, sizeof( com_token ), "%f %f %f", entity->v.angles[0], angle, entity->v.angles[2] );
				}
				Q_strcpy( keyname, "angles" );
			}
			KeyValueData key = { pr_strings + entity->v.classname, keyname, com_token, 0 };
			gEntityInterface.pfnKeyValue( entity, &key );
		}
		cursor = COM_Parse( cursor );
	}
	if ( !initialized )
	{
		++entity->serialnumber;
		entity->free = true;
	}
	return const_cast<char*>( cursor );
}

void InitEntityDLLFields( edict_t* entity )
{
	entity->v.pContainingEntity = entity;
}

void ReleaseEntityDLLFields( edict_t* entity )
{
	FreeEntPrivateData( entity );
}

void FreeAllEntPrivateData()
{
	for ( int index = 0; index < sv.num_edicts; ++index )
	{
		FreeEntPrivateData( &sv.edicts[index] );
	}
}

void ED_ClearEdict( edict_t* entity )
{
	Q_memset( &entity->v, 0, sizeof( entity->v ) );
	entity->free = false;
	FreeEntPrivateData( entity );
	InitEntityDLLFields( entity );
}

edict_t* ED_Alloc()
{
	constexpr float ED_INITIAL_REUSE_WINDOW = 2.0f;
	constexpr double ED_REUSE_DELAY = 0.5;
	int index = svs.maxclients + 1;
	int allocatedCount = sv.num_edicts;
	edict_t* edicts = sv.edicts;
	if ( index < allocatedCount )
	{
		double serverTime = sv.time;
		edict_t* entity = &edicts[index];
		for ( ; index < allocatedCount; ++index, ++entity )
		{
			if ( entity->free
				 && ( entity->freetime < ED_INITIAL_REUSE_WINDOW || serverTime - entity->freetime > ED_REUSE_DELAY ) )
			{
				ED_ClearEdict( entity );
				return entity;
			}
		}
	}

	if ( index >= sv.max_edicts )
	{
		if ( !sv.max_edicts )
			Sys_Error( "ED_Alloc: No edicts yet" );
		Sys_Error( "ED_Alloc: no free edicts" );
	}

	++sv.num_edicts;
	edict_t* entity = &edicts[index];
	ED_ClearEdict( entity );
	return entity;
}

void SaveSpawnParms( edict_t* entity )
{
	int index = NUM_FOR_EDICT( entity );
	if ( index < 1 || index > svs.maxclients )
		Host_Error( "Entity is not a client" );
}

edict_t* EDICT_NUM( int index )
{
	if ( index < 0 || index >= sv.max_edicts )
		Sys_Error( "EDICT_NUM: bad number %i", index );
	return &sv.edicts[index];
}

extern cvar_t deathmatch;

void ED_LoadFromFile( char* data )
{
	constexpr int SPAWNFLAG_NOT_IN_DEATHMATCH = 0x800;
	int inhibited = 0;
	edict_t* entity = nullptr;
	gGlobalVariables.time = static_cast<float>( sv.time );
	for ( const char* cursor = COM_Parse( data ); cursor; cursor = COM_Parse( data ) )
	{
		if ( com_token[0] != '{' )
			Host_Error( "ED_LoadFromFile: found %s when expecting {", com_token );
		if ( entity )
		{
			entity = ED_Alloc();
		}
		else
		{
			entity = sv.edicts;
			FreeEntPrivateData( entity );
			InitEntityDLLFields( entity );
		}
		data = ED_ParseEdict( const_cast<char*>( cursor ), entity );
		if ( entity->free )
			continue;
		if ( deathmatch.value != 0.0f && ( entity->v.spawnflags & SPAWNFLAG_NOT_IN_DEATHMATCH ) )
		{
			ED_Free( entity );
			++inhibited;
			continue;
		}
//
// immediately call spawn function
//
		if ( !entity->v.classname )
		{
			Con_Printf( "No classname for:\n" );
			ED_Free( entity );
			continue;
		}
		if ( gEntityInterface.pfnSpawn( entity ) < 0 || ( entity->v.flags & FL_KILLME ) )
			ED_Free( entity );
	}
	Con_DPrintf( "%i entities inhibited\n", inhibited );
}

void ED_Count()
{
	int active = 0;
	int models = 0;
	int solid = 0;
	int step = 0;

	if ( sv.num_edicts > 0 )
	{
		edict_t* entity = sv.edicts;
		edict_t* end = entity + sv.num_edicts;
		do
		{
			if ( entity->free == 0 )
			{
				++active;
				if ( entity->v.solid >= 1 )
					++solid;
				if ( entity->v.model >= 1 )
					++models;
				if ( entity->v.movetype == MOVETYPE_STEP )
					++step;
			}
			++entity;
		} while ( entity != end );
	}

	Con_Printf( "num_edicts:%3i\n", sv.num_edicts );
	Con_Printf( "active    :%3i\n", active );
	Con_Printf( "view      :%3i\n", models );
	Con_Printf( "touch     :%3i\n", solid );
	Con_Printf( "step      :%3i\n", step );
}

void PR_Init()
{
}
