#include "a3dclsfc.h"
#include "ia3dapi.h"
#include "A3d3.h"
#include "apimapper.h"
#include "resman.h"
#include "version_info.h"
#include "A3dOpeningBuilder.h"
#include "A3dRoom.h"
#include "A3dScene.h"
#include "A3dWallBuilder.h"
#include "A3dSource.h"
#include "listener.h"

#include <mmsystem.h>
#include <new>
#include <string.h>

LONG g_cA3dObjects = 0;
LONG g_cA3dLocks = 0;

extern "C" HRESULT __stdcall DllGetClassObject( REFCLSID clsid, REFIID iid, void** ppObject )
{
	if ( memcmp( &clsid, &CLSID_A3dApi, sizeof( CLSID ) ) &&
		memcmp( &clsid, &CLSID_A3d, sizeof( CLSID ) ) )
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	CA3dClassFactory* pFactory = new ( std::nothrow ) CA3dClassFactory;
	if ( !pFactory )
		return E_OUTOFMEMORY;

	HRESULT hr = pFactory->QueryInterface( iid, ppObject );
	if ( FAILED( hr ) )
		delete pFactory;

	return hr;
}

extern "C" HRESULT __stdcall DllCanUnloadNow()
{
	return g_cA3dObjects || g_cA3dLocks;
}

static const CLSID s_A3dDalClassId =
{
	0x442d12a1, 0x2641, 0x11d2, { 0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41 }
};

static const IID s_A3dDalInterfaceId =
{
	0xa3c62f2a, 0x2671, 0x11d2, { 0xbb, 0x48, 0x00, 0x60, 0x08, 0x2f, 0x3c, 0x00 }
};

static const IID s_A3dPlatformInterfaceId =
{
	0xcbd91fa7, 0x41c6, 0x11d2, { 0xbb, 0x48, 0x00, 0x60, 0x08, 0x2f, 0x3c, 0x00 }
};

CA3dClassFactory::CA3dClassFactory()
{
	m_cReferences = 0;
}

CA3dClassFactory::~CA3dClassFactory()
{
}

HRESULT STDMETHODCALLTYPE CA3dClassFactory::QueryInterface( REFIID iid, void** ppObject )
{
	if ( memcmp( &iid, &IID_IUnknown, sizeof( IID ) ) &&
		memcmp( &iid, &IID_IClassFactory, sizeof( IID ) ) )
	{
		*ppObject = NULL;
		return E_NOINTERFACE;
	}

	*ppObject = this;
	AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CA3dClassFactory::AddRef()
{
	return InterlockedIncrement( &m_cReferences );
}

ULONG STDMETHODCALLTYPE CA3dClassFactory::Release()
{
	if ( m_cReferences > 0 && !InterlockedDecrement( &m_cReferences ) )
	{
		delete this;
		return 0;
	}

	return m_cReferences;
}

HRESULT STDMETHODCALLTYPE CA3dClassFactory::CreateInstance( IUnknown* pOuter, REFIID iid, void** ppObject )
{
	*ppObject = NULL;

	IUnknown* pDriver = NULL;
	IDirectSound* pDirectSound = NULL;
	IA3d2* pA3d2 = NULL;
	IA3d* pA3d = NULL;
	BOOL bLegacyDriver = FALSE;

	if ( pOuter )
		return CLASS_E_NOAGGREGATION;

	if ( SUCCEEDED( CoCreateInstance( s_A3dDalClassId, NULL, CLSCTX_INPROC_SERVER,
		s_A3dDalInterfaceId, (void**)&pDriver ) ) )
	{
		if ( FAILED( pDriver->QueryInterface( IID_IDirectSound, (void**)&pDirectSound ) ) )
		{
			pDriver->Release();
			return A3DERROR_FAILED_QUERY_DIRECTSOUND;
		}

		if ( FAILED( pDriver->QueryInterface( IID_IA3d2, (void**)&pA3d2 ) ) )
		{
			pDriver->Release();
			return A3DERROR_FAILED_QUERY_A3D2;
		}

		pDriver->Release();
	}
	else
	{
		HRESULT hr = CoCreateInstance( CLSID_A3d, NULL, CLSCTX_INPROC_SERVER,
			IID_IDirectSound, (void**)&pDirectSound );
		if ( FAILED( hr ) )
			return hr;

		if ( FAILED( pDirectSound->QueryInterface( IID_IA3d2, (void**)&pA3d2 ) ) )
		{
			hr = pDirectSound->QueryInterface( IID_IA3d, (void**)&pA3d );
			if ( FAILED( hr ) )
				return hr;
		}

		if ( !A3dCheckSystemDriverVersion() )
			return E_FAIL;

		bLegacyDriver = TRUE;
	}

	CA3d* pA3dObject = new ( std::nothrow ) CA3d( bLegacyDriver );
	if ( !pA3dObject )
		return E_OUTOFMEMORY;

	if ( FAILED( pA3dObject->InitializeInterfaces( pDirectSound, pA3d2, pA3d ) ) )
	{
		delete pA3dObject;
		return A3DERROR_FAILED_INIT_A3D3;
	}

	if ( !memcmp( &iid, &IID_IA3d3, sizeof( IID ) ) )
	{
		if ( FAILED( pA3dObject->QueryInterface( iid, ppObject ) ) )
			return A3DERROR_FAILED_QUERY_A3D3;
	}
	else if ( !memcmp( &iid, &IID_IDirectSound, sizeof( IID ) ) ||
		!memcmp( &iid, &IID_IA3d, sizeof( IID ) ) ||
		!memcmp( &iid, &IID_IA3d2, sizeof( IID ) ) )
	{
		CA3dMapper* pAdapter = new ( std::nothrow ) CA3dMapper( pA3dObject );
		if ( FAILED( pAdapter->QueryInterface( iid, ppObject ) ) )
			return A3DERROR_FAILED_QUERY_DIRECTSOUND;
	}
	else
	{
		if ( memcmp( &iid, &s_A3dPlatformInterfaceId, sizeof( IID ) ) )
			return E_FAIL;

		ResMan* pPlatformAudio = new ( std::nothrow ) ResMan;
		if ( FAILED( pPlatformAudio->QueryInterface( iid, ppObject ) ) )
			return E_FAIL;
	}

	InterlockedIncrement( &g_cA3dObjects );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dClassFactory::LockServer( BOOL bLock )
{
	if ( bLock )
		InterlockedIncrement( &g_cA3dLocks );
	else
		InterlockedDecrement( &g_cA3dLocks );

	return S_OK;
}

static const IID s_SceneFactoryInterfaceId =
{
	0x103e7222, 0x0113, 0x11d2, { 0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41 }
};

static const IID s_DirectSoundInterfaceId =
{
	0x62507921, 0x3ea6, 0x11d2, { 0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41 }
};

static const IID s_CompatibilityInterfaceId =
{
	0xbfe2be81, 0x4cb7, 0x11d2, { 0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41 }
};

CA3d::CA3d( BOOL bLegacyDriver )
{
	m_Sources.m_pFirst = NULL;
	m_Sources.m_pLast = NULL;
	m_Sources.m_pCurrent = NULL;
	m_Sources.m_nCount = 0;
	m_GeometryPools.m_pFirst = NULL;
	m_GeometryPools.m_pLast = NULL;
	m_GeometryPools.m_pCurrent = NULL;
	m_GeometryPools.m_nCount = 0;
	m_nMatrix = 0;
	m_dwEnabledFeatures = 0;
	m_dwOcclusionMode = A3D_FASTEST;
	m_dwReflectionMode = A3D_FASTEST;
	m_dwCompatValue1000 = 0;
	m_dwCompatValue1007 = 0;
	m_bCompat1009 = FALSE;
	m_flOutputGain = 1.0f;
	m_flMaxReflectionDelayTime = 0.3f;
	m_flReflectionGainScale = 1.0f;
	m_flReflectionDelayScale = 1.0f;
	m_flMaxOcclusionRayFraction = 1.0f;
	m_flMaxReflectionDistanceSquared = 1.0e38f;
	m_bLegacyDriver = bLegacyDriver;

	m_bListTransform = FALSE;
	m_flReflectionPlaneScale = A3D_DEFAULT_REFLECTION_PLANE_SCALE;
	m_dwPolygonTestLimit = A3D_DEFAULT_POLYGON_TEST_LIMIT;
	m_dwPolygonTestCount = 0;
	m_dwReflectionUpdateModulus = A3D_DEFAULT_REFLECTION_UPDATE_MODULUS;
	m_dwOcclusionUpdateModulus = A3D_DEFAULT_OCCLUSION_UPDATE_MODULUS;
	m_dwLastUpdateTick = 0;
	m_dwUpdateTickPeriod = A3D_DEFAULT_UPDATE_TICK_PERIOD;
}

CA3d::~CA3d()
{
	ShutdownBackend();
	if ( m_bListenerInitialized )
		ShutdownListenerState();
	if ( m_bGeometryInitialized )
		ShutdownGeometryState();
	if ( m_bSceneFactoryInitialized )
		ShutdownSceneFactory();

	m_GeometryPools.Clear();
	m_Sources.Clear();
}

HRESULT CA3d::InitializeInterfaces( IDirectSound* pDirectSound, IA3d2* pA3d2, IA3d* pA3d )
{
	m_cReferences = 0;
	SetupBackend( pDirectSound, pA3d2, pA3d );
	m_bGeometryInitialized = FALSE;
	m_bListenerInitialized = FALSE;
	m_bSceneFactoryInitialized = FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::QueryInterface( REFIID iid, void** ppObject )
{
	if ( !ppObject )
		return E_INVALIDARG;

	*ppObject = NULL;
	if ( !memcmp( &iid, &IID_IUnknown, sizeof( IID ) ) ||
		!memcmp( &iid, &IID_IA3d, sizeof( IID ) ) ||
		!memcmp( &iid, &IID_IA3d2, sizeof( IID ) ) ||
		!memcmp( &iid, &IID_IA3d3, sizeof( IID ) ) )
	{
		*ppObject = (IA3dTarget*)this;
	}
	else if ( !memcmp( &iid, &IID_IA3dListener, sizeof( IID ) ) )
	{
		if ( !m_bListenerInitialized && FAILED( InitializeListenerState() ) )
			return A3DERROR_FAILED_INIT_QUERIED_INTERFACE;
		*ppObject = (IA3dListenerTarget*)this;
	}
	else if ( !memcmp( &iid, &IID_IA3dGeom, sizeof( IID ) ) )
	{
		if ( !m_bGeometryInitialized && FAILED( InitializeGeometryState() ) )
			return A3DERROR_FAILED_INIT_QUERIED_INTERFACE;
		*ppObject = (IA3dGeomTarget*)this;
	}
	else if ( !memcmp( &iid, &s_SceneFactoryInterfaceId, sizeof( IID ) ) )
	{
		if ( !m_bSceneFactoryInitialized && FAILED( InitializeSceneFactory() ) )
			return A3DERROR_FAILED_INIT_QUERIED_INTERFACE;
		*ppObject = (IA3dSceneFactoryTarget*)this;
	}
	else if ( !memcmp( &iid, &s_DirectSoundInterfaceId, sizeof( IID ) ) )
	{
		if ( m_pBackend )
			m_pBackend->GetDS( (IDirectSound**)ppObject );
		else
			*ppObject = m_pDirectSound;
	}
	else if ( !memcmp( &iid, &s_CompatibilityInterfaceId, sizeof( IID ) ) )
	{
		*ppObject = (IA3dCompatibilityTarget*)this;
	}

	if ( !*ppObject )
		return E_NOINTERFACE;

	( (IUnknown*)*ppObject )->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CA3d::AddRef()
{
	return ++m_cReferences;
}

ULONG STDMETHODCALLTYPE CA3d::Release()
{
	ULONG cReferences = --m_cReferences;
	if ( !cReferences )
		delete this;
	return cReferences;
}

HRESULT STDMETHODCALLTYPE CA3d::SetOcclusionMode( DWORD dwMode )
{
	return SetOcclusionModeInternal( dwMode );
}

HRESULT STDMETHODCALLTYPE CA3d::SetReflectionMode( DWORD dwMode )
{
	return SetReflectionModeInternal( dwMode );
}

HRESULT STDMETHODCALLTYPE CA3d::SetOcclusionModeInternal( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3d::SetReflectionModeInternal( DWORD )
{
	return S_OK;
}

HRESULT CA3d::InitializeSceneFactory()
{
	m_bSceneFactoryInitialized = TRUE;
	return S_OK;
}

HRESULT CA3d::ShutdownSceneFactory()
{
	m_bSceneFactoryInitialized = FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::CompatibilityOperation( DWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::CompatibilityOperationPair( DWORD, DWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::CompatibilityOperationAlternate( DWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::Compat( DWORD dwOperation, DWORD dwValue )
{
	switch ( dwOperation )
	{
	case A3D_COMPAT_LEGACY_STATE:
		m_dwCompatValue1000 = dwValue;
		break;
	case A3D_COMPAT_LIST_TRANSFORM:
		if ( dwValue == 1 )
		{
			ReplayLists();
			m_bListTransform = TRUE;
		}
		else
		{
			m_bListTransform = FALSE;
		}
		break;
	case A3D_COMPAT_REFLECTION_PLANE_SCALE:
		memcpy( &m_flReflectionPlaneScale, &dwValue, sizeof( dwValue ) );
		break;
	case A3D_COMPAT_POLYGON_TEST_LIMIT:
		m_dwPolygonTestLimit = dwValue;
		break;
	case A3D_COMPAT_REFLECTION_UPDATE_MODULUS:
		m_dwReflectionUpdateModulus = dwValue;
		break;
	case A3D_COMPAT_OCCLUSION_UPDATE_MODULUS:
		m_dwOcclusionUpdateModulus = dwValue;
		break;
	case A3D_COMPAT_UPDATE_TICK_PERIOD:
		m_dwUpdateTickPeriod = dwValue;
		break;
	case A3D_COMPAT_LEGACY_STATE_ALTERNATE:
		m_dwCompatValue1007 = dwValue;
		break;
	case A3D_COMPAT_LEGACY_NO_OPERATION:
		break;
	case A3D_COMPAT_LEGACY_PRIMITIVE_STATE:
		m_bCompat1009 = (BOOL)dwValue;
		break;
	case A3D_COMPAT_OCCLUSION_RAY_FRACTION:
		memcpy( &m_flMaxOcclusionRayFraction, &dwValue, sizeof( dwValue ) );
		m_bCompat1010 = TRUE;
		break;
	case A3D_COMPAT_LEGACY_MARKER:
		if ( dwValue == A3D_COMPAT_LEGACY_MARKER_VALUE )
			m_bCompat1011 = TRUE;
		break;
	case A3D_COMPAT_REFLECTION_DISTANCE_SQUARED:
		memcpy( &m_flMaxReflectionDistanceSquared, &dwValue, sizeof( dwValue ) );
		break;
	default:
		return E_INVALIDARG;
	}

	return S_OK;
}

void CA3d::ReplayLists()
{
	m_Sources.m_pCurrent = m_Sources.m_pFirst;
	for ( int nSources = m_Sources.m_nCount; nSources > 0; nSources-- )
		( (IA3dGeomTarget*)this )->BindSource( (LPA3DSOURCE)m_Sources.Next() );

	memcpy( m_ListMatrix, m_Matrix[m_nMatrix], sizeof( m_ListMatrix ) );
}

HRESULT STDMETHODCALLTYPE CA3d::GetMaxReflectionDelayTime( LPA3DVAL pTime )
{
	if ( !pTime )
		return E_INVALIDARG;

	*pTime = m_flMaxReflectionDelayTime;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::SetCoordinateSystem( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3d::GetOcclusionMode( LPDWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3d::GetReflectionMode( LPDWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3d::NewEnvironment( LPA3DENVIRONMENT* )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3d::BindEnvironment( LPA3DENVIRONMENT )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3d::CreateScene( DWORD dwArgument, void** ppScene )
{
	CA3dScene* pScene = new( std::nothrow ) CA3dScene( dwArgument, this );
	if ( !pScene )
	{
		*ppScene = NULL;
		return A3DERROR_MEMORY_ALLOCATION;
	}

	pScene->AddRef();
	*ppScene = pScene;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::ProcessScene( void* pScene )
{
	if ( !pScene )
		return E_INVALIDARG;

	return ( (CA3dScene*)pScene )->Render();
}

HRESULT STDMETHODCALLTYPE CA3d::CreateGeometryGroup( DWORD dwType, void** ppGroup )
{
	if ( !ppGroup )
		return E_INVALIDARG;

	*ppGroup = NULL;
	if ( dwType == 2 )
		return A3DERROR_2D_GEOMETRY_UNIMPLEMENTED;

	CA3dRoomBuilder* pGroup = new( std::nothrow ) CA3dRoomBuilder( dwType );
	if ( !pGroup )
	{
		*ppGroup = NULL;
		return A3DERROR_MEMORY_ALLOCATION;
	}

	pGroup->AddRef();
	*ppGroup = pGroup;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::CreateWallBuilder( DWORD dwType, void** ppBuilder )
{
	if ( dwType == 2 )
		return A3DERROR_2D_GEOMETRY_UNIMPLEMENTED;
	if ( !ppBuilder )
		return E_INVALIDARG;

	CA3dWallBuilder* pBuilder = new( std::nothrow ) CA3dWallBuilder( (int)dwType );
	if ( !pBuilder )
	{
		*ppBuilder = NULL;
		return A3DERROR_MEMORY_ALLOCATION;
	}

	pBuilder->AddRef();
	*ppBuilder = pBuilder;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::CreatePortal( DWORD dwArgument, void** ppPortal )
{
	CA3dOpeningBuilder* pPortal = new( std::nothrow ) CA3dOpeningBuilder( (int)dwArgument );
	if ( !pPortal )
	{
		*ppPortal = NULL;
		return A3DERROR_MEMORY_ALLOCATION;
	}

	pPortal->AddRef();
	*ppPortal = pPortal;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::NewSource( DWORD dwFlags, LPA3DSOURCE* ppSource )
{
	if ( dwFlags > A3DSOURCE_INITIAL_RENDERMODE_NATIVE )
		return E_INVALIDARG;
	if ( !ppSource )
		return E_INVALIDARG;

	*ppSource = NULL;
	if ( !m_pPrimaryBuffer )
	{
		DSBUFFERDESC1 desc = {};
		desc.dwSize = sizeof( desc );
		desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D;
		if ( FAILED( m_pDirectSound->CreateSoundBuffer( (LPCDSBUFFERDESC)&desc, &m_pPrimaryBuffer, NULL ) ) )
			return A3DERROR_FAILED_CREATE_PRIMARY_BUFFER;

		m_pPrimaryBuffer->AddRef();
		if ( m_bLegacyDriver && SUCCEEDED( m_pPrimaryBuffer->QueryInterface( IID_IDirectSound3DListener,
			(LPVOID*)&m_pDirectSoundListener ) ) )
		{
			m_pDirectSoundListener->SetRolloffFactor( 0.0f, DS3D_IMMEDIATE );
		}
	}

	CA3dSource* pSource = new ( std::nothrow ) CA3dSource( m_pDirectSound, this, dwFlags );
	if ( !pSource )
		return A3DERROR_MEMORY_ALLOCATION;

	m_Sources.Append( pSource );
	*ppSource = (LPA3DSOURCE)pSource;
	pSource->AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::DuplicateSource( LPA3DSOURCE pSource, LPA3DSOURCE* ppDuplicate )
{
	if ( !pSource )
		return E_INVALIDARG;
	if ( !ppDuplicate )
		return E_INVALIDARG;

	CA3dListNode* pNode = m_Sources.m_pFirst;
	BOOL bFound = FALSE;
	for ( int i = 0; i < m_Sources.m_nCount; i++ )
	{
		if ( pNode->m_pObject == pSource )
		{
			m_Sources.m_pCurrent = pNode;
			bFound = TRUE;
			break;
		}
		pNode = pNode->m_pNext;
	}
	if ( !bFound )
		return A3DERROR_NOT_VALID_SOURCE;

	CA3dSource* pOriginal = (CA3dSource*)pSource;
	if ( !pOriginal->m_pBuffer )
		return A3DERROR_NO_WAVE_DATA;

	HRESULT status = S_OK;
	CA3dSource* pDuplicate = new ( std::nothrow ) CA3dSource( *pOriginal, &status );
	if ( !pDuplicate || FAILED( status ) )
		return A3DERROR_FAILED_DUPLICATION;

	*ppDuplicate = (LPA3DSOURCE)pDuplicate;
	m_Sources.Append( pDuplicate );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dPrivateUnknown::QueryInterface( REFIID, void** ppObject )
{
	return ppObject ? E_NOINTERFACE : E_INVALIDARG;
}

ULONG STDMETHODCALLTYPE CA3dRoomBuilder::AddRef()
{
	return ++m_cReferences;
}

ULONG STDMETHODCALLTYPE CA3dRoomBuilder::Release()
{
	ULONG cReferences = --m_cReferences;
	if ( cReferences )
		return cReferences;

	delete this;
	return 0;
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::GetWall( INT nIndex, IUnknown** ppWall )
{
	INT nWall = (INT)( (DWORD)nIndex - 1 );
	CA3dListNode* pNode = NULL;
	if ( nWall >= 0 && nWall < m_Walls.m_nCount )
	{
		pNode = m_Walls.m_pFirst;
		for ( INT i = 0; i < nWall; i++ )
			pNode = pNode->m_pNext;
	}

	IUnknown* pWall = NULL;
	if ( pNode )
	{
		m_Walls.m_pCurrent = pNode;
		pWall = (IUnknown*)pNode->m_pObject;
	}
	if ( !pWall )
		return A3DERROR_INVALID_WALL_INDEX;

	pWall->AddRef();
	*ppWall = pWall;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::RemoveWall( INT nIndex )
{
	INT nWall = (INT)( (DWORD)nIndex - 1 );
	CA3dListNode* pNode = NULL;
	if ( nWall >= 0 && nWall < m_Walls.m_nCount )
	{
		pNode = m_Walls.m_pFirst;
		for ( INT i = 0; i < nWall; i++ )
			pNode = pNode->m_pNext;
	}

	IUnknown* pWall = NULL;
	if ( pNode )
	{
		m_Walls.m_pCurrent = pNode;
		pWall = (IUnknown*)pNode->m_pObject;
	}
	if ( !pWall )
		return A3DERROR_INVALID_WALL_INDEX;

	nWall = 0;
	pNode = m_Walls.m_pFirst;
	for ( INT i = 0; i < m_Walls.m_nCount; i++ )
	{
		if ( pNode->m_pObject == pWall )
		{
			m_Walls.m_pCurrent = pNode;
			nWall = i + 1;
			break;
		}
		pNode = pNode->m_pNext;
	}

	if ( nWall > 0 )
	{
		nWall--;
		pNode = NULL;
		if ( nWall >= 0 && nWall < m_Walls.m_nCount )
		{
			pNode = m_Walls.m_pFirst;
			for ( INT i = 0; i < nWall; i++ )
				pNode = pNode->m_pNext;
		}

		if ( pNode )
		{
			if ( m_Walls.m_pFirst == pNode )
				m_Walls.m_pFirst = pNode->m_pNext;
			if ( m_Walls.m_pLast == pNode )
				m_Walls.m_pLast = pNode->m_pPrevious;
			if ( m_Walls.m_pCurrent == pNode )
			{
				m_Walls.m_pCurrent = pNode->m_pPrevious;
				if ( !m_Walls.m_pCurrent )
					m_Walls.m_pCurrent = m_Walls.m_pFirst;
			}
			if ( pNode->m_pNext )
				pNode->m_pNext->m_pPrevious = pNode->m_pPrevious;
			if ( pNode->m_pPrevious )
				pNode->m_pPrevious->m_pNext = pNode->m_pNext;
			delete pNode;
		}
		m_Walls.m_nCount--;
	}

	pWall->Release();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerSetPosition3f( A3DVAL x, A3DVAL y, A3DVAL z )
{
	return SetPosition3f( x, y, z );
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerGetPosition3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ )
{
	*pX = m_vPosition[0];
	*pY = m_vPosition[1];
	*pZ = m_vPosition[2];
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerSetPosition3fv( LPA3DVAL pPosition )
{
	return SetPosition3fv( pPosition );
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerGetPosition3fv( LPA3DVAL pPosition )
{
	pPosition[0] = m_vPosition[0];
	pPosition[1] = m_vPosition[1];
	pPosition[2] = m_vPosition[2];
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerSetOrientationAngles3f( A3DVAL yaw, A3DVAL pitch, A3DVAL roll )
{
	return SetOrientationAngles3f( yaw, pitch, roll );
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerGetOrientationAngles3f( LPA3DVAL pYaw, LPA3DVAL pPitch, LPA3DVAL pRoll )
{
	*pYaw = m_vOrientationAngles[0];
	*pPitch = m_vOrientationAngles[1];
	*pRoll = m_vOrientationAngles[2];
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerSetOrientationAngles3fv( LPA3DVAL pAngles )
{
	return SetOrientationAngles3fv( pAngles );
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerGetOrientationAngles3fv( LPA3DVAL pAngles )
{
	pAngles[0] = m_vOrientationAngles[0];
	pAngles[1] = m_vOrientationAngles[1];
	pAngles[2] = m_vOrientationAngles[2];
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerSetOrientation6f( A3DVAL frontX, A3DVAL frontY, A3DVAL frontZ,
		A3DVAL topX, A3DVAL topY, A3DVAL topZ )
{
	return SetOrientation6f( frontX, frontY, frontZ, topX, topY, topZ );
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerGetOrientation6f( LPA3DVAL pFrontX, LPA3DVAL pFrontY, LPA3DVAL pFrontZ,
		LPA3DVAL pTopX, LPA3DVAL pTopY, LPA3DVAL pTopZ )
{
	*pFrontX = m_vFront[0];
	*pFrontY = m_vFront[1];
	*pFrontZ = m_vFront[2];
	*pTopX = m_vTop[0];
	*pTopY = m_vTop[1];
	*pTopZ = m_vTop[2];
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerSetOrientation6fv( LPA3DVAL pOrientation )
{
	return SetOrientation6fv( pOrientation );
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerGetOrientation6fv( LPA3DVAL pOrientation )
{
	pOrientation[0] = m_vFront[0];
	pOrientation[1] = m_vFront[1];
	pOrientation[2] = m_vFront[2];
	pOrientation[3] = m_vTop[0];
	pOrientation[4] = m_vTop[1];
	pOrientation[5] = m_vTop[2];
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerSetVelocity3f( A3DVAL x, A3DVAL y, A3DVAL z )
{
	return SetVelocity3f( x, y, z );
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerGetVelocity3f( LPA3DVAL pX, LPA3DVAL pY, LPA3DVAL pZ )
{
	*pX = m_vVelocity[0];
	*pY = m_vVelocity[1];
	*pZ = m_vVelocity[2];
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerSetVelocity3fv( LPA3DVAL pVelocity )
{
	return SetVelocity3fv( pVelocity );
}

HRESULT STDMETHODCALLTYPE CA3d::ListenerGetVelocity3fv( LPA3DVAL pVelocity )
{
	pVelocity[0] = m_vVelocity[0];
	pVelocity[1] = m_vVelocity[1];
	pVelocity[2] = m_vVelocity[2];
	return S_OK;
}

#define VERSION_PRODUCT_NAME		0
#define VERSION_COMPANY_NAME		5

static const char* const s_pszVersionKeys[] =
{
	"ProductName",
	"ProductVersion",
	"OriginalFilename",
	"FileDescription",
	"FileVersion",
	"CompanyName",
	"LegalCopyright",
	"LegalTrademarks",
	"InternalName",
	"PrivateBuild",
	"SpecialBuild",
	"Comments"
};

typedef struct version_string_s
{
	const char*	name;
	const char*	value;
} version_string_t;

int A3dCheckFileVersion( LPCSTR pszFilename, DWORD nBufferSize, LPVOID pBuffer )
{
	char language[12];
	char query[80];
	UINT nValueLength;
	version_string_t strings[ARRAYSIZE( s_pszVersionKeys )];
	const char* pszValue = pszFilename;

	if ( !GetFileVersionInfoA( pszFilename, 0, nBufferSize, pBuffer ) )
		return 0;

	wsprintfA( language, "%04X", GetUserDefaultLangID() );
	memcpy( language + strlen( language ), "04B0", sizeof( "04B0" ) );

	for ( unsigned int i = 0; i < ARRAYSIZE( s_pszVersionKeys ); i++ )
	{
		lstrcpyA( query, "\\StringFileInfo\\" );
		lstrcatA( query, language );
		lstrcatA( query, "\\" );
		lstrcatA( query, s_pszVersionKeys[i] );

		strings[i].name = s_pszVersionKeys[i];
		if ( nBufferSize && VerQueryValueA( pBuffer, query, (LPVOID*)&pszValue, &nValueLength ) )
			strings[i].value = pszValue;
		else
			strings[i].value = "";
	}

	const char* pszCompanyName = strings[VERSION_COMPANY_NAME].value;
	if ( !_stricmp( pszCompanyName, "Aureal Semiconductor" ) )
		return 1;
	if ( !_stricmp( pszCompanyName, "Aureal Semiconductor Inc." ) )
		return 1;

	const char* pszProductName = strings[VERSION_PRODUCT_NAME].value;
	if ( !_stricmp( pszProductName, "SM Emulation" ) )
		return 1;

	if ( strlen( pszProductName ) < 1 && strlen( pszCompanyName ) < 1 )
		return -1;

	return 0;
}

int A3dCheckSystemDriverVersion()
{
	char data[1024];
	char filename[MAX_PATH];

	if ( !GetSystemDirectoryA( filename, sizeof( filename ) ) )
		return -1;

	const char* pszLibraryName = "A3D.DLL";
	if ( filename[strlen( filename ) - 1] != '\\' )
		pszLibraryName = "\\A3D.DLL";
	size_t nLibraryNameSize = strlen( pszLibraryName ) + 1;
	memcpy( filename + strlen( filename ), pszLibraryName, nLibraryNameSize );

	return A3dCheckFileVersion( filename, sizeof( data ), data );
}
