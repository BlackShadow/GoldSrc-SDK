#pragma once

#include "A3dGeom.h"

class CA3dCommandBuffer
{
public:
	CA3dCommandBuffer( int cCapacity, int iBufferType );
	virtual ~CA3dCommandBuffer();

	a3d_geometry_command_t* ReadCommand();
	void* Next( DWORD* pdwOpcode );

	a3d_geometry_command_t*	m_pCommands;
	int						m_iCursor;
	int						m_cCapacity;
	int						m_cCommands;
	int						m_iBufferType;
};

int	__cdecl CommandBuffer_CheckFit( const CA3dCommandBuffer* pDest, int iDest,
		const CA3dCommandBuffer* pSource );
int	__cdecl CommandBuffer_Copy( CA3dCommandBuffer* pDest, int iDest,
		const CA3dCommandBuffer* pSource, int iSource, int cCommands );
