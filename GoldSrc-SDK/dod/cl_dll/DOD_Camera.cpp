#include "hud.h"
#include "cl_util.h"
#include "DOD_Camera.h"
#include "tri.h"
#include "screenfade.h"
#include "vgui/idodviewport.h"
#include "../../utils/common/bspfile.h"
#include "../public/utlvector.h"

static dod_camera_t s_cameras[256];
static int s_num_cameras;

void DoD_SetKVPair( dod_bspparse_ent *pEnt, const char *key, const char *value )
{
	if ( !strcmp( key, "classname" ) )
	{
		strcpy( pEnt->classname, value );
	}
	else if ( pEnt->numpairs < 128 )
	{
		strncpy( pEnt->kvPairs[pEnt->numpairs].szKeyName, key, 32 );
		strncpy( pEnt->kvPairs[pEnt->numpairs].szValue, value, 128 );
		pEnt->numpairs++;
	}
}

char *ParseSingleEnt( char *buffer, dod_bspparse_ent *pEnt, int *error )
{
	char token[1024];
	char key[256];
	memset( pEnt, 0, sizeof( *pEnt ) );
	while ( 1 )
	{
		buffer = gEngfuncs.COM_ParseFile( buffer, token );
		if ( token[0] == '}' )
			return buffer;
		if ( !buffer )
		{
			*error = 1;
			return buffer;
		}
		strcpy( key, token );
		int length = strlen( key );
		while ( length && key[length - 1] == ' ' )
			key[--length] = 0;
		buffer = gEngfuncs.COM_ParseFile( buffer, token );
		if ( !buffer || token[0] == '}' )
		{
			*error = 1;
			return buffer;
		}
		DoD_SetKVPair( pEnt, key, token );
	}
}

void Parse_dod_camera( dod_bspparse_ent *pEnt )
{
	if ( s_num_cameras >= 256 )
		return;
	dod_camera_t *pCamera = &s_cameras[s_num_cameras++];
	pCamera->type = DOD_CAMERA;
	strcpy( pCamera->classname, pEnt->classname );
	for ( int i = 0; i < pEnt->numpairs; i++ )
	{
		const char *key = pEnt->kvPairs[i].szKeyName;
		const char *value = pEnt->kvPairs[i].szValue;
		if ( !stricmp( key, "target" ) )
			strcpy( pCamera->target, value );
		else if ( !stricmp( key, "targetname" ) )
			strcpy( pCamera->targetname, value );
		else if ( !stricmp( key, "cam_overlay" ) )
			pCamera->overlay = atoi( value );
		else if ( !stricmp( key, "fov" ) )
			pCamera->fov = atoi( value );
		else if ( !stricmp( key, "subtitle" ) )
			pCamera->subtitle = atoi( value );
		else if ( !stricmp( key, "cam_fade" ) )
			pCamera->fade = atoi( value );
	}
}

void Parse_path_corner( dod_bspparse_ent *pEnt )
{
	if ( s_num_cameras >= 256 )
		return;
	dod_camera_t *pCamera = &s_cameras[s_num_cameras++];
	pCamera->type = DOD_TARGET;
	strcpy( pCamera->classname, pEnt->classname );
	for ( int i = 0; i < pEnt->numpairs; i++ )
	{
		const char *key = pEnt->kvPairs[i].szKeyName;
		const char *value = pEnt->kvPairs[i].szValue;
		if ( !stricmp( key, "target" ) )
			strcpy( pCamera->target, value );
		else if ( !stricmp( key, "targetname" ) )
			strcpy( pCamera->targetname, value );
		else if ( !stricmp( key, "cam_overlay" ) )
			pCamera->overlay = atoi( value );
		else if ( !stricmp( key, "fov" ) )
			pCamera->fov = atoi( value );
		else if ( !stricmp( key, "subtitle" ) )
			pCamera->subtitle = atoi( value );
		else if ( !stricmp( key, "cam_fade" ) )
			pCamera->fade = atoi( value );
	}
}

void Parse_env_model( dod_bspparse_ent *pEnt )
{
	env_model_t model;
	memset( &model, 0, sizeof( model ) );
	for ( int i = 0; i < pEnt->numpairs; i++ )
	{
		const char *key = pEnt->kvPairs[i].szKeyName;
		const char *value = pEnt->kvPairs[i].szValue;
		if ( !stricmp( key, "targetname" ) )
			return;
		if ( !stricmp( key, "spawnflags" ) )
		{
			model.spawnflags |= atoi( value );
			if ( model.spawnflags & 1 )
				return;
		}
		else if ( !stricmp( key, "model" ) )
			strncpy( model.szModel, value, sizeof( model.szModel ) );
		else if ( !stricmp( key, "body" ) )
			model.iBody = atoi( value );
		else if ( !stricmp( key, "origin" ) )
			(void)sscanf( value, "%f %f %f\n", &model.vecOrigin.x, &model.vecOrigin.y, &model.vecOrigin.z );
		else if ( !stricmp( key, "angles" ) )
			(void)sscanf( value, "%f %f %f\n", &model.vecAngles.x, &model.vecAngles.y, &model.vecAngles.z );
		else if ( !stricmp( key, "m_iAction_On" ) )
		{
			if ( atoi( value ) > 0 )
				model.spawnflags |= 0x10;
		}
		else if ( !stricmp( key, "m_iszSequence_On" ) )
			strncpy( model.szSequence, value, sizeof( model.szSequence ) );
	}
	gHUD.m_CEnvModel.AddEnvModel( &model );
}

void Parse_particle_shooter( dod_bspparse_ent *pEnt )
{
	particle_shooter_t shooter;
	memset( &shooter, 0, sizeof( shooter ) );
	for ( int i = 0; i < pEnt->numpairs; i++ )
	{
		const char *key = pEnt->kvPairs[i].szKeyName;
		const char *value = pEnt->kvPairs[i].szValue;
		if ( !stricmp( key, "group_id" ) )
			shooter.id = atoi( value );
		else if ( !stricmp( key, "num_particles" ) )
			shooter.iNumParticles = atoi( value );
		else if ( !stricmp( key, "fire_delay" ) )
			shooter.fFireDelay = atof( value );
		else if ( !stricmp( key, "particle_life" ) )
			shooter.fParticleLife = atof( value );
		else if ( !stricmp( key, "spawnflags" ) )
			shooter.iFlags = atoi( value );
		else if ( !stricmp( key, "origin" ) )
			(void)sscanf( value, "%f %f %f", &shooter.vOrigin.x, &shooter.vOrigin.y, &shooter.vOrigin.z );
		else if ( !stricmp( key, "model" ) )
			strncpy( shooter.szSprite, value, sizeof( shooter.szSprite ) );
		else if ( !stricmp( key, "colour_red" ) )
			shooter.iColour[0] = atoi( value );
		else if ( !stricmp( key, "colour_green" ) )
			shooter.iColour[1] = atoi( value );
		else if ( !stricmp( key, "colour_blue" ) )
			shooter.iColour[2] = atoi( value );
		else if ( !stricmp( key, "size" ) )
			shooter.fSize = atof( value );
		else if ( !stricmp( key, "gravity" ) )
			shooter.fGravity = atof( value );
		else if ( !stricmp( key, "fade_speed" ) )
			shooter.fFadeSpeed = atof( value );
		else if ( !stricmp( key, "spin_speed" ) )
			shooter.iSpinDegPerSec = atoi( value );
		else if ( !stricmp( key, "damping_time" ) || !stricmp( key, "damping_vel" ) )
			shooter.fDampingTime = atof( value );
		else if ( !stricmp( key, "vel_variance" ) )
			shooter.fVariance = atof( value );
		else if ( !stricmp( key, "brightness" ) )
			shooter.fBrightness = atof( value );
		else if ( !stricmp( key, "scale_speed" ) )
			shooter.fScaleSpeed = atof( value );
		else if ( !stricmp( key, "framerate" ) )
			shooter.iFramerate = (int)atof( value );
		else if ( !stricmp( key, "rendermode" ) )
			shooter.iRenderMode = atoi( value );
	}
	if ( shooter.fFireDelay < 0.1 )
		shooter.fFireDelay = 0.1;
	gHUD.m_PShooter.AddParticleSystem( &shooter );
}

void Parse_info_doddetect( dod_bspparse_ent *pEnt )
{
	Vector vWind;
	vWind.x = vWind.y = 0;
	for ( int i = 0; i < pEnt->numpairs; i++ )
	{
		const char *key = pEnt->kvPairs[i].szKeyName;
		const char *value = pEnt->kvPairs[i].szValue;
		if ( !stricmp( key, "detect_wind_velocity_x" ) )
			vWind.x = atof( value );
		else if ( !stricmp( key, "detect_wind_velocity_y" ) )
			vWind.y = atof( value );
		else if ( !stricmp( key, "detect_weather_type" ) )
			g_iWeatherType = atoi( value );
		else if ( !stricmp( key, "detect_allies_country" ) )
			gHUD.m_bBritish = atoi( value ) == 1;
	}
	CDoDParticle::SetGlobalWind( vWind );
}

void DoD_ProcessEnts( char *buffer )
{
	char token[1024];
	CUtlVector<dod_bspparse_ent> parsedEntities;
	parsedEntities.AddToTail();
	dod_bspparse_ent& ent = parsedEntities[0];
	s_num_cameras = 0;
	gHUD.m_CEnvModel.RemoveAllModels();
	while ( 1 )
	{
		buffer = gEngfuncs.COM_ParseFile( buffer, token );
		if ( !buffer )
			return;
		if ( token[0] != '{' )
		{
			gEngfuncs.Con_Printf( "DoD_ProcessEnts: found %s when expecting {\n", token );
			return;
		}
		int error = 0;
		buffer = ParseSingleEnt( buffer, &ent, &error );
		if ( error )
		{
			gEngfuncs.Con_Printf( "DoD_ProcessEnts: error parsing entities\n" );
			return;
		}
		if ( !stricmp( ent.classname, "dod_camera" ) )
			Parse_dod_camera( &ent );
		else if ( !stricmp( ent.classname, "path_corner" ) )
			Parse_path_corner( &ent );
		else if ( !stricmp( ent.classname, "particle_shooter" ) )
			Parse_particle_shooter( &ent );
		else if ( !stricmp( ent.classname, "info_doddetect" ) )
			Parse_info_doddetect( &ent );
		else if ( !stricmp( ent.classname, "env_model" ) )
			Parse_env_model( &ent );
	}
}

char *DoD_LoadEntityLump( const char *filename )
{
	byte *file = gEngfuncs.COM_LoadFile( (char *)filename, 5, NULL );
	if ( !file )
		return NULL;
	dheader_t *header = (dheader_t *)file;
	if ( header->version != 29 && header->version != 30 )
	{
		gEngfuncs.Con_Printf( "DoD_LoadEntityLump:  Map [%s] has incorrect BSP version (%i should be %i).\n", filename, header->version, 30 );
		return NULL;
	}
	lump_t *lump = &header->lumps[LUMP_ENTITIES];
	char *buffer = (char *)malloc( lump->filelen + 1 );
	if ( !buffer )
	{
		gEngfuncs.Con_Printf( "DoD_LoadEntityLump:  Couldn't allocate %i bytes\n", lump->filelen + 1 );
		return NULL;
	}
	memcpy( buffer, file + lump->fileofs, lump->filelen );
	buffer[lump->filelen] = 0;
	return buffer;
}

void DoD_LoadClientEnts( const char *map )
{
	char filename[260];
	strcpy( filename, map );
	char *buffer = DoD_LoadEntityLump( filename );
	if ( buffer )
	{
		DoD_ProcessEnts( buffer );
		free( buffer );
	}
	else
	{
		gEngfuncs.Con_DPrintf( "ERROR - Couldn't load bsp to parse client ents - %s - Check working folder is set properly\n", filename );
	}
}

dod_camera_t *DoD_FindTarget( const char *name )
{
	for ( int i = 0; i < 256; i++ )
	{
		if ( !stricmp( s_cameras[i].targetname, name ) )
			return &s_cameras[i];
	}
	return NULL;
}

void DoD_ScreenFades( int flags, float holdtime, int r, int g, int b, int a, int speed )
{
	screenfade_t sf;
	gEngfuncs.pfnGetScreenFade( &sf );
	sf.fader = r;
	sf.fadeg = g;
	sf.fadeb = b;
	sf.fadealpha = a;
	sf.fadeSpeed = speed;
	sf.fadeFlags = flags;
	sf.fadeReset = gEngfuncs.GetClientTime() + holdtime;
	sf.fadeEnd = sf.fadeTotalEnd = sf.fadeReset;
	gEngfuncs.pfnSetScreenFade( &sf );
}

void DoD_ReadCameraSettings( const char *targetname )
{
	dod_camera_t *camera = DoD_FindTarget( targetname );
	if ( !camera )
		return;
	switch ( camera->overlay )
	{
	case 1:
		gHUD.m_Scope.SetScope( 9 );
		gHUD.m_Scope.m_iFlags |= HUD_ACTIVE;
		gDoDViewPortInterface->setWidescreenVisible( false );
		break;
	case 2:
		gHUD.m_Scope.SetScope( 33 );
		gHUD.m_Scope.m_iFlags |= HUD_ACTIVE;
		gDoDViewPortInterface->setWidescreenVisible( false );
		break;
	case 3:
		gHUD.m_Scope.SetScope( 0 );
		gDoDViewPortInterface->setWidescreenVisible( true );
		break;
	default:
		gHUD.m_Scope.SetScope( 0 );
		gDoDViewPortInterface->setWidescreenVisible( false );
		break;
	}
	gHUD.m_iFOV = camera->fov ? camera->fov : 90;
	g_lastFOV = camera->fov;
	if ( camera->subtitle )
	{
		gDoDViewPortInterface->setWidescreenVisible( true );
		gDoDViewPortInterface->setWidescreenSubtitle( camera->subtitle );
	}
}
