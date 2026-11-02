#pragma once

#include <windows.h>
#include <unknwn.h>
#include <mmsystem.h>
#include <dsound.h>
#include <ia3dapi.h>

class CA3d;

#define MAX_A3D_SOURCE_REFLECTIONS	16
#define A3D_SPEED_OF_SOUND			340.5f

typedef struct a3d_source_channel_s
{
	float	flAzimuth;
	float	flElevation;
	float	flGain;
	float	flDelay;
} a3d_source_channel_t;

typedef struct a3d_source_reflection_state_s
{
	DWORD					dwEnabled;
	DWORD					dwActive;
	DWORD					dwNewSlot;
	DWORD					dwReserved;
	float					flEq;
	a3d_source_channel_t	Left;
	a3d_source_channel_t	Right;
	float					flAudibility;
} a3d_source_reflection_state_t;

typedef struct A3DCTRL_SRC_SUPER
{
	DWORD					dwFlags;
	DWORD					dwMode;
	DWORD					dwEnabled;
	DWORD					dwReserved;
	float					flEq;
	float					flPitch;
	float					flMono;
	a3d_source_channel_t	Left;
	a3d_source_channel_t	Right;
	float					flReservedFactor;
	float					flAudibility;
	float					flDistance;
	DWORD					dwReservedDistance;
	a3d_source_reflection_state_t	Reflections[MAX_A3D_SOURCE_REFLECTIONS];
	BYTE					ReservedBackendData[64];
} A3DCTRL_SRC_SUPER;

typedef struct A3dSourceReflection
{
	A3dSourceReflection*	pNext;
	DWORD					dwTag;
	LONG					iSlot;
	BOOL					bNewSlot;
	float					vPosition[3];
	float					vSpherical[3];
	float					flDistance;
	float					flGain;
	float					flMaterialScale[2];
	float					flEq;
	float					flEarGain[2];
	float					flAudibility;
	float					flEarDelay[2];
	DWORD					dwReserved;
} A3dSourceReflection;

class IA3dSource202 : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE LoadWaveFile( LPSTR pFilename ) = 0;
	virtual HRESULT STDMETHODCALLTYPE LoadWaveData( LPVOID pData, DWORD dwSize ) = 0;
	virtual HRESULT STDMETHODCALLTYPE AllocateWaveData( INT nSize ) = 0;
	virtual HRESULT STDMETHODCALLTYPE FreeWaveData() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetWaveFormat( LPVOID pFormat ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWaveFormat( LPVOID pFormat ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWaveSize() = 0;
	virtual HRESULT STDMETHODCALLTYPE GetType( LPDWORD pdwType ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Lock( DWORD dwOffset, DWORD dwSize, LPVOID* ppBuffer1,
		LPDWORD pdwSize1, LPVOID* ppBuffer2, LPDWORD pdwSize2, DWORD dwFlags ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Unlock( LPVOID pBuffer1, DWORD dwSize1, LPVOID pBuffer2, DWORD dwSize2 ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Play( INT nPlayMode ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Stop() = 0;
	virtual HRESULT STDMETHODCALLTYPE Rewind() = 0;
	virtual HRESULT STDMETHODCALLTYPE SetWaveTime( A3DVAL flTime ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWaveTime( LPA3DVAL pTime ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetWavePosition( DWORD dwPosition ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWavePosition( LPDWORD pdwPosition ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPosition3f( A3DVAL x, A3DVAL y, A3DVAL z ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPosition3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPosition3fv( LPA3DVAL pPosition ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPosition3fv( LPA3DVAL pPosition ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOrientationAngles3f( A3DVAL x, A3DVAL y, A3DVAL z ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOrientationAngles3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOrientationAngles3fv( LPA3DVAL pAngles ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOrientationAngles3fv( LPA3DVAL pAngles ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOrientation6f( A3DVAL dirX, A3DVAL dirY, A3DVAL dirZ,
		A3DVAL upX, A3DVAL upY, A3DVAL upZ ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOrientation6f( LPA3DVAL pDirX, LPA3DVAL pDirY, LPA3DVAL pDirZ,
		LPA3DVAL pUpX, LPA3DVAL pUpY, LPA3DVAL pUpZ ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOrientation6fv( LPA3DVAL pOrientation ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOrientation6fv( LPA3DVAL pOrientation ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetVelocity3f( A3DVAL x, A3DVAL y, A3DVAL z ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetVelocity3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetVelocity3fv( LPA3DVAL pVelocity ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetVelocity3fv( LPA3DVAL pVelocity ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCone( A3DVAL flInnerAngle, A3DVAL flOuterAngle, A3DVAL flOuterGain ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCone( LPA3DVAL pInnerAngle, LPA3DVAL pOuterAngle, LPA3DVAL pOuterGain ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMinMaxDistance( A3DVAL flMinDistance, A3DVAL flMaxDistance,
		DWORD dwMaxDistanceMute ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetMinMaxDistance( LPA3DVAL pMinDistance, LPA3DVAL pMaxDistance,
		LPDWORD pdwMaxDistanceMute ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetGain( A3DVAL flGain ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetGain( LPA3DVAL pGain ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPitch( A3DVAL flPitch ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPitch( LPA3DVAL pPitch ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetDopplerScale( A3DVAL flScale ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDopplerScale( LPA3DVAL pScale ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetDistanceModelScale( A3DVAL flScale ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDistanceModelScale( LPA3DVAL pScale ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetEq( A3DVAL flEq ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetEq( LPA3DVAL pEq ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPriority( A3DVAL flPriority ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPriority( LPA3DVAL pPriority ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetRenderMode( DWORD dwMode ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetRenderMode( LPDWORD pdwMode ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAudibility( LPA3DVAL pAudibility ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOcclusionFactor( LPA3DVAL pFactor ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetStatus( LPDWORD pdwStatus ) = 0;
};

class IA3dSourcePrivate : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE CompatibilityOperation( DWORD dwArgument ) = 0;
	virtual HRESULT STDMETHODCALLTYPE InitializeReflectionSlots( A3dSourceReflection** ppReflections ) = 0;
};

class IA3dSourceRenderBuffer : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetA3dSuperCtrl( A3DCTRL_SRC_SUPER* pState, DWORD dwSize ) = 0;
};

typedef struct a3d_source_state_s
{
	A3dSourceReflection*	m_pReflections[2];
	DWORD				m_nReflections[2];
	DWORD				m_iReflectionBuffer;
	float				m_flCachedOcclusionFactor;
	float				m_flCachedOcclusionGain[2];
	ULONG				m_cReferences;
	float				m_vRenderPosition[4];
	BOOL				m_bRegistered;
	DWORD				m_iUpdateGroup;
	DWORD				m_nPlaybackState;
	DWORD				m_dwLoopingFlags;
	IDirectSound*		m_pDirectSound;
	IDirectSoundBuffer*	m_pBuffer;
	IDirectSound3DBuffer*	m_pBuffer3d;
	IA3dSourceRenderBuffer*	m_pSourceBackend;
	float				m_flPlaybackPitch;
	float				m_flEarDistance[2];
	float				m_flGain;
	float				m_flEq;
	float				m_flEqFactor;
	float				m_flPitch;
	float				m_flDopplerScale;
	float				m_flDistanceModelScale;
	float				m_flMinDistance;
	float				m_flMaxDistance;
	DWORD				m_dwMaxDistanceMute;
	float				m_flStereoSpread;
	float				m_flDopplerFactor;
	float				m_flDistanceGain[2];
	float				m_flDistanceEq;
	float				m_flTargetOcclusion[2];
	float				m_flOcclusion[2];
	float				m_flOcclusionStep[2];
	float				m_flOcclusionFactor;
	float				m_flInnerConeAngle;
	float				m_flOuterConeAngle;
	float				m_flOuterConeGain;
	float				m_flConeGain;
	float				m_flReservedFactor;
	DWORD				m_dwRenderMode;
	DWORD				m_dwType;
	DWORD				m_nPlaybackCounter;
	float				m_Transform[16];
	float				m_vPosition[4];
	float				m_vOrientationAngles[4];
	float				m_vVelocity[4];
	float				m_vDirection[4];
	float				m_vUp[4];
	CA3d*				m_pA3d;
	DWORD				m_nSamplesPerSec;
	WORD				m_wBitsPerSample;
	WORD				m_nBlockAlign;
	WORD				m_nChannels;
	WORD				m_wFormatTag;
	DWORD				m_nAvgBytesPerSec;
	DWORD				m_nWaveSize;
	float				m_flPriority;
	BOOL				m_bWithinReflectionRange;
	A3DCTRL_SRC_SUPER*	m_pRenderStates[2];
	DWORD				m_iRenderState;
	A3DCTRL_SRC_SUPER*	m_pRenderState;
} a3d_source_state_t;

class CA3dSource : public IA3dSource202, public IA3dSourcePrivate, private a3d_source_state_t
{
public:
	CA3dSource( IDirectSound* pDirectSound, CA3d* pA3d, DWORD dwType );
	CA3dSource( const CA3dSource& source, HRESULT* pStatus );

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE LoadWaveFile( LPSTR pFilename );
	HRESULT STDMETHODCALLTYPE LoadWaveData( LPVOID pData, DWORD dwSize );
	HRESULT STDMETHODCALLTYPE AllocateWaveData( INT nSize );
	HRESULT STDMETHODCALLTYPE FreeWaveData();
	HRESULT STDMETHODCALLTYPE SetWaveFormat( LPVOID pFormat );
	HRESULT STDMETHODCALLTYPE GetWaveFormat( LPVOID pFormat );
	HRESULT STDMETHODCALLTYPE GetWaveSize();
	HRESULT STDMETHODCALLTYPE GetType( LPDWORD pdwType );
	HRESULT STDMETHODCALLTYPE Lock( DWORD dwOffset, DWORD dwSize, LPVOID* ppBuffer1,
		LPDWORD pdwSize1, LPVOID* ppBuffer2, LPDWORD pdwSize2, DWORD dwFlags );
	HRESULT STDMETHODCALLTYPE Unlock( LPVOID pBuffer1, DWORD dwSize1, LPVOID pBuffer2, DWORD dwSize2 );
	HRESULT STDMETHODCALLTYPE Play( INT nPlayMode );
	HRESULT STDMETHODCALLTYPE Stop();
	HRESULT STDMETHODCALLTYPE Rewind();
	HRESULT STDMETHODCALLTYPE SetWaveTime( A3DVAL flTime );
	HRESULT STDMETHODCALLTYPE GetWaveTime( LPA3DVAL pTime );
	HRESULT STDMETHODCALLTYPE SetWavePosition( DWORD dwPosition );
	HRESULT STDMETHODCALLTYPE GetWavePosition( LPDWORD pdwPosition );
	HRESULT STDMETHODCALLTYPE SetPosition3f( A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT STDMETHODCALLTYPE GetPosition3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ );
	HRESULT STDMETHODCALLTYPE SetPosition3fv( LPA3DVAL pPosition );
	HRESULT STDMETHODCALLTYPE GetPosition3fv( LPA3DVAL pPosition );
	HRESULT STDMETHODCALLTYPE SetOrientationAngles3f( A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT STDMETHODCALLTYPE GetOrientationAngles3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ );
	HRESULT STDMETHODCALLTYPE SetOrientationAngles3fv( LPA3DVAL pAngles );
	HRESULT STDMETHODCALLTYPE GetOrientationAngles3fv( LPA3DVAL pAngles );
	HRESULT STDMETHODCALLTYPE SetOrientation6f( A3DVAL dirX, A3DVAL dirY, A3DVAL dirZ,
		A3DVAL upX, A3DVAL upY, A3DVAL upZ );
	HRESULT STDMETHODCALLTYPE GetOrientation6f( LPA3DVAL pDirX, LPA3DVAL pDirY, LPA3DVAL pDirZ,
		LPA3DVAL pUpX, LPA3DVAL pUpY, LPA3DVAL pUpZ );
	HRESULT STDMETHODCALLTYPE SetOrientation6fv( LPA3DVAL pOrientation );
	HRESULT STDMETHODCALLTYPE GetOrientation6fv( LPA3DVAL pOrientation );
	HRESULT STDMETHODCALLTYPE SetVelocity3f( A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT STDMETHODCALLTYPE GetVelocity3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ );
	HRESULT STDMETHODCALLTYPE SetVelocity3fv( LPA3DVAL pVelocity );
	HRESULT STDMETHODCALLTYPE GetVelocity3fv( LPA3DVAL pVelocity );
	HRESULT STDMETHODCALLTYPE SetCone( A3DVAL flInnerAngle, A3DVAL flOuterAngle, A3DVAL flOuterGain );
	HRESULT STDMETHODCALLTYPE GetCone( LPA3DVAL pInnerAngle, LPA3DVAL pOuterAngle, LPA3DVAL pOuterGain );
	HRESULT STDMETHODCALLTYPE SetMinMaxDistance( A3DVAL flMinDistance, A3DVAL flMaxDistance,
		DWORD dwMaxDistanceMute );
	HRESULT STDMETHODCALLTYPE GetMinMaxDistance( LPA3DVAL pMinDistance, LPA3DVAL pMaxDistance,
		LPDWORD pdwMaxDistanceMute );
	HRESULT STDMETHODCALLTYPE SetGain( A3DVAL flGain );
	HRESULT STDMETHODCALLTYPE GetGain( LPA3DVAL pGain );
	HRESULT STDMETHODCALLTYPE SetPitch( A3DVAL flPitch );
	HRESULT STDMETHODCALLTYPE GetPitch( LPA3DVAL pPitch );
	HRESULT STDMETHODCALLTYPE SetDopplerScale( A3DVAL flScale );
	HRESULT STDMETHODCALLTYPE GetDopplerScale( LPA3DVAL pScale );
	HRESULT STDMETHODCALLTYPE SetDistanceModelScale( A3DVAL flScale );
	HRESULT STDMETHODCALLTYPE GetDistanceModelScale( LPA3DVAL pScale );
	HRESULT STDMETHODCALLTYPE SetEq( A3DVAL flEq );
	HRESULT STDMETHODCALLTYPE GetEq( LPA3DVAL pEq );
	HRESULT STDMETHODCALLTYPE SetPriority( A3DVAL flPriority );
	HRESULT STDMETHODCALLTYPE GetPriority( LPA3DVAL pPriority );
	HRESULT STDMETHODCALLTYPE SetRenderMode( DWORD dwMode );
	HRESULT STDMETHODCALLTYPE GetRenderMode( LPDWORD pdwMode );
	HRESULT STDMETHODCALLTYPE GetAudibility( LPA3DVAL pAudibility );
	HRESULT STDMETHODCALLTYPE GetOcclusionFactor( LPA3DVAL pFactor );
	HRESULT STDMETHODCALLTYPE GetStatus( LPDWORD pdwStatus );
	virtual ~CA3dSource();

	HRESULT STDMETHODCALLTYPE CompatibilityOperation( DWORD dwArgument );
	HRESULT STDMETHODCALLTYPE InitializeReflectionSlots( A3dSourceReflection** ppReflections );
	void GetWorldTransform( float matrix[16] );
	void GetWorldVelocity( float velocity[3] );
	void InitializeRenderState();
	void UpdateRenderState( const float left[3], const float right[3] );
	void SubmitRenderState();
	void ResetReflectionChannels();
	void DisableReflectionChannels();
	void UpdateOcclusion( float flFactor, const float gainEq[2] );
	void UpdateStereoSpread( const float spherical[3] );
	void UpdateDoppler( const float sourceVelocity[3], const float listenerVelocity[3],
		const float relativeDirection[3], float flDistance );
	void UpdateConeGain( const float relativeDirection[3] );
	void UpdateDistanceModel( float flLeftDistance, float flRightDistance );
	void BeginReflectionFrame();
	void AddReflection( DWORD dwTag, const float direction[3], float flGain, const float material[6] );
	void UpdateReflections( int nSources );
	void ReuseReflections();
	void PruneReflections( int nSources );
	HRESULT StartPendingPlayback();

private:
	friend class CA3d;

	void SetRenderPosition( const float left[3], const float right[3] );
	void SetRenderDelays();
	void SetRenderGain();
	void SetRenderEq();
	void SetRenderAudibility();
	void SetRenderPitch();
	void ApplyDirectSoundState();
	void ComputeDistanceGain( float flDistance, float* pGain );
	void ComputeDistanceEq( float flDistance, float* pEq );
	A3dSourceReflection* AllocateReflection();
	A3dSourceReflection* NextReflection( A3dSourceReflection* pReflection );
};

extern DWORD	g_iSourceUpdateGroup;
