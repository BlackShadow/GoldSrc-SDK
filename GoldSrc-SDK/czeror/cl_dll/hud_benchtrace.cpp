// hud_benchtrace.cpp
// Functions for spawning a thread to get a hopcount to a particular ip address and returning the result in a specified
//  variable

#include "hud_benchtrace.h"

#ifdef _WIN32
#include "winsani_in.h"
#include <windows.h>
#include "winsani_out.h"
#include <string.h>

// For tracking the trace threads
typedef struct
{
	// Inputs
	char	server[256];

	// Outputs
	int*	p_nresults;
	int*	p_ndone;

	// Local variables
	DWORD	hThreadId;
	HANDLE	hThread;
	HANDLE	hEventDone;
} trace_params_t;

// For doing the actual traceroute
struct trace_options_s
{
	unsigned char ucTTL;
	unsigned char a[7];
};

// Static forces it to be zeroed out
static trace_params_t tp;

struct
{
	DWORD			dwAddress;
	unsigned long	ulStatus;
	unsigned long	ulRoundTripTime;
	unsigned char	a[8];
	trace_options_s Options;
} traceReturn;

int Trace_GetHopCount( char* server, int maxHops )
{
	HMODULE icmp = LoadLibrary( "ICMP.DLL" );
	if ( !icmp )
		return -1;
	HANDLE ( WINAPI *createFile )() = (HANDLE ( WINAPI * )())GetProcAddress( icmp, "IcmpCreateFile" );
	BOOL ( WINAPI *closeFile )( HANDLE ) = (BOOL ( WINAPI * )( HANDLE ))GetProcAddress( icmp, "IcmpCloseHandle" );
	DWORD ( WINAPI *sendEcho )( HANDLE, DWORD, LPVOID, WORD, LPVOID, LPVOID, DWORD, DWORD ) =
		(DWORD ( WINAPI * )( HANDLE, DWORD, LPVOID, WORD, LPVOID, LPVOID, DWORD, DWORD ))
		GetProcAddress( icmp, "IcmpSendEcho" );
	if ( !createFile || !closeFile || !sendEcho )
	{
		FreeLibrary( icmp );
		return -1;
	}

	HANDLE session = createFile();
	if ( !session || session == INVALID_HANDLE_VALUE )
	{
		FreeLibrary( icmp );
		return -1;
	}
	hostent* host = gethostbyname( server );
	if ( !host )
	{
		closeFile( session );
		FreeLibrary( icmp );
		return -1;
	}

	DWORD* address = (DWORD*)*host->h_addr_list;
	trace_options_s options = {};
	int hops;
	for ( hops = 1; hops <= maxHops; hops++ )
	{
		// Set TTL correctly
		options.ucTTL = (unsigned char)hops;

		// Clear out return structure
		memset( &traceReturn, 0, sizeof( traceReturn ) );
		sendEcho( session, *address, NULL, 0, &options, &traceReturn, sizeof( traceReturn ), 2000 );
		if ( traceReturn.dwAddress == *address )
			break;
	}
	closeFile( session );
	FreeLibrary( icmp );
	return hops > maxHops ? -1 : hops;
}

/*
==============
Trace_Cleanup

Destroys thread and event handle when trace is done, or when restarting a new trace
==============
*/
void Trace_Cleanup()
{
	if ( tp.hThread )
	{
#pragma warning( suppress : 6258 )
		TerminateThread( tp.hThread, 0 );
		CloseHandle( tp.hThread );
		tp.hThread = NULL;
	}
	if ( tp.hEventDone )
	{
		CloseHandle( tp.hEventDone );
		tp.hEventDone = NULL;
	}
}

DWORD WINAPI Trace_ThreadFunction( LPVOID parameter )
{
	*(int*)parameter = Trace_GetHopCount( tp.server, 30 );
	SetEvent( tp.hEventDone );
	return 0;
}

/*
==============
Trace_StartTrace

Create finish event, sets up data, and starts thread to do a traceroute.
==============
*/
void Trace_StartTrace( int* results, int* finished, const char* server )
{
	tp.p_nresults = results;
	strcpy( tp.server, server );
	*results = -1;
	Trace_Cleanup();
	tp.hEventDone = CreateEvent( NULL, TRUE, FALSE, NULL );
	if ( !tp.hEventDone )
		return;
	tp.p_ndone = finished;
	*finished = 0;
	tp.hThread = CreateThread( NULL, 0, Trace_ThreadFunction, results, 0, &tp.hThreadId );
}

/*
==============
Trace_Think

Invoked by general frame loop on client to periodically check if the traceroute thread has completed.
==============
*/
void Trace_Think()
{
	if ( !tp.hEventDone )
		return;
	if ( WaitForSingleObject( tp.hEventDone, 0 ) == WAIT_OBJECT_0 )
	{
		Trace_Cleanup();
		*tp.p_ndone = 1;
	}
}
#else
int Trace_GetHopCount( char*, int )
{
	return -1;
}

void Trace_StartTrace( int*, int*, const char* )
{
}

void Trace_Think()
{
}
#endif
