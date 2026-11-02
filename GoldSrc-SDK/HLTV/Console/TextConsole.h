#pragma once

#define MAX_CONSOLE_LINE		256
#define MAX_CONSOLE_HISTORY	30

class IBaseSystem;

class CTextConsole
{
public:
	virtual ~CTextConsole();

	virtual bool Init( IBaseSystem* system );
	virtual void ShutDown();
	virtual void Print( char* text );
	virtual void SetTitle( char* title );
	virtual void SetStatusLine( char* status );
	virtual void UpdateStatus();
	virtual void PrintRaw( char* text, int length = 0 ) = 0;
	virtual void Echo( char* text, int length = 0 ) = 0;
	virtual char* GetLine() = 0;
	virtual int GetWidth() = 0;
	virtual void SetVisible( bool visible );
	virtual bool IsVisible();

protected:
	void HandleKeyBackspace();
	void HandleKeyDown();
	void HandleKeyLeft();
	int HandleKeyEnter();
	void HandleKeyRight();
	void HandleKeyChar( char character );
	void HandleKeyTab();
	void HandleKeyUp();

	char			m_ConsoleLine[MAX_CONSOLE_LINE];
	int				m_ConsoleLineLen;
	int				m_CursorPosition;
	char			m_SavedConsoleLine[MAX_CONSOLE_LINE];
	int				m_SavedConsoleLineLen;
	char			m_History[MAX_CONSOLE_HISTORY][MAX_CONSOLE_LINE];
	int				m_NextHistoryLine;
	int				m_BrowseLine;
	int				m_HistoryCount;
	bool			m_Visible;
	IBaseSystem*	m_System;

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
