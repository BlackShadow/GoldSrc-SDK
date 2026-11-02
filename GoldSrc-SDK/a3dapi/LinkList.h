#pragma once

class CA3dListNode
{
public:
	CA3dListNode( CA3dListNode* pPrevious, void* pObject );

	CA3dListNode* GetLast();
	CA3dListNode* Append( void* pObject );

	CA3dListNode*	m_pPrevious;
	CA3dListNode*	m_pNext;
	void*			m_pObject;
};

class CA3dObjectList
{
public:
	void Clear();
	void* Next();
	void Append( void* pObject );

	CA3dListNode*	m_pFirst;
	CA3dListNode*	m_pLast;
	CA3dListNode*	m_pCurrent;
	int				m_nCount;
};
