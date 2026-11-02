#ifndef OBJECTDICTIONARY_H
#define OBJECTDICTIONARY_H

#include "IObjectContainer.h"

class ObjectDictionary : public IObjectContainer
{
public:
	ObjectDictionary();
	virtual ~ObjectDictionary() override;

	virtual void Init() override;
	virtual bool Add( void* object ) override;
	virtual bool Remove( void* object ) override;
	virtual void Clear( bool freeObjectssMemory ) override;

	virtual void* GetFirst() override;
	virtual void* GetNext() override;
	virtual int CountElements() override;
	virtual bool Contains( void* object ) override;
	virtual bool IsEmpty() override;

	bool Add( void* object, float key );
	bool RemoveSingle( void* object );
	bool RemoveKey( float key );
#if defined( __linux__ )
	bool RemoveRange( float, float );
#endif

	void Init( int baseSize );
	void* GetLast();

	bool ChangeKey( void* object, float newKey );
	bool UnsafeChangeKey( void* object, float newKey );
	void* FindClosestKey( float key );
	void* FindExactKey( float key );

protected:
	static const int CacheSize = 32;

	struct entry_s
	{
		void*	object;
		float	key;
	};

	typedef entry_s entry_t;

	static const int MinimumCachedByteOffset = 1 - (int)sizeof( entry_t );
#if defined( __linux__ )
	static const int MinimumContainsCachedByteOffset = MinimumCachedByteOffset;
#else
	static const int MinimumContainsCachedByteOffset = 0;
#endif

	int			currentEntry;
	float		findKey;
	entry_t*	entries;
	entry_t		cache[CacheSize];
	int			cacheIndex;
	int			size;
	int			maxSize;

	int FindClosestAsIndex( float key );
	void ClearCache();

	bool RemoveIndex( int index, bool freeObjectMemory );
	bool RemoveIndexRange( int minIndex, int maxIndex );
	bool CheckSize();

	void AddToCache( entry_t* entry );
	void AddToCache( entry_t* entry, float key );
	int FindKeyInCache( float key );
	int FindObjectInCache( void* object );
};

#endif
