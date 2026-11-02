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
#include "global_consts.h"
#include "ammo.h"

#define RGB_YELLOWISH	0x00FFA000 //255,160,0
#define RGB_REDISH		0x00FF1010 //255,160,0
#define RGB_GREENISH		0x0000A000 //0,160,0
#define DHN_DRAWZERO		1
#define DHN_2DIGITS		2
#define DHN_3DIGITS		4
#define MIN_ALPHA		100
#define HUDELEM_ACTIVE	1
#define HUD_ACTIVE		1
#define HUD_INTERMISSION	2
#define MAX_PLAYER_NAME_LENGTH	32
#define MAX_MOTD_LENGTH	1536

//
//-----------------------------------------------------
//
#define MAX_SPRITE_NAME_LENGTH	24
#define MAX_HUD_MESSAGES	16

typedef struct
{
	int x, y;
} POSITION;

typedef struct
{
	unsigned char r, g, b, a;
} RGBA;

typedef struct cvar_s cvar_t;

//
//-----------------------------------------------------
//
class CHudBase
{
public:
	POSITION m_pos;
	int m_type;
	int m_iFlags; // active, moving, 
	virtual ~CHudBase() {}
	virtual int Init() { return 0; }
	virtual int VidInit() { return 0; }
	virtual int Draw( float ) { return 0; }
	virtual void Think() {}
	virtual void Reset() {}
	virtual void InitHUDData() {}
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

struct HUDLIST
{
	CHudBase* p;
	HUDLIST* pNext;
};

struct extra_player_info_t
{
	short frags, deaths, playerclass, health;
	bool dead; // UNUSED currently, spectator UI would like this
	short teamnumber;
	char teamname[MAX_TEAM_NAME];
};

struct team_info_t
{
	char name[MAX_TEAM_NAME];
	short frags, deaths, ping, packetloss, ownteam, players;
	int already_drawn, scores_overriden, teamnumber;
};

#include "health.h"
#include "hud_spectator.h"


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
	int DrawCrosshair( float time, int weaponID );
	int MsgFunc_CurWeapon( const char* name, int size, void* buffer );
	int MsgFunc_WeaponList( const char* name, int size, void* buffer );
	int MsgFunc_AmmoX( const char* name, int size, void* buffer );
	int MsgFunc_AmmoPickup( const char* name, int size, void* buffer );
	int MsgFunc_WeapPickup( const char* name, int size, void* buffer );
	int MsgFunc_ItemPickup( const char* name, int size, void* buffer );
	int MsgFunc_HideWeapon( const char* name, int size, void* buffer );
	int MsgFunc_Crosshair( const char* name, int size, void* buffer );
	int MsgFunc_AccesBits( const char* name, int size, void* buffer );
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
	void UserCmd_Adjust_Crosshair();

private:
	float m_fFade;
	RGBA m_rgba;
	WEAPON* m_pWeapon;
	int m_HUD_bucket0;
	int m_HUD_selection;
	HSPRITE m_hObserverCrosshair;
	wrect_t m_rcObserverCrosshair;
	int m_bObserverCrosshair;
	int m_iAmmoLastCheck;
	float m_flCrosshairDistance;
	int m_iAlpha;
	int m_R, m_G, m_B;
	int m_iCurrentCrosshair;
	float m_hudOffTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

//
//-----------------------------------------------------
//

class CHudAmmoSecondary : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	void Reset() override;
	int Draw( float time ) override;
	int MsgFunc_SecAmmoVal( const char* name, int size, void* buffer );
	int MsgFunc_SecAmmoIcon( const char* name, int size, void* buffer );

private:
	enum { MAX_SEC_AMMO_VALUES = 4 };
	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
	float m_fFade;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};


//
//-----------------------------------------------------
//
class CHudGeiger : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	int MsgFunc_Geiger( const char* name, int size, void* buffer );

private:
	int m_iGeigerRange;
#if defined( _MSC_VER )
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
	int MsgFunc_Train( const char* name, int size, void* buffer );

private:
	HSPRITE m_hSprite;
	int m_iPos;
#if defined( _MSC_VER )
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
	void ParseStatusString( int line );
	int MsgFunc_StatusText( const char* name, int size, void* buffer );
	int MsgFunc_StatusValue( const char* name, int size, void* buffer );

protected:
	enum { MAX_STATUSTEXT_LENGTH = 128, MAX_STATUSBAR_VALUES = 8, MAX_STATUSBAR_LINES = 2 };
	char m_szStatusText[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];  // a text string describing how the status bar is to be drawn
	char m_szStatusBar[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];	// the constructed bar that is drawn
	int m_iStatusValues[MAX_STATUSBAR_VALUES];  // an array of values for use in the status bar
	int m_bReparseString; // set to TRUE whenever the m_szStatusBar needs to be recalculated

	// an array of colors...one color for each line
	float* m_pflNameColors[MAX_STATUSBAR_LINES];
#if defined( _MSC_VER )
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
	void InitHUDData() override;
	int VidInit() override;
	int Draw( float time ) override;
	int MsgFunc_DeathMsg( const char* name, int size, void* buffer );

private:
	int m_HUD_d_skull;  // sprite index of skull icon
#if defined( _MSC_VER )
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
	void InitHUDData() override;
	int VidInit() override;
	void Reset() override;
	int Draw( float time ) override;
	int MsgFunc_ShowMenu( const char* name, int size, void* buffer );
	void SelectMenuItem( int item );
	int m_fMenuDisplayed;
	int m_bitsValidSlots;
	float m_flShutoffTime;
	int m_fWaitingForMore;
#if defined( _MSC_VER )
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
	void InitHUDData() override;
	int VidInit() override;
	int Draw( float time ) override;
	int MsgFunc_SayText( const char* name, int size, void* buffer );
	void SayTextPrint( const char* text, int size, int clientIndex = -1 );
	void EnsureTextFitsInOneLineAndWrapIfHaveTo( int line );
	friend class CHudSpectator;

private:
	cvar_t* m_HUD_saytext;
	cvar_t* m_HUD_saytext_time;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

//
//-----------------------------------------------------
//
class CHudBattery : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	int MsgFunc_Battery( const char* name, int size, void* buffer );

private:
	HSPRITE m_hSprite1, m_hSprite2;
	wrect_t* m_prc1;
	wrect_t* m_prc2;
	int m_iBat;
	float m_fFade;
	int m_iHeight;		// width of the battery innards
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};


//
//-----------------------------------------------------
//
class CHudFlashlight : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_Flashlight( const char* name, int size, void* buffer );
	int MsgFunc_FlashBat( const char* name, int size, void* buffer );

private:
	HSPRITE m_hSprite1, m_hSprite2, m_hBeam;
	wrect_t* m_prc1;
	wrect_t* m_prc2;
	wrect_t* m_prcBeam;
	float m_flBat;
	int m_iBat;
	int m_fOn;
	float m_fFade;
	int m_iWidth;		// width of the battery innards
	float m_iconAlpha, m_nextBlinkTime, m_blinkStopTime;
	bool m_blinkIcon;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudNightVision : public CHudBase
{
public:
	bool IsOn() { return m_On; }
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_NightVision( const char* name, int size, void* buffer );

private:
	bool m_On;
	float m_Fade;
	wrect_t* m_prc1;
	HSPRITE m_hSprite1;
	float m_iconAlpha, m_nextBlinkTime, m_blinkStopTime;
	bool m_blinkIcon;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudSniperLaser : public CHudBase
{
public:
	bool IsOn() { return m_On; }
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_SniperLaser( const char* name, int size, void* buffer );

private:
	bool m_On;
	float m_Fade;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudLevelLocks : public CHudBase
{
public:
	int Init() override;
	int VidInit() override { return 1; }
	int Draw( float ) override { return 1; }
	void Reset() override {}
	int MsgFunc_LevelLocks( const char* name, int size, void* buffer );
};

class CHudFiberCamera : public CHudBase
{
public:
	~CHudFiberCamera() override;
	bool IsOn() { return m_On; }
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_FiberCamera( const char* name, int size, void* buffer );
	bool firstTime;
	Vector viewOrigin, viewAngles;

private:
	bool m_On;
	wrect_t *m_hTL, *m_hT, *m_hTR, *m_hL, *m_hR, *m_hBL, *m_hB, *m_hBR;
	wrect_t *m_hTL2, *m_hT2, *m_hTR2, *m_hL2, *m_hR2, *m_hBL2, *m_hB2, *m_hBR2;
	wrect_t *m_hTL3, *m_hT3, *m_hTR3, *m_hL3, *m_hR3, *m_hBL3, *m_hB3, *m_hBR3;
	wrect_t *m_hHor, *m_hVer;
	HSPRITE m_hTopLeftSmall, m_hTopRightSmall, m_hBottomLeftSmall, m_hBottomRightSmall;
	HSPRITE m_hTopLeft, m_hTop, m_hTopRight, m_hLeft, m_hRight, m_hBottomLeft, m_hBottom, m_hBottomRight;
	HSPRITE m_hHorizontal, m_hVertical;
	HSPRITE m_hTopLeft2, m_hTop2, m_hTopRight2, m_hLeft2, m_hRight2, m_hBottomLeft2, m_hBottom2, m_hBottomRight2;
	HSPRITE m_hHorizontal2, m_hVertical2;
	HSPRITE m_hTopLeft3, m_hTop3, m_hTopRight3, m_hLeft3, m_hRight3, m_hBottomLeft3, m_hBottom3, m_hBottomRight3;
	HSPRITE m_hHorizontal3, m_hVertical3;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudIcons : public CHudBase
{
public:
	bool HasIcons() { return m_icons != 0; }
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_HudIcons( const char* name, int size, void* buffer );

private:
	int m_icons, m_iconsCorrectWeapon, m_iconsInPosition;
	HSPRITE m_hBlowtorchSprite, m_hBriefcaseSprite, m_hCameraSprite, m_hFiberCameraSprite;
	HSPRITE m_hRadioSprite, m_hRadioBombSprite, m_hStealthSprite, m_hSyringeSprite, m_hZiplineSprite;
	HSPRITE m_hBombDefuseSprite, m_hHostageZoneSprite;
	wrect_t *m_hBlowtorchSpriteRect, *m_hBriefcaseSpriteRect, *m_hCameraSpriteRect, *m_hFiberCameraSpriteRect;
	wrect_t *m_hRadioSpriteRect, *m_hRadioBombSpriteRect, *m_hStealthSpriteRect, *m_hSyringeSpriteRect;
	wrect_t *m_hZiplineSpriteRect, *m_hBombDefuseSpriteRect, *m_hHostageZoneSpriteRect;
#if defined( _MSC_VER )
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
	int MsgFunc_NVGToggle( const char* name, int size, void* buffer );
	void UserCmd_NVGAdjustUp();
	void UserCmd_NVGAdjustDown();
	~CHudScope() override;
	int m_fOn;

private:
	int m_iAlpha;
	wrect_t *m_hTL, *m_hT, *m_hTR, *m_hL, *m_hR, *m_hBL, *m_hB, *m_hBR;
	wrect_t *m_hTL2, *m_hT2, *m_hTR2, *m_hL2, *m_hR2, *m_hBL2, *m_hB2, *m_hBR2;
	wrect_t *m_hTL3, *m_hT3, *m_hTR3, *m_hL3, *m_hR3, *m_hBL3, *m_hB3, *m_hBR3;
	wrect_t *m_hHor, *m_hVer;
	HSPRITE m_hTopLeftSmall, m_hTopRightSmall, m_hBottomLeftSmall, m_hBottomRightSmall;
	HSPRITE m_hTopLeft, m_hTop, m_hTopRight, m_hLeft, m_hRight, m_hBottomLeft, m_hBottom, m_hBottomRight;
	HSPRITE m_hHorizontal, m_hVertical;
	HSPRITE m_hTopLeft2, m_hTop2, m_hTopRight2, m_hLeft2, m_hRight2, m_hBottomLeft2, m_hBottom2, m_hBottomRight2;
	HSPRITE m_hHorizontal2, m_hVertical2;
	HSPRITE m_hTopLeft3, m_hTop3, m_hTopRight3, m_hLeft3, m_hRight3, m_hBottomLeft3, m_hBottom3, m_hBottomRight3;
	HSPRITE m_hHorizontal3, m_hVertical3;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudProgressBar : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_BarTime( const char* name, int size, void* buffer );

private:
	float m_tmEnd, m_tmStart, m_tmNewPeriod;
	int m_width, m_height, m_x0, m_y0, m_x1, m_y1;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

struct message_parms_t
{
	client_textmessage_t* pMessage;
	float time;
	int x, y;
	int totalWidth, totalHeight;
	int width, lines, lineLength, length;
	int r, g, b;
	int text, fadeBlend;
	float charTime, fadeTime;
};

//
//-----------------------------------------------------
//

class CHudTextMessage : public CHudBase
{
public:
	int Init() override;
	static char* LocaliseTextString( const char* message, char* buffer, int size );
	static char* BufferedLocaliseTextString( const char* message );
	char* LookupString( const char* name, int* destination = NULL );
	int MsgFunc_TextMsg( const char* name, int size, void* buffer );
};

//
//-----------------------------------------------------
//

class CHudMessage : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	int MsgFunc_HudText( const char* name, int size, void* buffer );
	int MsgFunc_GameTitle( const char* name, int size, void* buffer );
	float FadeBlend( float fadein, float fadeout, float hold, float localTime );
	int XPosition( float x, int width, int lineWidth );
	int YPosition( float y, int height );
	void MessageAdd( const char* name, float time );
	void MessageAdd( client_textmessage_t* message );
	void MessageDrawScan( client_textmessage_t* message, float time );
	void MessageScanStart();
	void MessageScanNextChar();
	void Reset() override;

private:
	client_textmessage_t* m_pMessages[MAX_HUD_MESSAGES];
	float m_startTime[MAX_HUD_MESSAGES];
	message_parms_t m_parms;
	float m_gameTitleTime;
	client_textmessage_t* m_pGameTitle;
	int m_HUD_title_life, m_HUD_title_half;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudStatusIcons : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	void Reset() override;
	int Draw( float time ) override;
	int MsgFunc_StatusIcon( const char* name, int size, void* buffer );
	enum { MAX_ICONSPRITENAME_LENGTH = MAX_SPRITE_NAME_LENGTH, MAX_ICONSPRITES = 4 };
	void EnableIcon( char* name, unsigned char r, unsigned char g, unsigned char b );
	void DisableIcon( char* name );

private:
	typedef struct
	{
		char szSpriteName[MAX_ICONSPRITENAME_LENGTH];
		HSPRITE spr;
		wrect_t rc;
		unsigned char r, g, b;
	} icon_sprite_t;
	icon_sprite_t m_IconList[MAX_ICONSPRITES];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

//
//-----------------------------------------------------
//
class CHudBenchmark : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void SetScore( float score );
	void Think() override;
	void StartNextSection( int section );
	int MsgFunc_Bench( const char* name, int size, void* buffer );
	void CountFrame( float time );
	int GetObjects() { return m_nObjects; }
	void SetCompositeScore();
	void Restart();
	int Bench_ScoreForValue( int stage, float raw );

private:
	float m_fDrawTime, m_fDrawScore, m_fAvgScore;
	float m_fSendTime, m_fReceiveTime;
	int m_nFPSCount;
	float m_fAverageFT, m_fAvgFrameRate;
	int m_nSentFinish;
	float m_fStageStarted;
	float m_StoredLatency, m_StoredPacketLoss;
	int m_nStoredHopCount, m_nTraceDone, m_nObjects, m_nScoreComputed, m_nCompositeScore;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudParticles : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	void Reset() override;
	int Draw( float time ) override;
	int MsgFunc_Particles( const char* name, int size, void* buffer );
	int MsgFunc_PartState( const char* name, int size, void* buffer );
	int MsgFunc_TmpSprite( const char* name, int size, void* buffer );
	int MsgFunc_BeamPts( const char* name, int size, void* buffer );
};

class CHudWorldMap : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Think() override;
	int MsgFunc_MapInfo( const char* name, int size, void* buffer );
	void UserCmd_ShowMap();
	void UserCmd_HideMap();
};

//
//-----------------------------------------------------
//


class CHud
{
private:
	HUDLIST* m_pHudList;
	HSPRITE m_hsprLogo;
	int m_iLogo;
	client_sprite_t* m_pSpriteList;
	int m_iSpriteCount, m_iSpriteCountAllRes;
	float m_flMouseSensitivity;
	int m_iConcussionEffect;
	int m_securityCamera;
	float m_flMaterialCheckTime;
	char m_szLastBSP[64];

public:
	HSPRITE m_hsprCursor;
	float m_flTime, m_fOldTime;
	double m_flTimeDelta; // the difference between flTime and fOldTime
	Vector m_vecOrigin, m_vecAngles;
	int m_iKeyBits, m_iHideHUDDisplay, m_iFOV, m_Teamplay, m_iRes;
	cvar_t* m_pCvarStealMouse;
	cvar_t* m_pCvarDraw;
	int m_levelLockBits;
	int m_iFontHeight;

	bool DrawHudNightVision( float time );
	bool DrawHudSniperLaser( float time );
	bool DrawHudFiberCamera( float time );
	bool DrawHudScoreboardAndTakeScreenshots( float time, int intermission );
	bool DrawHudElements( float time, int intermission );
	bool DrawHudDemoLogo( float time );
	void DrawHudParticles( float time );
	bool DrawHudSecurityCamera( float time );
	int DrawHudNumber( int x, int y, int flags, int number, int r, int g, int b );
	int DrawHudString( int x, int y, int maxX, char* text, int r, int g, int b );
	int DrawHudStringReverse( int x, int y, int minX, char* text, int r, int g, int b );
	int DrawHudNumberString( int x, int y, int minX, int number, int r, int g, int b );
	int GetNumWidth( int number, int flags );
	bool IsLevelLocked( int level );

private:
	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	HSPRITE* m_rghSprites;	/*[HUD_SPRITE_COUNT]*/			// the sprites loaded from hud.txt
	wrect_t* m_rgrcRects;	/*[HUD_SPRITE_COUNT]*/
	char* m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/
	cvar_t* default_fov;

public:
	HSPRITE GetSprite( int index ) { return index < 0 ? 0 : m_rghSprites[index]; }
	wrect_t& GetSpriteRect( int index ) { return m_rgrcRects[index]; }
	int GetSpriteIndex( const char* name );

	CHudAmmo m_Ammo;
	CHudHealth m_Health;
	CHudSpectator m_Spectator;
	CHudGeiger m_Geiger;
	CHudBattery m_Battery;
	CHudTrain m_Train;
	CHudFlashlight m_Flash;
	CHudNightVision m_NightVision;
	CHudSniperLaser m_SniperLaser;
	CHudFiberCamera m_FiberCamera;
	CHudIcons m_HudIcons;
	CHudMessage m_Message;
	CHudStatusBar m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText m_SayText;
	CHudMenu m_Menu;
	CHudAmmoSecondary m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;
	CHudBenchmark m_Benchmark;
	CHudParticles m_Particles;
	CHudProgressBar m_progressBar;
	CHudLevelLocks m_levelLocks;
	CHudWorldMap m_WorldMap;
	CHudScope m_scope;

	void Init();
	void VidInit();
	void Think();
	int Redraw( float time, int intermission );
	int UpdateClientData( client_data_t* data, float time );
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	CHud() : m_pHudList( NULL ), m_iSpriteCount( 0 ), m_securityCamera( 0 ) {}
	~CHud();
	int MsgFunc_Damage( const char* name, int size, void* buffer );
	int MsgFunc_GameMode( const char* name, int size, void* buffer );
	int MsgFunc_Logo( const char* name, int size, void* buffer );
	int MsgFunc_ResetHUD( const char* name, int size, void* buffer );
	void MsgFunc_InitHUD( const char* name, int size, void* buffer );
	void MsgFunc_ViewMode( const char* name, int size, void* buffer );
	int MsgFunc_SetFOV( const char* name, int size, void* buffer );
	int MsgFunc_Concuss( const char* name, int size, void* buffer );
	int MsgFunc_SecurityCam( const char* name, int size, void* buffer );

	// Screen information
	SCREENINFO m_scrinfo;
	int m_iWeaponBits, m_iAccessoryBits, m_fPlayerDead, m_iIntermission;

	// sprite indexes
	int m_HUD_number_0;
	void AddHudElem( CHudBase* element );
	float GetSensitivity();
	float m_flCheatCheckTime, m_flExpensiveCheckTime;
	bool m_bRenderGunSmoke;
	float m_fGLTEXSORT, m_flGLZMAX;
	bool m_bSoftwaremode;
};

class Element
{
public:
	float flTimeCreated;
	Element* next;
	Element* previous;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	Element( float time ) : flTimeCreated( time ) {}
};

class Queue
{
public:
	int count, maxelements;
	float m_flDuration;
	Element *first, *last, *current;
	Queue() : count( 0 ), maxelements( 60 ), m_flDuration( 2 ), first( NULL ), last( NULL ), current( NULL ) {}
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
			last = current = first;
		}
		++count;
		return true;
	}
	void Update( float time )
	{
		while ( last && last->flTimeCreated + m_flDuration < time )
		{
			Element* previous = last->previous;
			delete last;
			--count;
			last = current = previous;
			if ( !previous )
				first = current = NULL;
		}
	}
};

extern CHud gHUD;
extern Queue g_ShellQueue, g_SmokePuffQueue;
extern int g_iPlayerClass, g_iTeamNumber, g_iUser1, g_iUser2, g_iUser3;
extern hud_player_info_t g_PlayerInfoList[MAX_PLAYERS + 1];
extern extra_player_info_t g_PlayerExtraInfo[MAX_PLAYERS + 1];

void ScaleColors( int& r, int& g, int& b, int a );
float HUD_GetFOV();
int HUD_IsGame( const char* game );
void COM_FileBase( const char* path, char* name );

#endif
