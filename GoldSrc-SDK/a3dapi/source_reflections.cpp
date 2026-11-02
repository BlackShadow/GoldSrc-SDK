#include "A3dSource.h"

#include "A3d3.h"
#include "vector_math.h"

#include <stdlib.h>
#include <string.h>

#define A3D_REFLECTION_CHANNEL_BUDGET	32
#define A3D_MIN_SOURCE_REFLECTIONS		4
#define A3D_REFLECTION_SORT_STACK		30
#define A3D_REFLECTION_SHORT_SORT		8
#define A3D_REFLECTION_SLOT_MASK		31
#define A3D_REFLECTION_SLOT_UNASSIGNED	-1
#define A3D_REFLECTION_SLOT_CULLED		-2
#define A3D_REFLECTION_MAX_EQ			0.99f
#define A3D_REFLECTION_OCCLUSION_SCALE	0.65f
#define A3D_REFLECTION_OCCLUSION_BIAS	-0.35f
#define A3D_REFLECTION_DELAY_PER_UNIT	( 1.0f / A3D_SPEED_OF_SOUND )

static int __cdecl CompareReflections( const void* pLeft, const void* pRight )
{
	A3dSourceReflection* pFirst = *(A3dSourceReflection* const*)pLeft;
	A3dSourceReflection* pSecond = *(A3dSourceReflection* const*)pRight;
	if ( pFirst->dwTag == pSecond->dwTag )
	{
		if ( pFirst->flAudibility > pSecond->flAudibility )
		{
			pSecond->flAudibility = 0.0f;
			pSecond->flEarGain[0] = 0.0f;
			pSecond->flEarGain[1] = 0.0f;
		}
		else
		{
			pFirst->flAudibility = 0.0f;
			pFirst->flEarGain[0] = 0.0f;
			pFirst->flEarGain[1] = 0.0f;
		}
	}

	if ( pFirst->flAudibility > pSecond->flAudibility )
		return -1;
	if ( !( pFirst->flAudibility >= pSecond->flAudibility ) )
		return 1;
	return 0;
}

static void SwapReflections( A3dSourceReflection** pLeft, A3dSourceReflection** pRight )
{
	if ( pLeft == pRight )
		return;

	A3dSourceReflection* pReflection = *pLeft;
	*pLeft = *pRight;
	*pRight = pReflection;
}

static void ShortSortReflections( A3dSourceReflection** pLow, A3dSourceReflection** pHigh )
{
	while ( pHigh > pLow )
	{
		A3dSourceReflection** pMaximum = pLow;
		for ( A3dSourceReflection** p = pLow + 1; p <= pHigh; p++ )
		{
			if ( CompareReflections( p, pMaximum ) > 0 )
				pMaximum = p;
		}
		SwapReflections( pMaximum, pHigh );
		pHigh--;
	}
}

static void SortReflections( A3dSourceReflection** pReflections, size_t nReflections )
{
	if ( nReflections < 2 )
		return;

	A3dSourceReflection** lowStack[A3D_REFLECTION_SORT_STACK];
	A3dSourceReflection** highStack[A3D_REFLECTION_SORT_STACK];
	int nStack = 0;
	A3dSourceReflection** pLow = pReflections;
	A3dSourceReflection** pHigh = pReflections + nReflections - 1;
	for ( ;; )
	{
		size_t nCount = pHigh - pLow + 1;
		if ( nCount <= A3D_REFLECTION_SHORT_SORT )
		{
			ShortSortReflections( pLow, pHigh );
		}
		else
		{
			SwapReflections( pLow + nCount / 2, pLow );
			A3dSourceReflection** pLeft = pLow;
			A3dSourceReflection** pRight = pHigh + 1;
			for ( ;; )
			{
				do
				{
					pLeft++;
				}
				while ( pLeft <= pHigh && CompareReflections( pLeft, pLow ) <= 0 );

				do
				{
					pRight--;
				}
				while ( pRight > pLow && CompareReflections( pRight, pLow ) >= 0 );

				if ( pRight < pLeft )
					break;
				SwapReflections( pLeft, pRight );
			}
			SwapReflections( pRight, pLow );

			if ( pRight - pLow > pHigh - pLeft )
			{
				if ( pLow + 1 < pRight )
				{
					lowStack[nStack] = pLow;
					highStack[nStack++] = pRight - 1;
				}
				if ( pLeft < pHigh )
				{
					pLow = pLeft;
					continue;
				}
			}
			else
			{
				if ( pLeft < pHigh )
				{
					lowStack[nStack] = pLeft;
					highStack[nStack++] = pHigh;
				}
				if ( pLow + 1 < pRight )
				{
					pHigh = pRight - 1;
					continue;
				}
			}
		}

		if ( !nStack )
			break;
		pLow = lowStack[--nStack];
		pHigh = highStack[nStack];
	}
}

A3dSourceReflection* CA3dSource::AllocateReflection()
{
	A3dSourceReflection* pReflection = (A3dSourceReflection*)calloc( 1, sizeof( A3dSourceReflection ) );
	pReflection->pNext = m_pReflections[m_iReflectionBuffer];
	m_pReflections[m_iReflectionBuffer] = pReflection;
	m_nReflections[m_iReflectionBuffer]++;
	return pReflection;
}

A3dSourceReflection* CA3dSource::NextReflection( A3dSourceReflection* pReflection )
{
	if ( pReflection )
		return pReflection->pNext;
	return m_pReflections[m_iReflectionBuffer];
}

void CA3dSource::BeginReflectionFrame()
{
	m_iReflectionBuffer = m_iReflectionBuffer == 0;
	m_nReflections[m_iReflectionBuffer] = 0;
	A3dSourceReflection* pReflection = m_pReflections[m_iReflectionBuffer];
	while ( pReflection )
	{
		A3dSourceReflection* pNext = pReflection->pNext;
		if ( pReflection == m_pReflections[m_iReflectionBuffer] )
			m_pReflections[m_iReflectionBuffer] = NULL;
		free( pReflection );
		pReflection = pNext;
	}
}

void CA3dSource::AddReflection( DWORD dwTag, const float direction[3], float flGain, const float material[6] )
{
	float flOutputGain;
	m_pA3d->GetOutputGain( &flOutputGain );
	A3dSourceReflection* pReflection = AllocateReflection();
	pReflection->dwTag = dwTag;
	pReflection->iSlot = 0;
	memcpy( pReflection->vPosition, direction, sizeof( pReflection->vPosition ) );
	Vector_ToSphericalRadians( pReflection->vPosition, pReflection->vSpherical );
	pReflection->flGain = flGain;
	float flDistance = pReflection->vSpherical[2];
	pReflection->flDistance = flDistance;
	memcpy( pReflection->flMaterialScale, material, sizeof( pReflection->flMaterialScale ) );
	float flDistanceGain;
	float flDistanceEq;
	ComputeDistanceGain( flDistance, &flDistanceGain );
	ComputeDistanceEq( pReflection->flDistance, &flDistanceEq );

	double flEq = ( ( (double)m_flEqFactor * flDistanceEq ) * pReflection->flMaterialScale[1] ) *
		pReflection->flGain;
	pReflection->flEq = (float)flEq;
	if ( !( flEq >= 0.0 ) )
		pReflection->flEq = 0.0f;
	else if ( flEq > 1.0 )
		pReflection->flEq = 1.0f;

	pReflection->flEarGain[0] = (float)( ( ( ( ( (double)m_pA3d->m_flReflectionGainScale * flOutputGain ) *
		flDistanceGain ) * m_flGain ) * pReflection->flGain ) * pReflection->flMaterialScale[0] );
	pReflection->flEarGain[1] = (float)( ( ( ( ( (double)m_pA3d->m_flReflectionGainScale * flOutputGain ) *
		flDistanceGain ) * m_flGain ) * pReflection->flGain ) * pReflection->flMaterialScale[0] );
	if ( m_pA3d->m_dwCompatValue1007 )
	{
		pReflection->flEarGain[0] = (float)( ( (double)m_flOcclusion[0] * A3D_REFLECTION_OCCLUSION_SCALE -
			A3D_REFLECTION_OCCLUSION_BIAS ) * pReflection->flEarGain[0] );
		pReflection->flEarGain[1] = (float)( ( (double)m_flOcclusion[0] * A3D_REFLECTION_OCCLUSION_SCALE -
			A3D_REFLECTION_OCCLUSION_BIAS ) * pReflection->flEarGain[1] );
		pReflection->flEq = (float)( ( (double)m_flOcclusion[1] * A3D_REFLECTION_OCCLUSION_SCALE -
			A3D_REFLECTION_OCCLUSION_BIAS ) * pReflection->flEq );
	}

	for ( int i = 0; i < 2; i++ )
	{
		if ( !( pReflection->flEarGain[i] >= 0.0f ) )
			pReflection->flEarGain[i] = 0.0f;
		else if ( pReflection->flEarGain[i] > 1.0f )
			pReflection->flEarGain[i] = 1.0f;
	}

	pReflection->flEarDelay[0] = (float)( ( ( (double)pReflection->flDistance - m_flEarDistance[0] ) *
		m_pA3d->m_flReflectionDelayScale ) * A3D_REFLECTION_DELAY_PER_UNIT );
	pReflection->flEarDelay[1] = (float)( ( ( (double)pReflection->flDistance - m_flEarDistance[1] ) *
		m_pA3d->m_flReflectionDelayScale ) * A3D_REFLECTION_DELAY_PER_UNIT );
	pReflection->flAudibility = (float)( ( (double)pReflection->flEarGain[0] + pReflection->flEarGain[1] ) *
		0.25f + (double)pReflection->flEq * 0.5f );
	if ( !( pReflection->flEarDelay[0] >= 0.0f ) )
		pReflection->flEarDelay[0] = 0.0f;
	if ( !( pReflection->flEarDelay[1] >= 0.0f ) )
		pReflection->flEarDelay[1] = 0.0f;
	if ( pReflection->flEarDelay[0] > m_pA3d->m_flMaxReflectionDelayTime )
		pReflection->flEarDelay[0] = m_pA3d->m_flMaxReflectionDelayTime;
	if ( pReflection->flEarDelay[1] > m_pA3d->m_flMaxReflectionDelayTime )
		pReflection->flEarDelay[1] = m_pA3d->m_flMaxReflectionDelayTime;
}

void CA3dSource::UpdateReflections( int nSources )
{
	int nLimit = A3D_REFLECTION_CHANNEL_BUDGET / nSources;
	if ( nLimit < A3D_MIN_SOURCE_REFLECTIONS )
		nLimit = A3D_MIN_SOURCE_REFLECTIONS;
	else if ( nLimit > MAX_A3D_SOURCE_REFLECTIONS )
		nLimit = MAX_A3D_SOURCE_REFLECTIONS;

	for ( int i = 0; i < MAX_A3D_SOURCE_REFLECTIONS; i++ )
	{
		m_pRenderState->Reflections[i].dwEnabled = 0;
		m_pRenderState->Reflections[i].dwActive = 0;
	}

	A3dSourceReflection* pReflection = NextReflection( NULL );
	int nActive = 0;
	while ( pReflection && nActive < nLimit )
	{
		if ( pReflection->iSlot > A3D_REFLECTION_SLOT_UNASSIGNED && pReflection->flAudibility > 0.0f )
		{
			a3d_source_reflection_state_t* pState = &m_pRenderState->Reflections[pReflection->iSlot];
			pState->dwEnabled = 1;
			pState->dwActive = 1;
			pState->dwNewSlot = pReflection->bNewSlot;
			pState->dwReserved = 0;
			pState->flAudibility = pReflection->flAudibility;
			double flEq = 1.0 - pReflection->flEq;
			pState->flEq = (float)flEq;
			if ( flEq > A3D_REFLECTION_MAX_EQ )
				pState->flEq = A3D_REFLECTION_MAX_EQ;
			pState->Left.flAzimuth = pReflection->vSpherical[0];
			pState->Left.flElevation = pReflection->vSpherical[1];
			pState->Left.flGain = pReflection->flEarGain[0];
			pState->Left.flDelay = pReflection->flEarDelay[0];
			pState->Right.flAzimuth = pReflection->vSpherical[0];
			pState->Right.flElevation = pReflection->vSpherical[1];
			pState->Right.flGain = pReflection->flEarGain[1];
			pState->Right.flDelay = pReflection->flEarDelay[1];
			nActive++;
		}
		pReflection = NextReflection( pReflection );
	}
}

void CA3dSource::ReuseReflections()
{
	A3DCTRL_SRC_SUPER* pPrevious = m_pRenderStates[m_iRenderState == 0];
	memcpy( m_pRenderState->Reflections, pPrevious->Reflections, sizeof( pPrevious->Reflections ) );
}

void CA3dSource::PruneReflections( int nSources )
{
	int nLimit = A3D_REFLECTION_CHANNEL_BUDGET / nSources;
	if ( nLimit < A3D_MIN_SOURCE_REFLECTIONS )
		nLimit = A3D_MIN_SOURCE_REFLECTIONS;
	else if ( nLimit > MAX_A3D_SOURCE_REFLECTIONS )
		nLimit = MAX_A3D_SOURCE_REFLECTIONS;

	A3dSourceReflection* pReflection = m_pReflections[m_iReflectionBuffer];
	A3dSourceReflection* pPrevious = m_pReflections[m_iReflectionBuffer == 0];
	if ( !pReflection )
		return;

	A3dSourceReflection** pReflections = (A3dSourceReflection**)calloc( m_nReflections[m_iReflectionBuffer],
		sizeof( A3dSourceReflection* ) );
	int nReflections = 0;
	while ( pReflection )
	{
		pReflections[nReflections++] = pReflection;
		pReflection = NextReflection( pReflection );
	}
	SortReflections( pReflections, nReflections );

	int nCandidates = nReflections;
	if ( nCandidates > MAX_A3D_SOURCE_REFLECTIONS )
		nCandidates = MAX_A3D_SOURCE_REFLECTIONS;
	if ( nReflections > nLimit )
		nReflections = nLimit;

	DWORD dwUsedSlots = 0;
	int nUnassigned = 0;
	for ( int i = 0; i < nReflections; i++ )
	{
		pReflection = pPrevious;
		BOOL bFound = FALSE;
		while ( pReflection && !bFound )
		{
			if ( pReflection->dwTag == pReflections[i]->dwTag )
			{
				pReflections[i]->iSlot = pReflection->iSlot;
				dwUsedSlots |= 1U << ( pReflection->iSlot & A3D_REFLECTION_SLOT_MASK );
				pReflections[i]->bNewSlot = FALSE;
				bFound = TRUE;
			}
			else
			{
				pReflection = NextReflection( pReflection );
			}
		}
		if ( !bFound )
		{
			nUnassigned++;
			pReflections[i]->iSlot = A3D_REFLECTION_SLOT_UNASSIGNED;
		}
	}
	for ( int i = nReflections; i < nCandidates; i++ )
		pReflections[i]->iSlot = A3D_REFLECTION_SLOT_CULLED;

	int iSlot = 0;
	if ( nUnassigned )
	{
		for ( int i = 0; i < nReflections; i++ )
		{
			if ( pReflections[i]->iSlot == A3D_REFLECTION_SLOT_UNASSIGNED )
			{
				while ( ( dwUsedSlots & ( 1U << ( iSlot & A3D_REFLECTION_SLOT_MASK ) ) ) &&
					iSlot < MAX_A3D_SOURCE_REFLECTIONS )
				{
					iSlot++;
				}
				pReflections[i]->iSlot = iSlot;
				pReflections[i]->bNewSlot = TRUE;
				dwUsedSlots |= 1U << ( iSlot & A3D_REFLECTION_SLOT_MASK );
			}
		}
	}
	free( pReflections );
}

HRESULT CA3dSource::StartPendingPlayback()
{
	if ( !m_pBuffer )
		return A3DERROR_NO_WAVE_DATA;
	if ( m_nPlaybackState != 1 )
		return S_OK;

	if ( m_pA3d->m_bCompat1011 )
	{
		DWORD dwPosition;
		m_pBuffer->GetCurrentPosition( &dwPosition, &dwPosition );
		if ( FAILED( m_pBuffer->Play( 0, 0, m_dwLoopingFlags ) ) )
			return A3DERROR_FAILED_PLAY;
		SubmitRenderState();
	}
	else if ( FAILED( m_pBuffer->Play( 0, 0, m_dwLoopingFlags ) ) )
	{
		return A3DERROR_FAILED_PLAY;
	}
	m_nPlaybackState = 2;
	return S_OK;
}
