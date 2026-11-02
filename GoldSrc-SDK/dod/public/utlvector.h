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
// A growable array class that maintains a free list and keeps elements
// in the same location
//=============================================================================

#ifndef UTLVECTOR_H
#define UTLVECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include "UtlMemory.h"

#include <new>
#include <string.h>

//-----------------------------------------------------------------------------
// The CUtlVector class:
// A growable array class which doubles in size by default.
// It will always keep all elements consecutive in memory, and may move the
// elements around in memory (via a realloc) when elements are inserted or
// removed. Clients should therefore refer to the elements of the vector
// by index (they should *never* maintain pointers to elements in the vector).
//-----------------------------------------------------------------------------
template <class T>
class CUtlVector
{
public:
	typedef T ElemType_t;

	// constructor, destructor
	CUtlVector( int growSize = 0, int initSize = 0 );
	~CUtlVector();

	// element access
	T& operator[]( int i );
	const T& operator[]( int i ) const;
	T& Element( int i );
	const T& Element( int i ) const;

	T* Base();
	const T* Base() const;

	// Returns the number of elements in the vector
	// SIZE IS DEPRECATED!
	int Count() const;

	// Is element index valid?
	bool IsValidIndex( int i ) const;

	int AddToTail();
	int AddToTail( const T& src );
	int AddMultipleToTail( int num );
	int InsertBefore( int elem, const T& src );

	void Remove( int elem );

	// Element removal
	void FastRemove( int elem );	// doesn't preserve order
	void RemoveAll();

	// Memory deallocation
	void Purge();
	void EnsureCapacity( int num )
	{
		m_Memory.EnsureCapacity( num );
	}

private:
	void DestructElements();
	void GrowVector( int num );

	CUtlMemory<T>	m_Memory;
	int				m_Size;

	// For easier access to the elements through the debugger
	// it's in release builds so this can be used in libraries correctly
	T*				m_pElements;
};

template <class T>
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
inline CUtlVector<T>::CUtlVector( int growSize, int initSize )
	: m_Memory( growSize, initSize ), m_Size( 0 )
{
}

template <class T>
inline CUtlVector<T>::~CUtlVector()
{
	Purge();
}

//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------

template <class T>
inline T& CUtlVector<T>::operator[]( int i )
{
	return m_Memory[i];
}

template <class T>
inline const T& CUtlVector<T>::operator[]( int i ) const
{
	return m_Memory[i];
}

template <class T>
inline T& CUtlVector<T>::Element( int i )
{
	return m_Memory[i];
}

template <class T>
inline const T& CUtlVector<T>::Element( int i ) const
{
	return m_Memory[i];
}


//-----------------------------------------------------------------------------
// Gets the base address (can change when adding elements!)
//-----------------------------------------------------------------------------

template <class T>
inline T* CUtlVector<T>::Base()
{
	return m_Memory.Base();
}

template <class T>
inline const T* CUtlVector<T>::Base() const
{
	return m_Memory.Base();
}

template <class T>
inline int CUtlVector<T>::Count() const
{
	return m_Size;
}


//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------

template <class T>
inline bool CUtlVector<T>::IsValidIndex( int i ) const
{
	return i >= 0 && i < m_Size;
}

template <class T>
inline int CUtlVector<T>::AddToTail()
{
	int index = m_Size;
	GrowVector( 1 );
	new ( &m_Memory[index] ) T;
	return index;
}

template <class T>
inline int CUtlVector<T>::AddToTail( const T& src )
{
	return InsertBefore( m_Size, src );
}

template <class T>
inline int CUtlVector<T>::AddMultipleToTail( int num )
{
	int first;

	if ( num <= 0 )
	{
		return m_Size;
	}

	first = m_Size;
	GrowVector( num );

	for ( int i = 0; i < num; ++i )
	{
		new ( &m_Memory[first + i] ) T;
	}

	return first;
}

template <class T>
inline void CUtlVector<T>::GrowVector( int num )
{
	if ( m_Size + num > m_Memory.NumAllocated() )
	{
		m_Memory.Grow( m_Size + num - m_Memory.NumAllocated() );
	}

	m_Size += num;
}

template <class T>
inline int CUtlVector<T>::InsertBefore( int elem, const T& src )
{
	GrowVector( 1 );
	int numToMove = m_Size - elem - 1;
	if ( numToMove > 0 )
	{
		memmove( &m_Memory[elem + 1], &m_Memory[elem], numToMove * sizeof( T ) );
	}

	T* dest = &m_Memory[elem];
	if ( dest )
	{
		new ( dest ) T( src );
	}

	return elem;
}

template <class T>
inline void CUtlVector<T>::Remove( int elem )
{
	if ( !IsValidIndex( elem ) )
	{
		return;
	}

	for ( int i = elem; i < m_Size - 1; ++i )
	{
		m_Memory[i] = m_Memory[i + 1];
	}

	m_Memory[m_Size - 1].~T();
	--m_Size;
}

template <class T>
inline void CUtlVector<T>::FastRemove( int elem )
{
	if ( !IsValidIndex( elem ) )
	{
		return;
	}

	if ( elem != m_Size - 1 )
	{
		m_Memory[elem] = m_Memory[m_Size - 1];
	}

	m_Memory[m_Size - 1].~T();
	--m_Size;
}

template <class T>
inline void CUtlVector<T>::DestructElements()
{
	for ( int i = 0; i < m_Size; ++i )
	{
		m_Memory[i].~T();
	}

	m_Size = 0;
}

template <class T>
inline void CUtlVector<T>::RemoveAll()
{
	DestructElements();
}

template <class T>
inline void CUtlVector<T>::Purge()
{
	DestructElements();
	m_Memory.Purge();
}

#endif
