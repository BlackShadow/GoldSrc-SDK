#ifndef UTLRBTREE_CPP_INCLUDED
#define UTLRBTREE_CPP_INCLUDED

#ifndef UTLRBTREE_H
#define UTLRBTREE_EMIT_INSTANTIATIONS
#define UTLRBTREE_NO_IMPL
#include "utlrbtree.h"
#undef UTLRBTREE_NO_IMPL
#endif

#include <new>

template <class T, class I>
CUtlRBTree<T, I>::CUtlRBTree( int growSize, int initSize, LessFunc_t lessfunc )
	: m_LessFunc( lessfunc ),
	  m_Elements( growSize, initSize ),
	  m_Root( InvalidIndex() ),
	  m_NumElements( 0 ),
	  m_FirstFree( InvalidIndex() ),
	  m_TotalElements( 0 )
{
}

template <class T, class I>
I CUtlRBTree<T, I>::NewNode()
{
	I elem;

	if ( m_FirstFree == InvalidIndex() )
	{
		if ( m_Elements.NumAllocated() == (int)m_TotalElements )
		{
			m_Elements.Grow();
		}

		elem = m_TotalElements;
		m_TotalElements = (I)( m_TotalElements + 1 );
	}
	else
	{
		elem = m_FirstFree;
		m_FirstFree = RightChild( m_FirstFree );
	}

	new ( &Element( elem ) ) T;
	return elem;
}

template <class T, class I>
void CUtlRBTree<T, I>::FindInsertionPosition( const T& insert, I& parent, bool& leftchild )
{
#ifdef _WIN32
	if ( !m_LessFunc )
	{
		_wassert( L"m_LessFunc", UTLRBTREE_SOURCE_PATH, 1152 );
	}
#endif

	I current = m_Root;

	parent = InvalidIndex();
	leftchild = false;

	while ( current != InvalidIndex() )
	{
		parent = current;

		if ( m_LessFunc( insert, Element( current ) ) )
		{
			leftchild = true;
			current = LeftChild( current );
		}
		else
		{
			leftchild = false;
			current = RightChild( current );
		}
	}
}

template <class T, class I>
void CUtlRBTree<T, I>::LinkToParent( I i, I parent, bool isLeft )
{
	Links_t& elem = Links( i );

	elem.m_Parent = parent;
	elem.m_Left = InvalidIndex();
	elem.m_Right = InvalidIndex();
	elem.m_Tag = (I)RED;

	if ( parent != InvalidIndex() )
	{
		if ( isLeft )
		{
			Links( parent ).m_Left = i;
		}
		else
		{
			Links( parent ).m_Right = i;
		}
	}
	else
	{
		m_Root = i;
	}

	InsertRebalance( i );
}

template <class T, class I>
I CUtlRBTree<T, I>::InsertAt( I parent, bool leftchild )
{
	I i = NewNode();

	LinkToParent( i, parent, leftchild );
	m_NumElements = (I)( m_NumElements + 1 );

#ifdef _WIN32
	if ( m_NumElements != 0 && ( m_Root >= m_TotalElements || Parent( m_Root ) != InvalidIndex() ) )
	{
		_wassert( L"IsValid()", UTLRBTREE_SOURCE_PATH, 657 );
	}
#endif

	return i;
}

template <class T, class I>
void CUtlRBTree<T, I>::RotateLeft( I elem )
{
	I rightchild = RightChild( elem );

	SetRightChild( elem, LeftChild( rightchild ) );
	if ( LeftChild( rightchild ) != InvalidIndex() )
	{
		SetParent( LeftChild( rightchild ), elem );
	}

	if ( rightchild != InvalidIndex() )
	{
		SetParent( rightchild, Parent( elem ) );
	}

	if ( !IsRoot( elem ) )
	{
		if ( IsLeftChild( elem ) )
		{
			SetLeftChild( Parent( elem ), rightchild );
		}
		else
		{
			SetRightChild( Parent( elem ), rightchild );
		}
	}
	else
	{
		m_Root = rightchild;
	}

	SetLeftChild( rightchild, elem );
	if ( elem != InvalidIndex() )
	{
		SetParent( elem, rightchild );
	}
}

template <class T, class I>
void CUtlRBTree<T, I>::RotateRight( I elem )
{
	I leftchild = LeftChild( elem );

	SetLeftChild( elem, RightChild( leftchild ) );
	if ( RightChild( leftchild ) != InvalidIndex() )
	{
		SetParent( RightChild( leftchild ), elem );
	}

	if ( leftchild != InvalidIndex() )
	{
		SetParent( leftchild, Parent( elem ) );
	}

	if ( !IsRoot( elem ) )
	{
		if ( IsRightChild( elem ) )
		{
			SetRightChild( Parent( elem ), leftchild );
		}
		else
		{
			SetLeftChild( Parent( elem ), leftchild );
		}
	}
	else
	{
		m_Root = leftchild;
	}

	SetRightChild( leftchild, elem );
	if ( elem != InvalidIndex() )
	{
		SetParent( elem, leftchild );
	}
}

template <class T, class I>
void CUtlRBTree<T, I>::InsertRebalance( I elem )
{
	while ( !IsRoot( elem ) && Color( Parent( elem ) ) == RED )
	{
		I parent = Parent( elem );
		I grandparent = Parent( parent );

		if ( IsLeftChild( parent ) )
		{
			I uncle = RightChild( grandparent );

			if ( IsRed( uncle ) )
			{
				SetColor( parent, BLACK );
				SetColor( uncle, BLACK );
				SetColor( grandparent, RED );
				elem = grandparent;
			}
			else
			{
				if ( IsRightChild( elem ) )
				{
					elem = parent;
					RotateLeft( elem );
					parent = Parent( elem );
					grandparent = Parent( parent );
				}

				SetColor( parent, BLACK );
				SetColor( grandparent, RED );
				RotateRight( grandparent );
			}
		}
		else
		{
			I uncle = LeftChild( grandparent );

			if ( IsRed( uncle ) )
			{
				SetColor( parent, BLACK );
				SetColor( uncle, BLACK );
				SetColor( grandparent, RED );
				elem = grandparent;
			}
			else
			{
				if ( IsLeftChild( elem ) )
				{
					elem = parent;
					RotateRight( elem );
					parent = Parent( elem );
					grandparent = Parent( parent );
				}

				SetColor( parent, BLACK );
				SetColor( grandparent, RED );
				RotateLeft( grandparent );
			}
		}
	}

	SetColor( m_Root, BLACK );
}

template <class T, class I>
I CUtlRBTree<T, I>::NextInorder( I i ) const
{
#ifdef _WIN32
	if ( !IsValidIndex( i ) )
	{
		_wassert( L"IsValidIndex(i)", UTLRBTREE_SOURCE_PATH, 894 );
	}
#endif

	if ( RightChild( i ) != InvalidIndex() )
	{
		i = RightChild( i );

		while ( LeftChild( i ) != InvalidIndex() )
		{
			i = LeftChild( i );
		}

		return i;
	}

	I parent = Parent( i );

	while ( IsRightChild( i ) )
	{
		i = parent;
		if ( i == InvalidIndex() )
		{
			break;
		}

		parent = Parent( i );
	}

	return parent;
}

template <class T, class I>
I CUtlRBTree<T, I>::Find( const T& search ) const
{
#ifdef _WIN32
	if ( !m_LessFunc )
	{
		_wassert( L"m_LessFunc", UTLRBTREE_SOURCE_PATH, 1190 );
	}
#endif

	I current = m_Root;

	while ( current != InvalidIndex() )
	{
		if ( m_LessFunc( search, Element( current ) ) )
		{
			current = LeftChild( current );
		}
		else if ( m_LessFunc( Element( current ), search ) )
		{
			current = RightChild( current );
		}
		else
		{
			break;
		}
	}

	return current;
}

template <class T, class I>
I CUtlRBTree<T, I>::FirstInorder() const
{
	I current = m_Root;
	while ( LeftChild( current ) != InvalidIndex() )
	{
		current = LeftChild( current );
	}

	return current;
}

template <class T, class I>
bool CUtlRBTree<T, I>::IsValidIndex( I i ) const
{
	return LeftChild( i ) != i;
}

template <class T, class I>
void CUtlRBTree<T, I>::RemoveAll()
{
	I firstFree = InvalidIndex();
	for ( int i = (int)m_TotalElements - 1; i >= 0; --i )
	{
		I index = (I)i;
		if ( IsValidIndex( index ) )
		{
			Element( index ).~T();
		}

		SetRightChild( index, firstFree );
		SetLeftChild( index, index );
		firstFree = index;
	}

	m_FirstFree = firstFree;
	m_Root = InvalidIndex();
	m_NumElements = 0;
}

template <class T, class I>
I CUtlRBTree<T, I>::CreateInsertedNode( const T& insert )
{
	I parent;
	bool leftchild;

	FindInsertionPosition( insert, parent, leftchild );
	return InsertAt( parent, leftchild );
}

template <class T, class I>
I CUtlRBTree<T, I>::Insert( const T& insert )
{
	I idx = CreateInsertedNode( insert );

	new ( &Element( idx ) ) T( insert );
	return idx;
}

#ifdef UTLRBTREE_EMIT_INSTANTIATIONS
template unsigned short CUtlRBTree<unsigned int, unsigned short>::Find( const unsigned int& search ) const;
template void CUtlRBTree<unsigned int, unsigned short>::RotateLeft( unsigned short elem );
template void CUtlRBTree<unsigned int, unsigned short>::RotateRight( unsigned short elem );
template void CUtlRBTree<unsigned int, unsigned short>::InsertRebalance( unsigned short elem );
template unsigned short CUtlRBTree<unsigned int, unsigned short>::CreateInsertedNode( const unsigned int& insert );
#endif

#endif
