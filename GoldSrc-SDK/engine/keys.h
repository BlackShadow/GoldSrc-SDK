//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//=============================================================================

#pragma once

#include "const.h"
#include "filesystem.h"

#define KEY_BINDING_COUNT 256
#define KEY_HISTORY_LINES 32
#define KEY_CONSOLE_LINE_BYTES 256
#define KEY_CHAT_BUFFER_BYTES 120
#define KEY_MESSAGE_TYPE_BYTES 32

typedef enum
{
	key_game,
	key_message,
	key_menu
} keydest_t;

//
// these are the key numbers that should be passed to Key_Event
//
enum
{
	K_TAB = 9,
	K_ENTER = 13,
	K_ESCAPE = 27,
	K_SPACE = 32,

	// normal keys should be passed as lowercased ascii

	K_BACKSPACE = 127,
	K_UPARROW = 128,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,
	K_SHIFT,
	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_KP_HOME = 160,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_ENTER,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_MINUS,
	K_KP_PLUS,
	K_CAPSLOCK,
	K_KP_MUL,
	K_WIN,

	// joystick buttons
	K_JOY1 = 203,
	K_JOY2,
	K_JOY3,
	K_JOY4,

	// aux keys are for multi-buttoned joysticks to generate so they can use
	// the normal binding process
	K_AUX1,
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_AUX15,
	K_AUX16,
	K_AUX17,
	K_AUX18,
	K_AUX19,
	K_AUX20,
	K_AUX21,
	K_AUX22,
	K_AUX23,
	K_AUX24,
	K_AUX25,
	K_AUX26,
	K_AUX27,
	K_AUX28,
	K_AUX29,
	K_AUX30,
	K_AUX31,
	K_AUX32,

	//
	// mouse buttons generate virtual keys
	//
	K_MWHEELDOWN,
	K_MWHEELUP,
	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,

	K_PAUSE = 255
};

typedef struct
{
	char* name;
	int keynum;
} keyname_t;

extern keydest_t key_dest;
extern char* keybindings[KEY_BINDING_COUNT];
extern keyname_t keynames[];
extern int toggleconsole_key;

extern char key_lines[KEY_HISTORY_LINES][KEY_CONSOLE_LINE_BYTES];
extern int key_linepos;
extern int shift_down;

extern qboolean consolekeys[KEY_BINDING_COUNT]; // if true, can't be rebound while in console
extern qboolean menubound[KEY_BINDING_COUNT];	// if true, can't be rebound while in menu
extern int keyshift[KEY_BINDING_COUNT];			// key to map to if shift held down in console
extern qboolean keyGameUIdown[KEY_BINDING_COUNT];
extern qboolean keydown[KEY_BINDING_COUNT];
extern int key_repeats[KEY_BINDING_COUNT]; // if > 1, it is autorepeating

extern char chat_buffer[KEY_CHAT_BUFFER_BYTES];
extern int chat_bufferlen;
extern char message_type[KEY_MESSAGE_TYPE_BYTES];
extern qboolean message_type_is_privileged;

void			Key_Init();
void			Key_Shutdown();
void			Key_Event( int key, qboolean down );
void			Key_ClearStates();
void			Key_Console( int key );
void			Key_Message( int key );

void			Key_SetBinding( int keynum, char* binding );
const char*		Key_BindingForKey( int keynum );
const char*		Key_NameForBinding( const char* binding );
int				Key_StringToKeynum( char* name );
char*			Key_KeynumToString( int keynum );
int				Key_CountBindings();
void			Key_WriteBindings( FileHandle_t file );

void			Key_Bind_f();
void			Key_Unbind_f();
void			Key_Unbindall_f();
void			Key_Escape_f();

void			Cbuf_ReplaceSemiColons();
