#pragma once

struct event_args_s;
struct pmtrace_s;
struct tempent_s;

typedef enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM,
	BULLET_PLAYER_MP5,
	BULLET_PLAYER_357,
	BULLET_PLAYER_BUCKSHOT,
	BULLET_PLAYER_CROWBAR,
	BULLET_PLAYER_TF_BUCKSHOT,
	BULLET_PLAYER_TF_ASSAULT,
	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM
} Bullet;

enum shotgun_e
{
	SHOTGUN_IDLE = 0,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP,
	SHOTGUN_START_RELOAD,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE4,
	SHOTGUN_IDLE_DEEP
};

void	RunEventList();
void	ClearEventList();
void	EV_TFC_ClientProjectile( float* origin, float* velocity, short modelindex, int owner, int life,
			void ( *hitcallback )( tempent_s*, pmtrace_s* ) );
void	EV_TFC_FireBullets( int idx, float* forward, float* right, float* up, int shots, float* src,
			float* aim, float* spread, float distance, int bulletType, int tracerFrequency, int* tracerCount,
			int damage );

extern "C"
{
void EV_FireTFCShotgun( event_args_s* args );
void EV_ReloadTFCShotgun( event_args_s* args );
void EV_PumpTFCShotgun( event_args_s* args );
void EV_FireTFCNailgun( event_args_s* args );
void EV_FireTFCAutoRifle( event_args_s* args );
void EV_FireTFCSuperShotgun( event_args_s* args );
void EV_FireTFCSuperNailgun( event_args_s* args );
void EV_TFC_Assault_WindUp( event_args_s* args );
void EV_TFC_Assault_WindDown( event_args_s* args );
void EV_TFC_Assault_Start( event_args_s* args );
void EV_TFC_Assault_Fire( event_args_s* args );
void EV_TFC_Assault_StartSpin( event_args_s* args );
void EV_TFC_Assault_Spin( event_args_s* args );
void EV_TFC_Flame_Fire( event_args_s* args );
void EV_TFC_Knife( event_args_s* args );
void EV_TFC_Tranquilizer( event_args_s* args );
void EV_TFC_Railgun( event_args_s* args );
void EV_TFC_DoorGoDown( event_args_s* args );
void EV_TFC_DoorGoUp( event_args_s* args );
void EV_TFC_DoorHitBottom( event_args_s* args );
void EV_TFC_DoorHitTop( event_args_s* args );
void EV_TFC_Explosion( event_args_s* args );
void EV_TFC_Grenade( event_args_s* args );
void EV_TFC_EngineerGrenade( event_args_s* args );
void EV_TFC_Concussion( event_args_s* args );
void EV_TFC_NormalGrenade( event_args_s* args );
void EV_TFC_MirvGrenadeMain( event_args_s* args );
void EV_TFC_MirvGrenade( event_args_s* args );
void EV_TFC_NapalmFire( event_args_s* args );
void EV_TFC_NapalmBurn( event_args_s* args );
void EV_TFC_Gas( event_args_s* args );
void EV_TFC_EMP( event_args_s* args );
void EV_TFC_NailgrenadeNail( event_args_s* args );
void EV_TFC_NailGrenade( event_args_s* args );
void EV_TFC_FireRPG( event_args_s* args );
void EV_TFC_FireIC( event_args_s* args );
void EV_TFC_SniperHit( event_args_s* args );
void EV_FireTFCSniper( event_args_s* args );
void EV_TFC_GrenadeLauncher( event_args_s* args );
void EV_TFC_PipeLauncher( event_args_s* args );
void EV_TFC_Axe( event_args_s* args );
void EV_TFC_AxeDecal( event_args_s* args );
void EV_TFC_SteamShot( event_args_s* args );
void EV_TFC_NormalShot( event_args_s* args );
void EV_TFC_SuperShot( event_args_s* args );
void EV_TFC_Gibs( event_args_s* args );
void EV_Benchmark( event_args_s* args );
void EV_TFC_BuildingEvent( event_args_s* args );
}
