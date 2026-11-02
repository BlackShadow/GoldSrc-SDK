// ObjectList.cpp -- linked storage and traversal for object pointers

#include "ObjectList.h"
#include "mem.h"

#include <stddef.h>

ObjectList::ObjectList()
{
	head = tail = current = NULL;
	number = 0;
}

ObjectList::~ObjectList()
{
	Clear( false );
}

bool ObjectList::AddHead( void* newObject )
{
	element_t* newElement = (element_t*)Mem_ZeroMalloc( sizeof( element_t ) );
	if ( !newElement )
		return false;

	newElement->object = newObject;

	if ( head )
	{
		newElement->next = head;
		head->prev = newElement;
	}

	head = newElement;

	if ( !tail )
		tail = newElement;

	++number;
	return true;
}

void* ObjectList::RemoveHead()
{
	if ( !head )
		return NULL;

	void* object = head->object;
	element_t* newHead = head->next;

	if ( newHead )
		newHead->prev = NULL;

	if ( tail == head )
		tail = NULL;

	// removing an end element does not adjust the traversal cursor.
	Mem_Free( head );
	head = newHead;
	--number;
	return object;
}

bool ObjectList::AddTail( void* newObject )
{
	element_t* newElement = (element_t*)Mem_ZeroMalloc( sizeof( element_t ) );
	if ( !newElement )
		return false;

	newElement->object = newObject;

	if ( tail )
	{
		newElement->prev = tail;
		tail->next = newElement;
	}

	tail = newElement;

	if ( !head )
		head = newElement;

	++number;
	return true;
}

void* ObjectList::RemoveTail()
{
	if ( !tail )
		return NULL;

	void* object = tail->object;
	element_t* newTail = tail->prev;

	if ( newTail )
		newTail->next = NULL;

	if ( head == tail )
		head = NULL;

	Mem_Free( tail );
	tail = newTail;
	--number;
	return object;
}

bool ObjectList::IsEmpty()
{
	return head == NULL;
}

int ObjectList::CountElements()
{
	return number;
}

bool ObjectList::Contains( void* object )
{
	element_t* element = head;

	while ( element )
	{
		if ( element->object == object )
		{
			current = element;
			return true;
		}

		element = element->next;
	}

	return false;
}

void ObjectList::Clear( bool freeElementsMemory )
{
	element_t* element = head;

	while ( element )
	{
		element_t* next = element->next;

		if ( freeElementsMemory && element->object )
			Mem_Free( element->object );

		Mem_Free( element );
		element = next;
	}

	head = tail = current = NULL;
	number = 0;
}

bool ObjectList::Remove( void* object )
{
	element_t* element = head;

	while ( element && element->object != object )
		element = element->next;

	if ( !element )
		return false;

	if ( element->prev )
		element->prev->next = element->next;

	if ( element->next )
		element->next->prev = element->prev;

	if ( head == element )
		head = element->next;

	if ( tail == element )
		tail = element->prev;

	if ( current == element )
		current = element->next;

	Mem_Free( element );
	--number;
	return true;
}

void ObjectList::Init()
{
	head = tail = current = NULL;
	number = 0;
}

void* ObjectList::GetFirst()
{
	if ( !head )
	{
		current = NULL;
		return NULL;
	}

	current = head->next;
	return head->object;
}

void* ObjectList::GetNext()
{
	if ( !current )
		return NULL;

	void* object = current->object;
	current = current->next;
	return object;
}

bool ObjectList::Add( void* newObject )
{
	return AddTail( newObject );
}
