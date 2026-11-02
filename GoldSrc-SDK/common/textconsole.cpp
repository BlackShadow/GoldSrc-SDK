#include "textconsole.h"
#include "IBaseSystem.h"
#include "ObjectList.h"

#include <cstdio>
#include <cstring>

void Sys_Printf( char* format, ... );

bool CTextConsole::Init( IBaseSystem* system )
{
	m_System = system;
	memset( m_szConsoleText, 0, sizeof( m_szConsoleText ) );
	m_nConsoleTextLen = 0;
	m_nCursorPosition = 0;
	memset( m_szSavedConsoleText, 0, sizeof( m_szSavedConsoleText ) );
	m_nSavedConsoleTextLen = 0;
	memset( m_aszLineBuffer, 0, sizeof( m_aszLineBuffer ) );
	m_nTotalLines = 0;
	m_nInputLine = 0;
	m_nBrowseLine = 0;

	Sys_Printf( (char*)"\n" );
	Sys_Printf( (char*)"Console initialized.\n" );
	m_ConsoleVisible = true;
	return true;
}

void CTextConsole::ShutDown()
{
}

void CTextConsole::Print( char* text )
{
	for ( int i = m_nConsoleTextLen; i; --i )
		PrintRaw( (char*)"\b \b" );

	PrintRaw( text );
	if ( m_nConsoleTextLen )
		PrintRaw( m_szConsoleText, m_nConsoleTextLen );

	UpdateStatus();
}

void CTextConsole::SetVisible( bool visible )
{
	m_ConsoleVisible = visible;
}

bool CTextConsole::IsVisible()
{
	return m_ConsoleVisible;
}

int CTextConsole::ReceiveNewline()
{
	Echo( (char*)"\n" );
	int length = m_nConsoleTextLen;
	if ( !length )
		return 0;

	m_szConsoleText[length] = '\0';
	m_nConsoleTextLen = 0;
	m_nCursorPosition = 0;
	if ( !m_nInputLine || strcmp( m_aszLineBuffer[m_nInputLine - 1], m_szConsoleText ) )
	{
		strncpy( m_aszLineBuffer[m_nInputLine], m_szConsoleText, sizeof( m_szConsoleText ) );
		++m_nInputLine;
		if ( m_nInputLine > m_nTotalLines )
			m_nTotalLines = m_nInputLine;

		if ( m_nInputLine >= MAX_BUFFER_LINES )
			m_nInputLine = 0;
	}

	m_nBrowseLine = m_nInputLine;
	return length;
}

void CTextConsole::ReceiveBackspace()
{
	if ( !m_nCursorPosition )
		return;

	--m_nConsoleTextLen;
	--m_nCursorPosition;
	Echo( (char*)"\b" );
	for ( int i = m_nCursorPosition; i < m_nConsoleTextLen; ++i )
	{
		m_szConsoleText[i] = m_szConsoleText[i + 1];
		Echo( &m_szConsoleText[i], 1 );
	}

	Echo( (char*)" " );
	for ( int i = m_nConsoleTextLen; i >= m_nCursorPosition; --i )
		Echo( (char*)"\b" );

	m_nBrowseLine = m_nInputLine;
}

void CTextConsole::ReceiveTab()
{
	ObjectList matches;
	if ( !m_System )
		return;

	m_szConsoleText[m_nConsoleTextLen] = '\0';
	m_System->GetCommandMatches( m_szConsoleText, &matches );
	if ( matches.IsEmpty() )
		return;

	if ( matches.CountElements() == 1 )
	{
		char* completion = (char*)matches.GetFirst() + strlen( m_szConsoleText );
		if ( completion )
		{
			Echo( completion );
			strcat( m_szConsoleText, completion );
			m_nConsoleTextLen += (int)strlen( completion );
			Echo( (char*)" " );
			strcat( m_szConsoleText, " " );
			++m_nConsoleTextLen;
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
		Echo( (char*)"\n" );
		for ( char* command = (char*)matches.GetFirst(); command; command = (char*)matches.GetNext() )
		{
			if ( ++column > columns )
			{
				Echo( (char*)"\n" );
				column = 1;
			}

			char text[MAX_CONSOLE_TEXTLEN];
			_snprintf( text, sizeof( text ), "%-*s ", longest, command );
			Echo( text );
		}

		Echo( (char*)"\n" );
		Echo( m_szConsoleText );
	}

	m_nCursorPosition = m_nConsoleTextLen;
	m_nBrowseLine = m_nInputLine;
}

void CTextConsole::ReceiveStandardChar( char character )
{
	if ( (unsigned int)m_nConsoleTextLen >= sizeof( m_szConsoleText ) - 2 )
		return;

	for ( int i = m_nConsoleTextLen; i > m_nCursorPosition; --i )
		m_szConsoleText[i] = m_szConsoleText[i - 1];

	m_szConsoleText[m_nCursorPosition] = character;
	Echo( &m_szConsoleText[m_nCursorPosition], m_nConsoleTextLen - m_nCursorPosition + 1 );
	++m_nConsoleTextLen;
	++m_nCursorPosition;
	for ( int i = m_nConsoleTextLen; i > m_nCursorPosition; --i )
		Echo( (char*)"\b" );

	m_nBrowseLine = m_nInputLine;
}

void CTextConsole::ReceiveUpArrow()
{
	int oldest = m_nInputLine + 1;
	if ( oldest > m_nTotalLines )
		oldest = 0;

	if ( m_nBrowseLine == oldest )
		return;

	if ( m_nBrowseLine == m_nInputLine )
	{
		if ( m_nConsoleTextLen > 0 )
			strncpy( m_szSavedConsoleText, m_szConsoleText, m_nConsoleTextLen );

		m_nSavedConsoleTextLen = m_nConsoleTextLen;
	}

	--m_nBrowseLine;
	if ( m_nBrowseLine < 0 )
		m_nBrowseLine = m_nTotalLines - 1;

	while ( m_nConsoleTextLen-- )
		Echo( (char*)"\b \b" );

	Echo( m_aszLineBuffer[m_nBrowseLine] );
	strncpy( m_szConsoleText, m_aszLineBuffer[m_nBrowseLine], sizeof( m_szConsoleText ) );
	m_nConsoleTextLen = (int)strlen( m_aszLineBuffer[m_nBrowseLine] );
	m_nCursorPosition = m_nConsoleTextLen;
}

void CTextConsole::ReceiveDownArrow()
{
	if ( m_nBrowseLine == m_nInputLine )
		return;

	++m_nBrowseLine;
	if ( m_nBrowseLine > m_nTotalLines )
		m_nBrowseLine = 0;

	while ( m_nConsoleTextLen-- )
		Echo( (char*)"\b \b" );

	if ( m_nBrowseLine == m_nInputLine )
	{
		if ( m_nSavedConsoleTextLen > 0 )
		{
			strncpy( m_szConsoleText, m_szSavedConsoleText, m_nSavedConsoleTextLen );
			Echo( m_szConsoleText, m_nSavedConsoleTextLen );
		}
		m_nConsoleTextLen = m_nSavedConsoleTextLen;
	}
	else
	{
		Echo( m_aszLineBuffer[m_nBrowseLine] );
		strncpy( m_szConsoleText, m_aszLineBuffer[m_nBrowseLine], sizeof( m_szConsoleText ) );
		m_nConsoleTextLen = (int)strlen( m_aszLineBuffer[m_nBrowseLine] );
	}

	m_nCursorPosition = m_nConsoleTextLen;
}

void CTextConsole::ReceiveLeftArrow()
{
	if ( !m_nCursorPosition )
		return;

	Echo( (char*)"\b" );
	--m_nCursorPosition;
}

void CTextConsole::ReceiveRightArrow()
{
	if ( m_nCursorPosition == m_nConsoleTextLen )
		return;

	Echo( &m_szConsoleText[m_nCursorPosition], 1 );
	++m_nCursorPosition;
}
