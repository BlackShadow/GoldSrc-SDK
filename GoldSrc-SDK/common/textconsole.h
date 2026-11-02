#ifndef TEXTCONSOLE_H
#define TEXTCONSOLE_H

#define MAX_CONSOLE_TEXTLEN	256
#define MAX_BUFFER_LINES		30

class IBaseSystem;

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 26495 )
#endif

class CTextConsole
{
public:
	virtual ~CTextConsole()
	{
	}

	virtual bool Init( IBaseSystem* system );
	virtual void ShutDown();
	virtual void Print( char* text );
	virtual void SetTitle( char* )
	{
	}
	virtual void SetStatusLine( char* )
	{
	}
	virtual void UpdateStatus()
	{
	}
	virtual void PrintRaw( char* text, int length = 0 ) = 0;
	virtual void Echo( char* text, int length = 0 ) = 0;
	virtual char* GetLine() = 0;
	virtual int GetWidth() = 0;
	virtual void SetVisible( bool visible );
	virtual bool IsVisible();

protected:
	int ReceiveNewline();
	void ReceiveBackspace();
	void ReceiveTab();
	void ReceiveStandardChar( char character );
	void ReceiveUpArrow();
	void ReceiveDownArrow();
	void ReceiveLeftArrow();
	void ReceiveRightArrow();

	char			m_szConsoleText[MAX_CONSOLE_TEXTLEN];
	int				m_nConsoleTextLen;
	int				m_nCursorPosition;
	char			m_szSavedConsoleText[MAX_CONSOLE_TEXTLEN];
	int				m_nSavedConsoleTextLen;
	char			m_aszLineBuffer[MAX_BUFFER_LINES][MAX_CONSOLE_TEXTLEN];
	int				m_nInputLine;
	int				m_nBrowseLine;
	int				m_nTotalLines;
	bool			m_ConsoleVisible;
	IBaseSystem*	m_System;
};

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif
