//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#pragma once

#include "vgui/IScheme.h"

namespace vgui2
{
typedef unsigned int VPANEL;
}

struct VGuiLibraryPlayer_t
{
	const char*		name;
	int				ping;
	int				packetloss;
	bool			thisplayer;
	const char*		teamname;
	int				teamnumber;
	int				frags;
	int				deaths;
	int				playerclass;
	int				health;
	bool			dead;
	unsigned __int64	m_nSteamID;
};

struct VGuiLibraryTeamInfo_t
{
	const char*	name;
	int			frags;
	int			deaths;
	int			ping;
	int			players;
	int			packetloss;
	int			teamnumber;
};

struct VGuiLibraryInterface_t
{
	int ( *InIntermission )();
	void ( *FindNextPlayer )( bool reverse );
	void ( *FindPlayer )( const char* name );
	bool ( *PipInsetOff )();
	void ( *InsetValues )( int& x, int& y, int& wide, int& tall );
	void ( *CheckSettings )();
	int ( *SpectatorNumber )();
	bool ( *IsSpectator )();
	int ( *SpectatorMode )();
	int ( *SpectatorTarget )();
	bool ( *IsSpectateOnly )();
	float ( *HudTime )();
	void ( *MessageAdd )( void* message );
	void ( *MessageHud )( const char* name, int size, void* buffer );
	int ( *TeamPlay )();
	void ( *ClientCmd )( const char* command );
	int ( *TeamNumber )();
	const char* ( *GetLevelName )();
	int ( *GetLocalPlayerIndex )();
	char* ( *COM_ParseFile )( char* data, char* token );
	void* ( *COM_LoadFile )( char* path, int usehunk, int* length );
	void ( *COM_FreeFile )( void* buffer );
	void ( *COM_FileBase )( char* path, char* name );
	void ( *Con_DPrintf )( char* format, ... );
	VGuiLibraryPlayer_t ( *GetPlayerInfo )( int index );
	int ( *GetMaxPlayers )();
	void ( *GameVoice_StopSquelchMode )();
	bool ( *IsDemoPlayingBack )();
	float ( *CvarGetFloat )( const char* name );
};

class IViewPort
{
public:
	virtual VGuiLibraryInterface_t* GetClientDllInterface() = 0;
	virtual void SetClientDllInterface( VGuiLibraryInterface_t* callbacks ) = 0;

	//death.cpp
	virtual void UpdateScoreBoard() = 0;
	virtual bool AllowedToPrintText() = 0; // also saytext.cpp and text_message.cpp
	virtual void GetAllPlayersInfo() = 0;
	virtual void DeathMsg( int killer, int victim ) = 0;
	virtual void ShowScoreBoard() = 0;
	virtual bool CanShowScoreBoard() = 0;
	virtual void HideAllVGUIMenu() = 0;
	virtual void UpdateSpectatorPanel() = 0;

	// input.cpp
	virtual bool IsScoreBoardVisible() = 0; // also menu.cpp
	virtual void HideScoreBoard() = 0;
	virtual int KeyInput( int down, int keynum, const char* binding ) = 0;
	virtual void ShowVGUIMenu( int menu ) = 0;
	virtual void HideVGUIMenu( int menu ) = 0;
	virtual void ShowTutorTextWindow( const wchar_t* text, int id, int messageClass, int spectator ) = 0;
	virtual void ShowTutorLine( int entity, int id ) = 0;
	virtual void ShowTutorState( const wchar_t* text ) = 0;
	virtual void CloseTutorTextWindow() = 0;
	virtual bool IsTutorTextWindowOpen() const = 0;
	virtual void ShowSpectatorGUI() = 0;
	virtual void ShowSpectatorGUIBar() = 0;
	virtual void HideSpectatorGUI() = 0;
	virtual void DeactivateSpectatorGUI() = 0;
	virtual bool IsSpectatorGUIVisible() = 0;
	virtual bool IsSpectatorBarVisible() = 0;
	virtual int MsgFunc_ResetFade( const char* name, int size, void* buffer ) = 0;
	virtual void SetSpectatorBanner( const char* image ) = 0;
	virtual void SpectatorGUIEnableInsetView( int enabled ) = 0;
	virtual void ShowCommandMenu() = 0;
	virtual void UpdateCommandMenu() = 0;
	virtual void HideCommandMenu() = 0;
	virtual int IsCommandMenuVisible() = 0;
	virtual int GetValidClasses( int team ) = 0;
	virtual int GetNumberOfTeams() = 0;
	virtual bool GetIsFeigning() = 0;
	virtual int GetIsSettingDetpack() = 0;
	virtual int GetBuildState() = 0;
	virtual int IsRandomPC() = 0;
	virtual char* GetTeamName( int team ) = 0;
	virtual int GetCurrentMenuID() = 0;
	virtual const char* GetMapName() = 0;
	virtual const char* GetServerName() = 0;
	virtual void InputPlayerSpecial() = 0;
	virtual void OnTick() = 0;
	virtual vgui2::HScheme GetViewPortScheme() = 0;
	virtual vgui2::VPANEL GetViewPortPanel() = 0;
	virtual int GetAllowSpectators() = 0;
	virtual void OnLevelChange() = 0;
	virtual void HideBackGround() = 0;
	virtual void ChatInputPosition( int* x, int* y ) = 0;
	virtual int GetSpectatorBottomBarHeight() = 0;
	virtual int GetSpectatorTopBarHeight() = 0;
	virtual bool SlotInput( int slot ) = 0;
	virtual VGuiLibraryTeamInfo_t GetPlayerTeamInfo( int player ) = 0;
	virtual void MakeSafeName( const char* oldName, char* newName, int capacity ) = 0;
};

extern IViewPort* gViewPortInterface;
