/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//			
//  hud.h
//
// class CHud declaration
//
// CHud handles the message, calculation, and drawing the HUD
//


#ifndef HUD_H
#define HUD_H

#include "wrect.h"
#include "cl_dll.h"
#include "cl_entity.h"
#include "ammo.h"
#include "../../cl_dll/global_consts.h"

#define RGB_YELLOWISH 0x00FFA000 //255,160,0
#define RGB_REDISH 0x00FF1010 //255,160,0
#define RGB_GREENISH 0x0000A000 //0,160,0
#define DHN_DRAWZERO 1
#define DHN_2DIGITS 2
#define DHN_3DIGITS 4
#define MIN_ALPHA 100
#define HUDELEM_ACTIVE 1
#define HUD_ACTIVE 1
#define HUD_INTERMISSION 2
#define MAX_PLAYER_NAME_LENGTH 32
#define MAX_MOTD_LENGTH 1536

//
//-----------------------------------------------------
//
#define MAX_SPRITE_NAME_LENGTH 24
#define maxHUDMessages 16

typedef struct cvar_s cvar_t;
typedef struct model_s model_t;
struct tempent_s;

typedef struct
{
	int x;
	int y;
} POSITION;

typedef struct
{
	unsigned char r, g, b, a;
} RGBA;

//
//-----------------------------------------------------
//
class CHudBase
{
public:
	virtual ~CHudBase() {}
	virtual int Init() { return 0; }
	virtual int VidInit() { return 0; }
	virtual int Draw( float ) { return 0; }
	virtual void Think() {}
	virtual void Reset() {}
	virtual void PlayerDied() {}
	virtual void InitHUDData() {}

	POSITION m_pos;
	int m_type;
	int m_iFlags; // active, moving, 
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

struct HUDLIST
{
	CHudBase *p;
	HUDLIST *pNext;
};

#include "hud_spectator.h"

struct ClipInfo
{
	int weapon_id;
	int full_index;
	int empty_index;
	int extra_index;
	HSPRITE FullSprite;
	HSPRITE EmptySprite;
	HSPRITE ExtraSprite;
	wrect_t *FullArea;
	wrect_t *EmptyArea;
	wrect_t *ExtraArea;
	float lastDrop;
	float subseqDrop;
};


//
//-----------------------------------------------------
//
class CHudAmmo : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Think() override;
	void Reset() override;
	int DrawWList( float time );
	int DrawWeaponList( float time );
	void PlayerDied() override;
	ClipInfo *GetCurrentGun( WEAPON *weapon );
	int GetCurrentWeaponId();
	void SlotInput( int slot );
	void UserCmd_Slot1();
	void UserCmd_Slot2();
	void UserCmd_Slot3();
	void UserCmd_Slot4();
	void UserCmd_Slot5();
	void UserCmd_Slot6();
	void UserCmd_Slot7();
	void UserCmd_Slot8();
	void UserCmd_Slot9();
	void UserCmd_Slot10();
	void UserCmd_Close();
	void UserCmd_NextWeapon();
	void UserCmd_PrevWeapon();
	int MsgFunc_CurWeapon( const char *, int, void * );
	int MsgFunc_WeaponList( const char *, int, void * );
	int MsgFunc_AmmoX( const char *, int, void * );
	int MsgFunc_AmmoShort( const char *, int, void * );
	int MsgFunc_AmmoPickup( const char *, int, void * );
	int MsgFunc_WeapPickup( const char *, int, void * );
	int MsgFunc_ItemPickup( const char *, int, void * );
	int MsgFunc_HideWeapon( const char *, int, void * );
	int MsgFunc_ReloadDone( const char *, int, void * );

	float m_fFade;
	RGBA m_rgba;
	WEAPON *m_pWeapon;
	int m_HUD_clip;
	int m_HUD_clipempty;
	wrect_t *m_prc1;
	wrect_t *m_prc2;
	int m_iHeight;
	int m_iClipHeight;
	HSPRITE m_hSprite1;
	HSPRITE m_hSprite2;
	HSPRITE MGBarrelHUD;
	wrect_t *MGBarrelHUDArea;
	HSPRITE MGBarrel2HUD;
	wrect_t *MGBarrel2HUDArea;
	ClipInfo ClipInfoArray[64];
	ClipInfo BritGrenClipInfo;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

//
//-----------------------------------------------------
//
class CHudDeathNotice : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void InitHUDData() override;
	int MsgFunc_DeathMsg( const char *, int, void * );

	int m_HUD_d_skull;  // sprite index of skull icon
	cvar_t *hud_deathnotice_time;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

//
//-----------------------------------------------------
//
class CHudSayText : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void InitHUDData() override;
	int MsgFunc_SayText( const char *, int, void * );
	void SayTextPrint( const char *text, int length, int client, char *arg1, char *arg2, char *arg3, char *arg4 );
	void EnsureTextFitsInOneLineAndWrapIfHaveTo( int line );
	int GetTextPrintY();

	cvar_t *m_HUD_saytext;
	cvar_t *m_HUD_saytext_time;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

//
//-----------------------------------------------------
//
class CHudMenu : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	void InitHUDData() override;
	int MsgFunc_ShowMenu( const char *, int, void * );
	void SelectMenuItem( int item );

	int m_fMenuDisplayed;
	int m_bitsValidSlots;
	float m_flShutoffTime;
	int m_fWaitingForMore;
	int CanCancel;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

//
//-----------------------------------------------------
//
class CHudTrain : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	int MsgFunc_Train( const char *, int, void * );

	HSPRITE m_hSprite;
	int m_iPos;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

//
//-----------------------------------------------------
//

class CHudTextMessage : public CHudBase
{
public:
	int Init() override;
	int MsgFunc_TextMsg( const char *, int, void * );
	int MsgFunc_CapMsg( const char *, int, void * );
	static char *LocaliseTextString( const char *text, char *destination, int size );
	static char *BufferedLocaliseTextString( const char *text );
	char *LookupString( const char *text, int *destination = NULL );
};

typedef struct
{
	client_textmessage_t *pMessage;
	float time;
	int x, y;
	int totalWidth, totalHeight;
	int width, lines, lineLength, length;
	int r, g, b;
	int text;
	int fadeBlend;
	float charTime;
	float fadeTime;
} message_parms_t;

//
//-----------------------------------------------------
//

class CHudMessage : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_HudText( const char *, int, void * );
	int MsgFunc_GameTitle( const char *, int, void * );
	float FadeBlend( float fadein, float fadeout, float hold, float time );
	int XPosition( float x, int width, int lineWidth );
	int YPosition( float y, int height );
	void MessageAdd( const char *name, float time, int hint, unsigned int font );
	void MessageAdd( client_textmessage_t *message );
	void HintMessageAdd( const char *text );
	void MessageDrawScan( client_textmessage_t *message, float time, unsigned int font );
	void MessageScanStart();
	void MessageScanNextChar( unsigned int font );
	unsigned int GetFont();

	struct client_message_t
	{
		client_textmessage_t *pMessage;
		unsigned int font;
	};
	client_message_t m_pMessages[maxHUDMessages];
	float m_startTime[maxHUDMessages];
	message_parms_t m_parms;
	float m_gameTitleTime;
	client_textmessage_t *m_pGameTitle;
	int m_HUD_title_life;
	int m_HUD_title_half;
	unsigned int m_Fonts[3];
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CHudStatusIcons : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_StatusIcon( const char *, int, void * );
	void EnableIcon( char *name, unsigned char red, unsigned char green, unsigned char blue );
	void DisableIcon( char *name );
	enum { MAX_ICONSPRITENAME_LENGTH = 24, MAX_ICONSPRITES = 4 };
	struct icon_sprite_t
	{
		char szSpriteName[MAX_ICONSPRITENAME_LENGTH];
		HSPRITE spr;
		wrect_t rc;
		unsigned char r, g, b;
	};
	icon_sprite_t m_IconList[MAX_ICONSPRITES];
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

//
//-----------------------------------------------------
//
class CHudStatusBar : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	void Think() override;
	int MsgFunc_StatusValue( const char *, int, void * );
	char *GetTargetName();
	void CreateEntities();
	bool InDeathCamMode();
	void DrawEntitiesOverTeam();
	void DrawEntitiesOverTarget();

	float m_flNextUpdateTime;
	int m_iTargetTeam;
	int m_iTargetIndex;
	int m_iHealth;
	cl_entity_t m_TargetHeadModel[64];
	int m_TargetTalking[64];
	HSPRITE m_StatusBarAllieModel;
	HSPRITE m_StatusBarBritishModel;
	HSPRITE m_StatusBarGermanModel;
	HSPRITE m_StatusBarHeadModel;
	cvar_t *hud_centerid;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CHudScope : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	void Think() override;
	int MsgFunc_Scope( const char *, int, void * );
	void DrawTriApiScope();
	void SetScope( int id );

	int m_iWeaponId;
	bool m_bWeaponChanged;
	int m_nLastWpnId;
	HSPRITE spring_sprite;
	model_s *spring_model;
	HSPRITE k43_sprite;
	model_s *k43_model;
	HSPRITE binoc_sprite;
	model_s *binoc_model;
	HSPRITE enfield_sprite;
	model_s *enfield_model;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

struct ClientArea
{
	int nStatus;
	HSPRITE hSpr;
};

class CHudDodIcons : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	void PlayerDied() override;
	void DrawCredits( float time );
	void DrawMarkerIcon( int marker );
	void MapMarkerPosted();
	void ActivateHintBacking( int team, float duration );
	void StartDrawingCredits();
	void GetHintMessageLocation( int &x, int &y );
	int MsgFunc_Health( const char *, int, void * );
	int MsgFunc_Object( const char *, int, void * );
	int MsgFunc_ClCorpse( const char *, int, void * );
	int MsgFunc_ClientAreas( const char *, int, void * );

	int m_iHealth;
	char *m_szObjectIcon;
	HSPRITE m_hObjectSpr;
	wrect_t *m_wObjectArea;
	int sprite_array[5];
	wrect_t *area_array[5];
	int m_iKey;
	HSPRITE MapMarkerSprite;
	wrect_t *MapMarkerArea;
	float m_fLastMapMarkerTime;
	ClientArea m_Areas[128];
	HSPRITE IconMGDeploy;
	wrect_t *IconMGDeployArea;
	HSPRITE MainHUD;
	wrect_t *MainHUDArea;
	HSPRITE ObjectivesHUD;
	wrect_t *ObjectivesHUDArea;
	HSPRITE ReinforcementsHUD;
	wrect_t *ReinforcementsHUDArea;
	HSPRITE HeartHUD;
	wrect_t *HeartHUDArea;
	HSPRITE StaminaBarHUD;
	wrect_t *StaminaBarHUDArea;
	HSPRITE HealthBarHUD;
	wrect_t *HealthBarHUDArea;
	int i_HeartFrame;
	float f_HeartTime;
	float f_HeartSpeed;
	HSPRITE HealthOverlayHUD;
	wrect_t *HealthOverlayHUDArea;
	int m_iIconHeight;
	int m_iIconWidth;
	float m_flHintDieTime;
	int m_iHintTeam;
	int m_iMsgX;
	int m_iMsgY;
	int m_hHintHeads[3];
	wrect_t *m_rectHintHeads[3];
	HSPRITE m_hHorizHintBacking;
	HSPRITE m_hVertHintBacking;
	wrect_t *m_rectHorizHintBacking;
	wrect_t *m_rectVertHintBacking;
	int m_iCreditName;
	float m_flCreditChangeTime;
	HSPRITE m_hsprSelectedMarker;
	float m_flDrawSelectedMarkerTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

struct control_point_t
{
	int entindex;
	bool valid;
	int visible;
	int owner;
	int nextOwner;
	float animtime;
	float totaltime;
	int numplayers;
	int requiredplayers;
	int occupyingteam;
	int m_iIcons[3];
	wrect_t m_rAreas[3];
	Vector m_rOrigin;
	int m_iMapXPos;
	int m_iMapYPos;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CObjectiveIcons : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Think() override;
	int MsgFunc_SetObj( const char *, int, void * );
	int MsgFunc_InitObj( const char *, int, void * );
	int MsgFunc_PlayersIn( const char *, int, void * );
	int MsgFunc_StartProg( const char *, int, void * );
	int MsgFunc_CancelProg( const char *, int, void * );
	int MsgFunc_ProgUpdate( const char *, int, void * );
	int MsgFunc_StartProgF( const char *, int, void * );
	int MsgFunc_TimerStatus( const char *, int, void * );
	void SetVisible( int index, int visible );
	void ChangeCapPoint( int index, int owner, int animation );
	void StartCapProgress( int index, int team, float duration );
	void CalcIconLocations();
	void CancelCapProgress( int index, int team );
	void ClearAllCapPoints();
	void SetNumPlayersInArea( int index, int team, int number, int required );
	void UpdateObjectiveIcons();
	void DrawDigit( int digit, int x, int y );

	wrect_t m_iconarea;
	HSPRITE TimerHUD;
	wrect_t *TimerHUDArea;
	float m_fTimerSeconds;
	float m_flWaveTime;
	int m_Init;
	int m_Set;
	int m_StartProgress;
	int m_CancelProgress;
	int m_iWaveStatus;
	float m_fLastTime;
	control_point_t m_eControlPoints[12];
	int m_nTimerStatus;
	int m_TimerIcons[11];
	wrect_t *m_TimerAreas[11];
	wrect_t m_topwrect;
	wrect_t m_bottomwrect;
	HSPRITE m_TopNumber;
	HSPRITE m_BottomNumber;
	bool m_bWarmupMode;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CHudDoDCrossHair : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_ClanTimer( const char *, int, void * );
	void DrawClanTimer( float time );
	int GetCrossHairWidth();
	void DrawCustomCrossHair( int index );
	bool ShouldDrawCrossHair();
	void DrawDynamicCrossHair();
	void DrawSpectatorCrossHair();
	float GetCurrentWeaponAccuracy();

	float m_fClanTimer;
	int i_xSet;
	float f_xPos;
	float f_yPos;
	float f_xLPos;
	float f_xRPos;
	Vector v_evPunch;
	int m_MoveWidth;
	int m_MoveHeigth;
	int m_MoveWidthD;
	int m_MoveHeightD;
	HSPRITE CrossSprite2horiz;
	wrect_t *CrossArea2horiz;
	HSPRITE CrossSprite2vert;
	wrect_t *CrossArea2vert;
	HSPRITE CrossSprite2dot;
	wrect_t *CrossArea2dot;
	HSPRITE m_hCrosshair;
	wrect_t m_crosshairRect;
	HSPRITE m_hCustomCrosshair;
	wrect_t m_customCrosshairRect;
	int m_iXPos;
	int m_iYPos;
	float m_fMoveTime;
	int m_flLastSetTime;
	int m_iLastXHairWidth;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CHudDoDMap : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void InitHUDData() override;
	void SetMapState( int state );
	void DrawOverview();
	void HandleMapButton();
	void DrawOverviewIcon( Vector origin, int sprite, int scale, Vector angles );
	bool AddMapEntityToMap( int sprite, double lifetime, Vector origin );
	void DrawOverviewLayer();
	void GetSmallMapOffset( float &x, float &y, float &z );
	void HandleMapZoomButton();
	void DrawOverviewEntities();
	void CheckOverviewEntities();

	struct map_icon_t
	{
		Vector origin;
		HSPRITE hSprite;
		double killTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
	};
	map_icon_t m_ExtraOverviewEntities[32];
	cl_entity_t m_MapTag[64];
	float m_flZoom;
	float m_flZoomTime;
	float m_flSmallMapScale;
	float m_flIdealMapScale;
	float m_flPreviousMapScale;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CHudDoDCommon : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void InitHUDData() override;
	int MsgFunc_GameRules( const char *, int, void * );
	int MsgFunc_ResetSens( const char *, int, void * );
	int MsgFunc_CameraView( const char *, int, void * );
};

class CHudVGUI2Print : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void GetStringSize( const wchar_t *text, int *width, int *height );
	void VGUI2HudPrint( char *text, int x, int y, float red, float green, float blue );
	int DrawVGUI2String( char *text, int x, int y, float red, float green, float blue );
	int DrawVGUI2String( wchar_t *text, int x, int y, float red, float green, float blue );
	int GetHudFontHeight();
	void VGUI2HudPrintArgs( char *text, char *arg1, char *arg2, char *arg3, char *arg4,
		int x, int y, float red, float green, float blue );
	int DrawVGUI2StringReverse( char *text, int x, int y, float red, float green, float blue );
	int DrawVGUI2StringReverse( wchar_t *text, int x, int y, float red, float green, float blue );
	unsigned int GetFont();

	float m_flVGUI2StringTime;
	wchar_t m_wCharBuf[512];
	float m_fR;
	float m_fG;
	float m_fB;
	int m_iX;
	int m_iY;
	unsigned int m_Fonts[3];
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

struct trajectory_t
{
	float vTargetPos[3];
	float fPitch1;
	float fPitch2;
	float fYaw;
	float fLastUsedTime;
};

class CTrajectoryList
{
public:
	CTrajectoryList();
	~CTrajectoryList();
	trajectory_t *AddTrajectory( Vector target );
	void GetTrajectory( Vector launch, Vector target, float *pitch1, float *pitch2, float *yaw );
	void CalculateTrajectory( Vector launch, Vector target, float *pitch1, float *pitch2, float *yaw );
	void InvalidateAllTrajectories();

	Vector m_vecLaunchPos;
	trajectory_t m_Trajectories[32];
};

class CMortarHud : public CHudBase
{
public:
	CMortarHud();
	~CMortarHud() override;
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void CalculateFireAngle( float *pitch, float *yaw );
	void DrawPredictedMortarImpactSite();

	CTrajectoryList *m_TrajectoryList;
};

typedef struct particle_shooter_s
{
	int id;
	int iNumParticles;
	int iParticlesRemaining;
	float fParticleLife;
	float fFireDelay;
	int iFlags;
	Vector vOrigin;
	Vector vVelocity;
	float fVariance;
	float fNextShootTime;
	int iSprite;
	char szSprite[128];
	model_s *pSprite;
	int iColour[3];
	float fGravity;
	float fFadeSpeed;
	float fSize;
	float fDampingTime;
	float fDampingVel;
	int iSpinDegPerSec;
	float fBrightness;
	float fScaleSpeed;
	int iFramerate;
	int iRenderMode;
	int iState;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
} particle_shooter_t;

class CParticleShooter : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	void Think() override;
	void AddParticleSystem( particle_shooter_t *shooter );
	int MsgFunc_PReg( const char *, int, void * );
	int MsgFunc_PShoot( const char *, int, void * );

private:
	particle_shooter_t m_sShooters[64];
	int m_iNumShooters;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

struct env_model_t
{
	char szModel[64];
	char szSequence[64];
	int iBody;
	Vector vecOrigin;
	Vector vecAngles;
	int spawnflags;
	float frame;
	int sequence;
	int iModel;
	model_s *pModel;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CClientEnvModel : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	void Think() override;
	void RemoveAllModels();
	void AddEnvModel( env_model_t *model );
	int GetNumModels() { return m_iNumEnvModels; }
	env_model_t *GetModel( int index ) { return &m_sEnvModels[index]; }

private:
	tempent_s *m_teEnvModelTE;
	env_model_t m_sEnvModels[192];
	int m_iNumEnvModels;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CWeatherManager : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	void CreateRainParticle( float *origin );
	void CreateSnowParticle( float *origin );
	void SetRainSprite( model_s *sprite ) { m_pRainSprite = sprite; }
	void SetSnowSprite( model_s *sprite ) { m_pSnowSprite = sprite; }
	void SetSplashSprite( model_s *sprite ) { m_pSplashSprite = sprite; }
	void SetRippleSprite( model_s *sprite ) { m_pRippleSprite = sprite; }
	model_s *GetRainSprite() { return m_pRainSprite; }
	model_s *GetSnowSprite() { return m_pSnowSprite; }
	model_s *GetSplashSprite() { return m_pSplashSprite; }
	model_s *GetRippleSprite() { return m_pRippleSprite; }

private:
	model_s *m_pRainSprite;
	model_s *m_pSnowSprite;
	model_s *m_pSplashSprite;
	model_s *m_pRippleSprite;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CHud
{
public:
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
	CHud() : m_pHudList( NULL ), m_iSpriteCount( 0 ) {}
	~CHud();
	void Init();
	void VidInit();
	void Think();
	int Redraw( float time, int intermission );
	int UpdateClientData( client_data_t *data, float time );
	void AddHudElem( CHudBase *element );
	HSPRITE GetSprite( int index ) { return index < 0 ? 0 : m_rghSprites[index]; }
	wrect_t &GetSpriteRect( int index ) { return m_rgrcRects[index]; }
	int GetSpriteIndex( const char *name );
	float GetSensitivity();
	int DrawHudNumber( int x, int y, int flags, int number, int red, int green, int blue );
	int DrawHudString( int x, int y, int maxX, char *text, int red, int green, int blue );
	int DrawHudStringReverse( int x, int y, int minX, char *text, int red, int green, int blue );
	int DrawHudNumberString( int x, int y, int minX, int number, int red, int green, int blue );
	int GetNumWidth( int number, int flags );
	bool IsTeamPara( int team );
	char *GetTeamName( int team );
	int ZoomMinimap();
	void GetMapBounds( int &x, int &y, int &width, int &height );
	bool IsInMGDeploy();
	int GetWaterLevel();
	void InitMapBounds();
	void SetWaterLevel( int level );
	void VGUI2HudPrint( char *text, int x, int y, float red, float green, float blue );
	int GetMinimapState();
	bool IsProneDeployed();
	void PlaySoundOnChan( char *sound, float volume, int channel );
	void PostMortarValue( float value );
	void SetMinimapState( int state );
	void SetRecoilAmount( float pitch, float yaw );
	bool IsInMortarDeploy();
	bool IsSandbagDeployed();
	int GetCurrentWeaponId();
	char *GetPlayerClassName( int index );
	int GetMinimapZoomLevel();
	float GetMortarDeployTime();
	void SetMortarDeployTime();
	float GetMortarUnDeployTime();
	void GetWeaponRecoilAmount( int weapon, float &pitch, float &yaw );
	void SetMortarUnDeployTime();
	void DoRecoil( int weapon );
	bool IsProne();
	bool IsDucking();
	void PopRecoil( float time, float &pitch, float &yaw );
	int MsgFunc_HLTV( const char *, int, void * );
	int MsgFunc_Logo( const char *, int, void * );
	int MsgFunc_SetFOV( const char *, int, void * );
	int MsgFunc_TimeLeft( const char *, int, void * );
	int MsgFunc_UseSound( const char *, int, void * );
	int MsgFunc_RoundState( const char *, int, void * );
	int MsgFunc_ResetHUD( const char *, int, void * );
	int MsgFunc_YouDied( const char *, int, void * );
	int MsgFunc_Damage( const char *, int, void * );
	void MsgFunc_InitHUD( const char *, int, void * );

	HUDLIST *m_pHudList;
	HSPRITE m_hsprLogo;
	int m_iLogo;
	client_sprite_t *m_pSpriteList;
	int m_iSpriteCount;
	int m_iSpriteCountAllRes;
	HSPRITE m_hsprCursor;
	float m_flTime;	   // the current client time
	float m_fOldTime;  // the time at which the HUD was last redrawn
	double m_flTimeDelta; // the difference between flTime and fOldTime
	Vector m_vecOrigin;
	Vector m_vecAngles;
	Vector m_vecVelocity;
	int m_iKeyBits;
	int m_iHideHUDDisplay;
	int m_Teamplay;
	int m_iRes;
	cvar_t *m_pCvarStealMouse;
	cvar_t *m_pCvarDraw;
	float m_flMouseSensitivity;
	int m_PlayerFOV[64];
	bool m_bAllieParatrooper;
	bool m_bAllieInfiniteLives;
	bool m_bAxisParatrooper;
	bool m_bAxisInfiniteLives;
	bool m_bParatrooper;
	bool m_bInfiniteLives;
	bool m_bBritish;
	int m_iRoundState;
	int m_iSensLevel;
	int m_iFontHeight;
	int m_iFontEngineHeight;
	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	HSPRITE *m_rghSprites;	/*[HUD_SPRITE_COUNT]*/			// the sprites loaded from hud.txt
	wrect_t *m_rgrcRects;	/*[HUD_SPRITE_COUNT]*/
	char *m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/
	char m_szTeamNames[5][32];
	int m_iFOV;
	int m_iMapX;
	int m_iMapY;
	int m_iMapWidth;
	int m_iMapHeight;
	int m_iSmallMapX;
	int m_iSmallMapY;
	int m_iSmallMapWidth;
	int m_iSmallMapHeight;
	CHudScope m_Scope;
	CHudDodIcons m_Icons;
	CObjectiveIcons m_ObjectiveIcons;
	CParticleShooter m_PShooter;
	CClientEnvModel m_CEnvModel;
	CWeatherManager m_Weather;
	CHudAmmo m_Ammo;
	CHudSayText m_SayText;
	CHudSpectator m_Spectator;
	CHudTrain m_Train;
	CHudMessage m_Message;
	CHudStatusBar m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudMenu m_Menu;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;
	CHudDoDCrossHair m_DoDCrossHair;
	CHudDoDMap m_DoDMap;
	CHudDoDCommon m_DoDCommon;
	CHudVGUI2Print m_VGUI2Print;
	CMortarHud m_MortarHud;

	// Screen information
	SCREENINFO m_scrinfo;
	int m_iWeaponBits;
	int m_fPlayerDead;
	int m_iIntermission;
	int g_iClip;
	bool m_bAutoReloadComplete;
	int g_pModel;
	float g_NextAttack;
	int m_HUD_number_0;
	int m_MG_number_0;
	float i_MusicFadeCounter;
	float i_MusicFadeCounter2;
	int i_MusicPlay;
	int i_specmenutoggle;
	int i_Recoil;
	float i_TempFSpeed;
	float i_TempBSpeed;
	float i_TempSSpeed;
	int m_iWaterLevel;
	int m_iClipSize;
	float m_flPlaySprintSoundTime;
	int m_iMinimapState;
	float m_fRoundEndsTime;
	float m_fMortarDeployTime;
	float m_fMortarUnDeployTime;
	cvar_t *_cl_minimap;
	cvar_t *_cl_minimapzoom;
	cvar_t *zoom_sensitivity_ratio;
	cvar_t *hud_takesshots;
	cvar_t *r_drawentities;
	cvar_t *cl_lw;
	cvar_t *cl_pitchup;
	cvar_t *cl_pitchdown;
	cvar_t *crosshair;
	cvar_t *max_rubble;
	cvar_t *_ah;
	cvar_t *cl_corpsestay;
	cvar_t *developer;
	cvar_t *cl_hudfont;
	cvar_t *hud_fastswitch;
	float m_flPitchRecoilAccumulator;
	float m_flYawRecoilAccumulator;
	float m_flRecoilTimeRemaining;
};

struct extra_player_info_t
{
	short frags;
	short objscore;
	short deaths;
	short playerclass;
	short teamnumber;
	int status;
	char teamname[16];
	float showhealth;
	int health;
	int teamId;
	bool dead;
};

class Element
{
public:
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
	Element( float time ) : flTimeCreated( time ) {}
	float flTimeCreated;
	Element *next;
	Element *previous;
};

class Queue
{
public:
	Queue() : count( 0 ), maxelements( 60 ), m_flDuration( 5 ), first( NULL ), last( NULL ), current( NULL ) {}
	~Queue();
	bool Full() { return count >= maxelements; }
	bool Add( float time )
	{
		if ( Full() )
			return false;
		if ( first )
		{
			current->next = new Element( time );
			current->next->previous = current;
			current = current->next;
			last = current;
		}
		else
		{
			first = new Element( time );
			first->next = NULL;
			first->previous = NULL;
			last = first;
			current = first;
		}
		++count;
		return true;
	}
	void Update( float time )
	{
		while ( last && last->flTimeCreated + m_flDuration < time )
		{
			Element *previous = last->previous;
			delete last;
			--count;
			last = previous;
			current = previous;
			if ( !previous )
			{
				first = NULL;
				current = NULL;
				break;
			}
		}
	}
	int count;
	int maxelements;
	float m_flDuration;
	Element *first;
	Element *last;
	Element *current;
};

extern Queue g_RubbleQueue;
extern CHud gHUD;
struct pmodel_fx_t
{
	bool bSwitch;
	int iSwitchSeq;
	int iSwitchFrame;
	bool bAnim;
	int iAnimSeq;
	int iAnimFrame;
	int iAnimTargetSeq;
};

extern pmodel_fx_t g_PModelFxInfo[65];
extern extra_player_info_t g_PlayerExtraInfo[65];
extern hud_player_info_t g_PlayerInfoList[65];
extern float g_fStamina;
extern float g_fUser4;
extern int g_iVuser1x;
extern int g_iVuser1z;
extern int g_iMovetype;
extern int g_iEffects;
extern int g_iOnlyClientDraw;
extern float g_lastFOV;
extern int g_iWeaponBits2;
extern int g_iWeaponFlags;
extern float g_flWeaponHeat;
extern float i_ProneCounter;
extern float flBoltHideXHair;
extern int g_iPlayerClass;
extern int g_iTeamNumber;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;

#endif
