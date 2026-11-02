#pragma once

#include "common/BaseSystemModule.h"
#include "common/IBaseSystem.h"
#include "common/TokenLine.h"
#include "TextConsoleWin32.h"
#include "FileSystem.h"

#pragma warning( push )
#pragma warning( disable : 4250 )

class System : public BaseSystemModule, public IBaseSystem
{
public:
	System();
	~System() override;

	bool Init( IBaseSystem* system, int serial, char* name ) override;
	void RunFrame( double time ) override;
	void ExecuteCommand( int commandID, char* commandLine ) override;
	char* GetStatusLine() override;
	char* GetType() override;
	void ShutDown() override;

	double GetTime() override;
	unsigned int GetTick() override;
	void SetFPS( float fps ) override;
	void Printf( char* format, ... ) override;
	void DPrintf( char* format, ... ) override;
	void RedirectOutput( char* buffer, int maxSize ) override;
	IFileSystem* GetFileSystem() override;
	unsigned char* LoadFile( const char* name, int* length ) override;
	void FreeFile( unsigned char* buffer ) override;
	void SetTitle( char* title ) override;
	void SetStatusLine( char* line ) override;
	void ShowConsole( bool visible ) override;
	void LogConsole( char* filename ) override;
	bool InitVGUI( IVGuiModule* module ) override;
	vgui2::Panel* GetPanel() override;
	bool RegisterCommand( char* name, ISystemModule* module, int commandID ) override;
	void GetCommandMatches( char* string, ObjectList* matches ) override;
	void ExecuteString( char* commands ) override;
	void ExecuteFile( char* filename ) override;
	void Errorf( char* format, ... ) override;
	char* CheckParam( char* param ) override;
	bool AddModule( ISystemModule* module, char* name ) override;
	ISystemModule* GetModule( char* interfaceName, char* libraryName, char* instanceName ) override;
	bool RemoveModule( ISystemModule* module ) override;
	void Stop() override;
	char* COM_GetBaseDir() override;

private:
	friend int WINAPI WinMain( _In_ HINSTANCE instance, _In_opt_ HINSTANCE previous,
			_In_ LPSTR commandLine, _In_ int show );

	enum SystemCommand
	{
		CMD_DEVELOPER = 1,
		CMD_EXEC,
		CMD_MODULES,
		CMD_LIST,
		CMD_LOGFILE,
		CMD_QUIT,
		CMD_LOADMODULE,
		CMD_UNLOADMODULE,
		CMD_TITLE,
		CMD_SHOWCON,
		CMD_ECHO
	};

	enum
	{
		MODULE_PAUSED = 2
	};

	typedef struct command_s
	{
		char			name[32];
		int				id;
		ISystemModule*	module;
	} command_t;

	typedef struct library_s
	{
		char				name[MAX_PATH];
		CSysModule*			module;
		CreateInterfaceFn	factory;
	} library_t;

	ISystemModule* FindModule( char* interfaceName, char* instanceName );
	void OpenLogFile( char* filename );
	void CloseLogFile();
	bool StartVGUI();

	double				m_LastTime;
	CTextConsoleWin32	m_Console;
	unsigned char		m_ReservedConsole[4];
	CRITICAL_SECTION	m_ClockLock;
	int					m_ClockShift;
	double				m_ClockFrequency;
	unsigned char		m_ReservedClock[8];
	CSysModule*			m_FileSystemLibrary;
	IFileSystem*		m_FileSystem;
	CSysModule*			m_VGuiLibrary;
	TokenLine			m_CommandLine;
	char				m_BaseDir[MAX_PATH];
	char				m_StatusLine[80];
	FileHandle_t		m_LogFile;
	char				m_LogFileName[MAX_PATH];
	ObjectList			m_Modules;
	ObjectList			m_Libraries;
	ObjectList			m_Commands;
	float				m_MinFrameTime;
	char*				m_RedirectBuffer;
	int					m_RedirectSize;
	bool				m_Developer;
	int					m_NextSerial;
	unsigned int		m_Tick;
	DWORD				m_SleepTime;
	CreateInterfaceFn	m_Factories[3];
	bool				m_VGUI;
	vgui2::Panel*		m_MainPanel;
};

#pragma warning( pop )

void System_Printf( const char* format, ... );
