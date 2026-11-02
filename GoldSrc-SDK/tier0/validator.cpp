#include "tier0/validator.h"

#if defined( _WIN32 )
#include "tier0/dbg.h"
#include "tier0/memalloc.h"
#include "tier0/threadtools.h"
#include "tier0/vprof.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBG_SOURCE_FILE	DBG_BUILDSLAVE_PATH( "tier0\\validator.cpp" )

static const char* FormatMemorySize( float value, int digitsAfterDecimal, bool bUseBinary )
{
	static char output[8][32];
	static int current;

	float oneKb = bUseBinary ? 1024.0f : 1000.0f;
	float oneMb = oneKb * oneKb;

	char* out = output[current];
	current = ( current + 1 ) & 7;

	char suffix[8];
	if ( value > oneMb )
	{
		value /= oneMb;
		_snprintf( suffix, sizeof( suffix ), " Mb" );
	}
	else if ( value > oneKb )
	{
		value /= oneKb;
		_snprintf( suffix, sizeof( suffix ), " Kb" );
	}
	else
	{
		_snprintf( suffix, sizeof( suffix ), " bytes" );
	}

	if ( digitsAfterDecimal < 0 )
	{
		digitsAfterDecimal = 0;
	}

	char val[32];
	if ( fabs( value - (float)(int)value ) < 0.00001 )
	{
		_snprintf( val, sizeof( val ), "%i%s", (int)value, suffix );
	}
	else
	{
		char fmt[32];
		_snprintf( fmt, sizeof( fmt ), "%%.%if%s", digitsAfterDecimal, suffix );
		_snprintf( val, sizeof( val ), fmt, value );
	}

	const char* i = val;
	char* o = out;
	const char* dot = strstr( i, "." );
	if ( !dot )
	{
		dot = strstr( i, " " );
	}

	int pos = (int)( dot - i ) - 3;
	while ( *i )
	{
		if ( pos >= 0 && !( pos % 3 ) && o != out )
		{
			*o++ = ',';
		}

		pos--;
		*o++ = *i++;
	}

	*o = 0;
	return out;
}

#pragma warning( suppress: 26495 )
CValidator::CValidator( int nThreshold )
{
	m_pValObjectFirst = NULL;
	m_pValObjectLast = NULL;
	m_pValObjectCur = NULL;
	m_nClaimedBlocks = 0;
	m_bMemLeaks = false;
	m_bTooManyObjects = false;
	m_nThreshold = nThreshold;
	m_iThreadMutexValidationLock = 0;
	m_nMaxValObjects = MAX_VALIDATION_OBJECTS;
	m_pValObjects = (CValObject*)malloc( m_nMaxValObjects * sizeof( CValObject ) );
	AssertAt( m_pValObjects != NULL, 38 );
	m_nValObjects = 0;
}

CValidator::~CValidator()
{
	UnlockValidationLocks();
	free( m_pValObjects );
}

void CValidator::Push( const char* pType, void* pObject, const char* pName )
{
	if ( m_nValObjects < m_nMaxValObjects - 1 )
	{
		CValObject* pNext = &m_pValObjects[m_nValObjects++];
		pNext->Init( pType, pObject, pName, m_pValObjectCur, m_pValObjectLast );
		m_pValObjectLast = pNext;
		if ( !m_pValObjectFirst )
			m_pValObjectFirst = pNext;
		m_pValObjectCur = pNext;
	}
	else if ( !m_bTooManyObjects )
	{
		m_bTooManyObjects = true;
		AssertMsgAt( false, "CValidator : too many memory nodes to store, try again with a higher threshold value\n", 80 );
	}
}

void CValidator::Pop()
{
	if ( m_bTooManyObjects )
		return;

	AssertAt( NULL != m_pValObjectCur, 109 );
	CValObject* pObject = m_pValObjectCur;
	m_pValObjectCur = pObject->m_pParent;
	if ( pObject->m_nTotalBytes < m_nThreshold )
		--m_nValObjects;
}

void CValidator::ClaimMemory( void* pMemory )
{
	if ( !m_bTooManyObjects && pMemory )
	{
		AssertAt( NULL != m_pValObjectCur, 143 );
		m_pValObjectCur->ClaimMemory( pMemory );
		++m_nClaimedBlocks;
	}
}

void CValidator::ClaimArrayMemory( void* pMemory )
{
	if ( !m_bTooManyObjects && pMemory )
	{
		m_pValObjectCur->ClaimMemory( (unsigned char*)pMemory - sizeof( int ) );
		++m_nClaimedBlocks;
	}
}

CValObject* CValidator::FindObject( void* pObject )
{
	for ( CValObject* pNode = m_pValObjectFirst; pNode; pNode = pNode->m_pValObjectNext )
	{
		if ( pNode->m_pObject == pObject )
			return pNode;
	}
	return NULL;
}

void CValidator::AddValidationLock( CThreadMutex* pMutex )
{
	AssertAt( m_iThreadMutexValidationLock < ( sizeof(m_threadMutexValidationLock)/sizeof(m_threadMutexValidationLock[0]) ), 518 );
	m_threadMutexValidationLock[m_iThreadMutexValidationLock++] = pMutex;
	pMutex->Lock();
}

void CValidator::UnlockValidationLocks()
{
	for ( int i = 0; i < m_iThreadMutexValidationLock; ++i )
		m_threadMutexValidationLock[i]->Unlock();
	m_iThreadMutexValidationLock = 0;
}

bool CValidator::BExcludeAllocationFromTracking( const char* pFileName, int nLine )
{
	static const struct
	{
		const char* pFileName;
		int nLine;
	} exclusions[] =
	{
		{ "C-runtime internal", 0 },
		{ "server.dll", 0 },
		{ "steamclient.dll", 0 },
		{ "gsoapwrapper.dll", 0 },
		{ "misc.h", 67 },
		{ "secblock.h", 87 }
	};

	for ( int i = 0; i < sizeof( exclusions ) / sizeof( exclusions[0] ); ++i )
	{
		if ( !_stricmp( pFileName, exclusions[i].pFileName ) && nLine == exclusions[i].nLine )
			return true;
	}
	return false;
}

void CValidator::Finalize()
{
}

void CValidator::RenderLeaks()
{
}

void CValidator::RenderObjects( int nThreshold )
{
	if ( m_bTooManyObjects )
		return;

	for ( CValObject* pObject = m_pValObjectFirst; pObject; pObject = pObject->m_pValObjectNext )
	{
		if ( pObject->m_nTotalBytes >= nThreshold )
		{
			for ( int i = 0; i < pObject->m_nDepth; ++i )
				_DMsg( "console", 2, " " );
			_DMsg( "console", 2, "%s %s --> %d blocks = %s\n", pObject->m_pType, pObject->m_pName,
				pObject->m_nTotalBlocks, FormatMemorySize( (float)pObject->m_nTotalBytes, 2, true ) );
		}
	}

	AssertAt( g_pMemAlloc, 352 );
	const char* pSize = FormatMemorySize( (float)g_pMemAlloc->GetMemoryUsed(), 2, true );
	_DMsg( "console", 2, "Allocated:\t%d blocks\t%s\n", g_pMemAlloc->CountAllocations(), pSize );
}

void CValidator::DiffAgainst( CValidator* pPrevious )
{
	if ( m_bTooManyObjects )
		return;

	for ( CValObject* pObject = m_pValObjectFirst; pObject; pObject = pObject->m_pValObjectNext )
	{
		if ( pObject->m_nTotalBytes < 1024 || pObject->m_pObject == m_pValObjects )
			continue;

		CValObject* pOldObject = pPrevious->FindObject( pObject->m_pObject );
		pObject->m_bNew = pOldObject == NULL;
		if ( pOldObject && pObject->m_nTotalBytes - 1024 > pOldObject->m_nTotalBytes )
		{
			for ( int i = 0; i < pObject->m_nDepth; ++i )
				_DMsg( "console", 2, " " );

			if ( pObject->m_bNew )
			{
				_DMsg( "console", 2, "%s %s --> %d blocks = %s (new)\n", pObject->m_pType, pObject->m_pName,
					pObject->m_nTotalBlocks, FormatMemorySize( (float)pObject->m_nTotalBytes, 2, true ) );
			}
			else
			{
				const char* pGrowth = FormatMemorySize( (float)( pObject->m_nTotalBytes - pOldObject->m_nTotalBytes ), 2, true );
				const char* pSize = FormatMemorySize( (float)pObject->m_nTotalBytes, 2, true );
				_DMsg( "console", 2, "%s %s --> %d blocks = %s (%s larger)\n", pObject->m_pType,
					pObject->m_pName, pObject->m_nTotalBlocks, pSize, pGrowth );
			}
		}
	}
}

void CValidator::Validate( CValidator& validator, char* pName )
{
	validator.Push( "CValidator", this, pName );
	validator.ClaimMemory( this );
	validator.Pop();
}

void CTier0::ValidateGlobals( CValidator& validator )
{
	g_VProfCurrentProfile.Validate( validator, "g_VProfCurrentProfile" );
	ValidateMemory( validator );
	ValidatePME( validator );
}
#endif
