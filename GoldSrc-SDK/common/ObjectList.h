#pragma once

#include "IObjectContainer.h"

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
		element_s*	prev; // pointer to the last element or NULL
		element_s*	next; // pointer to the next elemnet or NULL
		void*		object; // the element's object
	} element_t;

protected:
	element_t*	head; // first element in list
	element_t*	tail; // last element in list
	element_t*	current; // current element in list
	int			number;
};
