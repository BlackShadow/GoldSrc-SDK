#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "tier0/platform.h"
#include "tier0/valobject.h"

#define MAX_VALIDATION_LOCKS	512
#define MAX_VALIDATION_OBJECTS	500000

class CThreadMutex;

class TIER0_API CValidator
{
public:
	CValidator( int nThreshold );
	~CValidator();
	CValidator& operator=( const CValidator& other ) = default;

	void Push( const char* pType, void* pObject, const char* pName );
	// Call this each time we exit a Validate function
	void Pop();
	void ClaimMemory( void* pMemory );
	void ClaimArrayMemory( void* pMemory );
	CValObject* FindObject( void* pObject );
	CValObject* PValObjectFirst() { return m_pValObjectFirst; }
	bool BMemLeaks() { return m_bMemLeaks; }

	void AddValidationLock( CThreadMutex* pMutex );
	void UnlockValidationLocks();
	void Finalize();
	void RenderLeaks();
	void RenderObjects( int nThreshold );
	void DiffAgainst( CValidator* pPrevious );
	void Validate( CValidator& validator, char* pName ); // Validate our internal structures

private:
	bool BExcludeAllocationFromTracking( const char* pFileName, int nLine );

	CValObject*	m_pValObjectFirst;
	CValObject*	m_pValObjectLast;
	CValObject*	m_pValObjectCur; // Object we're current processing
	CValObject*	m_pValObjects;
	int			m_nMaxValObjects;
	int			m_nValObjects;
	int			m_nThreshold;
	int			m_nClaimedBlocks;
	int			m_Reserved[2];
	bool		m_bMemLeaks; // Has any memory leaked?
	bool		m_bTooManyObjects;
	CThreadMutex* m_threadMutexValidationLock[MAX_VALIDATION_LOCKS];
	int			m_iThreadMutexValidationLock;
};

class TIER0_API CTier0
{
public:
	CTier0& operator=( const CTier0& other ) = default;
	CTier0& operator=( CTier0&& other ) = default;
	static void ValidateGlobals( CValidator& validator );
};

extern "C" TIER0_API void ValidateSpew( CValidator& validator );

void ValidateMemory( CValidator& validator );

void ValidatePME( CValidator& validator );

#endif // VALIDATOR_H
