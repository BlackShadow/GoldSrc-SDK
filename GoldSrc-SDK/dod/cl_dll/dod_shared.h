#pragma once

#include "cvardef.h"

typedef enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM,
	BULLET_PLAYER_MP5,
	BULLET_PLAYER_357,
	BULLET_PLAYER_BUCKSHOT,
	BULLET_PLAYER_CROWBAR,
	BULLET_PLAYER_COLT,
	BULLET_PLAYER_LUGER,
	BULLET_PLAYER_GARAND,
	BULLET_PLAYER_SCOPEDKAR,
	BULLET_PLAYER_THOMPSON,
	BULLET_PLAYER_MP44,
	BULLET_PLAYER_SPRING,
	BULLET_PLAYER_KAR,
	BULLET_PLAYER_BAR,
	BULLET_PLAYER_MP40,
	BULLET_PLAYER_MG42,
	BULLET_PLAYER_MG34,
	BULLET_PLAYER_30CAL,
	BULLET_PLAYER_M1CARBINE,
	BULLET_PLAYER_GREASEGUN,
	BULLET_PLAYER_FG42,
	BULLET_PLAYER_K43,
	BULLET_PLAYER_ENFIELD,
	BULLET_PLAYER_STEN,
	BULLET_PLAYER_BREN,
	BULLET_PLAYER_WEBLEY,
	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM
} Bullet;

#define WEAPON_AMERKNIFE		1
#define WEAPON_GERKNIFE		2
#define WEAPON_COLT			3
#define WEAPON_LUGER			4
#define WEAPON_GARAND		5
#define WEAPON_SCOPEDKAR		6
#define WEAPON_THOMPSON		7
#define WEAPON_MP44			8
#define WEAPON_SPRING		9
#define WEAPON_KAR			10
#define WEAPON_BAR			11
#define WEAPON_MP40			12
#define WEAPON_HANDGRENADE	13
#define WEAPON_STICKGRENADE	14
#define WEAPON_STICKGRENADE_EX	15
#define WEAPON_HANDGRENADE_EX	16
#define WEAPON_MG42			17
#define WEAPON_30CAL			18
#define WEAPON_SPADE			19
#define WEAPON_M1CARBINE		20
#define WEAPON_MG34			21
#define WEAPON_GREASEGUN		22
#define WEAPON_FG42			23
#define WEAPON_K43			24
#define WEAPON_ENFIELD		25
#define WEAPON_STEN			26
#define WEAPON_BREN			27
#define WEAPON_WEBLEY		28
#define WEAPON_BAZOOKA		29
#define WEAPON_PSCHRECK		30
#define WEAPON_PIAT			31
#define WEAPON_MORTAR		32
#define WEAPON_BINOCULARS		33
#define WEAPON_SATCHEL		34

enum VOICECOMSLOTS_e
{
	SLOT_VOICECOMMAND = 0,
	SLOT_TEXT,
	SLOT_GER_ALTTEXT,
	SLOT_BRIT_ALTTEXT
};

enum HANDSIGNALS_e
{
	HS_STICK_TOGETHER = 0,
	HS_FALL_BACK,
	HS_NO_SIR,
	HS_YES_SIR,
	HS_SNIPER,
	HS_BACKUP,
	HS_ENEMY_RIGHT,
	HS_ENEMY_LEFT,
	HS_GRENADE,
	HS_FLANK_LEFT,
	HS_MOVE_OUT,
	HS_FLANK_RIGHT,
	HS_AREA_CLEAR,
	HS_COVERING_FIRE,
	HS_TAKE_COVER,
	HS_HOLD_POSITION,
	HS_SPREAD_OUT,
	HS_ENEMY_SPOTTED,
	HS_DISPLACE,
	HS_PREPARE,
	HS_GOGOGO,
	HS_ENEMY_AHEAD,
	HS_ENEMY_BEHIND,
	HS_MG_AHEAD,
	HS_MG_MOVEUP,
	HS_CEASE_FIRE,
	HS_IDLE2,
	HS_IDLE3,
	HS_IDLE4,
	HS_IDLE5
};

enum
{
	PLAYERCLASS_UNDEFINED = 0,
	PLAYERCLASS_LIGHTINF_GARAND,
	PLAYERCLASS_LIGHTINF_CARBINE,
	PLAYERCLASS_ASSAULT_TOMMY,
	PLAYERCLASS_ASSAULT_GREASE,
	PLAYERCLASS_SNIPER,
	PLAYERCLASS_HEAVYWEAPONS,
	PLAYERCLASS_MACHINEGUNNER,
	PLAYERCLASS_BAZOOKA,
	PLAYERCLASS_MORTAR,
	PLAYERCLASS_LEICHTINF_KAR,
	PLAYERCLASS_LIECHTINF_K43,
	PLAYERCLASS_STURMTRUP,
	PLAYERCLASS_HEAVYWEAPONSGER,
	PLAYERCLASS_GERMANSNIPER,
	PLAYERCLASS_FG42_BIPOD,
	PLAYERCLASS_FG42_SCOPE,
	PLAYERCLASS_MG34,
	PLAYERCLASS_MG42,
	PLAYERCLASS_PSCHRECK,
	PLAYERCLASS_GERMORTAR,
	PLAYERCLASS_BRIT_LIGHTINF,
	PLAYERCLASS_BRIT_ASSAULTINF,
	PLAYERCLASS_BRIT_SNIPER,
	PLAYERCLASS_BRIT_MACHINEGUNNER,
	PLAYERCLASS_BRIT_PIAT,
	PLAYERCLASS_BRIT_MORTAR,
	PLAYERCLASS_RANDOM,

	NUM_PLAYERCLASSES
};

enum
{
	RND_GOODTOGO = 0,
	RND_RUNNING,
	RND_FINALCHECK,
	RND_ALLIES_WIN,
	RND_AXIS_WIN,
	RND_DRAW
};

typedef struct DodClassInfo_s
{
	char	selectcmd[32];
	char	classname[128];
	char	modelname[128];
	int		team;
	int		speed;
	int		weapons;
	int		weapons2;
	int		numgrenades;
	int		armskin;
	int		headgroup;
	int		helmetgroup;
	int		geargroup;
	int		bodygroup;
	int		hairgroup;
	char	limitcvar[32];
} DodClassInfo_t;

struct p_wpninfo_s
{
	char	pmodel[32];
	char	vmodel[32];
	char	wmodel[32];
	char	leftreload[32];
	char	bipodup[32];
	char	bipoddown[32];
	char	sandbagreload[32];
	char	pronereload[32];
	char	szAnimExt[16];
	char	szAnimReloadExt[16];
	float	anim_reloadtime;
	float	anim_firedelay;
	float	anim_reloadtime2;
	float	anim_firetime2;
	float	anim_drawtime;
	float	anim_up2downtime;
	float	anim_down2uptime;
	int		ammo_default;
	int		ammo_maxcarry;
	int		ammo_maxclip;
	int		ammo_give;
	int		misc_weight;
	float	base_accuracy;
	float	accuracy_penalty;
	float	base_accuracy2;
};

extern char			weaponnames[64][64];
extern cvar_t*		pcvar_classlimitcvars[28];
extern char			classlimitcvarNames[27][32];
extern char*		sPlayerModelFiles[6];
extern char*		s_USVoiceFiles[28];
extern char*		s_BRITVoiceFiles[28];
extern char*		s_GERVoiceFiles[29];
extern int			iVoiceToHandSignal[29];
extern char*		s_VoiceCommands[29][4];
extern char*		s_HandSignalSubtitles[25][3];
extern char*		sHandSignals[30];
extern DodClassInfo_t	g_ParaClassInfo[21];
extern DodClassInfo_t	g_ClassInfo[28];
extern p_wpninfo_s	P_WpnInfo[41];
extern char*		sHelmetModels[7];
extern char*		szMapMarkerIcons[15];
