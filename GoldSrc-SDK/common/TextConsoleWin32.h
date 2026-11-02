#pragma once

#include "textconsole.h"

#include <windows.h>

class CTextConsoleWin32 : public CTextConsole
{
public:
	bool Init( IBaseSystem* system ) override;
	void ShutDown() override;
	void SetTitle( char* title ) override;
	void SetStatusLine( char* status ) override;
	void UpdateStatus() override;
	void PrintRaw( char* text, int length = 0 ) override;
	void Echo( char* text, int length = 0 ) override;
	char* GetLine() override;
	int GetWidth() override;
	void SetVisible( bool visible ) override;

	void SetStatusLineAttrib( WORD attrib );

private:
	HANDLE	m_hInput;
	HANDLE	m_hOutput;
	WORD	m_Attrib;
	char	m_StatusLine[80];
#pragma warning( suppress : 26495 )
};
