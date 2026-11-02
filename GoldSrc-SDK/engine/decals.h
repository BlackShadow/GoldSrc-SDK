#pragma once

#include "draw.h"
#include "model.h"
#include "cvar.h"

#define MAX_BASE_DECALS 512
#define DECAL_NAME_BYTES 16

typedef struct decal_s decal_t;

typedef struct
{
	char* name;
	short entityIndex;
	byte depth;
	byte flags;
	vec3_t position;
} DECALLIST;

extern cvar_t r_decals;
extern cvar_t mp_decals;
extern cvar_t sp_decals;

extern char decal_names[MAX_BASE_DECALS][DECAL_NAME_BYTES];
extern qboolean m_bDrawInitialized;
extern cachewad_t* menu_wad;

void			Decal_Init();
void			Decal_MergeInDecals( cachewad_t* wad, const char* pathID );
void			Draw_Shutdown();
void			Draw_DecalShutdown();
void			Draw_DecalSetName( int decal, char* name );
int				Draw_DecalIndex( int id );
int				Draw_DecalIndexFromName( char* name );
int				Draw_DecalCount();
int				Draw_DecalSize( int number );
const char*		Draw_DecalName( int number );
texture_t*		Draw_DecalTexture( int index );
int				R_DecalUnProject( decal_t* decal, vec_t* position );

void			R_DecalShoot( int textureIndex, int entity, int modelIndex, vec_t* position, int flags );
int				DecalListCreate( DECALLIST* list );
