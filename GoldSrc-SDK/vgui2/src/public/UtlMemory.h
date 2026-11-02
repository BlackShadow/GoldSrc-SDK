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

#ifndef UTLMEMORY_H
#define UTLMEMORY_H

#ifdef _WIN32
#pragma once
#endif

#include <stdlib.h>
#include <new>

#ifdef _WIN32
extern "C" _ACRTIMP void __cdecl _wassert( const wchar_t* message, const wchar_t* file, unsigned int line );

#define UTLMEMORY_SOURCE_PATH	L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\public\\UtlMemory.h"
#endif

//-----------------------------------------------------------------------------
// The CUtlMemory class:
// A growable memory class which doubles in size by default.
//-----------------------------------------------------------------------------
template <class T>
class CUtlMemory
{
public:
	// constructor, destructor
	CUtlMemory( int growSize = 0, int initSize = 0 );
	CUtlMemory( T* pMemory, int numElements );
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

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( int num = 0 );
	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num );
	// Memory deallocation
	void Purge();

	bool IsExternallyAllocated() const;

	enum
	{
		EXTERNAL_BUFFER_MARKER	= -1
	};

	T*	m_pMemory;
	int	m_NumAllocated;
	int	m_GrowSize;

private:
	CUtlMemory( const CUtlMemory<T>& other );
	CUtlMemory<T>& operator=( const CUtlMemory<T>& other );
};

template <class T>
inline CUtlMemory<T>::CUtlMemory( int growSize, int initSize )
	: m_pMemory( NULL ), m_NumAllocated( initSize ), m_GrowSize( growSize )
{
	if ( m_NumAllocated > 0 )
	{
		m_pMemory = (T*)malloc( m_NumAllocated * sizeof( T ) );
	}
}

template <class T>
inline CUtlMemory<T>::CUtlMemory( T* pMemory, int numElements )
	: m_pMemory( pMemory ), m_NumAllocated( numElements ), m_GrowSize( EXTERNAL_BUFFER_MARKER )
{
}

template <class T>
inline CUtlMemory<T>::~CUtlMemory()
{
	Purge();
}

template <class T>
inline T& CUtlMemory<T>::operator[]( int i )
{
#ifdef _WIN32
	if ( !IsIdxValid( i ) )
	{
		_wassert( L"IsIdxValid(i)", UTLMEMORY_SOURCE_PATH, 155 );
	}
#endif

	return m_pMemory[i];
}

template <class T>
inline const T& CUtlMemory<T>::operator[]( int i ) const
{
#ifdef _WIN32
	if ( !IsIdxValid( i ) )
	{
		_wassert( L"IsIdxValid(i)", UTLMEMORY_SOURCE_PATH, 162 );
	}
#endif

	return m_pMemory[i];
}

//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------
template <class T>
inline bool CUtlMemory<T>::IsIdxValid( int i ) const
{
	return ( i >= 0 ) && ( i < m_NumAllocated );
}

//-----------------------------------------------------------------------------
// Gets the base address (can change when adding elements!)
//-----------------------------------------------------------------------------
template <class T>
inline T* CUtlMemory<T>::Base()
{
	return m_pMemory;
}

template <class T>
inline const T* CUtlMemory<T>::Base() const
{
	return m_pMemory;
}

//-----------------------------------------------------------------------------
// Size
//-----------------------------------------------------------------------------
template <class T>
inline int CUtlMemory<T>::NumAllocated() const
{
	return m_NumAllocated;
}

//-----------------------------------------------------------------------------
// is the memory externally allocated?
//-----------------------------------------------------------------------------
template <class T>
inline bool CUtlMemory<T>::IsExternallyAllocated() const
{
	return m_GrowSize < 0;
}

template <class T>
inline void CUtlMemory<T>::EnsureCapacity( int num )
{
	if ( m_NumAllocated >= num )
	{
		return;
	}

	Grow( num - m_NumAllocated );
}

//-----------------------------------------------------------------------------
// Grows the memory
//-----------------------------------------------------------------------------
template <class T>
inline void CUtlMemory<T>::Grow( int num )
{
	int growSize;

	if ( num > 0 )
	{
#ifdef _WIN32
		if ( IsExternallyAllocated() )
		{
			_wassert( L"0", UTLMEMORY_SOURCE_PATH, 233 );
		}
#endif

		growSize = num;
	}
	else
	{
		growSize = m_GrowSize;
		if ( growSize == 0 )
		{
			growSize = m_NumAllocated ? m_NumAllocated : 1;
		}
	}

	m_NumAllocated += growSize;

	if ( m_pMemory )
	{
		m_pMemory = (T*)realloc( m_pMemory, m_NumAllocated * sizeof( T ) );
	}
	else
	{
		m_pMemory = (T*)malloc( m_NumAllocated * sizeof( T ) );
	}

#ifdef _WIN32
	if ( !m_pMemory )
	{
		_wassert( L"m_pMemory", UTLMEMORY_SOURCE_PATH, 249 );
	}
#endif
}

//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template <class T>
inline void CUtlMemory<T>::Purge()
{
	if ( !IsExternallyAllocated() )
	{
		if ( m_pMemory )
		{
			free( m_pMemory );
			m_pMemory = NULL;
		}

		m_NumAllocated = 0;
	}
}

#endif // UTLMEMORY_H
