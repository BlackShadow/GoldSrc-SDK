#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <unknwn.h>

#include "ia3dapi.h"
#include "LinkList.h"

class CA3dGeometryFace;

class CA3dGeometryEdge
{
public:
	CA3dGeometryEdge( const A3DVERTEX* pEndpoints, CA3dGeometryFace* pFace );
	virtual ~CA3dGeometryEdge();

	void TransformEndpoints( A3DVERTEX* pOut );
	BOOL HasConflict();

	A3DVERTEX			m_EndPoints[2];
	CA3dGeometryFace*	m_pFace;
	CA3dObjectList		m_AdjacentEdges;
};

DWORD	__stdcall Mixer_MultiplyFixed16( DWORD left, DWORD right );
