//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================

#include "icommandline.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#if defined( _WIN32 )
#include <windows.h>
#endif

#define MAX_PARAMETER_LEN	128
#define COMMAND_LINE_SIZE	4096

class CCommandLine : public ICommandLine
{
public:
	// Construction
	CCommandLine();
	virtual ~CCommandLine();

	void CreateCmdLine( const char* commandline ) override;
#if !defined( _WIN32 )
	void CreateCmdLine( int argc, const char** argv ) override;
#endif
	const char* GetCmdLine() override;
	char* CheckParm( const char* psz, char** ppszValue ) override;

	void RemoveParm( const char* pszParm ) override;
	void AppendParm( const char* pszParm, const char* pszValues ) override;
	void SetParm( const char* pszParm, const char* pszValues ) override;
	void SetParm( const char* pszParm, int iValue ) override;

private:
	char* m_pszCmdLine;
};

static CCommandLine g_CmdLine;
ICommandLine* cmdline = &g_CmdLine;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCommandLine::CCommandLine()
{
	m_pszCmdLine = NULL;
}

CCommandLine::~CCommandLine()
{
	delete[] m_pszCmdLine;
}

#if !defined( _WIN32 )
void CCommandLine::CreateCmdLine( int argc, const char** argv )
{
	char szFull[COMMAND_LINE_SIZE];
	szFull[0] = '\0';

	for ( int i = 0; i < argc; ++i )
	{
		if ( strchr( argv[i], ' ' ) )
		{
			strcat( szFull, "\"" );
			strncat( szFull, argv[i], sizeof( szFull ) );
			strcat( szFull, "\"" );
		}
		else
		{
			strncat( szFull, argv[i], sizeof( szFull ) );
		}

		strcat( szFull, " " );
	}

	if ( argc > 0 )
		szFull[strlen( szFull ) - 1] = '\0';

	CreateCmdLine( szFull );
}
#endif

void CCommandLine::CreateCmdLine( const char* commandline )
{
	char szFull[COMMAND_LINE_SIZE];
	char* pOrig = NULL;

	delete[] m_pszCmdLine;

	if ( commandline )
	{
		pOrig = new char[strlen( commandline ) + 1];
		strcpy( pOrig, commandline );
	}

	char* pSrc = pOrig;
	char* pDst = szFull;
	bool allowAtSign = true;

#if defined( _MSC_VER )
#pragma warning( suppress: 6011 )
#endif
	while ( *pSrc && pDst - szFull < sizeof( szFull ) - 1 )
	{
		if ( allowAtSign && *pSrc == '@' )
		{
#if defined( _WIN32 )
			char szFileName[MAX_PATH];
			size_t len = strcspn( ++pSrc, "" );
#else
			char szFileName[COMMAND_LINE_SIZE];
			size_t len = strlen( ++pSrc );
#endif
			if ( len > sizeof( szFileName ) - 2 )
			{
				printf( "Parameter filename too long, skipping\n" );
				pSrc += len;
				if ( *pSrc )
					++pSrc;
				continue;
			}

			memcpy( szFileName, pSrc, len );
			szFileName[len] = '\0';
			// Skip the space after the file name
			if ( *pSrc )
				++pSrc;

			// Now read in parameters from file
			FILE* fp = fopen( szFileName, "r" );
			if ( !fp )
			{
				printf( "Parameter file '%s' not found, skipping...", szFileName );
				continue;
			}

			char c = (char)fgetc( fp );
			while ( c != EOF && pDst - szFull < sizeof( szFull ) - 2 )
			{
				*pDst++ = c == '\n' ? ' ' : c;
				// Get the next character, if there are more
				c = (char)fgetc( fp );
			}

			// Add a terminating space character
			*pDst++ = ' ';
			fclose( fp );
		}
		else
		{
			allowAtSign = isspace( (signed char)*pSrc ) != 0;
			*pDst++ = *pSrc++;
		}
	}

	*pDst = '\0';
	delete[] pOrig;
	m_pszCmdLine = new char[strlen( szFull ) + 1];
	strcpy( m_pszCmdLine, szFull );
}

//-----------------------------------------------------------------------------
// Purpose: Remove specified string ( and any args attached to it ) from command line
// Input  : *pszParm - 
//-----------------------------------------------------------------------------
void CCommandLine::RemoveParm( const char* pszParm )
{
	if ( !m_pszCmdLine || !pszParm || !*pszParm )
		return;

	char* found;
	while ( ( found = strstr( m_pszCmdLine, pszParm ) ) != NULL )
	{
		int curlen = (int)strlen( m_pszCmdLine );
		char* pnextparam = found + 1;

		while ( pnextparam && *pnextparam && *pnextparam != '-' && *pnextparam != '+' )
			++pnextparam;

		if ( pnextparam && *pnextparam )
		{
			int curpos = (int)( pnextparam - m_pszCmdLine );
			int n = curlen - curpos;
			memmove( found, pnextparam, n );
			found[n] = '\0';
		}
		else
		{
			memset( found, 0, pnextparam - found );
		}
	}

	while ( m_pszCmdLine[strlen( m_pszCmdLine ) - 1] == ' ' )
		m_pszCmdLine[strlen( m_pszCmdLine ) - 1] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: Append parameter and argument values to command line
// Input  : *pszParm - 
//			*pszValues - 
//-----------------------------------------------------------------------------
void CCommandLine::AppendParm( const char* pszParm, const char* pszValues )
{
	int nNewLength = (int)strlen( pszParm );
	if ( pszValues )
		nNewLength += (int)strlen( pszValues ) + 1; // Values + leading space character.
	nNewLength++; // Terminal 0;

	if ( !m_pszCmdLine )
	{
		m_pszCmdLine = new char[nNewLength];
		strcpy( m_pszCmdLine, pszParm );
		if ( pszValues )
		{
			strcat( m_pszCmdLine, " " );
			strcat( m_pszCmdLine, pszValues );
		}
		return;
	}

	// Remove any remnants from the current Cmd Line.
	RemoveParm( pszParm );
	nNewLength += (int)strlen( m_pszCmdLine ) + 2;
	char* pCmdString = new char[nNewLength];
	memset( pCmdString, 0, nNewLength );
	strcpy( pCmdString, m_pszCmdLine ); // Copy old command line.
	strcat( pCmdString, " " ); // Put in a space
	strcat( pCmdString, pszParm );
	if ( pszValues )
	{
		strcat( pCmdString, " " );
		strcat( pCmdString, pszValues );
	}

	delete[] m_pszCmdLine;
	m_pszCmdLine = pCmdString;
}

void CCommandLine::SetParm( const char* pszParm, const char* pszValues )
{
	RemoveParm( pszParm );
	AppendParm( pszParm, pszValues );
}

void CCommandLine::SetParm( const char* pszParm, int iValue )
{
	char buf[64];
	_snprintf( buf, sizeof( buf ), "%d", iValue );
	SetParm( pszParm, buf );
}

//-----------------------------------------------------------------------------
// Purpose: Return current command line
// Output : const char
//-----------------------------------------------------------------------------
const char* CCommandLine::GetCmdLine()
{
	return m_pszCmdLine;
}

char* CCommandLine::CheckParm( const char* psz, char** ppszValue )
{
	static char sz[MAX_PARAMETER_LEN];
	if ( !m_pszCmdLine )
		return NULL;

	char* pret = strstr( m_pszCmdLine, psz );
	if ( !pret || !ppszValue )
		return pret;

	*ppszValue = NULL;
	char* p1 = pret;
	while ( *p1 && *p1 != ' ' )
		++p1;

	char* p2 = p1 + 1;
	int i = 0;
	while ( i < MAX_PARAMETER_LEN && *p2 && *p2 != ' ' )
		sz[i++] = *p2++;

	sz[i] = '\0';
	*ppszValue = sz;
	return pret;
}
