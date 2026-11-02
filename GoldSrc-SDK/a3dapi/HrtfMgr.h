#pragma once

#include <windows.h>

struct a3d_hrtf_definition_s;

class CHrtfMgr
{
public:
	CHrtfMgr();
	virtual ~CHrtfMgr();

	HRESULT Initialize( DWORD dwSampleRate, DWORD nCoefficients, DWORD dwSelector );
	HRESULT GetCoeffs( const int indices[4], const float weights[4], DWORD dwReserved, LONG* pCoefficients );
	HRESULT GetIDXsAndWts( float azimuth, float elevation, int nEar, int indices[4], float weights[4] );

private:
	BOOL	m_bInitialized;
	BOOL	m_bInterpolationReady;
	DWORD	m_dwReserved;
	DWORD	m_nCoefficients;

	const a3d_hrtf_definition_s* m_pDefinition;
};
