#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include "ia3dapi.h"
#include "A3dGeom.h"

enum
{
	A3D_COMPAT_LEGACY_STATE					= 1000,
	A3D_COMPAT_LIST_TRANSFORM				= 1001,
	A3D_COMPAT_REFLECTION_PLANE_SCALE		= 1002,
	A3D_COMPAT_POLYGON_TEST_LIMIT			= 1003,
	A3D_COMPAT_REFLECTION_UPDATE_MODULUS	= 1004,
	A3D_COMPAT_OCCLUSION_UPDATE_MODULUS		= 1005,
	A3D_COMPAT_UPDATE_TICK_PERIOD			= 1006,
	A3D_COMPAT_LEGACY_STATE_ALTERNATE		= 1007,
	A3D_COMPAT_LEGACY_NO_OPERATION			= 1008,
	A3D_COMPAT_LEGACY_PRIMITIVE_STATE		= 1009,
	A3D_COMPAT_OCCLUSION_RAY_FRACTION		= 1010,
	A3D_COMPAT_LEGACY_MARKER				= 1011,
	A3D_COMPAT_REFLECTION_DISTANCE_SQUARED	= 2000
};

#define A3D_COMPAT_LEGACY_MARKER_VALUE	0x21

#define A3D_DEFAULT_REFLECTION_PLANE_SCALE		2.0f
#define A3D_DEFAULT_POLYGON_TEST_LIMIT			512
#define A3D_DEFAULT_REFLECTION_UPDATE_MODULUS	2
#define A3D_DEFAULT_OCCLUSION_UPDATE_MODULUS	1
#define A3D_DEFAULT_UPDATE_TICK_PERIOD			33

class CA3d;
class CA3dSource;
class IA3dDal;

class IA3dTarget : public IA3d3
{
};

class IA3dGeomTarget : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE Enable( DWORD dwFeatures ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Disable( DWORD dwFeatures ) = 0;
	virtual BOOL STDMETHODCALLTYPE IsEnabled( DWORD dwFeatures ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOcclusionMode( DWORD dwMode ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOcclusionMode( LPDWORD pMode ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetReflectionMode( DWORD dwMode ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetReflectionMode( LPDWORD pMode ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetReflectionGainScale( A3DVAL flScale ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetReflectionGainScale( LPA3DVAL pScale ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetReflectionDelayScale( A3DVAL flScale ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetReflectionDelayScale( LPA3DVAL pScale ) = 0;
	virtual ULONG STDMETHODCALLTYPE PushMatrix() = 0;
	virtual ULONG STDMETHODCALLTYPE PopMatrix() = 0;
	virtual HRESULT STDMETHODCALLTYPE LoadIdentity() = 0;
	virtual HRESULT STDMETHODCALLTYPE LoadMatrix( A3DMATRIX matrix ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetMatrix( A3DMATRIX matrix ) = 0;
	virtual HRESULT STDMETHODCALLTYPE MultMatrix( A3DMATRIX matrix ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Translate3f( A3DVAL x, A3DVAL y, A3DVAL z ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Translate3fv( LPA3DVAL pTranslation ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Rotate3f( A3DVAL flAngle, A3DVAL x, A3DVAL y, A3DVAL z ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Rotate3fv( A3DVAL flAngle, LPA3DVAL pAxis ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Scale3f( A3DVAL x, A3DVAL y, A3DVAL z ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Scale3fv( LPA3DVAL pScale ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Begin( DWORD dwMode ) = 0;
	virtual HRESULT STDMETHODCALLTYPE End() = 0;
	virtual HRESULT STDMETHODCALLTYPE Vertex3f( A3DVAL x, A3DVAL y, A3DVAL z ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Vertex3fv( LPA3DVAL pVertex ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Normal3f( A3DVAL x, A3DVAL y, A3DVAL z ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Normal3fv( LPA3DVAL pNormal ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Tag( DWORD dwTag ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOpeningFactorf( A3DVAL flFactor ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOpeningFactorfv( LPA3DVAL pFactor ) = 0;
	virtual HRESULT STDMETHODCALLTYPE NewMaterial( LPA3DMATERIAL* ppMaterial ) = 0;
	virtual HRESULT STDMETHODCALLTYPE BindMaterial( LPA3DMATERIAL pMaterial ) = 0;
	virtual HRESULT STDMETHODCALLTYPE NewList( LPA3DLIST* ppList ) = 0;
	virtual HRESULT STDMETHODCALLTYPE BindListener() = 0;
	virtual HRESULT STDMETHODCALLTYPE BindSource( LPA3DSOURCE pSource ) = 0;
	virtual HRESULT STDMETHODCALLTYPE NewEnvironment( LPA3DENVIRONMENT* ppEnvironment ) = 0;
	virtual HRESULT STDMETHODCALLTYPE BindEnvironment( LPA3DENVIRONMENT pEnvironment ) = 0;
};

class IA3dListenerTarget : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE ListenerSetPosition3f( A3DVAL x, A3DVAL y, A3DVAL z ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerGetPosition3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerSetPosition3fv( LPA3DVAL pPosition ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerGetPosition3fv( LPA3DVAL pPosition ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerSetOrientationAngles3f( A3DVAL yaw, A3DVAL pitch, A3DVAL roll ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerGetOrientationAngles3f( LPA3DVAL pYaw, LPA3DVAL pPitch, LPA3DVAL pRoll ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerSetOrientationAngles3fv( LPA3DVAL pAngles ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerGetOrientationAngles3fv( LPA3DVAL pAngles ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerSetOrientation6f( A3DVAL frontX, A3DVAL frontY, A3DVAL frontZ,
		A3DVAL topX, A3DVAL topY, A3DVAL topZ ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerGetOrientation6f( LPA3DVAL pFrontX, LPA3DVAL pFrontY, LPA3DVAL pFrontZ,
		LPA3DVAL pTopX, LPA3DVAL pTopY, LPA3DVAL pTopZ ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerSetOrientation6fv( LPA3DVAL pOrientation ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerGetOrientation6fv( LPA3DVAL pOrientation ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerSetVelocity3f( A3DVAL x, A3DVAL y, A3DVAL z ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerGetVelocity3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerSetVelocity3fv( LPA3DVAL pVelocity ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ListenerGetVelocity3fv( LPA3DVAL pVelocity ) = 0;
};

class IA3dSceneFactoryTarget : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE CreateScene( DWORD dwArgument, void** ppScene ) = 0;
	virtual HRESULT STDMETHODCALLTYPE ProcessScene( void* pScene ) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateGeometryGroup( DWORD dwType, void** ppGroup ) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreateWallBuilder( DWORD dwType, void** ppBuilder ) = 0;
	virtual HRESULT STDMETHODCALLTYPE CreatePortal( DWORD dwArgument, void** ppPortal ) = 0;
};

class IA3dCompatibilityTarget : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE CompatibilityOperation( DWORD dwArgument ) = 0;
	virtual HRESULT STDMETHODCALLTYPE CompatibilityOperationPair( DWORD dwFirst, DWORD dwSecond ) = 0;
	virtual HRESULT STDMETHODCALLTYPE CompatibilityOperationAlternate( DWORD dwArgument ) = 0;
};

class CA3d : public IA3dTarget, public IA3dGeomTarget, public IA3dListenerTarget,
	public IA3dSceneFactoryTarget, public IA3dCompatibilityTarget
{
public:
	CA3d( BOOL bLegacyDriver );

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE SetOutputMode( DWORD dwFront, DWORD dwRear, DWORD dwMode );
	HRESULT STDMETHODCALLTYPE GetOutputMode( LPDWORD pFront, LPDWORD pRear, LPDWORD pMode );
	HRESULT STDMETHODCALLTYPE SetResourceManagerMode( DWORD dwMode );
	HRESULT STDMETHODCALLTYPE GetResourceManagerMode( LPDWORD pMode );
	HRESULT STDMETHODCALLTYPE SetHFAbsorbFactor( FLOAT flFactor );
	HRESULT STDMETHODCALLTYPE GetHFAbsorbFactor( FLOAT* pFactor );
	HRESULT STDMETHODCALLTYPE RegisterVersion( DWORD dwVersion );
	HRESULT STDMETHODCALLTYPE GetSoftwareCaps( LPA3DCAPS_SOFTWARE pCaps );
	HRESULT STDMETHODCALLTYPE GetHardwareCaps( LPA3DCAPS_HARDWARE pCaps );
	HRESULT STDMETHODCALLTYPE Clear();
	HRESULT STDMETHODCALLTYPE Flush();
	HRESULT STDMETHODCALLTYPE Compat( DWORD dwOperation, DWORD dwValue );
	HRESULT STDMETHODCALLTYPE Init( LPGUID pDevice, DWORD dwFeatures, DWORD dwReserved );
	HRESULT STDMETHODCALLTYPE IsFeatureAvailable( DWORD dwFeatures );
	HRESULT STDMETHODCALLTYPE NewSource( DWORD dwFlags, LPA3DSOURCE* ppSource );
	HRESULT STDMETHODCALLTYPE DuplicateSource( LPA3DSOURCE pSource, LPA3DSOURCE* ppDuplicate );
	HRESULT STDMETHODCALLTYPE SetCooperativeLevel( HWND hWnd, DWORD dwLevel );
	HRESULT STDMETHODCALLTYPE GetCooperativeLevel( LPDWORD pLevel );
	HRESULT STDMETHODCALLTYPE SetMaxReflectionDelayTime( A3DVAL flTime );
	HRESULT STDMETHODCALLTYPE GetMaxReflectionDelayTime( LPA3DVAL pTime );
	HRESULT STDMETHODCALLTYPE SetCoordinateSystem( DWORD dwSystem );
	HRESULT STDMETHODCALLTYPE GetCoordinateSystem( LPDWORD pSystem );
	HRESULT STDMETHODCALLTYPE SetOutputGain( A3DVAL flGain );
	HRESULT STDMETHODCALLTYPE GetOutputGain( LPA3DVAL pGain );
	HRESULT STDMETHODCALLTYPE SetNumFallbackSources( DWORD dwSources );
	HRESULT STDMETHODCALLTYPE GetNumFallbackSources( LPDWORD pSources );

	virtual ~CA3d();
	virtual HRESULT STDMETHODCALLTYPE SetOcclusionModeInternal( DWORD dwMode );
	virtual HRESULT STDMETHODCALLTYPE SetReflectionModeInternal( DWORD dwMode );
	virtual HRESULT STDMETHODCALLTYPE SetPosition3f( A3DVAL x, A3DVAL y, A3DVAL z );
	virtual HRESULT STDMETHODCALLTYPE SetPosition3fv( LPA3DVAL pPosition );
	virtual HRESULT STDMETHODCALLTYPE SetOrientationAngles3f( A3DVAL yaw, A3DVAL pitch, A3DVAL roll );
	virtual HRESULT STDMETHODCALLTYPE SetOrientationAngles3fv( LPA3DVAL pAngles );
	virtual HRESULT STDMETHODCALLTYPE SetOrientation6f( A3DVAL frontX, A3DVAL frontY, A3DVAL frontZ,
		A3DVAL topX, A3DVAL topY, A3DVAL topZ );
	virtual HRESULT STDMETHODCALLTYPE SetOrientation6fv( LPA3DVAL pOrientation );
	virtual HRESULT STDMETHODCALLTYPE SetVelocity3f( A3DVAL x, A3DVAL y, A3DVAL z );
	virtual HRESULT STDMETHODCALLTYPE SetVelocity3fv( LPA3DVAL pVelocity );

	HRESULT STDMETHODCALLTYPE Enable( DWORD dwFeatures );
	HRESULT STDMETHODCALLTYPE Disable( DWORD dwFeatures );
	BOOL STDMETHODCALLTYPE IsEnabled( DWORD dwFeatures );
	HRESULT STDMETHODCALLTYPE SetOcclusionMode( DWORD dwMode );
	HRESULT STDMETHODCALLTYPE GetOcclusionMode( LPDWORD pMode );
	HRESULT STDMETHODCALLTYPE SetReflectionMode( DWORD dwMode );
	HRESULT STDMETHODCALLTYPE GetReflectionMode( LPDWORD pMode );
	HRESULT STDMETHODCALLTYPE SetReflectionGainScale( A3DVAL flScale );
	HRESULT STDMETHODCALLTYPE GetReflectionGainScale( LPA3DVAL pScale );
	HRESULT STDMETHODCALLTYPE SetReflectionDelayScale( A3DVAL flScale );
	HRESULT STDMETHODCALLTYPE GetReflectionDelayScale( LPA3DVAL pScale );
	ULONG STDMETHODCALLTYPE PushMatrix();
	ULONG STDMETHODCALLTYPE PopMatrix();
	HRESULT STDMETHODCALLTYPE LoadIdentity();
	HRESULT STDMETHODCALLTYPE LoadMatrix( A3DMATRIX matrix );
	HRESULT STDMETHODCALLTYPE GetMatrix( A3DMATRIX matrix );
	HRESULT STDMETHODCALLTYPE MultMatrix( A3DMATRIX matrix );
	HRESULT STDMETHODCALLTYPE Translate3f( A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT STDMETHODCALLTYPE Translate3fv( LPA3DVAL pTranslation );
	HRESULT STDMETHODCALLTYPE Rotate3f( A3DVAL flAngle, A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT STDMETHODCALLTYPE Rotate3fv( A3DVAL flAngle, LPA3DVAL pAxis );
	HRESULT STDMETHODCALLTYPE Scale3f( A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT STDMETHODCALLTYPE Scale3fv( LPA3DVAL pScale );
	HRESULT STDMETHODCALLTYPE Begin( DWORD dwMode );
	HRESULT STDMETHODCALLTYPE End();
	HRESULT STDMETHODCALLTYPE Vertex3f( A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT STDMETHODCALLTYPE Vertex3fv( LPA3DVAL pVertex );
	HRESULT STDMETHODCALLTYPE Normal3f( A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT STDMETHODCALLTYPE Normal3fv( LPA3DVAL pNormal );
	HRESULT STDMETHODCALLTYPE Tag( DWORD dwTag );
	HRESULT STDMETHODCALLTYPE SetOpeningFactorf( A3DVAL flFactor );
	HRESULT STDMETHODCALLTYPE SetOpeningFactorfv( LPA3DVAL pFactor );
	HRESULT STDMETHODCALLTYPE NewMaterial( LPA3DMATERIAL* ppMaterial );
	HRESULT STDMETHODCALLTYPE BindMaterial( LPA3DMATERIAL pMaterial );
	HRESULT STDMETHODCALLTYPE NewList( LPA3DLIST* ppList );
	HRESULT STDMETHODCALLTYPE BindListener();
	HRESULT STDMETHODCALLTYPE BindSource( LPA3DSOURCE pSource );
	HRESULT STDMETHODCALLTYPE NewEnvironment( LPA3DENVIRONMENT* ppEnvironment );
	HRESULT STDMETHODCALLTYPE BindEnvironment( LPA3DENVIRONMENT pEnvironment );

	HRESULT STDMETHODCALLTYPE ListenerSetPosition3f( A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT STDMETHODCALLTYPE ListenerGetPosition3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ );
	HRESULT STDMETHODCALLTYPE ListenerSetPosition3fv( LPA3DVAL pPosition );
	HRESULT STDMETHODCALLTYPE ListenerGetPosition3fv( LPA3DVAL pPosition );
	HRESULT STDMETHODCALLTYPE ListenerSetOrientationAngles3f( A3DVAL yaw, A3DVAL pitch, A3DVAL roll );
	HRESULT STDMETHODCALLTYPE ListenerGetOrientationAngles3f( LPA3DVAL pYaw, LPA3DVAL pPitch, LPA3DVAL pRoll );
	HRESULT STDMETHODCALLTYPE ListenerSetOrientationAngles3fv( LPA3DVAL pAngles );
	HRESULT STDMETHODCALLTYPE ListenerGetOrientationAngles3fv( LPA3DVAL pAngles );
	HRESULT STDMETHODCALLTYPE ListenerSetOrientation6f( A3DVAL frontX, A3DVAL frontY, A3DVAL frontZ,
		A3DVAL topX, A3DVAL topY, A3DVAL topZ );
	HRESULT STDMETHODCALLTYPE ListenerGetOrientation6f( LPA3DVAL pFrontX, LPA3DVAL pFrontY, LPA3DVAL pFrontZ,
		LPA3DVAL pTopX, LPA3DVAL pTopY, LPA3DVAL pTopZ );
	HRESULT STDMETHODCALLTYPE ListenerSetOrientation6fv( LPA3DVAL pOrientation );
	HRESULT STDMETHODCALLTYPE ListenerGetOrientation6fv( LPA3DVAL pOrientation );
	HRESULT STDMETHODCALLTYPE ListenerSetVelocity3f( A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT STDMETHODCALLTYPE ListenerGetVelocity3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ );
	HRESULT STDMETHODCALLTYPE ListenerSetVelocity3fv( LPA3DVAL pVelocity );
	HRESULT STDMETHODCALLTYPE ListenerGetVelocity3fv( LPA3DVAL pVelocity );

	HRESULT STDMETHODCALLTYPE CreateScene( DWORD dwArgument, void** ppScene );
	HRESULT STDMETHODCALLTYPE ProcessScene( void* pScene );
	HRESULT STDMETHODCALLTYPE CreateGeometryGroup( DWORD dwType, void** ppGroup );
	HRESULT STDMETHODCALLTYPE CreateWallBuilder( DWORD dwType, void** ppBuilder );
	HRESULT STDMETHODCALLTYPE CreatePortal( DWORD dwArgument, void** ppPortal );
	HRESULT STDMETHODCALLTYPE CompatibilityOperation( DWORD dwArgument );
	HRESULT STDMETHODCALLTYPE CompatibilityOperationPair( DWORD dwFirst, DWORD dwSecond );
	HRESULT STDMETHODCALLTYPE CompatibilityOperationAlternate( DWORD dwArgument );

	HRESULT InitializeInterfaces( IDirectSound* pDirectSound, IA3d2* pA3d2, IA3d* pA3d );
	HRESULT SetupBackend( IDirectSound* pDirectSound, IA3d2* pA3d2, IA3d* pA3d );
	HRESULT ShutdownBackend();
	HRESULT InitializeGeometryState();
	HRESULT ShutdownGeometryState();
	HRESULT InitializeListenerState();
	HRESULT ShutdownListenerState();
	HRESULT InitializeSceneFactory();
	HRESULT ShutdownSceneFactory();
	void ReplayLists();
	void NextFreeChunk();
	void RenderSources();
	void RenderSourceGeometry( CA3dSource* pSource );
	void AppendGeometryPool( void* pPool );
	void GetListenerMatrices( float left[16], float center[16], float right[16] );
	void GetListenerVelocity( float velocity[3] );
	void SetSourceDopplerScale( A3DVAL flScale );
	void SetSourceDistanceModelScale( A3DVAL flScale );
	HRESULT DirectSoundGetCaps( LPDSCAPS pCaps );
	HRESULT DirectSoundCompact();
	HRESULT DirectSoundSetSpeakerConfig( DWORD dwConfig );
	HRESULT DirectSoundGetSpeakerConfig( LPDWORD pConfig );
	void RemoveSource( CA3dSource* pSource );
	double ReflectPoint( const A3DVAL* pPoint, const A3DVAL* pNormal, const A3DVAL* pPlanePoint,
		A3DVAL* pReflectedPoint, A3DVAL* pScaledPoint );

private:
	friend class CA3dList;
	friend class CA3dSource;
	friend class CA3dScene;

	A3DVAL				m_flFrameTime;
	DWORD				m_dwCompatValue1000;
	DWORD				m_dwCompatValue1007;
	BOOL				m_bCompat1011;
	BOOL				m_bCompat1010;
	BOOL				m_bLegacyDriver;
	ULONG				m_cReferences;
	BOOL				m_bBackendInitialized;
	DWORD				m_dwCooperativeLevel;
	DWORD				m_dwNumFallbackSources;
	DWORD				m_dwRequestedFeatures;
	DWORD				m_dwFeaturesAvailable;
	IDirectSound*		m_pDirectSound;
	IDirectSound3DListener*	m_pDirectSoundListener;
	IA3d*				m_pA3d;
	IA3d2*				m_pA3d2;
	IA3dDal*	m_pBackend;
	IDirectSoundBuffer*	m_pPrimaryBuffer;
	CA3dObjectList		m_Sources;
	DWORD				m_dwBackendState;

	A3DMATRIX			m_Matrix[32];
	LONG				m_nMatrix;
	BOOL				m_bListTransform;
	A3DMATRIX			m_ListMatrix;
	A3DVAL				m_flMaxReflectionDelayTime;
	A3DVAL				m_flReflectionGainScale;
	A3DVAL				m_flReflectionDelayScale;
	BOOL				m_bGeometryInitialized;
	DWORD				m_nCommandCapacity;
	CA3dObjectList		m_GeometryPools;
	LONG				m_nRemainingGeometryPools;
	CA3dCommandBuffer*	m_pCommandBuffer;
	a3d_geometry_command_t*	m_pCurrentCommand;
	a3d_primitive_t		m_Primitive;
	LONG				m_nNextVertex;
	BOOL				m_bCalculateNormal;
	DWORD				m_dwInputMode;
	DWORD				m_dwEnabledFeatures;
	DWORD				m_dwOcclusionMode;
	DWORD				m_dwReflectionMode;
	A3DVAL				m_vOcclusionGain[2];
	A3DVAL				m_flOcclusionFactor;
	A3DVECTOR			m_vSourceToListener;
	A3DVAL				m_MaterialProperties[6];
	A3DVAL				m_flClosedFraction;
	A3DVAL				m_flOutputGain;
	A3DVAL				m_flReflectionPlaneScale;
	DWORD				m_dwPolygonTestLimit;
	DWORD				m_dwPolygonTestCount;
	DWORD				m_dwReflectionUpdateModulus;
	DWORD				m_dwOcclusionUpdateModulus;
	DWORD				m_dwLastUpdateTick;
	DWORD				m_dwUpdateTickPeriod;
	BOOL				m_bCompat1009;
	A3DVAL				m_flMaxOcclusionRayFraction;
	A3DVAL				m_flMaxReflectionDistanceSquared;

	BOOL				m_bListenerInitialized;
	A3DMATRIX			m_ListenerMatrix;
	A3DVECTOR			m_vPosition;
	A3DVECTOR			m_vOrientationAngles;
	A3DVECTOR			m_vLeftEar;
	A3DVECTOR			m_vRightEar;
	A3DVECTOR			m_vVelocity;
	A3DVECTOR			m_vFront;
	A3DVECTOR			m_vTop;
	A3DVECTOR			m_vWorldListenerPosition;
	BOOL				m_bSceneFactoryInitialized;
	DWORD				m_dwGeometryState;
};
