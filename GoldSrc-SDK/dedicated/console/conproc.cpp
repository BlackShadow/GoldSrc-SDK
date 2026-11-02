#include "conproc.h"

#if defined( _WIN32 )

#include "../sys_ded.h"
#include "../../common/icommandline.h"

#include <ctype.h>
#include <process.h>
#include <stdlib.h>
#include <windows.h>

static HANDLE	heventDone;
static HANDLE	hfileBuffer;
static HANDLE	heventChildSend;
static HANDLE	heventParentSend;
static HANDLE	hStdout;
static HANDLE	hStdin;

/*
==============
SetConsoleCXCY

==============
*/
BOOL SetConsoleCXCY( HANDLE output, int cx, int cy )
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	COORD coordMax = GetLargestConsoleWindowSize( output );

	if ( cy > coordMax.Y )
		cy = coordMax.Y;
	if ( cx > coordMax.X )
		cx = coordMax.X;

	if ( !GetConsoleScreenBufferInfo( output, &info ) )
		return FALSE;

// height
	info.srWindow.Left = 0;
	info.srWindow.Right = (SHORT)( info.dwSize.X - 1 );
	info.srWindow.Top = 0;
	info.srWindow.Bottom = (SHORT)( cy - 1 );

	if ( cy < info.dwSize.Y )
	{
		if ( !SetConsoleWindowInfo( output, TRUE, &info.srWindow ) )
			return FALSE;

		info.dwSize.Y = (SHORT)cy;
		if ( !SetConsoleScreenBufferSize( output, info.dwSize ) )
			return FALSE;
	}
	else if ( cy > info.dwSize.Y )
	{
		info.dwSize.Y = (SHORT)cy;
		if ( !SetConsoleScreenBufferSize( output, info.dwSize ) )
			return FALSE;
		if ( !SetConsoleWindowInfo( output, TRUE, &info.srWindow ) )
			return FALSE;
	}

	if ( !GetConsoleScreenBufferInfo( output, &info ) )
		return FALSE;

// width
	info.srWindow.Left = 0;
	info.srWindow.Right = (SHORT)( cx - 1 );
	info.srWindow.Top = 0;
	info.srWindow.Bottom = (SHORT)( info.dwSize.Y - 1 );

	if ( cx < info.dwSize.X )
	{
		if ( !SetConsoleWindowInfo( output, TRUE, &info.srWindow ) )
			return FALSE;

		info.dwSize.X = (SHORT)cx;
		if ( !SetConsoleScreenBufferSize( output, info.dwSize ) )
			return FALSE;
	}
	else if ( cx > info.dwSize.X )
	{
		info.dwSize.X = (SHORT)cx;
		if ( !SetConsoleScreenBufferSize( output, info.dwSize ) )
			return FALSE;
		if ( !SetConsoleWindowInfo( output, TRUE, &info.srWindow ) )
			return FALSE;
	}

	return TRUE;
}

void* GetMappedBuffer( HANDLE file )
{
	return MapViewOfFile( file, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0 );
}

void ReleaseMappedBuffer( void* buffer )
{
	UnmapViewOfFile( buffer );
}

/*
==============
GetScreenBufferLines

==============
*/
BOOL GetScreenBufferLines( int* lines )
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	BOOL success = GetConsoleScreenBufferInfo( hStdout, &info );
	if ( success )
		*lines = info.dwSize.Y;
	return success;
}

/*
==============
SetScreenBufferLines

==============
*/
BOOL SetScreenBufferLines( int lines )
{
	return SetConsoleCXCY( hStdout, 80, lines );
}

BOOL ReadText( char* text, int beginLine, int endLine )
{
	COORD coord;
	coord.X = 0;
	coord.Y = (SHORT)beginLine;

	DWORD read;
	BOOL success = ReadConsoleOutputCharacterA( hStdout, text, 80 * ( endLine - beginLine + 1 ), coord, &read );
	if ( success )
		text[read] = '\0';
	return success;
}

/*
==============
CharToCode

==============
*/
int CharToCode( char c )
{
	char upper = (char)toupper( c );
	if ( c == '\r' )
		return 28;
	if ( isalpha( c ) )
		return 30 + upper - 65;
	if ( isdigit( c ) )
		return 1 + upper - 47;
	return c;
}

BOOL WriteText( const char* text )
{
	char* sz = (char*)text;
	INPUT_RECORD rec;
	DWORD written;

	while ( *sz )
	{
		// 13 is the code for a carriage return (\n) instead of 10.
		if ( *sz == '\n' )
			*sz = '\r';

		char upper = (char)toupper( *sz );
		rec.EventType = KEY_EVENT;
		rec.Event.KeyEvent.bKeyDown = TRUE;
		rec.Event.KeyEvent.wRepeatCount = 1;
		rec.Event.KeyEvent.wVirtualKeyCode = (WORD)upper;
		rec.Event.KeyEvent.wVirtualScanCode = (WORD)CharToCode( *sz );
		rec.Event.KeyEvent.uChar.AsciiChar = *sz;
		rec.Event.KeyEvent.uChar.UnicodeChar = (WCHAR)*sz;
		rec.Event.KeyEvent.dwControlKeyState = isupper( *sz ) ? 0x80 : 0;

		WriteConsoleInputA( hStdin, &rec, 1, &written );
		rec.Event.KeyEvent.bKeyDown = FALSE;
		WriteConsoleInputA( hStdin, &rec, 1, &written );
		++sz;
	}

	return TRUE;
}

unsigned __stdcall RequestProc( void* )
{
	HANDLE waitEvents[2];
	waitEvents[0] = heventParentSend;
	waitEvents[1] = heventDone;

	for ( ;; )
	{
		DWORD event = WaitForMultipleObjects( 2, waitEvents, FALSE, INFINITE );
		// heventDone fired, so we're exiting.
		if ( event == WAIT_OBJECT_0 + 1 )
			break;

		int* buffer = (int*)GetMappedBuffer( hfileBuffer );
		if ( !buffer )
		{
			Sys_Printf( "Request Proc:  Invalid -HFILE handle\n" );
			break;
		}

		switch ( buffer[0] )
		{
		case CCOM_WRITE_TEXT:
			buffer[0] = WriteText( (const char*)( buffer + 1 ) );
			break;

		case CCOM_GET_TEXT:
		{
			int beginLine = buffer[1];
			int endLine = buffer[2];
			buffer[0] = ReadText( (char*)( buffer + 1 ), beginLine, endLine );
			break;
		}

		case CCOM_GET_SCR_LINES:
			buffer[0] = GetScreenBufferLines( &buffer[1] );
			break;

		case CCOM_SET_SCR_LINES:
			buffer[0] = SetScreenBufferLines( buffer[1] );
			break;
		}

		ReleaseMappedBuffer( buffer );
		SetEvent( heventChildSend );
	}

	_endthreadex( 0 );
	return 0;
}

/*
==============
DeinitConProc

==============
*/
void DeinitConProc()
{
	if ( heventDone )
		SetEvent( heventDone );
}

void InitConProc()
{
	HANDLE file = NULL;
	HANDLE parent = NULL;
	HANDLE child = NULL;
	char* p;

	if ( cmdline->CheckParm( "-HFILE", &p ) && p )
		file = (HANDLE)(INT_PTR)atoi( p );
	if ( cmdline->CheckParm( "-HPARENT", &p ) && p )
		parent = (HANDLE)(INT_PTR)atoi( p );
	if ( cmdline->CheckParm( "-HCHILD", &p ) && p )
		child = (HANDLE)(INT_PTR)atoi( p );

	if ( !file || !parent || !child )
		return;

	Sys_Printf( "\n\nInitConProc:  Setting up external control.\n" );
	hfileBuffer = file;
	heventParentSend = parent;
	heventChildSend = child;

	// So we'll know when to go away.
	heventDone = CreateEventA( NULL, FALSE, FALSE, NULL );
	if ( !heventDone )
	{
		Sys_Printf( "InitConProc:  Couldn't create heventDone\n" );
		return;
	}

	unsigned thread;
	if ( !_beginthreadex( NULL, 0, RequestProc, NULL, 0, &thread ) )
	{
		CloseHandle( heventDone );
		Sys_Printf( "InitConProc:  Couldn't create third party thread\n" );
		return;
	}

	// save off the input/output handles.
	hStdout = GetStdHandle( STD_OUTPUT_HANDLE );
	hStdin = GetStdHandle( STD_INPUT_HANDLE );

	int height = 50;
	if ( cmdline->CheckParm( "-conheight", &p ) && p )
		height = atoi( p );
	// Force 80 character width, at least 25 character height
	SetConsoleCXCY( hStdout, 80, height );
}

#endif
