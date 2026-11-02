/*==============================================================================
   Copyright (c) Valve, LLC. 1999
   All Rights Reserved.  Proprietary.
--------------------------------------------------------------------------------
   Author : DSpeyrer
   Purpose: Implements a circular buffer. Data can be written to and read from
			these buffers as though from a file.

			Circular buffers of a particular size may be declared using the
			SizedCircularBuffer() macro.
--------------------------------------------------------------------------------
 * $Log:                      $
 * 
 * $NoKeywords:
==============================================================================*/

#include "CircularBuffer.h"
#include <string.h>

#if defined( _WIN32 )
#include <assert.h>

#if defined( NDEBUG )
extern "C" _ACRTIMP void __cdecl _wassert( const wchar_t* message, const wchar_t* file, unsigned int line );
#endif

#define CIRCULARBUFFER_SOURCE_PATH L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\engine\\CircularBuffer.cpp"
#endif

CCircularBuffer::CCircularBuffer( int size )
{
	SetSize( size );
}

void CCircularBuffer::AssertValid()
{
}

void CCircularBuffer::SetSize( int size )
{
#if defined( _WIN32 )
	if ( !this )
		_wassert( L"this", CIRCULARBUFFER_SOURCE_PATH, 66 );
#endif
	m_nSize = size;
	Flush();
}

void CCircularBuffer::Flush()
{
	m_nRead = 0;
	m_nWrite = 0;
	m_nCount = 0;
}

int CCircularBuffer::GetWriteAvailable()
{
	return m_nSize - m_nCount;
}

int CCircularBuffer::GetSize()
{
	return m_nSize;
}

int CCircularBuffer::GetReadAvailable()
{
	return m_nCount;
}

int CCircularBuffer::Peek( char* pchDest, int nCount )
{
	if ( m_nCount == 0 )
		return 0;

	nCount = nCount < m_nCount ? nCount : m_nCount;

	// If buffer wrap occurs split the data into two chunks.
	//
	// Copy as many of the requested bytes as possible.
	//
	if ( m_nRead + nCount > m_nSize )
	{
		int nCount1 = m_nSize - m_nRead;
		memcpy( pchDest, &m_chData[m_nRead], nCount1 );
		memcpy( pchDest + nCount1, m_chData, nCount - nCount1 );
	}
	// Otherwise copy it in one go.
	else
	{
		memcpy( pchDest, &m_chData[m_nRead], nCount );
	}

	return nCount;
}

int CCircularBuffer::Advance( int nCount )
{
	if ( m_nCount == 0 )
		return 0;

	nCount = nCount < m_nCount ? nCount : m_nCount;
	//
	// Advance the read pointer, checking for buffer wrap.
	//
	m_nRead = ( m_nRead + nCount ) % m_nSize;
	m_nCount -= nCount;

	//
	// If we have emptied the buffer, reset the read and write indices
	// to minimize buffer wrap.
	//
	if ( m_nCount == 0 )
	{
		m_nRead = 0;
		m_nWrite = 0;
	}

	return nCount;
}

int CCircularBuffer::Read( void* pchDest, int nCount )
{
	int nPeeked = Peek( (char*)pchDest, nCount );
	if ( nPeeked == 0 )
		return 0;

	int nRead = Advance( nPeeked );
#if defined( _WIN32 )
	if ( nRead != nPeeked )
		_wassert( L"m_nRead == nPeeked", CIRCULARBUFFER_SOURCE_PATH, 233 );
#endif
	return nRead;
}

int CCircularBuffer::Write( void* pData, int nBytesRequested )
{
	char* pDataToWrite = (char*)pData;
	// Write all the data.
	int nBytesToWrite = nBytesRequested;

	while ( nBytesToWrite )
	{
		int from = m_nWrite;
		int to = m_nWrite + nBytesToWrite;
		if ( to > m_nSize )
			to = m_nSize;

		memcpy( &m_chData[from], pDataToWrite, to - from );
		pDataToWrite += to - from;

		m_nWrite = to % m_nSize;
		nBytesToWrite -= to - from;
	}

	// Did it cross the read pointer? Then slide the read pointer up.
	// This way, we will discard the old data.
	if ( nBytesRequested > m_nSize - m_nCount )
	{
		m_nRead = m_nWrite;
		m_nCount = m_nSize;
	}
	else
	{
		m_nCount += nBytesRequested;
	}

	return nBytesRequested;
}
