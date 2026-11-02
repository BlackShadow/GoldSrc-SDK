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
// saytext.cpp
//
// implementation of CHudSayText class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "../public/game_controls/iviewport.h"
#include "../../engine/controls.h"
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#define MAX_LINES 5
#define MAX_CHARS_PER_LINE 256  /* it can be less than this, depending on char size */
#define LINE_START 10

// allow 20 pixels on either side of the text
#define MAX_LINE_WIDTH (ScreenWidth - 40)

extern float* GetClientColor( int client );

static wchar_t g_wcLineBuffer[MAX_LINES + 1][MAX_CHARS_PER_LINE];
static float* g_pflNameColors[MAX_LINES + 1];
static int g_iNameLengths[MAX_LINES + 1];
static float flScrollTime;
static int Y_START;
static int line_height;
float ColorBlue[3] = { 1, 0.7f, 0 };

DECLARE_MESSAGE( m_SayText, SayText );

void HudSayTextToggle()
{
	gHUD.m_SayText.m_HUD_saytext->value = gHUD.m_SayText.m_HUD_saytext->value == 0 ? 1.0f : 0.0f;
}

int CHudSayText::Init()
{
	gHUD.AddHudElem( this );
	HOOK_MESSAGE( SayText );
	InitHUDData();
	gEngfuncs.pfnAddCommand( "hud_saytext", HudSayTextToggle );
	m_HUD_saytext = CVAR_CREATE( "hud_saytext_internal", "1", 0 );
	m_HUD_saytext_time = CVAR_CREATE( "hud_saytext_time", "5", 0 );
	m_iFlags |= HUD_INTERMISSION; // is always drawn during an intermission
	return 1;
}

void CHudSayText::InitHUDData()
{
	memset( g_pflNameColors, 0, sizeof( g_pflNameColors ) );
	memset( g_iNameLengths, 0, sizeof( g_iNameLengths ) );
	memset( g_wcLineBuffer, 0, sizeof( g_wcLineBuffer ) );
}

int CHudSayText::VidInit()
{
	return 1;
}

int ScrollTextUp()
{
	char buffer[512];
	vgui2::localize()->ConvertUnicodeToANSI( g_wcLineBuffer[0], buffer, sizeof( buffer ) );
	strcat( buffer, "\n" );
	ConsolePrint( buffer );
	g_wcLineBuffer[MAX_LINES][0] = 0;
	for ( int i = 0; i < MAX_LINES; ++i )
		memcpy( g_wcLineBuffer[i], g_wcLineBuffer[i + 1], sizeof( g_wcLineBuffer[i] ) );
	memmove( g_pflNameColors, g_pflNameColors + 1, sizeof( g_pflNameColors ) - sizeof( g_pflNameColors[0] ) );
	memmove( g_iNameLengths, g_iNameLengths + 1, sizeof( g_iNameLengths ) - sizeof( g_iNameLengths[0] ) );
	g_wcLineBuffer[MAX_LINES - 1][0] = 0;
	if ( g_wcLineBuffer[0][0] == ' ' )
	{
		g_wcLineBuffer[0][0] = 2;
		return 1 + ScrollTextUp();
	}
	return 1;
}

int CHudSayText::Draw( float time )
{
	if ( (gViewPortInterface && !gViewPortInterface->AllowedToPrintText()) || !m_HUD_saytext->value )
		return 1;
	flScrollTime = min( flScrollTime, time + m_HUD_saytext_time->value );
	if ( time >= flScrollTime )
	{
		if ( g_wcLineBuffer[0][0] )
		{
			flScrollTime = time + m_HUD_saytext_time->value;
			// push the console up
			ScrollTextUp();
		}
		else
			m_iFlags &= ~HUD_ACTIVE;
	}
	DrawConsoleString( LINE_START, Y_START, "" );
	int red, green, blue;
	if ( sscanf( CVAR_GET_STRING( "con_color" ), "%i %i %i", &red, &green, &blue ) != 3 )
		return 1;
	float r = red / 255.0f;
	float g = green / 255.0f;
	float b = blue / 255.0f;
	int y = Y_START;
	wchar_t buffer[MAX_CHARS_PER_LINE];
	for ( int i = 0; i < MAX_LINES; i++, y += line_height )
	{
		if ( !g_wcLineBuffer[i][0] )
			continue;
		if ( g_wcLineBuffer[i][0] == 2 && g_pflNameColors[i] )
		{
			int nameLength = max( 0, min( g_iNameLengths[i], (int)wcslen( g_wcLineBuffer[i] ) ) );
			wcsncpy( buffer, g_wcLineBuffer[i], min( nameLength, 64 ) );
			buffer[min( nameLength, 63 )] = 0;
			int x = gHUD.m_VGUI2Print.DrawVGUI2String( buffer + 1, LINE_START, y,
				g_pflNameColors[i][0], g_pflNameColors[i][1], g_pflNameColors[i][2] );
			wcsncpy( buffer, g_wcLineBuffer[i] + nameLength, MAX_CHARS_PER_LINE - 1 );
			buffer[MAX_CHARS_PER_LINE - 1] = 0;
			gHUD.m_VGUI2Print.DrawVGUI2String( buffer, x, y, r, g, b );
		}
		else
			gHUD.m_VGUI2Print.DrawVGUI2String( g_wcLineBuffer[i], LINE_START, y, r, g, b );
	}
	return 1;
}

int CHudSayText::GetTextPrintY()
{
	int y;
	if ( !g_iUser1 && !gEngfuncs.IsSpectateOnly() )
		y = ScreenHeight - (int)(gHUD.m_iFontHeight * 2.75);
	else
		y = ScreenHeight - gViewPortInterface->GetSpectatorBottomBarHeight() - 4;
	return y - MAX_LINES * line_height - (int)(line_height * 0.5);
}

int CHudSayText::MsgFunc_SayText( const char*, int size, void* data )
{
	BEGIN_READ( data, size );
	int client = READ_BYTE();
	SayTextPrint( READ_STRING(), size - 1, client, NULL, NULL, NULL, NULL );
	return 1;
}

void CHudSayText::SayTextPrint( const char* text, int, int client, char* arg1, char* arg2, char* arg3, char* arg4 )
{
	if ( gViewPortInterface && !gViewPortInterface->AllowedToPrintText() )
	{
		// Print it straight to the console
		ConsolePrint( text );
		return;
	}
	int line;
	for ( line = 0; line < MAX_LINES; line++ )
	{
		if ( !g_wcLineBuffer[line][0] )
			break;
	}
	if ( line == MAX_LINES )
	{
		ScrollTextUp();
		line = MAX_LINES - 1;
	}
	g_iNameLengths[line] = 0;
	g_pflNameColors[line] = NULL;
	if ( gEngfuncs.IsSpectateOnly() && client == 0 )
		g_pflNameColors[line] = ColorBlue;
	bool colorName = text[0] == 2 && client > 0;
	char buffer[512];
	int length;
	while ( (length = strlen( text )) > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r') )
	{
		strncpy( buffer, text, sizeof( buffer ) );
		buffer[length - 1] = 0;
		text = buffer;
	}
	wchar_t temporary[256];
	wchar_t output[256];
	wchar_t* format = vgui2::localize()->Find( text );
	if ( !format )
	{
		vgui2::localize()->ConvertANSIToUnicode( text, temporary, sizeof( temporary ) );
		int outPosition = 0;
		int remaining = wcslen( temporary );
		wchar_t* word = NULL;
		bool token = false;
		for ( wchar_t* ch = temporary; *ch; ch++, remaining-- )
		{
			if ( token )
			{
				if ( iswspace( *ch ) || remaining == 1 )
				{
					wchar_t wordBuffer[128];
					int size = ch - word;
					wcsncpy( wordBuffer, word, size + 1 );
					wordBuffer[size + (remaining == 1 ? 1 : 0)] = 0;
					char term[128];
					vgui2::localize()->ConvertUnicodeToANSI( wordBuffer, term, sizeof( term ) );
					wchar_t* localized = vgui2::localize()->Find( term );
					if ( !localized )
						localized = wordBuffer;
					int count = min( (int)wcslen( localized ), 256 - outPosition );
					wcsncpy( output + outPosition, localized, count );
					outPosition += count;
					token = false;
					if ( remaining > 1 )
						output[outPosition++] = *ch;
				}
			}
			else if ( *ch == '#' )
			{
				word = ch;
				token = true;
			}
			else if ( outPosition < 256 )
				output[outPosition++] = *ch;
		}
		output[min( outPosition, 255 )] = 0;
		format = output;
	}
	char* arguments[4] = { arg1, arg2, arg3, arg4 };
	wchar_t argumentBuffers[4][64];
	wchar_t* wideArguments[4];
	for ( int i = 0; i < 4; i++ )
	{
		wideArguments[i] = vgui2::localize()->Find( arguments[i] );
		if ( !wideArguments[i] )
		{
			vgui2::localize()->ConvertANSIToUnicode( arguments[i], argumentBuffers[i], sizeof( argumentBuffers[i] ) );
			wideArguments[i] = argumentBuffers[i];
		}
	}
	vgui2::localize()->ConstructString( g_wcLineBuffer[line], sizeof( g_wcLineBuffer[line] ), format,
		4, wideArguments[0], wideArguments[1], wideArguments[2], wideArguments[3] );
	if ( colorName )
	{
		gEngfuncs.pfnGetPlayerInfo( client, &g_PlayerInfoList[client] );
		char* name = g_PlayerInfoList[client].name;
		wchar_t wideName[128];
		vgui2::localize()->ConvertANSIToUnicode( name, wideName, sizeof( wideName ) );
		if ( name )
		{
			wchar_t* found = wcsstr( g_wcLineBuffer[line], wideName );
			if ( found )
			{
				g_iNameLengths[line] = wcslen( wideName ) + found - g_wcLineBuffer[line];
				g_pflNameColors[line] = GetClientColor( client );
			}
		}
	}
	EnsureTextFitsInOneLineAndWrapIfHaveTo( line );
	if ( line == 0 )
		flScrollTime = gHUD.m_flTime + m_HUD_saytext_time->value;
	m_iFlags |= HUD_ACTIVE;
	Y_START = GetTextPrintY();
}

void CHudSayText::EnsureTextFitsInOneLineAndWrapIfHaveTo( int line )
{
	int width = 0;
	gHUD.m_VGUI2Print.GetStringSize( g_wcLineBuffer[line], &width, &line_height );
	if ( width + LINE_START <= MAX_LINE_WIDTH )
		return;
	// scan the string until we find what word is too long,  and wrap the end of the sentence after the word
	int length = LINE_START;
	wchar_t* lastBreak = NULL;
	for ( wchar_t* ch = g_wcLineBuffer[line]; *ch; ch++ )
	{
		// check for a color change, if so skip past it
		if ( ch[0] == '/' && ch[1] == '(' )
		{
			ch += 2;
			while ( *ch && *ch != ')' )
				ch++;
			if ( *ch )
				ch++;
			if ( !*ch )
				return;
		}
		if ( *ch == ' ' && ch != g_wcLineBuffer[line] )
			lastBreak = ch;
		wchar_t character[2] = { *ch, 0 };
		int characterWidth = 0;
		gHUD.m_VGUI2Print.GetStringSize( character, &characterWidth, &line_height );
		length += characterWidth;
		if ( length <= MAX_LINE_WIDTH )
			continue;
		if ( !lastBreak )
			lastBreak = ch - 1;
		int nextLine;
		do
		{
			for ( nextLine = 0; nextLine < MAX_LINES; nextLine++ )
			{
				if ( !g_wcLineBuffer[nextLine][0] )
					break;
			}
			if ( nextLine == MAX_LINES )
				lastBreak -= sizeof( g_wcLineBuffer[0] ) * ScrollTextUp();
		}
		while ( nextLine == MAX_LINES );
		if ( *lastBreak == ' ' )
		{
			if ( (int)(wcslen( g_wcLineBuffer[nextLine] ) - wcslen( lastBreak )) <= MAX_CHARS_PER_LINE )
				wcsncat( g_wcLineBuffer[nextLine], lastBreak,
					MAX_CHARS_PER_LINE - wcslen( g_wcLineBuffer[nextLine] ) - 1 );
		}
		else if ( wcslen( g_wcLineBuffer[nextLine] ) - wcslen( lastBreak ) - 2 < MAX_CHARS_PER_LINE )
		{
			wchar_t space[2];
			vgui2::localize()->ConvertANSIToUnicode( " ", space, sizeof( space ) );
			wcsncat( g_wcLineBuffer[nextLine], space,
				MAX_CHARS_PER_LINE - wcslen( g_wcLineBuffer[nextLine] ) - 1 );
			wcsncat( g_wcLineBuffer[nextLine], lastBreak,
				MAX_CHARS_PER_LINE - wcslen( g_wcLineBuffer[nextLine] ) - 1 );
		}
		*lastBreak = 0;
		EnsureTextFitsInOneLineAndWrapIfHaveTo( nextLine );
		return;
	}
}
