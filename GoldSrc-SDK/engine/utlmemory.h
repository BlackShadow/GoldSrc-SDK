//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// A growable memory class.
//=============================================================================

#pragma once

#include <cstdlib>
#include <new>

#if ( defined( HL25_HW_ENGINE ) || defined( SW_RENDERER ) ) && defined( _WIN32 )
#define UTL_ENGINE_ASSERTS
#endif

#if defined( _WIN32 )
extern "C" _ACRTIMP void __cdecl _wassert( const wchar_t* message, const wchar_t* file, unsigned int line );
#endif

#ifdef UTL_ENGINE_ASSERTS
#include <assert.h>

#define UTLMEMORY_SOURCE_PATH	L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\public\\UtlMemory.h"
#endif

//-----------------------------------------------------------------------------
// The CUtlMemory class:
// A growable memory class which doubles in size by default.
//-----------------------------------------------------------------------------
template <class T> class CUtlMemory
{
public:
	// constructor, destructor
	CUtlMemory( int growSize = 0, int initialSize = 0 );
	CUtlMemory( T* memory, int numElements );
	~CUtlMemory();

	// element access
	T& operator[]( int i );
	const T& operator[]( int i ) const;

	// Can we use this index?
	bool IsIdxValid( int i ) const;

	// Gets the base address (can change when adding elements!)
	T* Base();
	const T* Base() const;

	int NumAllocated() const;

	void Grow( int num );
	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num );
	// Memory deallocation
	void Purge();

	// is the memory externally allocated?
	bool IsExternallyAllocated() const;

	// Set the size by which the memory grows
	void SetGrowSize( int size );
	int GetGrowSize() const;

private:
	T* m_pMemory;
	int m_NumAllocated;
	int m_GrowSize;
};

template <class T>
CUtlMemory<T>::CUtlMemory( int growSize, int initialSize )
	: m_pMemory( NULL ), m_NumAllocated( initialSize ), m_GrowSize( growSize )
{
	if ( m_NumAllocated )
	{
		m_pMemory = (T*)malloc( m_NumAllocated * sizeof( T ) );
	}
}

template <class T>
CUtlMemory<T>::CUtlMemory( T* memory, int numElements )
	: m_pMemory( memory ), m_NumAllocated( numElements ), m_GrowSize( -1 )
{
}

template <class T> CUtlMemory<T>::~CUtlMemory()
{
	Purge();
}

//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------
template <class T> T& CUtlMemory<T>::operator[]( int i )
{
#ifdef UTL_ENGINE_ASSERTS
	if ( !IsIdxValid( i ) )
		_wassert( L"IsIdxValid(i)", UTLMEMORY_SOURCE_PATH, 155 );
#endif
	return m_pMemory[i];
}

template <class T> const T& CUtlMemory<T>::operator[]( int i ) const
{
#ifdef UTL_ENGINE_ASSERTS
	if ( !IsIdxValid( i ) )
		_wassert( L"IsIdxValid(i)", UTLMEMORY_SOURCE_PATH, 162 );
#endif
	return m_pMemory[i];
}

//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------
template <class T> bool CUtlMemory<T>::IsIdxValid( int i ) const
{
	return ( i >= 0 ) && ( i < m_NumAllocated );
}

//-----------------------------------------------------------------------------
// Gets the base address (can change when adding elements!)
//-----------------------------------------------------------------------------
template <class T> T* CUtlMemory<T>::Base()
{
	return m_pMemory;
}

template <class T> const T* CUtlMemory<T>::Base() const
{
	return m_pMemory;
}

//-----------------------------------------------------------------------------
// Size
//-----------------------------------------------------------------------------
template <class T> int CUtlMemory<T>::NumAllocated() const
{
	return m_NumAllocated;
}

//-----------------------------------------------------------------------------
// is the memory externally allocated?
//-----------------------------------------------------------------------------
template <class T> bool CUtlMemory<T>::IsExternallyAllocated() const
{
	return m_GrowSize < 0;
}

template <class T> int CUtlMemory<T>::GetGrowSize() const
{
	return m_GrowSize;
}

template <class T> void CUtlMemory<T>::Grow( int num )
{
	int growSize;

	if ( num > 0 )
	{
#ifdef UTL_ENGINE_ASSERTS
		if ( IsExternallyAllocated() )
			_wassert( L"0", UTLMEMORY_SOURCE_PATH, 233 );
#endif
		growSize = num;
	}
	else
	{
		growSize = m_GrowSize;
		if ( !growSize )
			growSize = m_NumAllocated ? m_NumAllocated : 1;
	}

	int numAllocated = m_NumAllocated + growSize;
	T* memory;
	if ( m_pMemory )
		memory = (T*)realloc( m_pMemory, numAllocated * sizeof( T ) );
	else
		memory = (T*)malloc( numAllocated * sizeof( T ) );
	if ( !memory )
	{
#ifdef UTL_ENGINE_ASSERTS
		_wassert( L"m_pMemory", UTLMEMORY_SOURCE_PATH, 249 );
#endif
		std::abort();
	}
	m_pMemory = memory;
	m_NumAllocated = numAllocated;
}

//-----------------------------------------------------------------------------
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
template <class T> void CUtlMemory<T>::EnsureCapacity( int num )
{
	if ( m_NumAllocated >= num )
		return;

	Grow( num - m_NumAllocated );
}

//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template <class T> void CUtlMemory<T>::Purge()
{
	if ( IsExternallyAllocated() )
		return;

	if ( m_pMemory )
	{
		free( m_pMemory );
		m_pMemory = NULL;
	}
	m_NumAllocated = 0;
}
