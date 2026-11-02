#include "utlpriorityqueue.h"
#include "vgui_messages.h"

template <>
void CUtlPriorityQueue<MessageItem_t>::RemoveAtHead()
{
	m_heap.FastRemove( 0 );

	int count = Count();
	int index = 0;
	int half = count / 2;
	while ( index < half )
	{
		int left = index * 2 + 1;
		int right = left + 1;
		int chosen = index;

		if ( left < count && m_LessFunc( m_heap[index], m_heap[left] ) )
		{
			chosen = left;
		}

		if ( right < count && m_LessFunc( m_heap[chosen], m_heap[right] ) )
		{
			chosen = right;
		}

		if ( chosen == index )
		{
			break;
		}

		Swap( index, chosen );
		index = chosen;
	}
}

template <>
void CUtlPriorityQueue<MessageItem_t>::Insert( const MessageItem_t& element )
{
	int index = m_heap.AddToTail();
	m_heap[index] = element;

	while ( index != 0 )
	{
		int parent = ( ( index + 1 ) / 2 ) - 1;
		if ( m_LessFunc( m_heap[index], m_heap[parent] ) )
		{
			break;
		}

		Swap( parent, index );
		index = parent;
	}
}
