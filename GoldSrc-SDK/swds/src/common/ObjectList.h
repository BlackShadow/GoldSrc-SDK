#ifdef _WIN32

#include "../../../HL25-Engine/common/ObjectList.h"

#else

#ifndef OBJECTLIST_H
#define OBJECTLIST_H

#include "IObjectContainer.h"

//-----------------------------------------------------------------------------
// Purpose: keep object pointers in an ordered chain of linked elements
//-----------------------------------------------------------------------------
class ObjectList : public IObjectContainer
{
public:
	ObjectList();
	virtual ~ObjectList() override;

	virtual void Init() override;

	virtual bool Add( void* newObject ) override;
	virtual bool Remove( void* object ) override;
	virtual void Clear( bool freeElementsMemory ) override;

	virtual void* GetFirst() override;
	virtual void* GetNext() override;

	virtual int CountElements() override;
	virtual bool Contains( void* object ) override;
	virtual bool IsEmpty() override;

	bool AddHead( void* newObject );
	void* RemoveHead();
	bool AddTail( void* newObject );
	void* RemoveTail();

	typedef struct element_s
	{
		element_s*	prev;
		element_s*	next;
		void*		object;
	} element_t;

protected:
	element_t*	head;
	element_t*	tail;
	element_t*	current;
	int			number;
};

#endif

#endif
