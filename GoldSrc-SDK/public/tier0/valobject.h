#ifndef VALOBJECT_H
#define VALOBJECT_H

class CValObject
{
public:
	void Init( const char* pType, void* pObject, const char* pName, CValObject* pParent, CValObject* pValObjectPrev );
	void ClaimMemory( void* pMemory );
	void ClaimChildMemory( int nBytes );

	const char*	m_pType;
	const char*	m_pName;
	void*		m_pObject;
	CValObject*	m_pParent;
	CValObject*	m_pValObjectNext;
	int			m_nBlocks;
	int			m_nBytes;
	int			m_nTotalBlocks;
	int			m_nTotalBytes;
	int			m_nReserved;
	unsigned short m_nDepth;
	bool		m_bNew;
};

#endif
