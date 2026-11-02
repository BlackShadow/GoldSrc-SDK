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

	T* Base();
	const T* Base() const;

	// Size
	int NumAllocated() const;

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( int num = 1 );

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num );

	// Memory deallocation
	void Purge();

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
		if ( !m_pMemory )
			throw std::bad_alloc();
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


//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------
template <class T>
inline T& CUtlMemory<T>::operator[]( int i )
{
	return m_pMemory[i];
}

template <class T>
inline const T& CUtlMemory<T>::operator[]( int i ) const
{
	return m_pMemory[i];
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
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
template <class T>
inline void CUtlMemory<T>::EnsureCapacity( int num )
{
	if ( m_NumAllocated >= num )
	{
		return;
	}

	if ( m_GrowSize < 0 )
	{
		return;
	}

	T* memory = (T*)realloc( m_pMemory, num * sizeof( T ) );
	if ( !memory )
		throw std::bad_alloc();
	m_pMemory = memory;
	m_NumAllocated = num;
}

template <class T>
inline void CUtlMemory<T>::Grow( int num )
{
	int allocationRequested;

	if ( m_GrowSize < 0 && num > 0 )
	{
		return;
	}

	if ( num > 0 )
	{
		allocationRequested = m_NumAllocated + num;
	}
	else if ( m_GrowSize != 0 )
	{
		allocationRequested = m_NumAllocated + m_GrowSize;
	}
	else
	{
		allocationRequested = ( m_NumAllocated == 0 ) ? 1 : ( m_NumAllocated * 2 );
	}

	T* memory;
	if ( m_pMemory )
	{
		memory = (T*)realloc( m_pMemory, (size_t)allocationRequested * sizeof( T ) );
	}
	else
	{
		memory = (T*)malloc( (size_t)allocationRequested * sizeof( T ) );
	}
	if ( !memory )
		throw std::bad_alloc();
	m_pMemory = memory;
	m_NumAllocated = allocationRequested;
}

template <class T>
inline void CUtlMemory<T>::Purge()
{
	if ( m_GrowSize >= 0 && m_pMemory )
	{
		free( m_pMemory );
	}

	m_pMemory = NULL;
	m_NumAllocated = 0;
}

#endif // UTLMEMORY_H
