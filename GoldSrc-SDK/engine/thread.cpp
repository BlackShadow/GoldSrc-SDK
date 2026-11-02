#include "thread.h"

#include <cstring>

#if defined( _WIN32 )
#include <process.h>

extern "C" _ACRTIMP void __cdecl _wassert( const wchar_t* message, const wchar_t* file, unsigned int line );
#else
#include <cerrno>
#include <unistd.h>
#endif

#if defined( _WIN32 )
//-----------------------------------------------------------------------------
// Purpose: Thread processing function
// Input  : pv - 
// Output : static DWORD WINAPI
//-----------------------------------------------------------------------------
static DWORD WINAPI ThreadFunc( void* pv )
{
	CThread* self = static_cast<CThread*>( pv );
	if ( !self )
		_wassert( L"thread", L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\engine\\thread.cpp", 110 );

	while ( self->m_bThreadRunning )
	{
		WaitForSingleObject( self->m_hEvent, INFINITE );
		EnterCriticalSection( &self->m_mutex );

		int count = self->m_nCmds;
		memcpy( self->_m_rgCmds, self->m_rgCmds, sizeof( CThread::threadcmds ) * count );
		self->_m_nCmds = count;
		self->m_nCmds = 0;
		ResetEvent( self->m_hEvent );
		LeaveCriticalSection( &self->m_mutex );

		for ( int i = 0; i < self->_m_nCmds; i++ )
		{
			CThread::threadcmds* cmd = &self->_m_rgCmds[i];
			if ( cmd->pfn )
				( GetInteralCDAudio()->*cmd->pfn )( cmd->param1, cmd->param2 );
		}
	}

	self->m_bThreadStopped = true;
	return 1;
}
#else
static void* ThreadFunc( void* pv )
{
	CThread* self = (CThread*)pv;
	while ( self->m_bThreadRunning )
	{
		int count;

		pthread_mutex_lock( &self->m_mutex );
		if ( !self->m_cSet )
		{
			while ( pthread_cond_wait( &self->m_Condition, &self->m_mutex ) == EINTR )
				;
		}

		count = self->m_nCmds;
		self->m_cSet = 0;
		memcpy( self->_m_rgCmds, self->m_rgCmds, sizeof( CThread::threadcmds ) * count );
		self->_m_nCmds = count;
		self->m_nCmds = 0;
		pthread_mutex_unlock( &self->m_mutex );

		for ( int i = 0; i < self->_m_nCmds; i++ )
		{
			CThread::threadcmds* cmd = &self->_m_rgCmds[i];
			if ( cmd->pfn )
				( GetInteralCDAudio()->*cmd->pfn )( cmd->param1, cmd->param2 );
		}
	}

	self->m_bThreadStopped = true;
	return (void*)1;
}
#endif

CThread::CThread()
#if defined( _WIN32 )
	: m_hEvent( NULL ), m_mutex{}, m_bThreadRunning( false ), m_bThreadStopped( false ), m_nCmds( 0 ),
	  _m_nCmds( 0 ), m_rgCmds{}, _m_rgCmds{}, m_hThread( NULL ), m_dwThreadID( 0 )
#endif
{
#if !defined( _WIN32 )
	m_bThreadRunning = false;
	m_dwThreadID = 0;
	m_cSet = 0;
	m_bThreadStopped = false;
#endif
}

CThread::~CThread()
{
}

bool CThread::AddThreadItem( vfunc pfn, int param1, int param2 )
{
#if defined( _WIN32 )
	EnterCriticalSection( &m_mutex );
#else
	pthread_mutex_lock( &m_mutex );
#endif

	// Check for duplicates
	for ( int i = 0; i < m_nCmds; i++ )
	{
		if ( m_rgCmds[i].pfn == pfn )
		{
#if defined( _WIN32 )
			LeaveCriticalSection( &m_mutex );
#else
			pthread_mutex_unlock( &m_mutex );
#endif
			return false;
		}
	}

	if ( m_nCmds >= MAX_THREAD_CMDS )
	{
#if defined( _WIN32 )
		LeaveCriticalSection( &m_mutex );
#else
		pthread_mutex_unlock( &m_mutex );
#endif
		return false;
	}

	m_rgCmds[m_nCmds].pfn = pfn;
	m_rgCmds[m_nCmds].param1 = param1;
	m_rgCmds[m_nCmds].param2 = param2;
	m_nCmds++;

#if defined( _WIN32 )
	LeaveCriticalSection( &m_mutex );
	SetEvent( m_hEvent );
#else
	pthread_mutex_unlock( &m_mutex );
	pthread_mutex_lock( &m_mutex );
	m_cSet = 1;
	pthread_cond_signal( &m_Condition );
	pthread_mutex_unlock( &m_mutex );
#endif
	return true;
}

bool CThread::Shutdown()
{
#if defined( _WIN32 )
	if ( !m_hEvent )
		return true;

	m_bThreadStopped = false;
	m_bThreadRunning = false;
	SetEvent( m_hEvent );

	for ( int i = 100; !m_bThreadStopped && --i > 0; )
		Sleep( 1 );

	CloseHandle( m_hEvent );
	DeleteCriticalSection( &m_mutex );
#else
	int i;

	m_bThreadStopped = false;
	m_bThreadRunning = false;

	pthread_mutex_lock( &m_mutex );
	m_cSet = 1;
	pthread_cond_signal( &m_Condition );
	pthread_mutex_unlock( &m_mutex );

	for ( i = 0; i < 99 && !m_bThreadStopped; i++ )
		usleep( 1000 );

	pthread_cond_destroy( &m_Condition );
	pthread_mutex_destroy( &m_mutex );
#endif
	cdaudio->Shutdown();
	return true;
}

bool CThread::Init()
{
#if defined( _WIN32 )
	InitializeCriticalSection( &m_mutex );
	m_hEvent = CreateEventA( NULL, TRUE, FALSE, NULL );
	m_bThreadRunning = true;
	cdaudio->Init();
	m_hThread = CreateThread( NULL, 0, ThreadFunc, this, 0, &m_dwThreadID );
	return m_hThread != NULL;
#else
	pthread_mutexattr_t attr;
	pthread_t tid;
	int created;

	pthread_mutexattr_init( &attr );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
	pthread_mutex_init( &m_mutex, &attr );
	pthread_mutexattr_destroy( &attr );
	pthread_cond_init( &m_Condition, NULL );
	m_cSet = 0;
	m_bThreadRunning = true;
	cdaudio->Init();
	created = pthread_create( &tid, NULL, ThreadFunc, this );
	m_dwThreadID = tid;
	return created == 0;
#endif
}

static CThread g_Thread;
IThread* thread = &g_Thread;
