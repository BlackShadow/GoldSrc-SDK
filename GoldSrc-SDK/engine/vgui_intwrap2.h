#pragma once

#include "vgui/IScheme.h"

vgui2::HFont	VGUI2_GetConsoleFont();
vgui2::HFont	VGUI2_GetCreditsFont();
void			VGUI2_Draw_Init();
void			VGUI2_Draw_SetTextColor( int red, int green, int blue );
void			VGUI2_Draw_SetTextColorAlpha( int red, int green, int blue, int alpha );
const wchar_t*	VGUI2_Find_String( const char* text );
int				VGUI2_DrawString( int x, int y, const char* text, unsigned int font );
int				VGUI2_DrawStringClient( int x, int y, const char* text, int red, int green, int blue );
int				VGUI2_DrawStringReverseClient( int x, int y, const char* text, int red, int green, int blue );
int				VGUI2_Draw_Character( int x, int y, int ch, unsigned int font );
int				VGUI2_Draw_CharacterAdd( int x, int y, int ch, int red, int green, int blue, unsigned int font );
int				VGUI2_Draw_StringLenW( const wchar_t* text, unsigned int font );
int				VGUI2_Draw_StringLen( const char* text, unsigned int font );
int				VGUI2_GetFontTall( unsigned int font );
int				VGUI2_GetFontWide( wchar_t ch, unsigned int font );
int				VGUI2_MessageFontInfo( short* widths, unsigned int font );
void			VGUI2_ResetCurrentTexture();

void			VGuiWrap2_Startup();
void			VGuiWrap2_Shutdown();
void			VGuiWrap2_Paint();
int				VGuiWrap2_CallEngineSurfaceAppHandler( void* event, void* userData );
int				VGuiWrap2_GetLocalizedStringLength( const char* label );
int				VGuiWrap2_UseVGUI1();
void*			VGuiWrap2_GetPanel();

void			VGuiWrap2_ReleaseMouse();
void			VGuiWrap2_GetMouse();
void			VguiWrap2_GetMouseDelta( int* x, int* y );
void			VGuiWrap2_SetVisible( int state );

int				VGuiWrap2_Key_Event( int down, int keynum, const char* binding );
int				VGuiWrap2_GameUIKeyPressed();
int				VGuiWrap2_IsGameUIVisible();
void			VGuiWrap2_HideGameUI();
void			VGuiWrap2_ShowDemoPlayer();
int				VGuiWrap2_IsInCareerMatch();
void*			VguiWrap2_GetCareerUI();

void			VGuiWrap2_ShowConsole();
void			VGuiWrap2_HideConsole();
int				VGuiWrap2_IsConsoleVisible();
void			VGuiWrap2_ClearConsole();
void			VGuiWrap2_ConPrintf( const char* message );
void			VGuiWrap2_ConDPrintf( const char* message );

void			VGuiWrap2_NotifyOfServerConnect( const char* game, int address, int port );
void			VGuiWrap2_NotifyOfServerDisconnect();
void			VGUI2_OnDisconnectFromServer( int failureCode );
void			ValidateCDKey( int force, int inConnect );

void			VGuiWrap2_LoadingStarted( const char* resourceType, const char* resourceName );
void			VGuiWrap2_LoadingFinished( const char* resourceType, const char* resourceName );
void			StartLoadingProgressBar( const char* loadingType, int numProgressPoints );
void			ContinueLoadingProgressBar( const char* loadingType, int progressPoint, float progressFraction );
void			StopLoadingProgressBar();
void			SetLoadingProgressBarStatusText( const char* statusText );
void			SetSecondaryProgressBar( float progress );
void			SetSecondaryProgressBarText( const char* statusText );

void			ConstructTutorMessageDecayBuffer( int* buffer, int bufferLength );
void			ProcessTutorMessageDecayBuffer( int* buffer, int bufferLength );
void			ResetTutorMessageDecayData();
