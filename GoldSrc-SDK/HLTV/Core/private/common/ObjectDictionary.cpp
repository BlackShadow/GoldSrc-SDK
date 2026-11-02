#include "common/ObjectDictionary.h"

#include "HLTV/common/mem.h"

#include <stdlib.h>
#include <string.h>

ObjectDictionary::ObjectDictionary()
{
	maxSize = 0;
	size = 0;
	entries = NULL;
}

ObjectDictionary::~ObjectDictionary()
{
	if ( entries != NULL )
		free( entries );
}

void ObjectDictionary::Init()
{
	maxSize = 0;
	size = 0;
	entries = NULL;
	CheckSize();
	ClearCache();
}

void ObjectDictionary::Init( int baseSize )
{
	entry_t* allocated;

	size = 0;
	maxSize = 0;
	allocated = (entry_t*)Mem_ZeroMalloc( sizeof( entry_t ) * baseSize );
	entries = allocated;
	if ( allocated != NULL )
		maxSize = baseSize;
}

int ObjectDictionary::CountElements()
{
	return size;
}

bool ObjectDictionary::IsEmpty()
{
	return size == 0;
}

void* ObjectDictionary::GetNext()
{
	int index;

	index = currentEntry;
	if ( index < 0 || index >= size )
		return NULL;

	currentEntry = index + 1;
	return entries[index].object;
}

void* ObjectDictionary::GetFirst()
{
	currentEntry = 0;
	return GetNext();
}

void* ObjectDictionary::GetLast()
{
	if ( size <= 0 )
		return NULL;

	return entries[size - 1].object;
}

void ObjectDictionary::ClearCache()
{
	memset( cache, 0, sizeof( cache ) );
	cacheIndex = 0;
}

void ObjectDictionary::AddToCache( entry_t* entry )
{
	int slot;

	slot = cacheIndex % (int)( sizeof( cache ) / sizeof( cache[0] ) );
	cache[slot].object = entry;
	cache[slot].key = entry->key;
	++cacheIndex;
}

void ObjectDictionary::AddToCache( entry_t* entry, float key )
{
	int slot;

	slot = cacheIndex % (int)( sizeof( cache ) / sizeof( cache[0] ) );
	cache[slot].key = key;
	cache[slot].object = entry;
	++cacheIndex;
}

int ObjectDictionary::FindKeyInCache( float key )
{
	int i;
	entry_t* entry;

	for ( i = 0; i < (int)( sizeof( cache ) / sizeof( cache[0] ) ); ++i )
	{
		entry = (entry_t*)cache[i].object;
		if ( entry != NULL && cache[i].key == key )
			return (int)( entry - entries );
	}

	return -1;
}

int ObjectDictionary::FindObjectInCache( void* object )
{
	int i;
	entry_t* entry;

	for ( i = 0; i < (int)( sizeof( cache ) / sizeof( cache[0] ) ); ++i )
	{
		entry = (entry_t*)cache[i].object;
		if ( entry != NULL && entry->object == object )
			return (int)( entry - entries );
	}

	return -1;
}

int ObjectDictionary::FindClosestAsIndex( float key )
{
	int first;
	int last;
	int middle;
	int index;

	if ( size <= 0 )
		return -1;

	if ( entries[0].key >= key )
		return 0;

	index = FindKeyInCache( key );
	if ( index >= 0 )
		return index;

	first = 0;
	last = size - 1;
	if ( key < entries[last].key )
	{
		for ( ;; )
		{
			middle = ( first + last ) >> 1;
			if ( key == entries[middle].key )
			{
				last = middle;
				break;
			}

			if ( key > entries[middle].key )
			{
				if ( entries[middle + 1].key >= key )
				{
					if ( key - entries[middle].key <= entries[middle + 1].key - key )
						last = middle;
					else
						last = middle + 1;

					break;
				}

				first = middle;
			}
			else
			{
				last = middle;
			}
		}
	}

	while ( key == entries[last - 1].key )
		--last;

	AddToCache( &entries[last], key );
	return last;
}

void* ObjectDictionary::FindClosestKey( float key )
{
	currentEntry = FindClosestAsIndex( key );
	return GetNext();
}

void* ObjectDictionary::FindExactKey( float key )
{
	int index;

	index = FindClosestAsIndex( key );
	currentEntry = index;
	if ( index < 0 )
		return NULL;

	if ( entries[index].key != key )
		return NULL;

	return GetNext();
}

bool ObjectDictionary::CheckSize()
{
	int newMax;
	entry_t* newEntries;

	if ( maxSize == size )
	{
		newMax = (int)( maxSize * 1.25f ) + 1;
	}
	else
	{
		if ( maxSize * 0.5f <= size )
			return true;

		newMax = (int)( maxSize * 0.75f );
	}

	if ( newMax == maxSize )
		return true;

	newEntries = (entry_t*)malloc( sizeof( entry_t ) * newMax );
	if ( newEntries == NULL )
		return false;

	memset( &newEntries[size], 0, sizeof( entry_t ) * ( newMax - size ) );
	if ( entries != NULL )
	{
		if ( size != 0 )
		{
			memcpy( newEntries, entries, sizeof( entry_t ) * size );
			free( entries );
		}
	}

	entries = newEntries;
	maxSize = newMax;
	return true;
}

bool ObjectDictionary::Add( void* object, float key )
{
	entry_t* entry;
	entry_t* end;

	if ( size == maxSize )
	{
		if ( !CheckSize() )
			return false;
	}

	if ( size != 0 && key < entries[size - 1].key )
	{
		entry = &entries[FindClosestAsIndex( key )];
		while ( key >= entry->key )
			++entry;

		for ( end = &entries[size]; end != entry; --end )
			*end = *( end - 1 );
	}
	else
	{
		entry = &entries[size];
	}

	entry->key = key;
	entry->object = object;
	++size;
	ClearCache();
	AddToCache( entry );
	return true;
}

bool ObjectDictionary::Add( void* object )
{
	entry_t* entry;
	entry_t* end;

	if ( size == maxSize )
	{
		if ( !CheckSize() )
			return false;
	}

	if ( size != 0 && 0.0f < entries[size - 1].key )
	{
		entry = &entries[FindClosestAsIndex( 0.0f )];
		while ( entry->key <= 0.0f )
			++entry;

		for ( end = &entries[size]; end != entry; --end )
			*end = *( end - 1 );
	}
	else
	{
		entry = &entries[size];
	}

	entry->key = 0.0f;
	entry->object = object;
	++size;
	ClearCache();
	AddToCache( entry );
	return true;
}

bool ObjectDictionary::Contains( void* object )
{
	int i;
	entry_t* slot;

	for ( i = 0; i < (int)( sizeof( cache ) / sizeof( cache[0] ) ); ++i )
	{
		slot = (entry_t*)cache[i].object;
		if ( slot != NULL && slot->object == object )
		{
			if ( (char*)slot - (char*)entries >= -7 )
				return true;

			break;
		}
	}

	if ( size <= 0 )
		return false;

	for ( i = 0; i < size; ++i )
	{
		if ( entries[i].object == object )
		{
			AddToCache( &entries[i] );
			return true;
		}
	}

	return false;
}

bool ObjectDictionary::RemoveIndex( int index, bool freeObjectMemory )
{
	entry_t* last;
	void* object;

	if ( index < 0 || index >= size )
		return false;

	object = entries[index].object;
	if ( freeObjectMemory && object != NULL )
		free( object );

	last = &entries[size - 1];
	for ( entry_t* p = &entries[index]; p != last; ++p )
		*p = *( p + 1 );

	last->object = NULL;
	last->key = 0.0f;
	--size;
	CheckSize();
	ClearCache();
	return true;
}

bool ObjectDictionary::RemoveIndexRange( int minIndex, int maxIndex )
{
	int span;

	if ( minIndex > maxIndex )
	{
		int swap;

		swap = minIndex;
		minIndex = maxIndex;
		maxIndex = swap;
	}

	if ( minIndex < 0 )
		minIndex = 0;
	if ( maxIndex >= size )
		maxIndex = size - 1;

	span = maxIndex - minIndex + 1;
	size -= span;
	CheckSize();
	return true;
}

bool ObjectDictionary::RemoveKey( float key )
{
	int index;
	int maxIndex;

	index = FindClosestAsIndex( key );
	if ( entries[index].key != key )
		return false;

	maxIndex = index;
	while ( entries[maxIndex + 1].key == key )
		++maxIndex;

	return RemoveIndexRange( index, maxIndex );
}

bool ObjectDictionary::Remove( void* object )
{
	int i;
	bool found;

	if ( size <= 0 )
		return false;

	found = false;
	for ( i = 0; i < size; ++i )
	{
		if ( entries[i].object == object )
		{
			found = true;
			RemoveIndex( i, false );
		}
	}

	return found;
}

bool ObjectDictionary::RemoveSingle( void* object )
{
	int index;
	int i;

	if ( size <= 0 )
		return false;

	index = 0;
	while ( index < size && entries[index].object != object )
		++index;

	if ( index == size )
		return false;

	for ( i = index; i < size - 1; ++i )
		entries[i] = entries[i + 1];

	entries[size - 1].object = NULL;
	entries[size - 1].key = 0.0f;
	--size;
	CheckSize();
	ClearCache();
	return true;
}

void ObjectDictionary::Clear( bool freeObjectsMemory )
{
	int i;

	if ( freeObjectsMemory )
	{
		for ( i = 0; i < size; ++i )
		{
			if ( entries[i].object != NULL )
				free( entries[i].object );
		}
	}

	size = 0;
	CheckSize();
	ClearCache();
}

bool ObjectDictionary::ChangeKey( void* object, float newKey )
{
	int index;
	int closest;
	entry_t* entry;
	entry_t* dest;

	index = FindObjectInCache( object );
	if ( index < 0 )
	{
		if ( size < 0 )
		{
			index = 0;
			entry = entries;
		}
		else if ( size == 0 )
		{
			return false;
		}
		else
		{
			index = 0;
			entry = entries;
			while ( entry->object != object )
			{
				++index;
				++entry;
				if ( index == size )
					return false;
			}

			AddToCache( entry );
		}
	}
	else
	{
		entry = &entries[index];
	}

	if ( entry->key == newKey )
		return false;

	closest = FindClosestAsIndex( newKey );
	dest = &entries[closest];
	if ( index < closest )
	{
		if ( dest->key > newKey )
			--dest;

		if ( entry < dest )
		{
			for ( ; entry < dest; ++entry )
				*entry = *( entry + 1 );
		}
	}
	else if ( index > closest )
	{
		if ( newKey > dest->key )
			++dest;

		if ( entry > dest )
		{
			for ( ; entry > dest; --entry )
				*entry = *( entry - 1 );
		}
	}

	entry->object = object;
	entry->key = newKey;
	ClearCache();
	return true;
}

bool ObjectDictionary::UnsafeChangeKey( void* object, float newKey )
{
	int i;
	entry_t* entry;
	bool found;

	found = false;
	entry = NULL;
	for ( i = 0; i < (int)( sizeof( cache ) / sizeof( cache[0] ) ); ++i )
	{
		entry_t* slot;

		slot = (entry_t*)cache[i].object;
		if ( slot != NULL && slot->object == object )
		{
			if ( (char*)slot - (char*)entries >= -7 )
			{
				entry = slot;
				found = true;
			}

			break;
		}
	}

	if ( !found )
	{
		if ( size == 0 )
			return false;

		entry = entries;
		if ( size > 0 && entry->object != object )
		{
			i = 0;
			do
			{
				++i;
				++entry;
				if ( i == size )
					return false;
			}
			while ( entry->object != object );
		}
	}

	entry->key = newKey;
	ClearCache();
	return true;
}
