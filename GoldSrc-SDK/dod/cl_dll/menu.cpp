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
//
// menu.cpp
//
// generic menu handler
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "../public/game_controls/iviewport.h"
#include "../../engine/controls.h"
#include "../../vgui2/controls/controls.h"
#include <stdlib.h>
#include <wchar.h>

#define MAX_MENU_STRING 512

char g_szMenuString[MAX_MENU_STRING];
char g_szPrelocalisedMenuString[MAX_MENU_STRING];


/*=================================
  ParseEscapeToken

  Interprets the given escape token (backslash followed by a letter). The
  first character of the token must be a backslash.  The second character
  specifies the operation to perform:

   \w : White text (this is the default)
   \d : Dim (gray) text
   \y : Yellow text
   \r : Red text
   \R : Right-align (just for the remainder of the current line)
=================================*/

static int menu_r, menu_g, menu_b, menu_x, menu_ralign;

extern int KB_ConvertString( char *text, char **output );

DECLARE_MESSAGE(m_Menu, ShowMenu);

int CHudMenu::Init()
{
	gHUD.AddHudElem(this);
	HOOK_MESSAGE(ShowMenu);
	InitHUDData();
	return 1;
}

void CHudMenu::InitHUDData()
{
	m_iFlags &= ~HUD_ACTIVE;
	m_fMenuDisplayed = 0;
	m_bitsValidSlots = 0;
	Reset();
}

void CHudMenu::Reset()
{
	g_szPrelocalisedMenuString[0] = 0;
	m_fWaitingForMore = 0;
}

int CHudMenu::VidInit()
{
	return 1;
}

int CHudMenu::Draw( float )
{
	if ( !(m_flShutoffTime <= 0 || gHUD.m_flTime < m_flShutoffTime) )
	{  // times up, shutoff
		m_fMenuDisplayed = 0;
		m_iFlags &= ~HUD_ACTIVE;
		return 1;
	}
	if ( gViewPortInterface != NULL && gViewPortInterface->IsScoreBoardVisible() )
		return 1;
	menu_r = menu_g = menu_b = 255;
	menu_x = 20;
	menu_ralign = 0;
	char stripped[MAX_MENU_STRING];
	char *text = g_szMenuString;
	int length = strlen(text);
	if ( text[length-1] == '\r' )
	{
		strcpy(stripped,text);
		stripped[length-1] = 0;
		text = stripped;
	}
	wchar_t *localized = vgui2::localize()->Find(text);
	int lines = 0;
	if ( localized != NULL )
	{
		for ( int i = 0; i < MAX_MENU_STRING && localized[i]; ++i )
			if ( localized[i] == L'\n' ) ++lines;
		int y = ScreenHeight/2+12*(lines/-2)-40;
		while ( *localized )
		{
			if ( *localized == L'\n' )
			{
				++localized;
				menu_x = 20;
				y += gHUD.m_VGUI2Print.GetHudFontHeight();
			}
			else
			{
				wchar_t *end = localized;
				while ( *end && *end != L'\n' && *end != L'\\' ) ++end;
				wchar_t line[80];
				size_t count = end-localized;
				wcsncpy(line,localized,min(wcslen(localized),count));
				line[min(count,sizeof(line)-1)] = 0;
				localized = end;
				menu_x = gHUD.m_VGUI2Print.DrawVGUI2String(line,menu_x,y,
					menu_r/255.0f,menu_g/255.0f,menu_b/255.0f);
			}
		}
		return 1;
	}
	for ( int i = 0; i < MAX_MENU_STRING && g_szMenuString[i]; ++i )
		if ( g_szMenuString[i] == '\n' ) ++lines;
	int y = ScreenHeight/2+12*(lines/-2)-40;
	while ( *text )
	{
		if ( *text == '\\' )
		{
			++text;
			switch ( *text )
			{
			case 0: return 1;
			case 'w': menu_r = menu_g = menu_b = 255; break;
			case 'd': menu_r = menu_g = menu_b = 100; break;
			case 'y': menu_r = 255; menu_g = 210; menu_b = 64; break;
			case 'r': menu_r = 210; menu_g = 24; menu_b = 0; break;
			case 'R': menu_x = ScreenWidth/2; menu_ralign = 1; break;
			}
			++text;
		}
		else if ( *text == '\n' )
		{
			menu_ralign = 0;
			menu_x = 20;
			y += gHUD.m_VGUI2Print.GetHudFontHeight();
			++text;
		}
		else
		{
			char *start = text;
			do
			{
				++text;
			}
			while ( *text && *text != '\n' && *text != '\\' );
			char line[80];
			int count = text-start;
			strncpy(line,start,min(count,(int)sizeof(line)));
			line[min(count,(int)sizeof(line)-1)] = 0;
			if ( menu_ralign )
				menu_x = gHUD.m_VGUI2Print.DrawVGUI2StringReverse(line,menu_x,y,
					menu_r/255.0f,menu_g/255.0f,menu_b/255.0f);
			else
				menu_x = gHUD.m_VGUI2Print.DrawVGUI2String(line,menu_x,y,
					menu_r/255.0f,menu_g/255.0f,menu_b/255.0f);
		}
	}
	return 1;
}

void CHudMenu::SelectMenuItem( int item )
{
	if ( item <= 0 || !(m_bitsValidSlots & (1 << (item-1))) )
		return;
	char command[36];
	sprintf(command,"menuselect %d\n",item);
	EngineClientCmd(command);
	m_iFlags &= ~HUD_ACTIVE;
	m_fMenuDisplayed = 0;
}


// Message handler for ShowMenu message
// takes four values:
//		short: a bitfield of keys that are valid input
//		char : the duration, in seconds, the menu should stay up. -1 means is stays until something is chosen.
//		byte : a boolean, TRUE if there is more string yet to be received before displaying the menu, FALSE if it's the last string
//		string: menu string to display
// if this message is never received, then scores will simply be the combined totals of the players.
int CHudMenu::MsgFunc_ShowMenu( const char *, int size, void *buffer )
{
	char *converted = NULL;
	BEGIN_READ(buffer,size);
	m_bitsValidSlots = READ_SHORT();
	int displayTime = READ_CHAR();
	int needMore = READ_BYTE();
	m_flShutoffTime = displayTime > 0 ? displayTime+gHUD.m_flTime : -1;
	if ( m_bitsValidSlots )
	{
		if ( m_fWaitingForMore )
			strncat(g_szPrelocalisedMenuString,READ_STRING(),sizeof(g_szPrelocalisedMenuString)-strlen(g_szPrelocalisedMenuString));
		else
			strncpy(g_szPrelocalisedMenuString,READ_STRING(),sizeof(g_szPrelocalisedMenuString));
		g_szPrelocalisedMenuString[sizeof(g_szPrelocalisedMenuString)-1] = 0;
		if ( !needMore )
		{
			strncpy(g_szMenuString,CHudTextMessage::BufferedLocaliseTextString(g_szPrelocalisedMenuString),sizeof(g_szMenuString));
			if ( KB_ConvertString(g_szMenuString,&converted) )
			{
				strncpy(g_szMenuString,converted,sizeof(g_szMenuString));
				free(converted);
			}
		}
		m_fMenuDisplayed = 1;
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		m_iFlags &= ~HUD_ACTIVE;
		m_fMenuDisplayed = 0;
	}
	m_fWaitingForMore = needMore;
	return 1;
}
