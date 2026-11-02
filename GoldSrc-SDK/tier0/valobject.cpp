#include "tier0/valobject.h"

#if defined( _WIN32 )
#include "tier0/dbg.h"
#include "tier0/memalloc.h"

#define DBG_SOURCE_FILE	DBG_BUILDSLAVE_PATH( "tier0\\valobject.cpp" )

void CValObject::Init( const char* pType, void* pObject, const char* pName, CValObject* pParent, CValObject* pValObjectPrev )
{
	m_pType = pType;
	m_pName = pName;
	m_pObject = pObject;
	m_pParent = pParent;
	m_nReserved = 0;
	m_nDepth = pParent ? pParent->m_nDepth + 1 : 0;
	m_nBlocks = 0;
	m_nBytes = 0;
	m_nTotalBlocks = 0;
	m_nTotalBytes = 0;
	if ( pValObjectPrev )
	{
		AssertAt( NULL == pValObjectPrev->m_pValObjectNext, 48 );
		pValObjectPrev->m_pValObjectNext = this;
	}
	m_pValObjectNext = NULL;
}

void CValObject::ClaimMemory( void* pMemory )
{
	int nBytes = (int)g_pMemAlloc->GetSize( pMemory );
	++m_nBlocks;
	m_nBytes += nBytes;
	ClaimChildMemory( nBytes );
}

void CValObject::ClaimChildMemory( int nBytes )
{
	for ( CValObject* pObject = this; pObject; pObject = pObject->m_pParent )
	{
		++pObject->m_nTotalBlocks;
		pObject->m_nTotalBytes += nBytes;
	}
}
#endif
