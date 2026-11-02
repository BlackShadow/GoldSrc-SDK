#include "TextConsole.h"
#include "System.h"
#include "common/IBaseSystem.h"
#include "../../common/ObjectList.h"

#include <cstdio>
#include <cstring>

CTextConsole::~CTextConsole()
{
}

bool CTextConsole::Init( IBaseSystem* system )
{
	m_System = system;
	memset( m_ConsoleLine, 0, sizeof( m_ConsoleLine ) );
	m_ConsoleLineLen = 0;
	m_CursorPosition = 0;
	memset( m_SavedConsoleLine, 0, sizeof( m_SavedConsoleLine ) );
	m_SavedConsoleLineLen = 0;
	memset( m_History, 0, sizeof( m_History ) );
	m_HistoryCount = 0;
	m_NextHistoryLine = 0;
	m_BrowseLine = 0;

	System_Printf( "\n" );
	System_Printf( "Console initialized.\n" );
	m_Visible = true;
	return true;
}

void CTextConsole::ShutDown()
{
}

void CTextConsole::Print( char* text )
{
	for ( int i = m_ConsoleLineLen; i != 0; --i )
		PrintRaw( "\b \b" );

	PrintRaw( text );
	if ( m_ConsoleLineLen )
		PrintRaw( m_ConsoleLine, m_ConsoleLineLen );

	UpdateStatus();
}

void CTextConsole::SetTitle( char* )
{
}

void CTextConsole::SetStatusLine( char* )
{
}

void CTextConsole::UpdateStatus()
{
}

void CTextConsole::SetVisible( bool visible )
{
	m_Visible = visible;
}

bool CTextConsole::IsVisible()
{
	return m_Visible;
}

void CTextConsole::HandleKeyBackspace()
{
	if ( !m_CursorPosition )
		return;

	--m_ConsoleLineLen;
	--m_CursorPosition;
	Echo( "\b" );
	for ( int i = m_CursorPosition; i < m_ConsoleLineLen; ++i )
	{
		m_ConsoleLine[i] = m_ConsoleLine[i + 1];
		Echo( &m_ConsoleLine[i], 1 );
	}

	Echo( " " );
	for ( int i = m_ConsoleLineLen; i >= m_CursorPosition; --i )
		Echo( "\b" );

	m_BrowseLine = m_NextHistoryLine;
}

void CTextConsole::HandleKeyDown()
{
	if ( m_BrowseLine == m_NextHistoryLine )
		return;

	if ( ++m_BrowseLine > m_HistoryCount )
		m_BrowseLine = 0;

	while ( m_ConsoleLineLen-- )
		Echo( "\b \b" );

	if ( m_BrowseLine == m_NextHistoryLine )
	{
		if ( m_SavedConsoleLineLen > 0 )
		{
			strncpy( m_ConsoleLine, m_SavedConsoleLine, m_SavedConsoleLineLen );
			Echo( m_ConsoleLine, m_SavedConsoleLineLen );
		}
		m_ConsoleLineLen = m_SavedConsoleLineLen;
	}
	else
	{
		Echo( m_History[m_BrowseLine] );
		strncpy( m_ConsoleLine, m_History[m_BrowseLine], sizeof( m_ConsoleLine ) );
		m_ConsoleLineLen = (int)strlen( m_History[m_BrowseLine] );
	}

	m_CursorPosition = m_ConsoleLineLen;
}

void CTextConsole::HandleKeyLeft()
{
	if ( !m_CursorPosition )
		return;

	Echo( "\b" );
	--m_CursorPosition;
}

int CTextConsole::HandleKeyEnter()
{
	Echo( "\n" );
	int length = m_ConsoleLineLen;
	if ( !length )
		return 0;

	m_ConsoleLine[length] = 0;
	m_ConsoleLineLen = 0;
	m_CursorPosition = 0;
	if ( !m_NextHistoryLine || strcmp( m_History[m_NextHistoryLine - 1], m_ConsoleLine ) )
	{
		strncpy( m_History[m_NextHistoryLine], m_ConsoleLine, sizeof( m_ConsoleLine ) );
		++m_NextHistoryLine;
		if ( m_NextHistoryLine > m_HistoryCount )
			m_HistoryCount = m_NextHistoryLine;

		if ( m_NextHistoryLine >= MAX_CONSOLE_HISTORY )
			m_NextHistoryLine = 0;
	}

	m_BrowseLine = m_NextHistoryLine;
	return length;
}

void CTextConsole::HandleKeyRight()
{
	if ( m_CursorPosition == m_ConsoleLineLen )
		return;

	Echo( &m_ConsoleLine[m_CursorPosition], 1 );
	++m_CursorPosition;
}

void CTextConsole::HandleKeyChar( char character )
{
	if ( (unsigned int)m_ConsoleLineLen >= sizeof( m_ConsoleLine ) - 2 )
		return;

	for ( int i = m_ConsoleLineLen; i > m_CursorPosition; --i )
		m_ConsoleLine[i] = m_ConsoleLine[i - 1];

	m_ConsoleLine[m_CursorPosition] = character;
	Echo( &m_ConsoleLine[m_CursorPosition], m_ConsoleLineLen - m_CursorPosition + 1 );
	++m_ConsoleLineLen;
	++m_CursorPosition;
	for ( int i = m_ConsoleLineLen; i > m_CursorPosition; --i )
		Echo( "\b" );

	m_BrowseLine = m_NextHistoryLine;
}

void CTextConsole::HandleKeyTab()
{
	ObjectList matches;
	if ( !m_System )
		return;

	m_ConsoleLine[m_ConsoleLineLen] = 0;
	m_System->GetCommandMatches( m_ConsoleLine, &matches );
	if ( matches.IsEmpty() )
		return;

	if ( matches.CountElements() == 1 )
	{
		char* command = (char*)matches.GetFirst();
		char* suffix = command + strlen( m_ConsoleLine );
		if ( suffix )
		{
			Echo( suffix );
			strcat( m_ConsoleLine, suffix );
			m_ConsoleLineLen += (int)strlen( suffix );
			Echo( " " );
			strcat( m_ConsoleLine, " " );
			++m_ConsoleLineLen;
		}
	}
	else
	{
		int longest = 0;
		for ( char* command = (char*)matches.GetFirst(); command; command = (char*)matches.GetNext() )
		{
			int length = (int)strlen( command );
			if ( length > longest )
				longest = length;
		}

		int columns = ( GetWidth() - 1 ) / ( longest + 1 );
		int column = 0;
		Echo( "\n" );
		for ( char* command = (char*)matches.GetFirst(); command; command = (char*)matches.GetNext() )
		{
			if ( ++column > columns )
			{
				Echo( "\n" );
				column = 1;
			}

			char text[MAX_CONSOLE_LINE];
			_snprintf( text, sizeof( text ), "%-*s ", longest, command );
			Echo( text );
		}

		Echo( "\n" );
		Echo( m_ConsoleLine );
	}

	m_CursorPosition = m_ConsoleLineLen;
	m_BrowseLine = m_NextHistoryLine;
}

void CTextConsole::HandleKeyUp()
{
	int oldest = m_NextHistoryLine + 1;
	if ( oldest > m_HistoryCount )
		oldest = 0;

	if ( m_BrowseLine == oldest )
		return;

	if ( m_BrowseLine == m_NextHistoryLine )
	{
		if ( m_ConsoleLineLen > 0 )
			strncpy( m_SavedConsoleLine, m_ConsoleLine, m_ConsoleLineLen );

		m_SavedConsoleLineLen = m_ConsoleLineLen;
	}

	--m_BrowseLine;
	if ( m_BrowseLine < 0 )
		m_BrowseLine = m_HistoryCount - 1;

	while ( m_ConsoleLineLen-- )
		Echo( "\b \b" );

	Echo( m_History[m_BrowseLine] );
	strncpy( m_ConsoleLine, m_History[m_BrowseLine], sizeof( m_ConsoleLine ) );
	m_ConsoleLineLen = (int)strlen( m_History[m_BrowseLine] );
	m_CursorPosition = m_ConsoleLineLen;
}
