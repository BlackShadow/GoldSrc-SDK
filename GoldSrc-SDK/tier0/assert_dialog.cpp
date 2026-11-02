#include "tier0/dbg.h"
#include "tier0/platform.h"
#include "tier0/threadtools.h"

#if defined( _WIN32 )
#include <windows.h>
#include <mbstring.h>
#else
#include <strings.h>
#endif
#include <stdio.h>
#include <string.h>

struct CDialogInitInfo
{
	const char*	m_pFilename;
	int			m_iLine;
	const char*	m_pExpression;
};

struct CAssertDisable
{
	char			m_Filename[512];
	int				m_LineMin;
	int				m_LineMax;
	int				m_nIgnoreTimes;
	CAssertDisable*	m_pNext;
};

CAssertDisable* g_pAssertDisables = NULL;
bool g_bBreak = false;
CDialogInitInfo g_Info;
static bool s_bInAssert = false;
#if defined( _WIN32 )
static HINSTANCE g_hInstance;
static HWND g_hParentWindow;
static bool g_bAssertsEnabled = true;
static unsigned int g_nIgnoreRange = 5;
static unsigned int g_nIgnoreTimes = 1;

enum AssertDialogControl_t
{
	IDD_ASSERT				= 101,
	IDC_ASSERT_FILE			= 1000,
	IDC_ASSERT_LINE			= 1001,
	IDC_IGNORE_FILE			= 1002,
	IDC_IGNORE_NEARBY		= 1003,
	IDC_IGNORE_RANGE		= 1004,
	IDC_IGNORE_TIMES		= 1005,
	IDC_BREAK				= 1006,
	IDC_IGNORE_ALL			= 1008,
	IDC_IGNORE_ALWAYS		= 1009,
	IDC_IGNORE_COUNT		= 1010,
	IDC_ASSERT_EXPRESSION	= 1011
};

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void* )
{
	if ( reason == DLL_PROCESS_ATTACH )
	{
		g_hInstance = instance;
	}
	return TRUE;
}

static BOOL CALLBACK EnumFunc( HWND window, LPARAM processId )
{
	DWORD windowProcessId;
	if ( IsWindowVisible( window ) )
	{
		GetWindowThreadProcessId( window, &windowProcessId );
		if ( windowProcessId == (DWORD)processId )
		{
			g_hParentWindow = window;
			return FALSE;
		}
	}
	return TRUE;
}

static INT_PTR CALLBACK DialogFunc( HWND dialog, UINT message, WPARAM wParam, LPARAM );
#endif

CAssertDisable* CreateNewAssertDisable( const char* )
{
	CAssertDisable* pDisable = new CAssertDisable;
	pDisable->m_pNext = g_pAssertDisables;
	g_pAssertDisables = pDisable;
	pDisable->m_LineMin = -1;
	pDisable->m_LineMax = -1;
	pDisable->m_nIgnoreTimes = -1;
#if defined( _WIN32 )
	_mbsnbcpy( (unsigned char*)pDisable->m_Filename, (const unsigned char*)g_Info.m_pFilename,
		sizeof( pDisable->m_Filename ) - 1 );
#else
	strncpy( pDisable->m_Filename, g_Info.m_pFilename, sizeof( pDisable->m_Filename ) - 1 );
#endif
	pDisable->m_Filename[sizeof( pDisable->m_Filename ) - 1] = 0;
	return pDisable;
}

void IgnoreAssertsInCurrentFile()
{
	CreateNewAssertDisable( NULL );
}

CAssertDisable* IgnoreAssertsNearby( int nRange )
{
	CAssertDisable* pDisable = CreateNewAssertDisable( NULL );

	pDisable->m_LineMin = g_Info.m_iLine - nRange;
	pDisable->m_LineMax = g_Info.m_iLine - nRange;
	return pDisable;
}

#if defined( _WIN32 )
static bool CommandLineHas( const char* pParm )
{
	return _mbsstr( (const unsigned char*)Plat_GetCommandLine(), (const unsigned char*)pParm ) != NULL;
}
#else
static bool CommandLineHas( const char* pParm )
{
	return strstr( Plat_GetCommandLine(), pParm ) != NULL;
}
#endif

bool ShouldUseNewAssertDialog()
{
#if defined( _WIN32 )
	if ( CommandLineHas( "-mpi_worker" ) )
	{
		return false;
	}
	return Plat_IsInDebugSession();
#else
	Plat_GetCommandLine();
	return false;
#endif
}

bool DoNewAssertDialog( const char* pFilename, int line, const char* pExpression )
{
	if ( CommandLineHas( "-noassert" ) )
	{
		return false;
	}

	if ( CommandLineHas( "-nocrashdialog" ) )
	{
		return false;
	}

	if ( !ThreadInMainThread() )
	{
		return true;
	}

	if ( CommandLineHas( "-debugbreak" ) )
	{
		return true;
	}
#if defined( _WIN32 )
	if ( !g_bAssertsEnabled )
	{
		return false;
	}
#endif

	CAssertDisable** ppPrev = &g_pAssertDisables;
	CAssertDisable* pNode = g_pAssertDisables;
	while ( pNode )
	{
#if defined( _WIN32 )
		bool bNameMatch = _mbsicmp( (const unsigned char*)pNode->m_Filename, (const unsigned char*)pFilename ) == 0;
#else
		bool bNameMatch = strcasecmp( pNode->m_Filename, pFilename ) == 0;
#endif
		bool bLineMatch = false;

		if ( bNameMatch )
		{
			if ( pNode->m_LineMin == -1 )
			{
				bLineMatch = ( line >= -1 && line <= pNode->m_LineMax ) || pNode->m_LineMax == -1;
			}
			else
			{
				bLineMatch = line >= pNode->m_LineMin && line <= pNode->m_LineMax;
			}
		}

		if ( !bNameMatch || !bLineMatch )
		{
			ppPrev = &pNode->m_pNext;
			pNode = pNode->m_pNext;
			continue;
		}

		if ( pNode->m_nIgnoreTimes <= 0 )
		{
			return false;
		}

		pNode->m_nIgnoreTimes--;
		if ( pNode->m_nIgnoreTimes != 0 )
		{
			return false;
		}

		*ppPrev = pNode->m_pNext;
		delete pNode;
		pNode = *ppPrev;
	}

	g_bBreak = false;
	g_Info.m_pFilename = pFilename;
	g_Info.m_iLine = line;
	g_Info.m_pExpression = pExpression;
#if defined( _WIN32 )
	g_hParentWindow = NULL;
	EnumWindows( EnumFunc, GetCurrentProcessId() );
	DialogBoxParamA( g_hInstance, MAKEINTRESOURCEA( IDD_ASSERT ), g_hParentWindow, DialogFunc, 0 );
#else
	fprintf( stderr, "%s %i %s", pFilename, line, pExpression );
#endif
	return g_bBreak;
}

void SetInAssert( bool bState )
{
	s_bInAssert = bState;
}

bool IsInAssert()
{
	return s_bInAssert;
}

#if defined( _WIN32 )
static INT_PTR CALLBACK DialogFunc( HWND dialog, UINT message, WPARAM wParam, LPARAM )
{
	if ( message == WM_INITDIALOG )
	{
		SetDlgItemTextA( dialog, IDC_ASSERT_EXPRESSION, g_Info.m_pExpression );
		SetDlgItemTextA( dialog, IDC_ASSERT_FILE, g_Info.m_pFilename );
		SetDlgItemInt( dialog, IDC_ASSERT_LINE, g_Info.m_iLine, FALSE );
		SetDlgItemInt( dialog, IDC_IGNORE_RANGE, g_nIgnoreRange, FALSE );
		SetDlgItemInt( dialog, IDC_IGNORE_COUNT, g_nIgnoreTimes, FALSE );
		RECT rect;
		RECT desktop;
		GetWindowRect( dialog, &rect );
		GetWindowRect( GetDesktopWindow(), &desktop );
		SetWindowPos( dialog, NULL, ( desktop.right + rect.left - rect.right - desktop.left ) / 2,
			( desktop.bottom + rect.top - rect.bottom - desktop.top ) / 2, 0, 0, SWP_NOSIZE );
		return TRUE;
	}

	if ( message == WM_COMMAND )
	{
		BOOL translated = FALSE;
		switch ( LOWORD( wParam ) )
		{
		case IDC_IGNORE_FILE:
			IgnoreAssertsInCurrentFile();
			break;
		case IDC_IGNORE_NEARBY:
		{
			const UINT range = GetDlgItemInt( dialog, IDC_IGNORE_RANGE, &translated, FALSE );
			if ( !translated || range == 0 )
			{
				return TRUE;
			}
			IgnoreAssertsNearby( range );
			break;
		}
		case IDC_IGNORE_TIMES:
		{
			const UINT times = GetDlgItemInt( dialog, IDC_IGNORE_COUNT, &translated, FALSE );
			if ( translated && times > 1 )
			{
				CAssertDisable* pDisable = IgnoreAssertsNearby( 0 );
				g_nIgnoreTimes = times;
				pDisable->m_nIgnoreTimes = times - 1;
			}
			break;
		}
		case IDC_BREAK:
			g_bBreak = true;
			break;
		case IDC_IGNORE_ALL:
			g_bAssertsEnabled = false;
			break;
		case IDC_IGNORE_ALWAYS:
			IgnoreAssertsNearby( 0 );
			break;
		default:
			if ( wParam != IDCANCEL )
			{
				return TRUE;
			}
			break;
		}
		EndDialog( dialog, 0 );
		return TRUE;
	}

	if ( message == WM_KEYDOWN )
	{
		if ( wParam == IDCANCEL )
		{
			EndDialog( dialog, 0 );
		}
		return TRUE;
	}
	return FALSE;
}
#endif
