#include "engine_apiproxy.h"
#include "client.h"
#include "decals.h"

particle_t* R_AllocParticle( void ( *callback )( particle_t* particle, float frametime ) );
void R_BlobExplosion( float* origin );
void R_Blood( float* origin, float* direction, int color, int speed );
void R_BloodSprite( float* origin, int colorIndex, int modelIndex, int splatterModelIndex, float size );
void R_BloodStream( float* origin, float* direction, int color, int speed );
void R_BreakModel( float* position, float* size, float* direction, float randomness, float life, int count,
	int modelIndex, char flags );
void R_Bubbles( float* mins, float* maxs, float height, int modelIndex, int count, float speed );
void R_BubbleTrail( float* start, float* end, float height, int modelIndex, int count, float speed );
void R_BulletImpactParticles( float* position );
void R_EntityParticles( cl_entity_t* entity );
void R_Explosion( float* position, int model, float scale, float framerate, int flags );
void R_FizzEffect( cl_entity_t* entity, int modelIndex, int density );
void R_FireField( float* origin, int radius, int modelIndex, int count, int flags, float life );
void R_FlickerParticles( float* origin );
void R_FunnelSprite( float* origin, int modelIndex, int reverse );
void R_Implosion( float* end, float radius, int count, float life );
void R_LargeFunnel( float* origin, int reverse );
void R_LavaSplash( float* origin );
void R_MultiGunshot( float* origin, float* direction, float* noise, int count, int decalCount, int* decalIndices );
void R_MuzzleFlash( float* position, int type );
void R_ParticleBox( float* mins, float* maxs, unsigned char red, unsigned char green, unsigned char blue, float life );
void R_ParticleBurst( float* position, int size, int color, float life );
void R_ParticleExplosion( float* origin );
void R_ParticleExplosion2( float* origin, int colorStart, int colorLength );
void R_ParticleLine( float* start, float* end, unsigned char red, unsigned char green, unsigned char blue, float life );
void R_PlayerSprites( int client, int modelIndex, int count, int size );
void R_Projectile( float* origin, float* velocity, int modelIndex, int life, int owner,
	void ( *hitcallback )( tempent_s* entity, pmtrace_s* trace ) );
void R_RicochetSound( float* position );
void R_RicochetSprite( float* position, model_s* model, float duration, float scale );
void R_RocketFlare( float* position );
void R_RocketTrail( float* start, float* end, int type );
void R_RunParticleEffect( float* origin, float* direction, int color, int count );
void R_ShowLine( float* start, float* end );
void R_SparkEffect( float* position, int count, int velocityMin, int velocityMax );
void R_SparkShower( float* position );
void R_SparkStreaks( float* position, int count, int velocityMin, int velocityMax );
void R_Spray( float* position, float* direction, int modelIndex, int count, int speed, int spread, int renderMode );
void R_Sprite_Explode( TEMPENTITY* temp, float scale, int flags );
void R_Sprite_Smoke( TEMPENTITY* temp, float scale );
void R_Sprite_Spray( float* position, float* direction, int modelIndex, int count, int speed, int randomness );
void R_Sprite_Trail( int type, float* start, float* end, int modelIndex, int count, float life, float size,
	float amplitude, int renderAmount, float speed );
void R_Sprite_WallPuff( TEMPENTITY* temp, float scale );
void R_StreakSplash(
	float* position, float* direction, int color, int count, float speed, int velocityMin, int velocityMax );
void R_TracerEffect( float* start, float* end );
void R_UserTracerParticle( float* origin, float* velocity, float life, int colorIndex, float length,
	unsigned char context, void ( *deathfunc )( particle_t* particle ) );
particle_t* R_TracerParticles( float* origin, float* velocity, float life );
void R_TeleportSplash( float* origin );
void R_TempSphereModel( float* position, float speed, float life, int count, int modelIndex );
TEMPENTITY* R_TempModel( float* position, float* direction, float* angles, float life, int modelIndex, int soundType );
TEMPENTITY* R_DefaultSprite( float* position, int spriteIndex, float framerate );
TEMPENTITY* R_TempSprite( float* position, float* direction, float scale, int modelIndex, int renderMode,
	int renderEffect, float alpha, float life, int flags );
void R_AttachTentToPlayer( int client, int modelIndex, float zoffset, float life );
void R_KillAttachedTents( int client );
BEAM* R_BeamCirclePoints( int type, float* start, float* end, int modelIndex, float life, float width, float amplitude,
	float brightness, float speed, int startFrame, float framerate, float red, float green, float blue );
BEAM* R_BeamEntPoint( int startEntity, float* end, int modelIndex, float life, float width, float amplitude,
	float brightness, float speed, int startFrame, float framerate, float red, float green, float blue );
BEAM* R_BeamEnts( int startEntity, int endEntity, int modelIndex, float life, float width, float amplitude,
	float brightness, float speed, int startFrame, float framerate, float red, float green, float blue );
BEAM* R_BeamFollow(
	int startEntity, int modelIndex, float life, float width, float red, float green, float blue, float brightness );
void R_BeamKill( int deadEntity );
BEAM* R_BeamLightning(
	float* start, float* end, int modelIndex, float life, float width, float amplitude, float brightness, float speed );
BEAM* R_BeamPoints( float* start, float* end, int modelIndex, float life, float width, float amplitude,
	float brightness, float speed, int startFrame, float framerate, float red, float green, float blue );
BEAM* R_BeamRing( int startEntity, int endEntity, int modelIndex, float life, float width, float amplitude,
	float brightness, float speed, int startFrame, float framerate, float red, float green, float blue );
TEMPENTITY* CL_TempEntAlloc( float* origin, model_s* model );
TEMPENTITY* CL_TempEntAllocNoModel( float* origin );
TEMPENTITY* CL_TempEntAllocHigh( float* origin, model_s* model );
TEMPENTITY* CL_AllocCustomTempEntity( float* origin, model_s* model, int high,
	void ( *callback )( tempent_s* entity, float frametime, float currenttime ) );
void R_GetPackedColor( short* packed, short color );
short R_LookupColor( unsigned char red, unsigned char green, unsigned char blue );
void R_DecalRemoveNonPermanent( int textureIndex );
#if defined( SW_RENDERER )
void R_FireCustomDecal( int textureIndex, int entity, int modelIndex, float* position, int flags, float scale, float scaletime );
#else
void R_FireCustomDecal( int textureIndex, int entity, int modelIndex, float* position, int flags, float scale );
#endif

efx_api_t efx = { R_AllocParticle, R_BlobExplosion, R_Blood, R_BloodSprite, R_BloodStream, R_BreakModel, R_Bubbles,
	R_BubbleTrail, R_BulletImpactParticles, R_EntityParticles, R_Explosion, R_FizzEffect, R_FireField,
	R_FlickerParticles, R_FunnelSprite, R_Implosion, R_LargeFunnel, R_LavaSplash, R_MultiGunshot, R_MuzzleFlash,
	R_ParticleBox, R_ParticleBurst, R_ParticleExplosion, R_ParticleExplosion2, R_ParticleLine, R_PlayerSprites,
	R_Projectile, R_RicochetSound, R_RicochetSprite, R_RocketFlare, R_RocketTrail, R_RunParticleEffect, R_ShowLine,
	R_SparkEffect, R_SparkShower, R_SparkStreaks, R_Spray, R_Sprite_Explode, R_Sprite_Smoke, R_Sprite_Spray,
	R_Sprite_Trail, R_Sprite_WallPuff, R_StreakSplash, R_TracerEffect, R_UserTracerParticle, R_TracerParticles,
	R_TeleportSplash, R_TempSphereModel, R_TempModel, R_DefaultSprite, R_TempSprite, Draw_DecalIndex,
	Draw_DecalIndexFromName, R_DecalShoot, R_AttachTentToPlayer, R_KillAttachedTents, R_BeamCirclePoints,
	R_BeamEntPoint, R_BeamEnts, R_BeamFollow, R_BeamKill, R_BeamLightning, R_BeamPoints, R_BeamRing, CL_AllocDlight,
	CL_AllocElight, CL_TempEntAlloc, CL_TempEntAllocNoModel, CL_TempEntAllocHigh, CL_AllocCustomTempEntity,
	R_GetPackedColor, R_LookupColor, R_DecalRemoveNonPermanent,
#if defined( SW_RENDERER )
	(void ( * )( int, int, int, float*, int, float ))R_FireCustomDecal
#else
	R_FireCustomDecal
#endif
};
