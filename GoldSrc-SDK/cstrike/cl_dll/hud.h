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
#include <vector>

#define RGB_YELLOWISH	0x00FFA000 //255,160,0
#define RGB_REDISH		0x00FF1010 //255,160,0
#define RGB_GREENISH		0x0000A000 //0,160,0
#define DHN_DRAWZERO		1
#define DHN_2DIGITS		2
#define DHN_3DIGITS		4
#define DHN_FILLZERO		8
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

typedef struct
{
	int x, y;
} POSITION;

typedef struct
{
	unsigned char r, g, b, a;
} RGBA;

typedef struct cvar_s cvar_t;
struct WEAPON;

//
//-----------------------------------------------------
//
class CHudBase
{
public:
	POSITION m_pos;
	int m_type;
	int m_iFlags; // active, moving, 
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
	short frags;
	short deaths;
	short team_id;
	int has_c4;
	int vip;
	Vector origin;
	float radarflash;
	int radarflashon;
	int radarflashes;
	short playerclass;
	short teamnumber;
	char teamname[16];
	bool dead;
	float showhealth;
	int health;
	char location[32];
	int sb_health;
	int sb_account;
	int has_defuse_kit;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

struct hostage_info_t
{
	bool dead;
	Vector origin;
	int health;
	float radarflash;
	int radarflashon;
	int radarflashes;
	char teamname[16];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
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
	void CalculateCrosshairColor();
	void CalculateCrosshairDrawMode();
	void CalculateCrosshairSize();

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
	int m_cvarR, m_cvarG, m_cvarB;
	int m_iCurrentCrosshair;
	bool m_bAdditive;
	int m_iCrosshairScaleBase;
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
	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[4];
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
	char m_szStatusText[1][128];
	char m_szStatusBar[1][128];
	int m_iStatusValues[8];
	int m_bReparseString; // set to TRUE whenever the m_szStatusBar needs to be recalculated
	float* m_pflNameColors[1];
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
	int GetDeathNoticeY();
	int MsgFunc_DeathMsg( const char* name, int size, void* buffer );
	int m_HUD_d_skull;  // sprite index of skull icon
	int m_iDeathNoticeGap;
	int m_iDeathNoticeTop;
	int m_iDeathTextGap;
	int m_iDeathNoticeGapRight;
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
	void SayTextPrint( const char* text, int size, int clientIndex, char* arg1, char* arg2, char* arg3, char* arg4 );
	int GetTextPrintY();
	void EnsureTextFitsInOneLineAndWrapIfHaveTo( int line );
	void FindAudMessage( const char* text, int clientIndex, int pitch );
	int MsgFunc_SendAudio( const char* name, int size, void* buffer );
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
	int MsgFunc_ArmorType( const char* name, int size, void* buffer );
	int GetArmorPercent() const;
	bool HasHelmet() const;
	int m_HUD_suit_empty;
	int m_HUD_suit_full;
	int m_HUD_suithelmet_empty;
	int m_HUD_suithelmet_full;
	int m_iBat;
	float m_fFade;
	int m_iArmorType;
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
	HSPRITE m_hSprite1, m_hSprite2, m_hBeam;
	wrect_t* m_prc1;
	wrect_t* m_prc2;
	wrect_t* m_prcBeam;
	float m_flBat;
	int m_iBat;
	int m_fOn;
	float m_fFade;
	int m_iWidth;		// width of the battery innards
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
	int text;
	int fadeBlend;
	float charTime;
	float fadeTime;
};

//
//-----------------------------------------------------
//

class CHudTextMessage : public CHudBase
{
public:
	int Init() override;
	static char* LocaliseTextString( const char* text, char* buffer, int size );
	static char* BufferedLocaliseTextString( const char* text );
	char* LookupString( const char* text, int* destination = NULL );
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
	int MsgFunc_HudTextPro( const char* name, int size, void* buffer );
	int MsgFunc_GameTitle( const char* name, int size, void* buffer );
	int MsgFunc_HudTextArgs( const char* name, int size, void* buffer );
	float FadeBlend( float fadein, float fadeout, float hold, float time );
	int XPosition( float x, int width, int totalWidth );
	int YPosition( float y, int height );
	void MessageAdd( client_textmessage_t* message );
	void Reset() override;
	int MessageAdd( const char* message, float time, int hintMessage = 0, unsigned int font = 0 );

private:
	enum
	{
		MESSAGE_ARG_LEN = 64,
		MAX_MESSAGE_ARGS = 4
	};
	typedef struct
	{
		client_textmessage_t* pMessage;
		unsigned int font;
		wchar_t args[MAX_MESSAGE_ARGS][MESSAGE_ARG_LEN];
		int numArgs;
	} client_message_t;
	void MessageDrawScan( client_message_t* message, float time, unsigned int font );
	void MessageScanStart();
	void MessageScanNextChar( unsigned int font );
	client_message_t m_pMessages[16];
	float m_startTime[16];
	message_parms_t m_parms;
	float m_gameTitleTime;
	client_textmessage_t* m_pGameTitle;
	int m_HUD_title_life;
	int m_HUD_title_half;
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
	void EnableIcon( char* name, unsigned char r, unsigned char g, unsigned char b, bool flash );
	void DisableIcon( char* name );
	bool InBuyZone() const;

private:
	enum
	{
		MAX_ICONSPRITENAME_LENGTH = 24,
		MAX_ICONSPRITES = 4
	};
	int m_iCrossWidth;
	int m_bFlashOn;
	float m_tmNextFlash;
	struct
	{
		char szSpriteName[MAX_ICONSPRITENAME_LENGTH];
		HSPRITE spr;
		wrect_t rc;
		unsigned char r, g, b;
		int bFlash;
	} m_IconList[MAX_ICONSPRITES];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

//
//-----------------------------------------------------
//
class CHudAccountBalance : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	void Reset() override;
	int Draw( float time ) override;
	int MsgFunc_Money( const char* name, int size, void* buffer );
	int MsgFunc_BlinkAcct( const char* name, int size, void* buffer );
	int GetBalance();
	int m_iAccount;
	int m_HUD_dollar;
	int m_HUD_minus;
	int m_HUD_plus;
	int m_iAccountDelta;
	float m_fFade;
	float m_fFadeFast;
	int m_bShowDelta;
	int m_iBlinkCount;
	float m_fBlinkTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudNightVision : public CHudBase
{
public:
	~CHudNightVision();
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_NVGToggle( const char* name, int size, void* buffer );
	void UserCmd_NVGAdjustUp();
	void UserCmd_NVGAdjustDown();
	int m_fOn;
	int m_iAlpha;
	int m_iScopeTextureID;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudScenarioStatus : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	int MsgFunc_Scenario( const char* name, int size, void* buffer );
	void Reset() override;
	HSPRITE m_hSprite;
	wrect_t m_rect;
	int m_alpha;
	float m_nextFlash;
	float m_flashInterval;
	int m_flashAlpha;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudSniperScope : public CHudBase
{
public:
	~CHudSniperScope();
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int m_iScopeTextureID;
	int m_iScopeNWTextureID;
	int m_iScopeNETextureID;
	int m_iScopeSWTextureID;
	int m_iScopeSETextureID;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudVGUI2Print : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int DrawVGUI2String( char* text, int x, int y, float r, float g, float b );
	int DrawVGUI2StringReverse( char* text, int x, int y, float r, float g, float b );
	int DrawVGUI2String( wchar_t* text, int x, int y, float r, float g, float b );
	int DrawVGUI2StringReverse( wchar_t* text, int x, int y, float r, float g, float b );
	void VGUI2HudPrintArgs( char* text, char* arg1, char* arg2, char* arg3, char* arg4,
		int x, int y, float r, float g, float b );
	void VGUI2HudPrint( char* text, int x, int y, float r, float g, float b );
	void VGUI2LocalizeArg( char* text, wchar_t* buffer, int size );
	int GetHudFontHeight();
	void GetStringSize( const wchar_t* text, int* width, int* height );
	int Draw( float time ) override;
	unsigned long m_hudfont;
	float m_flVGUI2StringTime;
	wchar_t m_wCharBuf[512];
	float m_fR, m_fG, m_fB;
	int m_iX, m_iY;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
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
	int m_iSpriteCount;
	int m_iSpriteCountAllRes;
	float m_flMouseSensitivity;
	int m_iConcussionEffect;

public:
	HSPRITE m_hsprCursor;
	float m_flTime;	   // the current client time
	float m_fOldTime;  // the time at which the HUD was last redrawn
	double m_flTimeDelta; // the difference between flTime and fOldTime
	Vector m_vecOrigin;
	Vector m_vecAngles;
	int m_iKeyBits;
	int m_iHideHUDDisplay;
	int m_iFOV;
	int m_Teamplay;
	int m_iRes;
	cvar_t* m_pCvarStealMouse;
	cvar_t* m_pCvarDraw;
	int m_PlayerFOV[MAX_PLAYERS];
	int m_iFontHeight;
	int m_iFontEngineHeight;
	cvar_t* default_fov;

private:
	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	HSPRITE* m_rghSprites;	/*[HUD_SPRITE_COUNT]*/			// the sprites loaded from hud.txt
	wrect_t* m_rgrcRects;	/*[HUD_SPRITE_COUNT]*/
	char* m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/

public:
	CHudAmmo m_Ammo;
	CHudHealth m_Health;
	CHudSpectator m_Spectator;
	CHudGeiger m_Geiger;
	CHudBattery m_Battery;
	CHudTrain m_Train;
	CHudFlashlight m_Flash;
	CHudMessage m_Message;
	CHudStatusBar m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText m_SayText;
	CHudMenu m_Menu;
	CHudAmmoSecondary m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;
	CHudAccountBalance m_accountBalance;
	CHudRoundTimer m_roundTimer;
	CHudProgressBar m_progressBar;
	CHudNightVision m_NightVision;
	CHudCareerTaskTimer m_careerTaskTimer;
	CHudScenarioStatus m_scenarioStatus;
	CHudSniperScope m_sniperScope;
	CHudVGUI2Print m_VGUI2Print;
	SCREENINFO m_scrinfo;
	int m_iWeaponBits;
	bool m_fPlayerDead;
	int m_iIntermission;
	int m_HUD_number_0;
	float m_flCheatCheckTime;
	float m_flExpensiveCheckTime;
	bool m_bRenderGunSmoke;
	float m_fGLTEXSORT;
	float m_flGLZMAX;
	bool m_bSoftwaremode;
	bool m_bShowTimer;
	bool m_autoBuyStringSent;
	bool m_rebuyStringSent;

private:
	std::vector<int> m_botVoiceEntities;

public:
	CHud();
	~CHud();
	int DrawHudNumber( int x, int y, int number, int r, int g, int b );
	int DrawHudNumber( int x, int y, int flags, int number, int r, int g, int b );
	int DrawHudString( int x, int y, int maxX, const char* text, int r, int g, int b );
	int DrawHudStringReverse( int x, int y, int minX, const char* text, int r, int g, int b );
	int DrawHudNumberString( int x, int y, int minX, int number, int r, int g, int b );
	int GetNumWidth( int number, int flags );
	void VGUI2HudPrint( char* text, int x, int y, int r, int g, int b );
	int IsGame( const char* game );
	HSPRITE GetSprite( int index ) { return index < 0 ? 0 : m_rghSprites[index]; }
	wrect_t& GetSpriteRect( int index ) { return m_rgrcRects[index]; }
	int GetSpriteWidth( int index ) { return m_rgrcRects[index].right - m_rgrcRects[index].left; }
	int GetSpriteHeight( int index ) { return m_rgrcRects[index].bottom - m_rgrcRects[index].top; }
	int GetSpriteIndex( const char* name );
	void Init();
	void VidInit();
	void Think();
	int Redraw( float time, int intermission );
	int UpdateClientData( client_data_t* data, float time );
	int MsgFunc_Damage( const char* name, int size, void* buffer );
	int MsgFunc_GameMode( const char* name, int size, void* buffer );
	int MsgFunc_Logo( const char* name, int size, void* buffer );
	int MsgFunc_ResetHUD( const char* name, int size, void* buffer );
	void MsgFunc_InitHUD( const char* name, int size, void* buffer );
	void MsgFunc_ViewMode( const char* name, int size, void* buffer );
	int MsgFunc_SetFOV( const char* name, int size, void* buffer );
	int MsgFunc_HLTV( const char* name, int size, void* buffer );
	int MsgFunc_SpecHealth( const char* name, int size, void* buffer );
	int MsgFunc_Concuss( const char* name, int size, void* buffer );
	int MsgFunc_ReloadSound( const char* name, int size, void* buffer );
	void MsgFunc_BombDrop( const char* name, int size, void* buffer );
	void MsgFunc_BombPickup( const char* name, int size, void* buffer );
	void MsgFunc_ADStop( const char* name, int size, void* buffer );
	void MsgFunc_HostagePos( const char* name, int size, void* buffer );
	void MsgFunc_HostageK( const char* name, int size, void* buffer );
	int MsgFunc_SpecHealth2( const char* name, int size, void* buffer );
	int MsgFunc_Brass( const char* name, int size, void* buffer );
	void MsgFunc_Fog( const char* name, int size, void* buffer );
	void MsgFunc_ShowTimer( const char* name, int size, void* buffer );
	void AddHudElem( CHudBase* element );
	float GetSensitivity();
	void AutoBuy();
	void SendAutoBuyString();
	void Rebuy();
	void SendRebuyString();
	void RunCredits();
	int MsgFunc_BotVoice( const char* name, int size, void* buffer );
};

extern CHud gHUD;
extern int g_iPlayerClass;
extern int g_iTeamNumber;
extern int g_iUser1, g_iUser2, g_iUser3;
extern hud_player_info_t g_PlayerInfoList[MAX_PLAYERS + 1];
extern extra_player_info_t g_PlayerExtraInfo[MAX_PLAYERS + 1];
extern hostage_info_t g_HostageInfo[25];

class Element
{
public:
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	Element( float time ) : flTimeCreated( time ) {}
	float flTimeCreated;
	Element* next;
	Element* previous;
};

class Queue
{
public:
	Queue() : count( 0 ), maxelements( 60 ), m_flDuration( 2 ), first( NULL ), last( NULL ), current( NULL ) {}
	~Queue();
	bool Full() { return count >= maxelements; }
	bool Add( float time )
	{
		if ( Full() )
			return false;
		Element* element = new Element( time );
		if ( !first )
		{
			first = last = current = element;
			element->next = element->previous = NULL;
		}
		else
		{
			current->next = element;
			element->previous = current;
			current = current->next;
			last = current;
		}
		count++;
		return true;
	}
	void Update( float time )
	{
		while ( last && time > last->flTimeCreated + m_flDuration )
		{
			Element* previous = last->previous;
			delete last;
			count--;
			last = current = previous;
			if ( !previous )
			{
				first = current = NULL;
				return;
			}
		}
	}
	int count;
	int maxelements;
	float m_flDuration;
	Element* first;
	Element* last;
	Element* current;
};

extern Queue g_ShellQueue;
extern Queue g_SmokePuffQueue;

#endif
