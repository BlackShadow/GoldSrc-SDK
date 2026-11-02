/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

#ifndef HEALTH_H
#define HEALTH_H

#define DMG_IMAGE_LIFE	2	// seconds that image is up
#define DMG_IMAGE_POISON	0
#define DMG_IMAGE_ACID	1
#define DMG_IMAGE_COLD	2
#define DMG_IMAGE_DROWN	3
#define DMG_IMAGE_BURN	4
#define DMG_IMAGE_NERVE	5
#define DMG_IMAGE_RAD		6
#define DMG_IMAGE_SHOCK	7
//tf defines
#define DMG_IMAGE_CALTROP	8
#define DMG_IMAGE_TRANQ	9
#define DMG_IMAGE_CONCUSS	10
#define DMG_IMAGE_HALLUC	11
#define NUM_DMG_TYPES	12

#ifndef DMG_BURN
#define DMG_BURN			( 1 << 3 )	// heat burned
#define DMG_FREEZE		( 1 << 4 )	// frozen
#define DMG_SHOCK		( 1 << 8 )	// electric shock
#define DMG_SONIC		( 1 << 9 )	// sound pulse shockwave
#define DMG_DROWN		( 1 << 14 )	// Drowning
#define DMG_NERVEGAS		( 1 << 16 )	// nerve toxins, very bad
#define DMG_POISON		( 1 << 17 )	// blood poisioning
#define DMG_RADIATION		( 1 << 18 )	// radiation exposure
#define DMG_ACID			( 1 << 20 )	// toxic chemicals or acid burns
#define DMG_SLOWBURN		( 1 << 21 )	// in an oven
#define DMG_SLOWFREEZE	( 1 << 22 )	// in a subzero freezer
#define DMG_MORTAR		( 1 << 23 )	// Hit by air raid (done to distinguish grenade from mortar)
#endif
#ifndef DMG_CALTROP
#define DMG_CALTROP		( 1 << 30 )
#endif
#ifndef DMG_HALLUC
#define DMG_HALLUC		( 1 << 31 )
#endif
#ifndef DMG_TRANQ
#define DMG_TRANQ		DMG_MORTAR
#endif
#ifndef DMG_CONCUSS
#define DMG_CONCUSS		DMG_SONIC
#endif

typedef struct
{
	float fExpire;
	float fBaseline;
	int x, y;
} DAMAGE_IMAGE;

//
//-----------------------------------------------------
//
class CHudHealth : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_Health( const char* name, int size, void* buffer );
	int MsgFunc_Damage( const char* name, int size, void* buffer );
	int MsgFunc_Money( const char* name, int size, void* buffer );
	int MsgFunc_Radar( const char* name, int size, void* buffer );
	int MsgFunc_Account( const char* name, int size, void* buffer );
	int MsgFunc_HealthInfo( const char* name, int size, void* buffer );
	int MsgFunc_ScoreAttrib( const char* name, int size, void* buffer );
	int MsgFunc_ClCorpse( const char* name, int size, void* buffer );
	int MsgFunc_ReceiveW( const char* name, int size, void* buffer );
	void UserCmd_TrackPlayer();
	void UserCmd_ClearPlayers();
	void UserCmd_DrawRadar();
	void UserCmd_HideRadar();
	void DrawRadarDot( int x, int y, float z, int size, int flags, int r, int g, int b, int alpha );
	void DrawRadar( float time );
	void DrawPlayerLocation();
	void GetAllPlayersInfo();
	void GetPainColor( int& r, int& g, int& b );
	int DrawPain( float time );
	int DrawDamage( float time );
	void CalcDamageDirection( Vector source );
	void UpdateTiles( float time, long damageBits );

	int m_iHealth;
	int m_HUD_dmg_bio;
	int m_HUD_cross;
	float m_fFade;
	float m_fAttackFront, m_fAttackRear, m_fAttackLeft, m_fAttackRight;
	bool m_bTrackArray[65];
	int m_iPlayerLastPointedAt;
	int m_iPlayerNum;
	bool m_bDrawRadar;
	HSPRITE m_hSprite;
	HSPRITE m_hDamage;
	wrect_t* m_hrad;
	wrect_t* m_hradopaque;
	HSPRITE m_hRadar;
	HSPRITE m_hRadaropaque;
	DAMAGE_IMAGE m_dmg[NUM_DMG_TYPES];
	int m_bitsDamage;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudRoundTimer : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_RoundTime( const char* name, int size, void* buffer );
	int GetClosestRight();
	float m_flTimeEnd;
	int m_HUD_stopwatch;
	float m_fFade;
	float m_flNewPeriod;
	float m_flNextToggle;
	int m_bPanicColor;
	int m_closestRight;
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
	int MsgFunc_BarTime2( const char* name, int size, void* buffer );
	float m_tmEnd;
	float m_tmStart;
	float m_tmNewPeriod;
	float m_tmNewElapsed;
	int m_width, m_height;
	int m_x0, m_y0, m_x1, m_y1;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHudCareerTaskTimer : public CHudBase
{
public:
	int Init() override;
	int VidInit() override;
	int Draw( float time ) override;
	void Reset() override;
	int MsgFunc_TaskTime( const char* name, int size, void* buffer );
	float m_flTimeEnd;
	int m_HUD_stopwatch;
	float m_fFade;
	float m_flNewPeriod;
	float m_flNextToggle;
	int m_bPanicColor;
	bool m_bTaskComplete;
	bool m_bCountdown;
	float m_flAlpha;
	float m_flFadeDelay;
	float m_flFadeStart;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

#endif
