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

#ifdef _WIN32
#ifndef UTLLINKEDLIST_SOURCE_PATH
#define UTLLINKEDLIST_SOURCE_PATH	L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\public\\utllinkedlist.h"
#endif
#endif

template <class T, class I>
struct UtlLinkedListElem_t
{
	T	m_Element;
	I	m_Previous;
	I	m_Next;
};

//-----------------------------------------------------------------------------
// class CUtlLinkedList:
// description:
//		A lovely index-based linked list! T is the class type, I is the index
//		type, which usually should be an unsigned short or smaller.
//-----------------------------------------------------------------------------

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
	I InsertBefore( I before );
	I InsertBefore( I before, const T& src );

	// Removal methods
	void Remove( I elem );
	void RemoveAll();

	void Free( I elem );

	// list modification
	void LinkBefore( I before, I elem );
	void LinkToTail( I elem );
	void Unlink( I elem );

	static I InvalidIndex()
	{
		return (I)-1;
	}

	bool IsValidIndex( I i ) const;
	bool IsInList( I i ) const;
	int Count() const;

	I Head() const;
	I Previous( I i ) const;
	I Next( I i ) const;

protected:
	typedef UtlLinkedListElem_t<T, I> ListElem_t;

	// constructs the class
	I AllocInternal( bool multilist = false );

	ListElem_t& InternalElement( I i );
	const ListElem_t& InternalElement( I i ) const;

	CUtlMemory<ListElem_t>	m_Memory;

	I						m_Head;
	I						m_Tail;

	I						m_FirstFree;
	I						m_ElementCount; // The number actually in the list
	I						m_TotalElements; // The number allocated

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
	RemoveAll();
}

template <class T, class I>
inline typename CUtlLinkedList<T, I>::ListElem_t& CUtlLinkedList<T, I>::InternalElement( I i )
{
	return m_Memory[i];
}

template <class T, class I>
inline const typename CUtlLinkedList<T, I>::ListElem_t& CUtlLinkedList<T, I>::InternalElement( I i ) const
{
	return m_Memory[i];
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
	return ( i < m_TotalElements ) && ( i >= 0 ) &&
		( ( m_Memory[i].m_Previous != i ) || ( m_Memory[i].m_Next == i ) );
}

template <class T, class I>
inline bool CUtlLinkedList<T, I>::IsInList( I i ) const
{
	return ( i < m_TotalElements ) && ( i >= 0 ) && ( Previous( i ) != i );
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
inline I CUtlLinkedList<T, I>::Previous( I i ) const
{
#ifdef _WIN32
	if ( !IsValidIndex( i ) )
	{
		_wassert( L"IsValidIndex(i)", UTLLINKEDLIST_SOURCE_PATH, 246 );
	}
#endif

	return InternalElement( i ).m_Previous;
}

template <class T, class I>
inline I CUtlLinkedList<T, I>::Next( I i ) const
{
#ifdef _WIN32
	if ( !IsValidIndex( i ) )
	{
		_wassert( L"IsValidIndex(i)", UTLLINKEDLIST_SOURCE_PATH, 253 );
	}
#endif

	return InternalElement( i ).m_Next;
}

//-----------------------------------------------------------------------------
// Node allocation/deallocation
//-----------------------------------------------------------------------------
template <class T, class I>
I CUtlLinkedList<T, I>::AllocInternal( bool multilist )
{
	I elem;

	if ( m_FirstFree == InvalidIndex() )
	{
		if ( (int)m_TotalElements == m_Memory.NumAllocated() )
		{
			m_Memory.Grow();
		}

		elem = m_TotalElements;
		++m_TotalElements;
	}
	else
	{
		elem = m_FirstFree;
		m_FirstFree = InternalElement( m_FirstFree ).m_Next;
	}

	if ( !multilist )
	{
		InternalElement( elem ).m_Next = InternalElement( elem ).m_Previous = elem;
	}
	else
	{
		InternalElement( elem ).m_Next = InternalElement( elem ).m_Previous = InvalidIndex();
	}

	new ( &Element( elem ) ) T;
	m_pElements = m_Memory.Base();

	return elem;
}

template <class T, class I>
void CUtlLinkedList<T, I>::Free( I elem )
{
#ifdef _WIN32
	if ( !IsValidIndex( elem ) )
	{
		_wassert( L"IsValidIndex(elem)", UTLLINKEDLIST_SOURCE_PATH, 371 );
	}
#endif

	Element( elem ).~T();

	if ( IsInList( elem ) )
	{
		Unlink( elem );
	}

	InternalElement( elem ).m_Next = m_FirstFree;
	m_FirstFree = elem;
}

template <class T, class I>
inline I CUtlLinkedList<T, I>::AddToTail()
{
	return InsertBefore( InvalidIndex() );
}

template <class T, class I>
inline I CUtlLinkedList<T, I>::AddToTail( const T& src )
{
	return InsertBefore( InvalidIndex(), src );
}

template <class T, class I>
I CUtlLinkedList<T, I>::InsertBefore( I before )
{
	// Make a new node
	I newNode = AllocInternal();
	// Link it in
	LinkBefore( before, newNode );
	new ( &Element( newNode ) ) T;
	return newNode;
}

template <class T, class I>
I CUtlLinkedList<T, I>::InsertBefore( I before, const T& src )
{
	// Make a new node
	I newNode = AllocInternal();
	// Link it in
	LinkBefore( before, newNode );
	new ( &Element( newNode ) ) T( src );
	return newNode;
}

//-----------------------------------------------------------------------------
// list modification
//-----------------------------------------------------------------------------

template <class T, class I>
void CUtlLinkedList<T, I>::LinkBefore( I before, I elem )
{
#ifdef _WIN32
	if ( !IsValidIndex( elem ) )
	{
		_wassert( L"IsValidIndex(elem)", UTLLINKEDLIST_SOURCE_PATH, 535 );
	}
#endif

	// Unlink it if it's in the list at the moment
	Unlink( elem );

	ListElem_t& newElem = InternalElement( elem );

	// The element *after* our newly linked one is the one we linked before.
	newElem.m_Next = before;

	if ( before == InvalidIndex() )
	{
		// In this case, we're linking to the end of the list, so reset the tail
		newElem.m_Previous = m_Tail;
		m_Tail = elem;
	}
	else
	{
#ifdef _WIN32
		if ( !IsInList( before ) )
		{
			_wassert( L"IsInList(before)", UTLLINKEDLIST_SOURCE_PATH, 555 );
		}
#endif

		ListElem_t& beforeElem = InternalElement( before );
		newElem.m_Previous = beforeElem.m_Previous;
		beforeElem.m_Previous = elem;
	}

	// Reset the head if we linked to the head of the list
	if ( newElem.m_Previous == InvalidIndex() )
	{
		m_Head = elem;
	}
	else
	{
		InternalElement( newElem.m_Previous ).m_Next = elem;
	}

	++m_ElementCount;
}

template <class T, class I>
inline void CUtlLinkedList<T, I>::LinkToTail( I elem )
{
	LinkBefore( InvalidIndex(), elem );
}

template <class T, class I>
void CUtlLinkedList<T, I>::Unlink( I elem )
{
#ifdef _WIN32
	if ( !IsValidIndex( elem ) )
	{
		_wassert( L"IsValidIndex(elem)", UTLLINKEDLIST_SOURCE_PATH, 613 );
	}
#endif

	if ( IsInList( elem ) )
	{
		ListElem_t& oldElem = InternalElement( elem );

		if ( oldElem.m_Previous != InvalidIndex() )
		{
			InternalElement( oldElem.m_Previous ).m_Next = oldElem.m_Next;
		}
		else
		{
			m_Head = oldElem.m_Next;
		}

		if ( oldElem.m_Next != InvalidIndex() )
		{
			InternalElement( oldElem.m_Next ).m_Previous = oldElem.m_Previous;
		}
		else
		{
			m_Tail = oldElem.m_Previous;
		}

		oldElem.m_Previous = oldElem.m_Next = elem;

		--m_ElementCount;
	}
}

template <class T, class I>
inline void CUtlLinkedList<T, I>::Remove( I elem )
{
	Free( elem );
}

template <class T, class I>
void CUtlLinkedList<T, I>::RemoveAll()
{
	if ( m_TotalElements == 0 )
	{
		return;
	}

	// Put everything into the free list
	I prev = InvalidIndex();
	for ( int i = (int)m_TotalElements; --i >= 0; )
	{
		if ( IsValidIndex( (I)i ) )
		{
			Element( (I)i ).~T();
		}

		// next points to the next free list item
		InternalElement( (I)i ).m_Next = prev;
		// Indicates it's in the free list
		InternalElement( (I)i ).m_Previous = (I)i;
		prev = (I)i;
	}

	// First free points to the first element
	m_FirstFree = 0;

	// Clear everything else out
	m_Head = InvalidIndex();
	m_Tail = InvalidIndex();
	m_ElementCount = 0;
}

#endif // UTLLINKEDLIST_H
