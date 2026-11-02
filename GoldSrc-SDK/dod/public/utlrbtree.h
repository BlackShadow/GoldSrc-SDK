//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#ifndef UTLRBTREE_H
#define UTLRBTREE_H

#ifdef _WIN32
#pragma once
#endif

#include <stddef.h>

template <class I>
struct UtlRBTreeLinks_t
{
	I	m_Left;
	I	m_Right;
	I	m_Parent;
	I	m_Tag;
};

template <class T, class I>
struct UtlRBTreeNode_t : public UtlRBTreeLinks_t<I>
{
	T	m_Data;
};

//-----------------------------------------------------------------------------
// A red-black binary search tree
//-----------------------------------------------------------------------------
template <class T, class I = unsigned short>
class CUtlRBTree
{
public:
	typedef T KeyType_t;
	typedef T ElemType_t;
	typedef I IndexType_t;
	typedef bool ( *LessFunc_t )( const T&, const T& );

	CUtlRBTree( int growSize = 0, int initSize = 0, LessFunc_t lessfunc = NULL );
	~CUtlRBTree();

	// gets particular elements
	T& Element( I i );
	const T& Element( I i ) const;
	T& operator[]( I i );
	const T& operator[]( I i ) const;

	I Find( const T& search ) const;
	I NextInorder( I i ) const;
	I Insert( const T& insert );

	// Iteration
	I FirstInorder() const;
	void RemoveAll();
	bool IsValidIndex( I i ) const;

	I Count() const
	{
		return m_NumElements;
	}

	I MaxElement() const
	{
		return m_TotalElements;
	}

	void SetLessFunc( LessFunc_t lessfunc )
	{
		if ( !m_LessFunc )
		{
			m_LessFunc = lessfunc;
		}
	}

	I CreateInsertedNode( const T& insert );

protected:
	enum NodeColor_t
	{
		RED	= 0,
		BLACK
	};

	typedef UtlRBTreeNode_t<T, I> Node_t;
	typedef UtlRBTreeLinks_t<I> Links_t;

	struct NodeMemory_t
	{
		Node_t*	m_pMemory;
		int		m_nAllocationCount;
		int		m_nGrowSize;
	};

	// Sets the children
	void SetParent( I i, I parent );
	void SetLeftChild( I i, I child );
	void SetRightChild( I i, I child );
	void LinkToParent( I i, I parent, bool isLeft );

	// Gets at the links
	const Links_t& Links( I i ) const;
	Links_t& Links( I i );

	// Checks if a link is red or black
	bool IsRed( I i ) const;

	// Sets/gets node color
	NodeColor_t Color( I i ) const;
	void SetColor( I i, NodeColor_t c );

	// Gets the children                               
	I Parent( I i ) const;
	I LeftChild( I i ) const;
	I RightChild( I i ) const;

	// Tests if root or leaf
	bool IsRoot( I i ) const;

	// Tests if a node is a left or right child
	bool IsLeftChild( I i ) const;
	bool IsRightChild( I i ) const;

	// Invalid index
	static I InvalidIndex();

	void RotateLeft( I elem );
	void RotateRight( I elem );
	void InsertRebalance( I elem );

	I NewNode();

	// Insertion, removal
	I InsertAt( I parent, bool leftchild );
	void FindInsertionPosition( const T& insert, I& parent, bool& leftchild );
	void GrowElements();

private:
	CUtlRBTree( const CUtlRBTree<T, I>& tree );
	CUtlRBTree<T, I>& operator=( const CUtlRBTree<T, I>& other );

	// Used for sorting.
	LessFunc_t		m_LessFunc;

	NodeMemory_t	m_Elements;

	I				m_Root;
	I				m_NumElements;
	I				m_FirstFree;
	I				m_TotalElements;
};

//-----------------------------------------------------------------------------
// gets particular elements
//-----------------------------------------------------------------------------

template <class T, class I>
inline T& CUtlRBTree<T, I>::Element( I i )
{
	return m_Elements.m_pMemory[i].m_Data;
}

template <class T, class I>
inline const T& CUtlRBTree<T, I>::Element( I i ) const
{
	return m_Elements.m_pMemory[i].m_Data;
}

template <class T, class I>
inline T& CUtlRBTree<T, I>::operator[]( I i )
{
	return Element( i );
}

template <class T, class I>
inline const T& CUtlRBTree<T, I>::operator[]( I i ) const
{
	return Element( i );
}

template <class T, class I>
inline I CUtlRBTree<T, I>::InvalidIndex()
{
	return (I)-1;
}

template <class T, class I>
inline const typename CUtlRBTree<T, I>::Links_t& CUtlRBTree<T, I>::Links( I i ) const
{
	// Sentinel node, makes life easier
	static Links_t s_Sentinel =
	{
		InvalidIndex(), InvalidIndex(), InvalidIndex(), BLACK
	};

	if ( i != InvalidIndex() )
	{
		return *(const Links_t*)&m_Elements.m_pMemory[i];
	}

	return s_Sentinel;
}

template <class T, class I>
inline typename CUtlRBTree<T, I>::Links_t& CUtlRBTree<T, I>::Links( I i )
{
	return *(Links_t*)&m_Elements.m_pMemory[i];
}

//-----------------------------------------------------------------------------
// Sets the children
//-----------------------------------------------------------------------------

template <class T, class I>
inline void CUtlRBTree<T, I>::SetParent( I i, I parent )
{
	Links( i ).m_Parent = parent;
}

template <class T, class I>
inline void CUtlRBTree<T, I>::SetLeftChild( I i, I child )
{
	Links( i ).m_Left = child;
}

template <class T, class I>
inline void CUtlRBTree<T, I>::SetRightChild( I i, I child )
{
	Links( i ).m_Right = child;
}

//-----------------------------------------------------------------------------
// Checks if a link is red or black
//-----------------------------------------------------------------------------

template <class T, class I>
inline bool CUtlRBTree<T, I>::IsRed( I i ) const
{
	return Links( i ).m_Tag == (I)RED;
}


//-----------------------------------------------------------------------------
// Sets/gets node color
//-----------------------------------------------------------------------------

template <class T, class I>
inline typename CUtlRBTree<T, I>::NodeColor_t CUtlRBTree<T, I>::Color( I i ) const
{
	return (NodeColor_t)Links( i ).m_Tag;
}

template <class T, class I>
inline void CUtlRBTree<T, I>::SetColor( I i, NodeColor_t c )
{
	Links( i ).m_Tag = (I)c;
}


//-----------------------------------------------------------------------------
// Gets the children                               
//-----------------------------------------------------------------------------

template <class T, class I>
inline I CUtlRBTree<T, I>::Parent( I i ) const
{
	return Links( i ).m_Parent;
}

template <class T, class I>
inline I CUtlRBTree<T, I>::LeftChild( I i ) const
{
	return Links( i ).m_Left;
}

template <class T, class I>
inline I CUtlRBTree<T, I>::RightChild( I i ) const
{
	return Links( i ).m_Right;
}


//-----------------------------------------------------------------------------
// Tests if root or leaf
//-----------------------------------------------------------------------------

template <class T, class I>
inline bool CUtlRBTree<T, I>::IsRoot( I i ) const
{
	return i == m_Root;
}

//-----------------------------------------------------------------------------
// Tests if a node is a left or right child
//-----------------------------------------------------------------------------

template <class T, class I>
inline bool CUtlRBTree<T, I>::IsLeftChild( I i ) const
{
	return LeftChild( Parent( i ) ) == i;
}

template <class T, class I>
inline bool CUtlRBTree<T, I>::IsRightChild( I i ) const
{
	return RightChild( Parent( i ) ) == i;
}

#ifndef UTLRBTREE_NO_IMPL
#include "utlrbtree.cpp"
#endif

#endif // UTLRBTREE_H
