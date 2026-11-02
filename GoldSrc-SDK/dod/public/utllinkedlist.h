//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Linked list container class 
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#ifndef UTLLINKEDLIST_H
#define UTLLINKEDLIST_H

#ifdef _WIN32
#pragma once
#endif

#include "UtlMemory.h"

#include <new>

template <class T, class I>
struct UtlLinkedListElem_t
{
	T	m_Element;
	I	m_Previous;
	I	m_Next;
};

template <class T, class I = int>
class CUtlLinkedList
{
public:
	typedef T ElemType_t;
	typedef I IndexType_t;

	// constructor, destructor
	CUtlLinkedList( int growSize = 0, int initSize = 0 );
	~CUtlLinkedList();

	T& operator[]( I i );
	const T& operator[]( I i ) const;
	T& Element( I i );
	const T& Element( I i ) const;

	I AddToTail();
	I AddToTail( const T& src );
	I InsertBefore( I before, const T& src );

	// Removal methods
	void Remove( I elem );
	void RemoveAll();

	static I InvalidIndex()
	{
		return (I)-1;
	}

	bool IsValidIndex( I i ) const;

	// list statistics
	int Count() const;
	I MaxElementIndex() const
	{
		return m_TotalElements;
	}

	// Traversing the list
	I Head() const;
	I Next( I i ) const;

protected:
	typedef UtlLinkedListElem_t<T, I> ListElem_t;

	I AllocInternal();
	void LinkToTail( I elem );

	CUtlMemory<ListElem_t>	m_Memory;

	I						m_Head;
	I						m_Tail;

	I						m_FirstFree;
	I						m_ElementCount;		// The number actually in the list
	I						m_TotalElements;	// The number allocated

	// For debugging purposes; 
	// it's in release builds so this can be used in libraries correctly
	ListElem_t*				m_pElements;
};

template <class T, class I>
inline CUtlLinkedList<T, I>::CUtlLinkedList( int growSize, int initSize )
	: m_Memory( growSize, initSize ),
	  m_Head( InvalidIndex() ),
	  m_Tail( InvalidIndex() ),
	  m_FirstFree( InvalidIndex() ),
	  m_ElementCount( 0 ),
	  m_TotalElements( 0 ),
	  m_pElements( m_Memory.Base() )
{
}

template <class T, class I>
inline CUtlLinkedList<T, I>::~CUtlLinkedList()
{
	for ( I i = 0; i < m_TotalElements; ++i )
	{
		m_Memory[i].~UtlLinkedListElem_t<T, I>();
	}
}

template <class T, class I>
inline T& CUtlLinkedList<T, I>::operator[]( I i )
{
	return m_Memory[i].m_Element;
}

template <class T, class I>
inline const T& CUtlLinkedList<T, I>::operator[]( I i ) const
{
	return m_Memory[i].m_Element;
}


//-----------------------------------------------------------------------------
// gets particular elements
//-----------------------------------------------------------------------------

template <class T, class I>
inline T& CUtlLinkedList<T, I>::Element( I i )
{
	return m_Memory[i].m_Element;
}

template <class T, class I>
inline const T& CUtlLinkedList<T, I>::Element( I i ) const
{
	return m_Memory[i].m_Element;
}


//-----------------------------------------------------------------------------
// Are nodes in the list or valid?
//-----------------------------------------------------------------------------

template <class T, class I>
inline bool CUtlLinkedList<T, I>::IsValidIndex( I i ) const
{
	if ( i == InvalidIndex() )
	{
		return false;
	}

	if ( (unsigned int)i >= (unsigned int)m_TotalElements )
	{
		return false;
	}

	return m_Memory[i].m_Previous != i || m_Memory[i].m_Next == i;
}

//-----------------------------------------------------------------------------
// list statistics
//-----------------------------------------------------------------------------

template <class T, class I>
inline int CUtlLinkedList<T, I>::Count() const
{
	return (int)m_ElementCount;
}


//-----------------------------------------------------------------------------
// Traversing the list
//-----------------------------------------------------------------------------

template <class T, class I>
inline I CUtlLinkedList<T, I>::Head() const
{
	return m_Head;
}

template <class T, class I>
inline I CUtlLinkedList<T, I>::Next( I i ) const
{
	return m_Memory[i].m_Next;
}

template <class T, class I>
inline I CUtlLinkedList<T, I>::AllocInternal()
{
	I elem;

	if ( m_FirstFree != InvalidIndex() )
	{
		elem = m_FirstFree;
		m_FirstFree = m_Memory[elem].m_Next;
	}
	else
	{
		elem = m_TotalElements;
		if ( (int)m_TotalElements == m_Memory.NumAllocated() )
		{
			m_Memory.Grow( 0 );
		}

		new ( &m_Memory[elem] ) ListElem_t();
		m_TotalElements = (I)( (int)m_TotalElements + 1 );
	}

	m_Memory[elem].m_Previous = elem;
	m_Memory[elem].m_Next = elem;
	m_pElements = m_Memory.Base();
	return elem;
}

template <class T, class I>
inline void CUtlLinkedList<T, I>::LinkToTail( I elem )
{
	m_Memory[elem].m_Previous = m_Tail;
	m_Memory[elem].m_Next = InvalidIndex();

	if ( m_Head == InvalidIndex() )
	{
		m_Head = elem;
	}
	else
	{
		m_Memory[m_Tail].m_Next = elem;
	}

	m_Tail = elem;
	m_ElementCount = (I)( (int)m_ElementCount + 1 );
}

template <class T, class I>
inline I CUtlLinkedList<T, I>::AddToTail()
{
	I elem = AllocInternal();
	LinkToTail( elem );
	return elem;
}

template <class T, class I>
inline I CUtlLinkedList<T, I>::AddToTail( const T& src )
{
	I elem = AddToTail();
	m_Memory[elem].m_Element = src;
	return elem;
}

template <class T, class I>
inline void CUtlLinkedList<T, I>::Remove( I elem )
{
	I prev;
	I next;

	prev = m_Memory[elem].m_Previous;
	next = m_Memory[elem].m_Next;

	if ( (unsigned int)elem < (unsigned int)m_TotalElements && prev != elem )
	{
		if ( prev != InvalidIndex() )
		{
			m_Memory[prev].m_Next = next;
		}
		else
		{
			m_Head = next;
		}

		if ( next != InvalidIndex() )
		{
			m_Memory[next].m_Previous = prev;
		}
		else
		{
			m_Tail = prev;
		}

		m_Memory[elem].m_Next = elem;
		m_Memory[elem].m_Previous = elem;
		m_ElementCount = (I)( (int)m_ElementCount - 1 );
		m_Memory[elem].m_Element.~T();
	}

	m_Memory[elem].m_Next = m_FirstFree;
	m_FirstFree = elem;
}

template <class T, class I>
inline void CUtlLinkedList<T, I>::RemoveAll()
{
	I i;

	i = m_Head;
	while ( i != InvalidIndex() )
	{
		I next = m_Memory[i].m_Next;
		m_Memory[i].m_Next = m_FirstFree;
		m_Memory[i].m_Previous = i;
		m_FirstFree = i;
		i = next;
	}

	// Clear everything else out
	m_Head = InvalidIndex();
	m_Tail = InvalidIndex();
	m_ElementCount = 0;
}

#endif // UTLLINKEDLIST_H
