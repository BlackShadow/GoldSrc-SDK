#include "TextConsoleWin32.h"
#include "common/IBaseSystem.h"

#include <string.h>

#define MAX_CONSOLE_EVENTS	1024

static BOOL WINAPI HandlerRoutine( DWORD )
{
	return TRUE;
}

static HWND GetConsoleHwnd()
{
	char oldTitle[1024];
	char title[1024];

	GetConsoleTitleA( oldTitle, sizeof( oldTitle ) );
	DWORD processID = GetCurrentProcessId();
#pragma warning( suppress : 28159 )
	DWORD ticks = GetTickCount();
	wsprintfA( title, "%d/%d", (int)ticks, (int)processID );
	SetConsoleTitleA( title );
	Sleep( 40 );
	HWND window = FindWindowA( NULL, title );
	SetConsoleTitleA( oldTitle );
	return window;
}

bool CTextConsoleWin32::Init( IBaseSystem* system )
{
	if ( !AllocConsole() )
		m_System = system;

	SetTitle( m_System ? m_System->GetName() : "Console" );
	m_hInput = GetStdHandle( STD_INPUT_HANDLE );
	m_hOutput = GetStdHandle( STD_OUTPUT_HANDLE );

	if ( !SetConsoleCtrlHandler( HandlerRoutine, TRUE ) )
		Print( "WARNING! TextConsole::Init: Could not attach console hook.\n" );

	m_Attrib = BACKGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
	SetWindowPos( GetConsoleHwnd(), NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOOWNERZORDER );
	return CTextConsole::Init( system );
}

void CTextConsoleWin32::ShutDown()
{
	FreeConsole();
	CTextConsole::ShutDown();
}

void CTextConsoleWin32::SetTitle( char* title )
{
	SetConsoleTitleA( title );
}

void CTextConsoleWin32::SetStatusLine( char* status )
{
	strncpy( m_StatusLine, status, sizeof( m_StatusLine ) );
	m_StatusLine[sizeof( m_StatusLine ) - 1] = '\0';
	UpdateStatus();
}

void CTextConsoleWin32::UpdateStatus()
{
	WORD attributes[sizeof( m_StatusLine )];
	for ( int i = 0; i < sizeof( m_StatusLine ); i++ )
		attributes[i] = m_Attrib;

	DWORD written = 0;
	COORD origin = { 0, 0 };
	WriteConsoleOutputAttribute( m_hOutput, attributes, sizeof( m_StatusLine ), origin, &written );
	WriteConsoleOutputCharacterA( m_hOutput, m_StatusLine, sizeof( m_StatusLine ), origin, &written );
}

void CTextConsoleWin32::PrintRaw( char* text, int length )
{
	DWORD written;
	if ( length )
		WriteFile( m_hOutput, text, length, &written, NULL );
	else
		WriteFile( m_hOutput, text, (DWORD)strlen( text ), &written, NULL );
}

void CTextConsoleWin32::Echo( char* text, int length )
{
	PrintRaw( text, length );
}

char* CTextConsoleWin32::GetLine()
{
	while ( true )
	{
		INPUT_RECORD events[MAX_CONSOLE_EVENTS];
		DWORD count;
		DWORD pending;

		if ( !GetNumberOfConsoleInputEvents( m_hInput, &pending ) )
		{
			if ( m_System )
				m_System->Errorf( "CTextConsoleWin32::GetLine: !GetNumberOfConsoleInputEvents" );
			return NULL;
		}

		if ( !pending )
			break;

		if ( !ReadConsoleInputA( m_hInput, events, MAX_CONSOLE_EVENTS, &count ) )
		{
			if ( m_System )
				m_System->Errorf( "CTextConsoleWin32::GetLine: !ReadConsoleInput" );
			return NULL;
		}

		if ( !count )
			return NULL;

		for ( int i = 0; i < (int)count; i++ )
		{
			if ( events[i].EventType != KEY_EVENT || !events[i].Event.KeyEvent.bKeyDown )
				continue;

			KEY_EVENT_RECORD& key = events[i].Event.KeyEvent;
			switch ( key.wVirtualKeyCode )
			{
			case VK_UP:
				HandleKeyUp();
				break;
			case VK_DOWN:
				HandleKeyDown();
				break;
			case VK_LEFT:
				HandleKeyLeft();
				break;
			case VK_RIGHT:
				HandleKeyRight();
				break;
			default:
				char character = key.uChar.AsciiChar;
				switch ( character )
				{
				case '\b':
					HandleKeyBackspace();
					break;
				case '\t':
					HandleKeyTab();
					break;
				case '\r':
					if ( HandleKeyEnter() )
						return m_ConsoleLine;
					break;
				default:
					if ( character >= ' ' && character <= '~' )
						HandleKeyChar( character );
					break;
				}
				break;
			}
		}
	}

	return NULL;
}

int CTextConsoleWin32::GetWidth()
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	if ( !GetConsoleScreenBufferInfo( m_hOutput, &info ) || info.dwSize.X <= 1 )
		return 80;
	return info.dwSize.X;
}

void CTextConsoleWin32::SetVisible( bool visible )
{
	ShowWindow( GetConsoleHwnd(), visible ? SW_SHOW : SW_HIDE );
	m_Visible = visible;
}
