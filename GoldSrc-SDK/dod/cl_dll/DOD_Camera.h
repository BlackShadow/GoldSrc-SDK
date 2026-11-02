#ifndef DOD_CAMERA_H
#define DOD_CAMERA_H

enum dod_cameratype_t
{
	DOD_CAMERA,
	DOD_TARGET
};

struct key_value_pair_s
{
	char szKeyName[32];
	char szValue[128];
};

struct dod_bspparse_ent
{
	char classname[32];
	key_value_pair_s kvPairs[128];
	int numpairs;
};

struct dod_camera_t
{
	dod_cameratype_t type;
	char classname[32];
	char target[32];
	char targetname[32];
	int overlay;
	int fov;
	int subtitle;
	int fade;
};

void DoD_SetKVPair( dod_bspparse_ent *pEnt, const char *key, const char *value );
char *ParseSingleEnt( char *buffer, dod_bspparse_ent *pEnt, int *error );
void Parse_dod_camera( dod_bspparse_ent *pEnt );
void Parse_path_corner( dod_bspparse_ent *pEnt );
void Parse_particle_shooter( dod_bspparse_ent *pEnt );
void Parse_info_doddetect( dod_bspparse_ent *pEnt );
void Parse_env_model( dod_bspparse_ent *pEnt );
void DoD_ProcessEnts( char *buffer );
char *DoD_LoadEntityLump( const char *filename );
void DoD_LoadClientEnts( const char *map );
dod_camera_t *DoD_FindTarget( const char *name );
void DoD_ReadCameraSettings( const char *targetname );
void DoD_ScreenFades( int flags, float holdtime, int r, int g, int b, int a, int speed );

#endif
