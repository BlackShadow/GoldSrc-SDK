/************ (C) Copyright 2003 Valve, L.L.C. All rights reserved. ***********
**
** The copyright to the contents herein is the property of Valve, L.L.C.
** The contents may be used and/or copied only with the written permission of
** Valve, L.L.C., or in accordance with the terms and conditions stipulated in
** the agreement/contract under which the contents have been supplied.
**
*******************************************************************************
**
** Contents:
**
**		ObjectList.cpp: implementation of the ObjectList class.
**
******************************************************************************/
#include "ObjectList.h"

#include "mem.h"
#include <cstdlib>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

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
	// create new element
	element_t* newElement = (element_t*)Mem_ZeroMalloc( sizeof( element_t ) );
	if ( !newElement )
		return false; // out of memory

	// insert element
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

	void* retObj = head->object;
	element_t* newHead = head->next;

	if ( newHead )
		newHead->prev = NULL;

	// if only one element is in list also update tail
	// if we remove this prev element
	if ( tail == head )
		tail = NULL;

	std::free( head );
	head = newHead;
	--number;
	return retObj;
}

bool ObjectList::AddTail( void* newObject )
{
	element_t* newElement = (element_t*)Mem_ZeroMalloc( sizeof( element_t ) );
	if ( !newElement )
		return false; // out of memory;

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

	void* retObj = tail->object;
	element_t* newTail = tail->prev;

	if ( newTail )
		newTail->next = NULL;

	// if only one element is in list also update tail
	// if we remove this prev element
	if ( head == tail )
		head = NULL;

	std::free( tail );
	tail = newTail;
	--number;
	return retObj;
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
			std::free( element->object );

		std::free( element );
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

	std::free( element );
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
	if ( head )
	{
		current = head->next;
		return head->object;
	}

	current = NULL;
	return NULL;
}

void* ObjectList::GetNext()
{
	if ( !current )
		return NULL;

	void* retObj = current->object;
	current = current->next;
	return retObj;
}

bool ObjectList::Add( void* newObject )
{
	return AddTail( newObject );
}
