#pragma once

#include "common/IObjectContainer.h"

class ObjectDictionary : public IObjectContainer
{
public:
	ObjectDictionary();
	virtual ~ObjectDictionary() override;

	virtual void Init() override;

	virtual bool Add( void* object ) override;
	virtual bool Remove( void* object ) override;
	virtual void Clear( bool freeObjectsMemory ) override;

	virtual void* GetFirst() override;
	virtual void* GetNext() override;

	virtual int CountElements() override;
	virtual bool Contains( void* object ) override;
	virtual bool IsEmpty() override;

	void Init( int baseSize );

	bool Add( void* object, float key );
	bool RemoveKey( float key );
	bool RemoveSingle( void* object );

	bool ChangeKey( void* object, float newKey );
	bool UnsafeChangeKey( void* object, float newKey );

	void* FindClosestKey( float key );
	void* FindExactKey( float key );
	void* GetLast();

protected:
	typedef struct entry_s
	{
		void*	object;
		float	key;
	} entry_t;

	bool CheckSize();

	void ClearCache();
	void AddToCache( entry_t* entry );
	void AddToCache( entry_t* entry, float key );
	int FindKeyInCache( float key );
	int FindObjectInCache( void* object );

	int FindClosestAsIndex( float key );
	bool RemoveIndex( int index, bool freeObjectMemory );
	bool RemoveIndexRange( int minIndex, int maxIndex );

	int			currentEntry = 0;
	float		findKey = 0.0f;
	entry_t*	entries;
	entry_t		cache[32]{};
	int			cacheIndex = 0;

	int			size;
	int			maxSize;
};
