#pragma once

#include "render.h"
#include "draw.h"

#include "cvar.h"

extern vrect_t*	pconupdate;
extern vrect_t	scr_vrect;
extern int		sb_lines;
extern float	scr_con_current;
extern float	scr_conlines; // lines of console to display
extern float	oldscreensize;
extern float	oldfov;
extern float	scr_fov_value;
extern int		scr_fullupdate; // set to 0 to force full redraw
extern int		clearnotify; // set to 0 whenever notify text is drawn
extern int		clearconsole;
extern int		scr_copytop;
extern int		scr_copyeverything;
extern qboolean	scr_skipupdate;
extern qboolean	scr_skiponeupdate;
extern qboolean	scr_initialized;
extern qboolean	scr_drawloading;
extern qpic_t*	scr_ram;
extern qpic_t*	scr_net;
extern qpic_t*	scr_paused;
extern char		scr_centerstring[1024];
extern float	scr_centertime_start;
extern float	scr_centertime_off;
extern int		scr_center_lines;
extern int		scr_erase_lines;
extern int		scr_erase_center;
extern cvar_t	scr_viewsize;
extern cvar_t	scr_centertime;
extern cvar_t	scr_showpause;
extern cvar_t	scr_printspeed;
extern cvar_t	scr_graphheight;
extern cvar_t	scr_downloading;
extern cvar_t	scr_connectmsg;
extern cvar_t	scr_connectmsg1;
extern cvar_t	scr_connectmsg2;
extern cvar_t	gl_vsync;

void	SCR_SizeUp_f();
void	SCR_SizeDown_f();
void	SCR_ScreenShot_f();
void	SCR_CenterPrint( char* text );
void	SCR_EraseCenterString();
void	SCR_DrawCenterString();
void	SCR_CheckDrawCenterString();
void	SCR_Init();
void	SCR_DrawPause();
void	SCR_DrawLoading();
void	SCR_SetUpToDrawConsole();
void	SCR_DrawConsole();
void	WritePCXfile( char* filename, byte* data, int width, int height, int rowbytes, word* palette );
void	SCR_EndLoadingPlaque();
void	SCR_ConnectMsg();
void	SCR_UpdateScreen();
void	SCR_BeginLoadingPlaque( qboolean reconnect );
void	SCR_DrawDownloadText();

void	HudSizeUp();
void	HudSizeDown();
void	Sbar_Draw();
void	FillBackGround();
