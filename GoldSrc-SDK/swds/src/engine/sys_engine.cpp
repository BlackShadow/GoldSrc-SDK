// sys_engine.cpp -- engine lifetime and launcher frame dispatch

#include "iengine.h"
#include "igame.h"
#include "sys.h"
#include "host.h"

#ifdef _WIN32
#include "cd.h"
#include "keys.h"
#include "client.h"

#define QUIT_TODESKTOP	1
#define QUIT_RESTART		2
#endif

//-----------------------------------------------------------------------------
// Purpose: maintains the state and timing of the launcher engine instance.
//-----------------------------------------------------------------------------
class CEngine : public IEngine
{
public:
	CEngine();
	~CEngine() override;
	bool Load( bool dedicated, char* basedir, char* cmdline ) override;
	void Unload() override;
	void SetState( int state ) override;
	int GetState() override;
	void SetSubState( int state ) override;
	int GetSubState() override;
	int Frame() override;
	double GetFrameTime() override;
	double GetCurTime() override;
	void TrapKey_Event( int key, bool down ) override;
	void TrapMouse_Event( int buttons, bool down ) override;
	void StartTrapMode() override;
	bool IsTrapping() override;
	bool CheckDoneTrapping( int& buttons, int& key ) override;
	int GetQuitting() override;
	void SetQuitting( int quitting ) override;

private:
	int		m_nQuitting;
	int		m_nDLLState;
	int		m_nSubState;
	double	m_fCurTime;
	double	m_fFrameTime;
	double	m_fOldTime;
	bool	m_bTrapMode;
	bool	m_bDoneTrapping;
	int		m_nTrapKey;
	int		m_nTrapButtons;
};

static CEngine g_Engine;
IEngine* eng = &g_Engine;

#ifdef _WIN32
IEngine::~IEngine()
{
}
#endif

CEngine::CEngine()
{
	m_fFrameTime = 0.0;
	m_nSubState = 0;
	m_nDLLState = DLL_INACTIVE;
	m_fOldTime = 0.0;
	m_bTrapMode = false;
	m_bDoneTrapping = false;
	m_nTrapKey = 0;
	m_nTrapButtons = 0;
	m_nQuitting = 0;
}

CEngine::~CEngine()
{
}

bool CEngine::Load( bool dedicated, char* basedir, char* cmdline )
{
	SetState( DLL_ACTIVE );
	if ( !Sys_InitGame( cmdline, basedir, game->GetMainWindowAddress(), dedicated ) )
		return false;

#ifdef _WIN32
	ForceReloadProfile();
#endif
	return true;
}

void CEngine::Unload()
{
	Sys_ShutdownGame();
	m_nDLLState = DLL_INACTIVE;
}

void CEngine::SetState( int state )
{
	m_nDLLState = state;
	GameSetState( state );
}

int CEngine::GetState()
{
	return m_nDLLState;
}

void CEngine::SetSubState( int state )
{
	if ( state != ENG_NORMAL )
		GameSetSubState( state );
}

int CEngine::GetSubState()
{
	return m_nSubState;
}

int CEngine::Frame()
{
#ifdef _WIN32
	cdaudio->Frame();
#endif
	if ( !game->IsActiveApp() )
		game->SleepUntilInput( m_nDLLState == DLL_PAUSED ? 50 : 20 );

	m_fCurTime = Sys_FloatTime();
	m_fFrameTime = m_fCurTime - m_fOldTime;
	m_fOldTime = m_fCurTime;
	if ( m_fFrameTime < 0.0 )
		m_fFrameTime = 0.01f;

	if ( m_nDLLState != DLL_INACTIVE )
	{
		int dummy;
		int iState = Host_Frame( (float)m_fFrameTime, m_nDLLState, &dummy );
		if ( iState != m_nDLLState )
		{
			SetState( iState );

			if ( m_nDLLState == DLL_CLOSE )
				SetQuitting( QUIT_TODESKTOP );
			else if ( m_nDLLState == DLL_RESTARTING )
				SetQuitting( QUIT_RESTART );
		}
	}

	return m_nDLLState;
}

double CEngine::GetFrameTime()
{
	return m_fFrameTime;
}

double CEngine::GetCurTime()
{
	return m_fCurTime;
}

#ifdef _WIN32
void CEngine::TrapKey_Event( int key, bool down )
{
	if ( m_bTrapMode && down )
	{
		m_bTrapMode = false;
		m_bDoneTrapping = true;
		m_nTrapKey = key;
		m_nTrapButtons = 0;
	}
	else
	{
		Key_Event( key, down );
	}
}
#else
void CEngine::TrapKey_Event( int, bool )
{
}
#endif

void CEngine::TrapMouse_Event( int buttons, bool down )
{
	if ( m_bTrapMode && buttons != 0 && !down )
	{
		m_bTrapMode = false;
		m_bDoneTrapping = true;
		m_nTrapKey = 0;
		m_nTrapButtons = buttons;
	}
#ifdef _WIN32
	else
	{
		ClientDLL_MouseEvent( buttons );
	}
#endif
}

void CEngine::StartTrapMode()
{
	if ( !m_bTrapMode )
	{
		m_bTrapMode = true;
		m_bDoneTrapping = false;
	}
}

bool CEngine::IsTrapping()
{
	return m_bTrapMode;
}

bool CEngine::CheckDoneTrapping( int& buttons, int& key )
{
	if ( m_bTrapMode || !m_bDoneTrapping )
		return false;

	key = m_nTrapKey;
	buttons = m_nTrapButtons;
	m_bDoneTrapping = false;
	return true;
}

int CEngine::GetQuitting()
{
	return m_nQuitting;
}

void CEngine::SetQuitting( int quitting )
{
	m_nQuitting = quitting;
}
