#pragma once

#include "server.h"
#include "cmd.h"
#include "crc.h"
#include "delta.h"
#include "engine_edict.h"
#include "eiface.h"

extern enginefuncs_t g_engfuncsExportedToDlls;

int					PF_precache_model_I( char* name );
int					PF_precache_sound_I( char* name );
void				PF_setmodel_I( edict_t* entity, const char* name );
int					PF_modelindex( const char* name );
int					ModelFrames( int modelIndex );
void				PF_setsize_I( edict_t* entity, const float* mins, const float* maxs );
void				PF_changelevel_I( char* levelName, char* landmark );
void				PF_setspawnparms_I( edict_t* entity );
void				SaveSpawnParms( edict_t* entity );

float				PF_vectoyaw_I( const float* vector );
void				PF_vectoangles_I( const float* vector, float* angles );
void				SV_MoveToOrigin_I( edict_t* entity, const float* goal, float distance, int moveType );
void				PF_changeyaw_I( edict_t* entity );
void				PF_changepitch_I( edict_t* entity );

edict_t*			FindEntityByString( edict_t* start, const char* field, const char* value );
int					GetEntityIllum( edict_t* entity );
edict_t*			FindEntityInSphere( edict_t* start, const float* origin, float radius );
edict_t*			PF_checkclient_I( edict_t* entity );
edict_t*			PVSFindEntities( edict_t* player );
void				PF_makevectors_I( const float* angles );
edict_t*			PF_Spawn_I();
void				PF_Remove_I( edict_t* entity );
edict_t*			CreateNamedEntity( int classname );
void				PF_makestatic_I( edict_t* entity );
int					PF_checkbottom_I( edict_t* entity );
int					PF_droptofloor_I( edict_t* entity );
int					PF_walkmove_I( edict_t* entity, float yaw, float distance, int mode );
void				PF_setorigin_I( edict_t* entity, const float* origin );

void				PF_sound_I( edict_t* entity, int channel, const char* sample, float volume, float attenuation, int flags,
						int pitch );
void				PF_ambientsound_I( edict_t* entity, float* origin, const char* sample, float volume, float attenuation,
						int flags, int pitch );

void				PF_traceline_I( const float* start, const float* end, int noMonsters, edict_t* skip, TraceResult* result );
void				PF_TraceToss_DLL( edict_t* entity, edict_t* ignore, TraceResult* result );
int					TraceMonsterHull( edict_t* entity, const float* start, const float* end, int noMonsters, edict_t* skip,
						TraceResult* result );
void				PF_tracehull_I( const float* start, const float* end, int noMonsters, int hull, edict_t* skip,
						TraceResult* result );
void				TraceModel( const float* start, const float* end, int hull, edict_t* entity, TraceResult* result );
const char*			TraceTexture( edict_t* entity, const float* start, const float* end );
void				TraceSphere( const float* start, const float* end, int noMonsters, float radius, edict_t* skip,
						TraceResult* result );
void				PF_aim_I( edict_t* entity, float speed, float* direction );

void				PF_localcmd_I( char* command );
void				PF_localexec_I();
void				PF_stuffcmd_I( edict_t* entity, char* format, ... );
void				PF_particle_I( const float* origin, const float* direction, float color, float count );
void				PF_lightstyle_I( int style, char* value );
int					PF_DecalIndex( const char* name );
int					PF_pointcontents_I( const float* point );

void				PF_MessageBegin_I( int destination, int type, const float* origin, edict_t* entity );
void				PF_MessageEnd_I();
void				PF_WriteByte_I( int value );
void				PF_WriteChar_I( int value );
void				PF_WriteShort_I( int value );
void				PF_WriteLong_I( int value );
void				PF_WriteAngle_I( float value );
void				PF_WriteCoord_I( float value );
void				PF_WriteString_I( const char* text );
void				PF_WriteEntity_I( int value );

void				CVarRegister( cvar_t* variable );
float				CVarGetFloat( const char* name );
const char*			CVarGetString( const char* name );
void				CVarSetFloat( const char* name, float value );
void				CVarSetString( const char* name, const char* value );
cvar_t*				CVarGetPointer( const char* name );
void				PF_Cvar_DirectSet( cvar_t* variable, char* value );

void				AlertMessage( ALERT_TYPE type, char* format, ... );
void				EngineFprintf( void* file, char* format, ... );
void				ClientPrintf( edict_t* entity, PRINT_TYPE type, const char* message );
void				ServerPrint( const char* message );

void*				PvAllocEntPrivateData( edict_t* entity, int32 size );
void*				PvEntPrivateData( edict_t* entity );
const char*			SzFromIndex( int index );
int					AllocEngineString( const char* value );
entvars_t*			GetVarsOfEnt( edict_t* entity );
edict_t*			PEntityOfEntOffset( int offset );
int					EntOffsetOfPEntity( const edict_t* entity );
edict_t*			PEntityOfEntIndex( int index );
edict_t*			PEntityOfEntIndexAllEntities( int index );
edict_t*			FindEntityByVars( entvars_t* variables );
uint32				FunctionFromName( const char* name );
const char*			NameForFunction( uint32 address );

void*				GetModelPtr( edict_t* entity );
void				AnimationAutomove( const edict_t* entity, float time );
void				GetBonePosition( const edict_t* entity, int bone, float* origin, float* angles );
void				GetAttachment( const edict_t* entity, int attachment, float* origin, float* angles );

int					RegUserMsg( const char* name, int size );
const char*			Cmd_Args();
const char*			Cmd_Argv( int index );
int32				RandomLong( int32 low, int32 high );
float				RandomFloat( float low, float high );
void				PF_setview_I( const edict_t* clientEntity, const edict_t* viewEntity );
float				PF_Time();
void				PF_crosshairangle_I( const edict_t* clientEntity, float pitch, float yaw );
void				Host_EndSection( const char* section );
void				PF_FadeVolume( const edict_t* clientEntity, int fadePercent, int fadeOutSeconds, int holdTime,
						int fadeInSeconds );
void				PF_SetClientMaxspeed( const edict_t* clientEntity, float maxspeed );
int					EngCheckParm( const char* token, char** nextValue );

edict_t*			PF_CreateFakeClient_I( const char* netname );
void				PF_RunPlayerMove_I( edict_t* fakeclient, const float* viewangles, float forwardmove, float sidemove,
						float upmove, unsigned short buttons, byte impulse, byte msec );
int					PF_NumberOfEntities_I();

char*				PF_GetInfoKeyBuffer_I( edict_t* entity );
char*				PF_InfoKeyValue_I( char* info, char* key );
void				PF_SetKeyValue_I( char* info, char* key, char* value );
void				PF_SetClientKeyValue_I( int clientIndex, char* info, char* key, char* value );
void				PF_RemoveKey_I( char* info, const char* key );
const char*			PF_GetPhysicsKeyValue( const edict_t* entity, const char* key );
void				PF_SetPhysicsKeyValue( const edict_t* entity, const char* key, const char* value );
const char*			PF_GetPhysicsInfoString( const edict_t* entity );

int					PF_IsMapValid_I( char* mapname );
void				PF_StaticDecal( const float* origin, int decalIndex, int entityIndex, int modelIndex );
int					PF_precache_generic_I( char* name );
int					PF_GetPlayerUserId( edict_t* entity );
unsigned int		PF_GetPlayerWONId( edict_t* entity );
const char*			PF_GetPlayerAuthId( edict_t* entity );
void				PF_GetPlayerStats( const edict_t* player, int* ping, int* packetLoss );
void				PF_BuildSoundMsg_I( edict_t* entity, int channel, const char* sample, float volume, float attenuation,
						int flags, int pitch, int destination, int type, const float* origin, edict_t* messageEntity );
int					PF_IsDedicatedServer();

void				EV_Playback( int flags, const edict_t* invoker, unsigned short eventindex, float delay, float* origin,
						float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
byte*				SV_FatPVS( float* origin );
byte*				SV_FatPAS( float* origin );
int					SV_CheckVisibility( const edict_t* entity, byte* visibility );

void				DELTA_AddEncoder( char* name, encoder_t conditionalencode );
int					PF_GetCurrentPlayer();
int					PF_CanSkipPlayer( const edict_t* player );
void				PF_SetGroupMask( int mask, int operation );
int					PF_CreateInstancedBaseline( int classname, entity_state_t* baseline );
void				PF_ForceUnmodified( FORCE_TYPE type, float* mins, float* maxs, const char* filename );

qboolean			Voice_GetClientListening( int receiver, int sender );
qboolean			Voice_SetClientListening( int receiver, int sender, qboolean listen );

sequenceEntry_s*	SequenceGet( const char* fileName, const char* entryName );
sentenceEntry_s*	SequencePickSentence( const char* groupName, int pickMethod, int* picked );

int					VGuiWrap2_IsInCareerMatch();
int					VGuiWrap2_GetLocalizedStringLength( const char* label );
void				RegisterTutorMessageShown( int message );
int					GetTimesTutorMessageShown( int message );
void				ProcessTutorMessageDecayBuffer( int* buffer, int bufferLength );
void				ConstructTutorMessageDecayBuffer( int* buffer, int bufferLength );
void				ResetTutorMessageDecayData();

void				QueryClientCvarValue( const edict_t* player, const char* cvarName );
void				QueryClientCvarValue2( const edict_t* player, const char* cvarName, int requestID );
