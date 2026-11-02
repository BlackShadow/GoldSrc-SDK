#include "tier0/threadtools.h"
#include "tier0/dbg.h"

#define DBG_SOURCE_FILE	DBG_BUILDSLAVE_PATH( "tier0\\testthread.cpp" )

#if defined( _WIN32 )

typedef void ( __stdcall *TestFunc )( void* );

class CTestHarness
{
public:
	CTestHarness();
	~CTestHarness();

	void RunTest( TestFunc func, void* arg );
	void RunFrame();
	void Yield();
	void TerminateTestThread();

	static DWORD WINAPI TestThreadProxy( void* arg );

	HANDLE		m_hThreadTestDriver;
	HANDLE		m_hRunTestThread;
	HANDLE		m_hRunMainThread;

	bool		m_bTestThreadRunning;
	bool		m_bStopTestThread;
	bool		m_bLetTestThreadRun;
	bool		m_bTestFailed;

	DWORD		m_ulTestThreadID;
	DWORD		m_ulMainThreadID;
	void*		m_pvTestThreadArg;
	TestFunc	m_pTestFunc;
};

CTestHarness g_TestHarness;

CTestHarness::CTestHarness()
{
	m_ulMainThreadID = GetCurrentThreadId();
	m_ulTestThreadID = 0;
	m_hRunTestThread = NULL;
	m_hRunMainThread = NULL;
	m_hThreadTestDriver = NULL;
}

CTestHarness::~CTestHarness()
{
	if ( m_bTestThreadRunning )
	{
		const DWORD TEST_SHUTDOWN_TIMEOUT = 100;
		m_bStopTestThread = true;
		SetEvent( m_hRunTestThread );
		WaitForSingleObject( m_hThreadTestDriver, TEST_SHUTDOWN_TIMEOUT );
		CloseHandle( m_hThreadTestDriver );
		CloseHandle( m_hRunTestThread );
		CloseHandle( m_hRunMainThread );
	}
}

void CTestHarness::RunTest( TestFunc func, void* arg )
{
	m_ulMainThreadID = GetCurrentThreadId();
	AssertAt( !m_bTestThreadRunning, 122 );
	m_pvTestThreadArg = arg;
	m_pTestFunc = func;
	if ( !m_bTestThreadRunning )
	{
		m_bTestThreadRunning = true;
		if ( !m_hRunTestThread )
		{
			m_hRunTestThread = CreateEventA( NULL, FALSE, FALSE, NULL );
		}
		if ( !m_hRunMainThread )
		{
			m_hRunMainThread = CreateEventA( NULL, FALSE, FALSE, NULL );
		}
		m_hThreadTestDriver = CreateThread( NULL, 0, TestThreadProxy, this, 0, &m_ulTestThreadID );
	}
}

void CTestHarness::RunFrame()
{
	if ( m_bTestThreadRunning )
	{
		AssertAt( GetCurrentThreadId() == m_ulMainThreadID, 141 );
		if ( m_bLetTestThreadRun )
		{
			m_bLetTestThreadRun = false;
			SetEvent( m_hRunTestThread );
			WaitForSingleObject( m_hRunMainThread, INFINITE );
		}
	}
}

void CTestHarness::Yield()
{
	AssertAt( GetCurrentThreadId() == m_ulTestThreadID, 173 );
	SetEvent( m_hRunMainThread );
	if ( m_bStopTestThread )
	{
		ExitThread( 0 );
	}
	m_bLetTestThreadRun = true;
	WaitForSingleObject( m_hRunTestThread, INFINITE );
}

void CTestHarness::TerminateTestThread()
{
	AssertAt( GetCurrentThreadId() == m_ulMainThreadID, 239 );
	if ( m_hThreadTestDriver )
	{
#pragma warning( suppress: 6258 )
		TerminateThread( m_hThreadTestDriver, 0 );
		m_hThreadTestDriver = NULL;
		m_bTestThreadRunning = false;
		m_bStopTestThread = false;
	}
}

DWORD WINAPI CTestHarness::TestThreadProxy( void* arg )
{
	CTestHarness* harness = (CTestHarness*)arg;
	harness->Yield();
	harness->m_pTestFunc( harness->m_pvTestThreadArg );
	harness->m_hThreadTestDriver = NULL;
	harness->m_bTestThreadRunning = false;
	harness->m_bStopTestThread = false;
	SetEvent( harness->m_hRunMainThread );
	return 0;
}

extern "C"
{

TIER0_API void Test_RunTest( TestFunc func, void* arg )
{
	g_TestHarness.RunTest( func, arg );
}

TIER0_API void Test_RunFrame()
{
	g_TestHarness.RunFrame();
}

TIER0_API void TestThread_Yield()
{
	g_TestHarness.Yield();
}

void Test_SetFailed()
{
	g_TestHarness.m_bTestFailed = true;
}

TIER0_API bool Test_HasFailed()
{
	return g_TestHarness.m_bTestFailed;
}

TIER0_API bool Test_HasFinished()
{
	return !g_TestHarness.m_bTestThreadRunning && g_TestHarness.m_hRunMainThread != NULL;
}

TIER0_API bool Test_IsActive()
{
	return g_TestHarness.m_hRunMainThread != NULL;
}

TIER0_API void Test_TerminateThread()
{
	g_TestHarness.TerminateTestThread();
}

}

#else
#include <pthread.h>
#include <stddef.h>

typedef pthread_t THREAD_HANDLE;
typedef void ( *TestFunc )( void* );

class CTestHarness
{
public:
	CTestHarness();
	~CTestHarness();

	static void* TestThreadProxy( void* pvArg );

	THREAD_HANDLE	m_hThreadTestDriver;
	pthread_mutex_t	m_hRunTestThreadMutex;
	pthread_cond_t	m_hRunTestThread;
	pthread_mutex_t	m_hRunMainThreadMutex;
	pthread_cond_t	m_hRunMainThread;

	bool			m_bTestActive;
	bool			m_bTestThreadRunning;
	bool			m_bStopTestThread;
	bool			m_bLetTestThreadRun;
	bool			m_bTestFailed;

	unsigned long	m_ulTestThreadID;
	unsigned long	m_ulMainThreadID;
	void*			m_pvTestThreadArg;
	void ( *m_pTestFunc )( void* );
};

extern "C"
{
CTestHarness g_TestHarness;
}

CTestHarness::CTestHarness()
{
	m_bTestThreadRunning = false;
	m_bStopTestThread = false;
	m_bLetTestThreadRun = false;
	m_bTestFailed = false;
	m_bTestActive = false;
	m_hThreadTestDriver = 0;
	m_ulMainThreadID = pthread_self();
	m_ulTestThreadID = 0;
}

CTestHarness::~CTestHarness()
{
	if ( m_bTestThreadRunning )
	{
		m_bStopTestThread = true;
		m_bTestActive = false;
		pthread_mutex_lock( &m_hRunTestThreadMutex );
		pthread_cond_wait( &m_hRunTestThread, &m_hRunTestThreadMutex );
		pthread_mutex_unlock( &m_hRunTestThreadMutex );

		int nStatus;
		int* pStatus = &nStatus;
		pthread_join( m_hThreadTestDriver, (void**)&pStatus );

		pthread_cond_destroy( &m_hRunTestThread );
		pthread_cond_destroy( &m_hRunMainThread );
		pthread_mutex_destroy( &m_hRunTestThreadMutex );
		pthread_mutex_destroy( &m_hRunMainThreadMutex );
	}
}

void* CTestHarness::TestThreadProxy( void* pvArg )
{
	CTestHarness* pHarness = (CTestHarness*)pvArg;
	pthread_mutex_lock( &pHarness->m_hRunMainThreadMutex );
	pthread_cond_signal( &pHarness->m_hRunMainThread );
	pthread_mutex_unlock( &pHarness->m_hRunMainThreadMutex );
	pHarness->m_bTestActive = false;
	if ( pHarness->m_bStopTestThread )
	{
		pthread_exit( NULL );
	}

	pHarness->m_bLetTestThreadRun = true;
	pthread_mutex_lock( &pHarness->m_hRunTestThreadMutex );
	pthread_cond_wait( &pHarness->m_hRunTestThread, &pHarness->m_hRunTestThreadMutex );
	pthread_mutex_unlock( &pHarness->m_hRunTestThreadMutex );
	pHarness->m_pTestFunc( pHarness->m_pvTestThreadArg );
	pHarness->m_hThreadTestDriver = 0;
	pHarness->m_bTestThreadRunning = false;
	pHarness->m_bStopTestThread = false;

	pthread_mutex_lock( &pHarness->m_hRunMainThreadMutex );
	pthread_cond_signal( &pHarness->m_hRunMainThread );
	pthread_mutex_unlock( &pHarness->m_hRunMainThreadMutex );
	pHarness->m_bTestActive = false;
	return NULL;
}

extern "C"
{

void Test_RunTest( TestFunc func, void* pvArg )
{
	g_TestHarness.m_ulMainThreadID = pthread_self();
	g_TestHarness.m_pvTestThreadArg = pvArg;
	g_TestHarness.m_pTestFunc = func;
	if ( !g_TestHarness.m_bTestThreadRunning )
	{
		g_TestHarness.m_bTestThreadRunning = true;
		pthread_mutex_init( &g_TestHarness.m_hRunTestThreadMutex, NULL );
		pthread_cond_init( &g_TestHarness.m_hRunTestThread, NULL );
		pthread_mutex_init( &g_TestHarness.m_hRunMainThreadMutex, NULL );
		pthread_cond_init( &g_TestHarness.m_hRunMainThread, NULL );
		pthread_create(
			&g_TestHarness.m_hThreadTestDriver,
			NULL,
			CTestHarness::TestThreadProxy,
			&g_TestHarness.m_ulTestThreadID );
	}
}

void Test_RunFrame()
{
	if ( g_TestHarness.m_bTestThreadRunning && g_TestHarness.m_bLetTestThreadRun )
	{
		g_TestHarness.m_bLetTestThreadRun = false;
		pthread_mutex_lock( &g_TestHarness.m_hRunTestThreadMutex );
		pthread_cond_signal( &g_TestHarness.m_hRunTestThread );
		pthread_mutex_unlock( &g_TestHarness.m_hRunTestThreadMutex );
		g_TestHarness.m_bTestActive = true;
		pthread_mutex_lock( &g_TestHarness.m_hRunMainThreadMutex );
		pthread_cond_wait( &g_TestHarness.m_hRunMainThread, &g_TestHarness.m_hRunMainThreadMutex );
		pthread_mutex_unlock( &g_TestHarness.m_hRunMainThreadMutex );
	}
}

void TestThread_Yield()
{
	pthread_mutex_lock( &g_TestHarness.m_hRunMainThreadMutex );
	pthread_cond_signal( &g_TestHarness.m_hRunMainThread );
	pthread_mutex_unlock( &g_TestHarness.m_hRunMainThreadMutex );
	g_TestHarness.m_bTestActive = false;
	if ( g_TestHarness.m_bStopTestThread )
	{
		pthread_exit( NULL );
	}

	g_TestHarness.m_bLetTestThreadRun = true;
	pthread_mutex_lock( &g_TestHarness.m_hRunTestThreadMutex );
	pthread_cond_wait( &g_TestHarness.m_hRunTestThread, &g_TestHarness.m_hRunTestThreadMutex );
	pthread_mutex_unlock( &g_TestHarness.m_hRunTestThreadMutex );
}

void Test_SetFailed()
{
	g_TestHarness.m_bTestFailed = true;
}

bool Test_HasFailed()
{
	return g_TestHarness.m_bTestFailed;
}

bool Test_HasFinished()
{
	if ( !g_TestHarness.m_bTestThreadRunning )
	{
		return g_TestHarness.m_bTestActive;
	}

	return false;
}

bool Test_IsActive()
{
	return g_TestHarness.m_bTestActive;
}

void Test_TerminateThread()
{
	if ( g_TestHarness.m_hThreadTestDriver != 0 )
	{
		pthread_cancel( g_TestHarness.m_hThreadTestDriver );
		g_TestHarness.m_hThreadTestDriver = 0;
		g_TestHarness.m_bTestThreadRunning = false;
		g_TestHarness.m_bStopTestThread = false;
	}
}

}

#endif
