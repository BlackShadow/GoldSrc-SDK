#include "common/ObjectDictionary.h"
#include "HLTV/common/mem.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#define CapacityGrowthFactor		1.25f
#define CapacityShrinkThreshold	0.5f
#define CapacityShrinkFactor		0.75f

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
ObjectDictionary::ObjectDictionary()
{
	size = maxSize = 0;
	entries = NULL;
}

ObjectDictionary::~ObjectDictionary()
{
	if ( entries )
	{
		std::free( entries );
	}
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
	size = 0;
	maxSize = 0;

	entries = (entry_t*)Mem_ZeroMalloc( sizeof( entry_t ) * baseSize );

	if ( entries )
	{
		maxSize = baseSize;
	}
}

int ObjectDictionary::CountElements()
{
	return size;
}

bool ObjectDictionary::IsEmpty()
{
	return size == 0;
}

void* ObjectDictionary::GetFirst()
{
	currentEntry = 0;
	return GetNext();
}

void* ObjectDictionary::GetNext()
{
	int index;

	index = currentEntry;

	if ( index < 0 || index >= size )
	{
		return NULL;
	}

	currentEntry = index + 1;
	return entries[index].object;
}

void* ObjectDictionary::GetLast()
{
	if ( size <= 0 )
	{
		return NULL;
	}

	return entries[size - 1].object;
}

void ObjectDictionary::Clear( bool freeObjectssMemory )
{
	int i;

	if ( freeObjectssMemory )
	{
		for ( i = 0; i < size; i++ )
		{
			if ( entries[i].object )
			{
				std::free( entries[i].object );
			}
		}
	}

	size = 0;
	CheckSize();
	ClearCache();
}

void ObjectDictionary::ClearCache()
{
	std::memset( cache, 0, sizeof( cache ) );
	cacheIndex = 0;
}

void ObjectDictionary::AddToCache( entry_t* entry )
{
	int i;

	i = cacheIndex % CacheSize;
	cache[i].object = entry;
	cache[i].key = entry->key;
	cacheIndex = (int)( (unsigned int)cacheIndex + 1u );
}

void ObjectDictionary::AddToCache( entry_t* entry, float key )
{
	int i;

	i = cacheIndex % CacheSize;
	cache[i].key = key;
	cache[i].object = entry;
	cacheIndex = (int)( (unsigned int)cacheIndex + 1u );
}

int ObjectDictionary::FindKeyInCache( float key )
{
	int i;
	entry_t* e;

	for ( i = 0; i < CacheSize; i++ )
	{
		e = (entry_t*)cache[i].object;

		if ( e && cache[i].key == key )
		{
			return (int)( e - entries );
		}
	}

	return -1;
}

int ObjectDictionary::FindObjectInCache( void* object )
{
	int i;
	entry_t* e;

	for ( i = 0; i < CacheSize; i++ )
	{
		e = (entry_t*)cache[i].object;

		if ( e && e->object == object )
		{
			return (int)( e - entries );
		}
	}

	return -1;
}

bool ObjectDictionary::Contains( void* object )
{
	int i;
	entry_t* e;

	e = NULL;

	for ( i = 0; i < CacheSize; i++ )
	{
		e = (entry_t*)cache[i].object;

		if ( e && e->object == object )
		{
			break;
		}

		e = NULL;
	}

	if ( e && (std::intptr_t)( (std::uintptr_t)e - (std::uintptr_t)entries )
		>= MinimumContainsCachedByteOffset )
	{
		return true;
	}

	for ( i = 0; i < size; i++ )
	{
		if ( entries[i].object == object )
		{
			AddToCache( &entries[i] );
			return true;
		}
	}

	return false;
}

bool ObjectDictionary::CheckSize()
{
	int newSize;
	entry_t* newEntries;

	if ( maxSize == size )
	{
#if defined( __linux__ )
		newSize = (int)(float)( (long double)maxSize * CapacityGrowthFactor ) + 1;
#else
		newSize = 1 - (int)( (float)maxSize * -CapacityGrowthFactor );
#endif
	}
#if defined( __linux__ )
	else if ( (long double)maxSize * CapacityShrinkThreshold <= size )
#else
	else if ( (float)maxSize * CapacityShrinkThreshold <= (float)size )
#endif
	{
		return true;
	}
	else
	{
#if defined( __linux__ )
		newSize = (int)(float)( (long double)maxSize * CapacityShrinkFactor );
#else
		newSize = (int)( (float)maxSize * CapacityShrinkFactor );
#endif
	}

	if ( newSize == maxSize )
	{
		return true;
	}

	newEntries = (entry_t*)std::malloc( sizeof( entry_t ) * newSize );

	if ( !newEntries )
	{
		return false;
	}

	std::memset( newEntries + size, 0, sizeof( entry_t ) * ( newSize - size ) );

	if ( entries && size != 0 )
	{
		std::memcpy( newEntries, entries, sizeof( entry_t ) * size );
		std::free( entries );
	}

	entries = newEntries;
	maxSize = newSize;
	return true;
}

bool ObjectDictionary::Add( void* object, float key )
{
	entry_t* p;

	if ( size == maxSize )
	{
		if ( !CheckSize() )
		{
			return false;
		}
	}

	if ( size != 0 && !( key >= entries[size - 1].key ) )
	{
		p = &entries[FindClosestAsIndex( key )];

		while ( key >= p->key )
		{
			p++;
		}

		for ( entry_t* q = entries + size; q != p; --q )
		{
			*q = *( q - 1 );
		}
	}
	else
	{
		p = &entries[size];
	}

	p->key = key;
	p->object = object;
	size++;
	ClearCache();
	AddToCache( p );
	return true;
}

bool ObjectDictionary::Add( void* object )
{
	entry_t* p;

	if ( size == maxSize )
	{
		if ( !CheckSize() )
		{
			return false;
		}
	}

	if ( size != 0 && !( 0.0f >= entries[size - 1].key ) )
	{
		p = &entries[FindClosestAsIndex( 0.0f )];

		while ( p->key <= 0.0f )
		{
			p++;
		}

		for ( entry_t* q = entries + size; q != p; --q )
		{
			*q = *( q - 1 );
		}
	}
	else
	{
		p = &entries[size];
	}

	p->key = 0.0f;
	p->object = object;
	size++;
	ClearCache();
	AddToCache( p );
	return true;
}

bool ObjectDictionary::RemoveIndex( int index, bool freeObjectMemory )
{
	entry_t* p;
	entry_t* e1;

	if ( index < 0 || index >= size )
	{
		return false;
	}

	p = &entries[index];
	e1 = &entries[size - 1];

	if ( freeObjectMemory && p->object )
	{
		std::free( p->object );
	}

	for ( ; p != e1; ++p )
	{
		*p = *( p + 1 );
	}

	e1->object = NULL;
	e1->key = 0.0f;
	size--;
	CheckSize();
	ClearCache();
	return true;
}

bool ObjectDictionary::Remove( void* object )
{
	int i;
	bool found;

	if ( size <= 0 )
	{
		return false;
	}

	found = false;

	for ( i = 0; i < size; i++ )
	{
		if ( entries[i].object == object )
		{
			RemoveIndex( i, false );
			found = true;
		}
	}

	return found;
}

bool ObjectDictionary::RemoveSingle( void* object )
{
	int i;
	entry_t* p;
	entry_t* e1;
	entry_t* e2;

	if ( size <= 0 )
	{
		return false;
	}

	for ( i = 0; i < size; i++ )
	{
		if ( entries[i].object == object )
		{
			break;
		}
	}

	if ( i == size )
	{
		return false;
	}

	p = &entries[i];
	e1 = &entries[size - 1];
	e2 = p + 1;

	while ( e1 != p )
	{
		p->object = e2->object;
		p->key = e2->key;
		p++;
		e2++;
	}

	e1->object = NULL;
	e1->key = 0.0f;
	size--;
	CheckSize();
	ClearCache();
	return true;
}

bool ObjectDictionary::RemoveIndexRange( int minIndex, int maxIndex )
{
	int tmp;
	int offset;

	if ( minIndex > maxIndex )
	{
		tmp = minIndex;
		minIndex = maxIndex;
		maxIndex = tmp;
	}

	if ( minIndex < 0 )
	{
		minIndex = 0;
	}

	if ( maxIndex >= size )
	{
		maxIndex = size - 1;
	}

	offset = maxIndex - minIndex + 1;
	size -= offset;
	CheckSize();
	return true;
}

bool ObjectDictionary::RemoveKey( float key )
{
	int i;
	int j;

	i = FindClosestAsIndex( key );

	if ( entries[i].key != key )
	{
		return false;
	}

	j = i;

	while ( entries[j + 1].key == key )
	{
		j++;
	}

	RemoveIndexRange( i, j );
	return true;
}

int ObjectDictionary::FindClosestAsIndex( float key )
{
	int first;
	int last;
	int middle;
	int index;
	float keyMiddle;
	float keyNext;

	if ( size <= 0 )
	{
		return -1;
	}

	if ( entries[0].key >= key )
	{
		return 0;
	}

	index = FindKeyInCache( key );

	if ( index >= 0 )
	{
		return index;
	}

	first = 0;
	last = size - 1;
	middle = last;

	if ( !( key >= entries[last].key ) )
	{
		for ( ;; )
		{
			middle = ( first + last ) / 2;
			keyMiddle = entries[middle].key;

			if ( key == keyMiddle )
			{
				break;
			}

			if ( key > keyMiddle )
			{
				keyNext = entries[middle + 1].key;

				if ( keyNext >= key )
				{
#if defined( __linux__ )
					if ( !( (long double)key - keyMiddle > (long double)keyNext - key ) )
#else
					const float lowerDistance = key - keyMiddle;
					const float upperDistance = keyNext - key;
					if ( !( lowerDistance > upperDistance ) )
#endif
					{
						break;
					}

					middle++;
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

	while ( key == entries[middle - 1].key )
	{
		middle--;
	}

	AddToCache( &entries[middle], key );
	return middle;
}

void* ObjectDictionary::FindClosestKey( float key )
{
	currentEntry = FindClosestAsIndex( key );
	return GetNext();
}

void* ObjectDictionary::FindExactKey( float key )
{
	int index;

	if ( size <= 0 )
	{
		currentEntry = -1;
		return NULL;
	}

	index = FindClosestAsIndex( key );
	currentEntry = index;

	if ( entries[index].key != key )
	{
		return NULL;
	}

	return GetNext();
}

bool ObjectDictionary::ChangeKey( void* object, float newKey )
{
	int i;
	int pos;
	int newpos;
	entry_t* e;
	entry_t* p;

	e = NULL;
	pos = -1;

	for ( i = 0; i < CacheSize; i++ )
	{
		p = (entry_t*)cache[i].object;

		if ( p && p->object == object )
		{
			e = p;
			pos = (int)( e - entries );
			break;
		}
	}

	if ( !e || pos < 0 )
	{
		if ( size == 0 )
		{
			return false;
		}

		if ( size < 0 )
		{
			pos = 0;
			e = entries;
		}
		else
		{
			e = entries;
			pos = 0;

			while ( e->object != object )
			{
				pos++;
				e++;

				if ( pos == size )
				{
					return false;
				}
			}

			AddToCache( e );
		}
	}

	if ( e->key == newKey )
	{
		return false;
	}

	newpos = FindClosestAsIndex( newKey );
	p = &entries[newpos];

	if ( pos < newpos )
	{
		if ( p->key > newKey )
		{
			p--;
		}

		if ( e < p )
		{
			for ( ; e < p; ++e )
				*e = *( e + 1 );
		}
	}
	else if ( pos > newpos )
	{
		if ( newKey > p->key )
		{
			p++;
		}

		if ( e > p )
		{
			for ( ; e > p; --e )
				*e = *( e - 1 );
		}
	}

	e->object = object;
	e->key = newKey;
	ClearCache();
	return true;
}

bool ObjectDictionary::UnsafeChangeKey( void* object, float newKey )
{
	int i;
	entry_t* e;

	e = NULL;

	for ( i = 0; i < CacheSize; i++ )
	{
		e = (entry_t*)cache[i].object;

		if ( e && e->object == object )
		{
			break;
		}

		e = NULL;
	}

	if ( e && (std::intptr_t)( (std::uintptr_t)e - (std::uintptr_t)entries ) < MinimumCachedByteOffset )
	{
		e = NULL;
	}

	if ( !e )
	{
		if ( size == 0 )
		{
			return false;
		}

		if ( size < 0 )
		{
			e = entries;
		}
		else
		{
			for ( i = 0; i < size; i++ )
			{
				if ( entries[i].object == object )
				{
					break;
				}
			}

			if ( i == size )
			{
				return false;
			}

			e = &entries[i];
		}
	}

	e->key = newKey;
	ClearCache();
	return true;
}
