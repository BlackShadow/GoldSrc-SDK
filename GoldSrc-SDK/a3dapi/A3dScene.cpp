#include "A3dScene.h"
#include "A3d3.h"
#include "ChunkPage.h"
#include "A3dGeom.h"
#include "A3dRoom.h"
#include "A3dWallBuilder.h"
#include "A3dList.h"
#include "A3dOpeningBuilder.h"
#include "resman.h"
#include "A3dSource.h"

#include <math.h>
#include <new>
#include <string.h>

static const IID sceneIid =
{
	0x103e7224, 0x0113, 0x11d2, { 0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41 }
};

static DWORD g_nSceneNames;

CA3dScene::CA3dScene( DWORD dwContext, CA3d* pA3d ) :
	m_Rooms(),
	m_Walls(),
	m_NeighborRooms(),
	m_UnpairedRooms(),
	m_ZeroContextRooms(),
	m_ContextRooms()
{
	try
	{
		m_dwContext = dwContext;
		m_cReferences = 0;
		m_pList = NULL;
		m_pA3d = pA3d;
		m_pA3d->AddRef();
		m_pCurrentRoom = NULL;
		m_bValid = FALSE;
		m_bRestartRoom = FALSE;
		CA3dName::Format( "Scene", ++g_nSceneNames );
	}
	catch ( ... )
	{
		m_ContextRooms.Clear();
		m_ZeroContextRooms.Clear();
		m_UnpairedRooms.Clear();
		m_NeighborRooms.Clear();
		m_Walls.Clear();
		m_Rooms.Clear();
		throw;
	}
}

CA3dScene::~CA3dScene()
{
	Clear();
	m_pA3d->Release();
	m_ContextRooms.Clear();
	m_ZeroContextRooms.Clear();
	m_UnpairedRooms.Clear();
	m_NeighborRooms.Clear();
	m_Walls.Clear();
	m_Rooms.Clear();
}

HRESULT STDMETHODCALLTYPE CA3dScene::QueryInterface( REFIID iid, void** ppObject )
{
	if ( !ppObject )
		return E_INVALIDARG;

	if ( !memcmp( &iid, &IID_IUnknown, sizeof( IID ) ) || !memcmp( &iid, &sceneIid, sizeof( IID ) ) )
		*ppObject = this;

	if ( !*ppObject )
		return E_NOINTERFACE;

	( (IUnknown*)*ppObject )->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CA3dScene::AddRef()
{
	return ++m_cReferences;
}

ULONG STDMETHODCALLTYPE CA3dScene::Release()
{
	ULONG cReferences = --m_cReferences;
	if ( !cReferences )
		delete this;

	return cReferences;
}

INT STDMETHODCALLTYPE CA3dScene::NewRoom( DWORD dwContext, CA3dRoomBuilder* pBuilder,
	const A3DVAL* pPosition, const A3DVAL* pForward, const A3DVAL* pUp )
{
	if ( dwContext )
		return A3DERROR_DYNAMIC_OBJ_UNSUPPORTED;
	if ( !pBuilder || FAILED( pBuilder->Build() ) )
		return A3DERROR_INVALID_ROOM;

	A3DVECTOR position = { 0, 0, 0, 1 };
	A3DVECTOR forward = { 0, 0, 1, 0 };
	A3DVECTOR up = { 0, 1, 0, 0 };
	if ( !pPosition )
		pPosition = position;
	if ( !pForward )
		pForward = forward;
	if ( !pUp )
		pUp = up;

	if ( fabs( ( (double)pForward[1] * pUp[1] + (double)pForward[2] * pUp[2] ) +
		(double)pForward[0] * pUp[0] ) > A3D_EPSILON ||
		!( ( (double)pForward[1] * pForward[1] + (double)pForward[2] * pForward[2] ) +
		(double)pForward[0] * pForward[0] >= A3D_EPSILON_SQUARED ) ||
		!( ( (double)pUp[1] * pUp[1] + (double)pUp[2] * pUp[2] ) +
		(double)pUp[0] * pUp[0] >= A3D_EPSILON_SQUARED ) )
		return A3DERROR_DIR_AND_UP_VECTORS_NOT_PERPENDICULAR;

	CA3dRoom* pRoom = new( std::nothrow ) CA3dRoom( pBuilder, 0, pPosition, pForward, pUp, this );
	pRoom->AddRef();
	m_Rooms.Append( pRoom );
	m_bValid = FALSE;
	return m_Rooms.m_nCount;
}

INT STDMETHODCALLTYPE CA3dScene::NewWall( DWORD dwContext, CA3dWallBuilder* pBuilder,
	const A3DVAL* pPosition, const A3DVAL* pForward, const A3DVAL* pUp )
{
	if ( dwContext )
		return A3DERROR_DYNAMIC_OBJ_UNSUPPORTED;
	if ( FAILED( pBuilder->Validate() ) )
		return A3DERROR_INVALID_WALL;

	A3DVECTOR position = { 0, 0, 0, 1 };
	A3DVECTOR forward = { 0, 0, 1, 0 };
	A3DVECTOR up = { 0, 1, 0, 0 };
	if ( !pPosition )
		pPosition = position;
	if ( !pForward )
		pForward = forward;
	if ( !pUp )
		pUp = up;

	if ( fabs( ( (double)pForward[1] * pUp[1] + (double)pForward[2] * pUp[2] ) +
		(double)pForward[0] * pUp[0] ) > A3D_EPSILON ||
		!( ( (double)pForward[1] * pForward[1] + (double)pForward[2] * pForward[2] ) +
		(double)pForward[0] * pForward[0] >= A3D_EPSILON_SQUARED ) ||
		!( ( (double)pUp[1] * pUp[1] + (double)pUp[2] * pUp[2] ) +
		(double)pUp[0] * pUp[0] >= A3D_EPSILON_SQUARED ) )
		return A3DERROR_DIR_AND_UP_VECTORS_NOT_PERPENDICULAR;

	CA3dGeometryFace* pWall = new( std::nothrow ) CA3dGeometryFace( pBuilder, 0, pPosition, pForward, pUp );
	pWall->AddRef();
	m_Walls.Append( pWall );
	return m_Walls.m_nCount;
}

HRESULT STDMETHODCALLTYPE CA3dScene::RemoveRoom( INT nRoom )
{
	if ( nRoom <= 0 )
		return A3DERROR_INVALID_ROOM_INDEX;

	INT nOffset = nRoom - 1;
	CA3dListNode* pNode = NULL;
	if ( nOffset >= 0 && nOffset < m_Rooms.m_nCount )
	{
		pNode = m_Rooms.m_pFirst;
		for ( INT i = nOffset; i > 0; i-- )
			pNode = pNode->m_pNext;
	}

	CA3dRoom* pRoom = NULL;
	if ( pNode )
	{
		m_Rooms.m_pCurrent = pNode;
		pRoom = (CA3dRoom*)pNode->m_pObject;
	}

	if ( !pRoom )
		return A3DERROR_INVALID_ROOM_INDEX;

	pRoom->Release();
	pNode = NULL;
	if ( nOffset >= 0 && nOffset < m_Rooms.m_nCount )
	{
		pNode = m_Rooms.m_pFirst;
		for ( INT i = nOffset; i > 0; i-- )
			pNode = pNode->m_pNext;
	}

	if ( pNode )
	{
		if ( m_Rooms.m_pFirst == pNode )
			m_Rooms.m_pFirst = pNode->m_pNext;
		if ( m_Rooms.m_pLast == pNode )
			m_Rooms.m_pLast = pNode->m_pPrevious;
		if ( m_Rooms.m_pCurrent == pNode )
		{
			m_Rooms.m_pCurrent = pNode->m_pPrevious;
			if ( !m_Rooms.m_pCurrent )
				m_Rooms.m_pCurrent = m_Rooms.m_pFirst;
		}
		if ( pNode->m_pNext )
			pNode->m_pNext->m_pPrevious = pNode->m_pPrevious;
		if ( pNode->m_pPrevious )
			pNode->m_pPrevious->m_pNext = pNode->m_pNext;
		delete pNode;
	}

	m_Rooms.m_nCount--;
	m_bValid = FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::RemoveWall( INT nWall )
{
	if ( nWall <= 0 )
		return A3DERROR_INVALID_WALL_INDEX;

	INT nOffset = nWall - 1;
	CA3dListNode* pNode = NULL;
	if ( nOffset >= 0 && nOffset < m_Walls.m_nCount )
	{
		pNode = m_Walls.m_pFirst;
		for ( INT i = nOffset; i > 0; i-- )
			pNode = pNode->m_pNext;
	}

	CA3dGeometryFace* pWall = NULL;
	if ( pNode )
	{
		m_Walls.m_pCurrent = pNode;
		pWall = (CA3dGeometryFace*)pNode->m_pObject;
	}

	if ( !pWall )
		return A3DERROR_INVALID_WALL_INDEX;

	pNode = NULL;
	if ( nOffset >= 0 && nOffset < m_Walls.m_nCount )
	{
		pNode = m_Walls.m_pFirst;
		for ( INT i = nOffset; i > 0; i-- )
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
	pWall->Release();
	m_bValid = FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::UnsupportedOperationA( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

BOOL STDMETHODCALLTYPE CA3dScene::CompatibilityCheck()
{
	return TRUE;
}

HRESULT STDMETHODCALLTYPE CA3dScene::GetRoom( INT nRoom, CA3dRoom** ppRoom )
{
	if ( nRoom < 0 )
		return A3DERROR_INVALID_ROOM_INDEX;
	if ( !ppRoom )
		return E_INVALIDARG;

	*ppRoom = NULL;
	INT nOffset = nRoom - 1;
	if ( m_Rooms.m_nCount <= 0 )
		return A3DERROR_NO_ROOMS_IN_SCENE;

	CA3dListNode* pNode = NULL;
	if ( nOffset >= 0 && nOffset < m_Rooms.m_nCount )
	{
		pNode = m_Rooms.m_pFirst;
		while ( nOffset > 0 )
		{
			pNode = pNode->m_pNext;
			nOffset--;
		}
	}

	CA3dRoom* pRoom = NULL;
	if ( pNode )
	{
		m_Rooms.m_pCurrent = pNode;
		pRoom = (CA3dRoom*)pNode->m_pObject;
	}

	if ( !pRoom )
		return A3DERROR_INVALID_ROOM_INDEX;

	*ppRoom = pRoom;
	pRoom->AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::GetWall( INT nRoom, INT nWall, IUnknown** ppWall )
{
	if ( nRoom < 0 )
		return A3DERROR_INVALID_ROOM_INDEX;
	if ( nWall <= 0 )
		return A3DERROR_INVALID_WALL_INDEX;
	if ( !ppWall )
		return E_INVALIDARG;

	*ppWall = NULL;
	IUnknown* pWall = NULL;
	BOOL bAddReference;
	CA3dListNode* pNode = NULL;
	if ( !nRoom )
	{
		if ( m_Walls.m_nCount <= 0 )
			return A3DERROR_INVALID_WALL_INDEX;

		INT nOffset = nWall - 1;
		if ( nOffset >= 0 && nOffset < m_Walls.m_nCount )
		{
			pNode = m_Walls.m_pFirst;
			while ( nOffset > 0 )
			{
				pNode = pNode->m_pNext;
				nOffset--;
			}
		}
		if ( pNode )
		{
			m_Walls.m_pCurrent = pNode;
			pWall = (IUnknown*)pNode->m_pObject;
		}
		bAddReference = TRUE;
	}
	else
	{
		if ( m_Rooms.m_nCount <= 0 )
			return A3DERROR_INVALID_ROOM_INDEX;

		INT nOffset = nRoom - 1;
		if ( nOffset >= 0 && nOffset < m_Rooms.m_nCount )
		{
			pNode = m_Rooms.m_pFirst;
			while ( nOffset > 0 )
			{
				pNode = pNode->m_pNext;
				nOffset--;
			}
		}

		CA3dRoom* pRoom = NULL;
		if ( pNode )
		{
			m_Rooms.m_pCurrent = pNode;
			pRoom = (CA3dRoom*)pNode->m_pObject;
		}
		if ( !pRoom )
			return A3DERROR_INVALID_ROOM_INDEX;
		if ( FAILED( pRoom->GetWall( nWall, &pWall ) ) )
			return A3DERROR_INVALID_WALL_INDEX;

		bAddReference = FALSE;
	}

	if ( !pWall )
		return A3DERROR_INVALID_WALL_INDEX;

	*ppWall = pWall;
	if ( bAddReference )
		pWall->AddRef();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::GetOpening( INT nRoom, INT nWall, INT nOpening, IUnknown** ppOpening )
{
	if ( !ppOpening )
		return E_INVALIDARG;

	IUnknown* pWall;
	HRESULT hr = GetWall( nRoom, nWall, &pWall );
	if ( FAILED( hr ) )
		return hr;

	hr = ( (CA3dGeometryFace*)pWall )->GetOpening( nOpening, ppOpening );
	pWall->Release();
	if ( FAILED( hr ) )
		return hr;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::FindRoom( A3DVAL x, A3DVAL y, A3DVAL z, CA3dRoom** ppRoom )
{
	if ( !m_bValid )
		return A3DERROR_SCENE_INVALID;
	if ( !ppRoom )
		return E_INVALIDARG;

	A3DVECTOR point = { x, y, z };
	CA3dRoom* pRoom = NULL;
	m_ZeroContextRooms.m_pCurrent = m_ZeroContextRooms.m_pFirst;
	for ( INT i = m_ZeroContextRooms.m_nCount; i > 0; i-- )
	{
		pRoom = (CA3dRoom*)m_ZeroContextRooms.Next();
		if ( pRoom->ContainsPoint( point ) )
			break;
		pRoom = NULL;
	}

	*ppRoom = pRoom;
	if ( pRoom )
		pRoom->AddRef();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::NewTransform( CA3dTransform** ppTransform )
{
	if ( ppTransform )
		return E_INVALIDARG;

	CA3dTransform* pTransform = new( std::nothrow ) CA3dTransform;
	*ppTransform = pTransform;
	pTransform->AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::SetMaterial( IA3dMaterial* pMaterial )
{
	return CA3dMaterialReference::Set( pMaterial );
}

HRESULT STDMETHODCALLTYPE CA3dScene::GetMaterial( IA3dMaterial** ppMaterial )
{
	return CA3dMaterialReference::Clone( ppMaterial );
}

HRESULT STDMETHODCALLTYPE CA3dScene::UnsupportedOperationB( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dScene::UnsupportedOperationC( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dScene::EnableRoom( INT nRoom, DWORD dwFlags )
{
	if ( nRoom < 0 )
		return A3DERROR_INVALID_ROOM_INDEX;

	INT nOffset = nRoom - 1;
	CA3dListNode* pNode = NULL;
	if ( nOffset >= 0 && nOffset < m_Rooms.m_nCount )
	{
		pNode = m_Rooms.m_pFirst;
		while ( nOffset > 0 )
		{
			pNode = pNode->m_pNext;
			nOffset--;
		}
	}

	CA3dRoom* pRoom = NULL;
	if ( pNode )
	{
		m_Rooms.m_pCurrent = pNode;
		pRoom = (CA3dRoom*)pNode->m_pObject;
	}
	if ( !pRoom )
		return A3DERROR_INVALID_ROOM_INDEX;

	pRoom->m_dwRenderFlags |= dwFlags;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::DisableRoom( INT nRoom, DWORD dwFlags )
{
	if ( nRoom < 0 )
		return A3DERROR_INVALID_ROOM_INDEX;

	INT nOffset = nRoom - 1;
	CA3dListNode* pNode = NULL;
	if ( nOffset >= 0 && nOffset < m_Rooms.m_nCount )
	{
		pNode = m_Rooms.m_pFirst;
		while ( nOffset > 0 )
		{
			pNode = pNode->m_pNext;
			nOffset--;
		}
	}

	CA3dRoom* pRoom = NULL;
	if ( pNode )
	{
		m_Rooms.m_pCurrent = pNode;
		pRoom = (CA3dRoom*)pNode->m_pObject;
	}
	if ( !pRoom )
		return A3DERROR_INVALID_ROOM_INDEX;

	pRoom->m_dwRenderFlags ^= !dwFlags;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::SelectRoom( INT nRoom )
{
	if ( nRoom < 0 )
		return A3DERROR_INVALID_ROOM_INDEX;

	INT nOffset = nRoom - 1;
	CA3dListNode* pNode = NULL;
	if ( nOffset >= 0 && nOffset < m_Rooms.m_nCount )
	{
		pNode = m_Rooms.m_pFirst;
		while ( nOffset > 0 )
		{
			pNode = pNode->m_pNext;
			nOffset--;
		}
	}

	CA3dRoom* pRoom = NULL;
	if ( pNode )
	{
		m_Rooms.m_pCurrent = pNode;
		pRoom = (CA3dRoom*)pNode->m_pObject;
	}
	if ( !pRoom )
		return A3DERROR_INVALID_ROOM_INDEX;

	m_pCurrentRoom = pRoom;
	m_bRestartRoom = TRUE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::BeginGeometryQuery()
{
	if ( !m_bValid )
		return A3DERROR_SCENE_INVALID;

	delete m_pList;
	m_pList = new( std::nothrow ) CA3dList( m_pA3d );
	m_pA3d->Clear();
	m_pA3d->LoadIdentity();
	m_pList->Begin();
	m_ZeroContextRooms.m_pCurrent = m_ZeroContextRooms.m_pFirst;
	for ( INT i = m_ZeroContextRooms.m_nCount; i > 0; i-- )
	{
		CA3dRoom* pRoom = (CA3dRoom*)m_ZeroContextRooms.Next();
		pRoom->Render( m_pA3d );
	}

	m_Walls.m_pCurrent = m_Walls.m_pFirst;
	for ( INT i = m_Walls.m_nCount; i > 0; i-- )
	{
		CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_Walls.Next();
		pWall->Render( m_pA3d );
	}

	m_pList->End();
	m_nReadVertex = 0;
	if ( m_pList->m_pCommands )
		m_pList->m_pCommands->m_iCursor = 0;

	return S_OK;
}

BOOL STDMETHODCALLTYPE CA3dScene::GetNextVertex( A3DVAL* pVertex, INT* pnVertex, DWORD* pdwTag )
{
	if ( !m_bValid || !m_pList || !pdwTag )
		return FALSE;

	if ( !m_nReadVertex )
	{
		a3d_primitive_t* pPolygon = NULL;
		if ( m_pList->m_pCommands )
			pPolygon = (a3d_primitive_t*)m_pList->m_pCommands->Next( pdwTag );
		m_pReadPolygon = pPolygon;
		if ( !pPolygon )
			return FALSE;
	}

	memcpy( pVertex, m_pReadPolygon->m_Vertices[m_nReadVertex], 3 * sizeof( A3DVAL ) );
	*pnVertex = m_nReadVertex;
	m_nReadVertex = (INT)( (DWORD)m_nReadVertex + 1 );
	if ( m_nReadVertex == m_pReadPolygon->m_nVertices )
		m_nReadVertex = 0;

	return TRUE;
}

HRESULT STDMETHODCALLTYPE CA3dScene::EndGeometryQuery()
{
	if ( !m_bValid )
		return A3DERROR_SCENE_INVALID;

	m_pList->Release();
	m_pList = NULL;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::Validate()
{
	m_bValid = FALSE;
	if ( m_Rooms.m_nCount <= 0 )
	{
		m_bValid = TRUE;
		return S_OK;
	}

	m_ZeroContextRooms.Clear();
	m_ContextRooms.Clear();
	m_Rooms.m_pCurrent = m_Rooms.m_pFirst;
	for ( INT i = m_Rooms.m_nCount; i > 0; i-- )
	{
		CA3dRoom* pRoom = (CA3dRoom*)m_Rooms.Next();
		if ( pRoom->m_dwContext )
			m_ContextRooms.Append( pRoom );
		else
			m_ZeroContextRooms.Append( pRoom );
	}

	m_UnpairedRooms.Clear();
	CA3dObjectList rooms = {};
	try
	{
		m_ZeroContextRooms.m_pCurrent = m_ZeroContextRooms.m_pFirst;
		rooms.Clear();
		for ( INT i = m_ZeroContextRooms.m_nCount; i > 0; i-- )
			rooms.Append( m_ZeroContextRooms.Next() );

		m_ZeroContextRooms.m_pCurrent = m_ZeroContextRooms.m_pFirst;
		for ( INT i = m_ZeroContextRooms.m_nCount; i > 0; i-- )
		{
			CA3dRoom* pRoom = (CA3dRoom*)m_ZeroContextRooms.Next();
			pRoom->ClearConnections();
			rooms.m_pCurrent = rooms.m_pFirst;
			for ( INT j = rooms.m_nCount; j > 0; j-- )
			{
				CA3dRoom* pOtherRoom = (CA3dRoom*)rooms.Next();
				if ( pRoom != pOtherRoom )
				{
					pRoom->FindAdjacentRoom( pOtherRoom );
					if ( FAILED( pRoom->ConnectRoom( pOtherRoom ) ) )
					{
						rooms.Clear();
						return A3DERROR_SCENE_HAS_OVERLAPPING_STATIC_ROOMS;
					}
				}
			}
			if ( pRoom->HasUnpairedWall() )
				m_UnpairedRooms.Append( pRoom );
		}

		m_ZeroContextRooms.m_pCurrent = m_ZeroContextRooms.m_pFirst;
		for ( INT i = m_ZeroContextRooms.m_nCount; i > 0; i-- )
		{
			CA3dRoom* pRoom = (CA3dRoom*)m_ZeroContextRooms.Next();
			pRoom->UpdateWalls();
		}

		m_ZeroContextRooms.m_pCurrent = m_ZeroContextRooms.m_pFirst;
		for ( INT i = m_ZeroContextRooms.m_nCount; i > 0; i-- )
		{
			CA3dRoom* pRoom = (CA3dRoom*)m_ZeroContextRooms.Next();
			pRoom->RebuildRenderList( m_pA3d );
		}

		m_pCurrentRoom = NULL;
		m_bValid = TRUE;
		rooms.Clear();
	}
	catch ( ... )
	{
		rooms.Clear();
		throw;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::Clear()
{
	m_Rooms.m_pCurrent = m_Rooms.m_pFirst;
	for ( INT i = m_Rooms.m_nCount; i > 0; i-- )
	{
		IUnknown* pRoom = (IUnknown*)m_Rooms.Next();
		pRoom->Release();
	}

	m_Walls.m_pCurrent = m_Walls.m_pFirst;
	for ( INT i = m_Walls.m_nCount; i > 0; i-- )
	{
		IUnknown* pWall = (IUnknown*)m_Walls.Next();
		pWall->Release();
	}

	m_bValid = FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dScene::UnsupportedOperationD( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dScene::UnsupportedOperationE( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dScene::UnsupportedOperationF( DWORD, DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dScene::UnsupportedOperationG( DWORD, DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dScene::UnsupportedOperationH( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dScene::SetName( const char* pszName )
{
	return CA3dName::Set( pszName );
}

HRESULT STDMETHODCALLTYPE CA3dScene::GetName( char* pszName, DWORD dwSize )
{
	return CA3dName::Get( pszName, dwSize );
}

void CA3dScene::UpdateCurrentRoom()
{
	if ( m_pCurrentRoom )
	{
		if ( m_bRestartRoom )
		{
			m_bRestartRoom = FALSE;
			return;
		}
		if ( m_pCurrentRoom->ContainsPoint( m_pA3d->m_vWorldListenerPosition ) )
			return;
	}

	CA3dRoom* pRoom = NULL;
	m_ZeroContextRooms.m_pCurrent = m_ZeroContextRooms.m_pFirst;
	for ( INT i = m_ZeroContextRooms.m_nCount; i > 0; i-- )
	{
		pRoom = (CA3dRoom*)m_ZeroContextRooms.Next();
		if ( pRoom->ContainsPoint( m_pA3d->m_vWorldListenerPosition ) )
			break;
		pRoom = NULL;
	}
	m_pCurrentRoom = pRoom;
}

HRESULT CA3dScene::Render()
{
	if ( !m_bValid )
		return A3DERROR_SCENE_INVALID;

	m_NeighborRooms.Clear();
	UpdateCurrentRoom();
	CA3dObjectList* pRooms = &m_UnpairedRooms;
	if ( m_pCurrentRoom && m_pCurrentRoom->m_AdjacentRooms.m_nCount > 0 && !m_pCurrentRoom->m_bDynamicWalls )
	{
		m_NeighborRooms.Clear();
		CA3dObjectList* pAdjacentRooms = &m_pCurrentRoom->m_AdjacentRooms;
		pAdjacentRooms->m_pCurrent = pAdjacentRooms->m_pFirst;
		for ( INT i = pAdjacentRooms->m_nCount; i > 0; i-- )
			m_NeighborRooms.Append( pAdjacentRooms->Next() );
		m_NeighborRooms.Append( m_pCurrentRoom );
		pRooms = &m_NeighborRooms;
	}

	if ( m_pMaterial )
		m_pA3d->BindMaterial( m_pMaterial );
	m_pA3d->LoadIdentity();
	m_Walls.m_pCurrent = m_Walls.m_pFirst;
	for ( INT i = m_Walls.m_nCount; i > 0; i-- )
	{
		CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_Walls.Next();
		pWall->Render( m_pA3d );
	}

	pRooms->m_pCurrent = pRooms->m_pFirst;
	for ( INT i = pRooms->m_nCount; i > 0; i-- )
	{
		CA3dRoom* pRoom = (CA3dRoom*)pRooms->Next();
		if ( pRoom->m_dwRenderFlags && pRoom->m_pRenderList )
			pRoom->m_pRenderList->Call();
	}

	return S_OK;
}

#define A3D_DEFAULT_ROOM_RENDER_FLAGS	0x46
#define A3D_WALL_SKIP_RENDER_MASK		0x02

static const IID roomIid =
{
	0x103e7226, 0x0113, 0x11d2, { 0x90, 0xfb, 0x00, 0x60, 0x08, 0xa1, 0xf4, 0x41 }
};

static DWORD g_nRoomBuilderNames;

CA3dRoom::CA3dRoom( CA3dRoomBuilder* pBuilder, DWORD dwContext, const A3DVAL* pPosition,
	const A3DVAL* pForward, const A3DVAL* pUp, CA3dScene* pScene )
{
	m_Walls.m_pFirst = NULL;
	m_Walls.m_pLast = NULL;
	m_Walls.m_pCurrent = NULL;
	m_Walls.m_nCount = 0;
	m_ShellWalls.m_pFirst = NULL;
	m_ShellWalls.m_pLast = NULL;
	m_ShellWalls.m_pCurrent = NULL;
	m_ShellWalls.m_nCount = 0;
	m_AdjacentRooms.m_pFirst = NULL;
	m_AdjacentRooms.m_pLast = NULL;
	m_AdjacentRooms.m_pCurrent = NULL;
	m_AdjacentRooms.m_nCount = 0;
	m_cReferences = 0;
	m_dwContext = dwContext;
	try
	{
		m_Transform.SetFrame( this, pPosition, pForward, pUp );

		pBuilder->m_Walls.m_pCurrent = pBuilder->m_Walls.m_pFirst;
		for ( int i = pBuilder->m_Walls.m_nCount; i > 0; i-- )
		{
			CA3dGeometryFace* pPrototype = (CA3dGeometryFace*)pBuilder->m_Walls.Next();
			CA3dGeometryFace* pWall = new( std::nothrow ) CA3dGeometryFace( this, pPrototype );
			m_Walls.Append( pWall );
			pWall->AddRef();
			if ( pWall->m_dwFlags & A3D_SHELL_WALL )
				m_ShellWalls.Append( pWall );
		}

		m_pRenderList = NULL;
		m_dwRenderFlags = A3D_DEFAULT_ROOM_RENDER_FLAGS;
		CA3dMaterialReference::Copy( pBuilder );
		m_pScene = pScene;
		m_bPointTesting = pBuilder->m_bPointTesting;
		m_bDynamicWalls = FALSE;
	}
	catch ( ... )
	{
		m_AdjacentRooms.Clear();
		m_ShellWalls.Clear();
		m_Walls.Clear();
		throw;
	}
}

CA3dRoom::~CA3dRoom()
{
	m_Walls.m_pCurrent = m_Walls.m_pFirst;
	for ( int i = m_Walls.m_nCount; i > 0; i-- )
	{
		IUnknown* pWall = (IUnknown*)m_Walls.Next();
		pWall->Release();
	}

	m_ShellWalls.Clear();
	if ( m_pRenderList )
	{
		m_pRenderList->Release();
		m_pRenderList = NULL;
	}

	m_AdjacentRooms.Clear();
	m_ShellWalls.Clear();
	m_Walls.Clear();
}

HRESULT STDMETHODCALLTYPE CA3dRoom::QueryInterface( REFIID iid, void** ppObject )
{
	if ( !ppObject )
		return E_INVALIDARG;

	if ( !memcmp( &iid, &IID_IUnknown, sizeof( IID ) ) || !memcmp( &iid, &roomIid, sizeof( IID ) ) )
		*ppObject = this;

	if ( !*ppObject )
		return E_NOINTERFACE;

	( (IUnknown*)*ppObject )->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CA3dRoom::AddRef()
{
	return ++m_cReferences;
}

ULONG STDMETHODCALLTYPE CA3dRoom::Release()
{
	ULONG cReferences = --m_cReferences;
	if ( !cReferences )
		delete this;

	return cReferences;
}

HRESULT STDMETHODCALLTYPE CA3dRoom::UnsupportedOperationA( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dRoom::UnsupportedOperationB( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dRoom::GetScene( CA3dScene** ppScene )
{
	if ( !ppScene )
		return E_INVALIDARG;

	*ppScene = m_pScene;
	m_pScene->AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dRoom::SetMaterial( IA3dMaterial* pMaterial )
{
	return CA3dMaterialReference::Set( pMaterial );
}

HRESULT STDMETHODCALLTYPE CA3dRoom::GetMaterial( IA3dMaterial** ppMaterial )
{
	return CA3dMaterialReference::Clone( ppMaterial );
}

HRESULT STDMETHODCALLTYPE CA3dRoom::GetWall( INT nIndex, IUnknown** ppWall )
{
	if ( nIndex <= 0 )
		return A3DERROR_INVALID_WALL_INDEX;

	if ( !ppWall )
		return E_INVALIDARG;

	*ppWall = NULL;
	if ( m_Walls.m_nCount <= 0 )
		return A3DERROR_INVALID_WALL_INDEX;

	CA3dListNode* pNode = NULL;
	int nOffset = nIndex - 1;
	if ( nOffset >= 0 && nOffset < m_Walls.m_nCount )
	{
		pNode = m_Walls.m_pFirst;
		while ( nOffset > 0 )
		{
			pNode = pNode->m_pNext;
			nOffset--;
		}
	}

	IUnknown* pWall = NULL;
	if ( pNode )
	{
		m_Walls.m_pCurrent = pNode;
		pWall = (IUnknown*)pNode->m_pObject;
	}

	if ( !pWall )
		return A3DERROR_INVALID_WALL_INDEX;

	*ppWall = pWall;
	pWall->AddRef();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dRoom::SetTransform( const CA3dTransform* pTransform )
{
	if ( !pTransform )
		return A3DERROR_INVALID_ARGUMENT;

	m_Transform.CopyMatrixFrom( pTransform );
	InvalidateScene();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dRoom::NewTransform( CA3dTransform** ppTransform )
{
	CA3dTransform* pTransform = new( std::nothrow ) CA3dTransform;
	if ( !pTransform )
		return A3DERROR_MEMORY_ALLOCATION;

	pTransform->CopyFrom( this, &m_Transform );
	pTransform->AddRef();
	*ppTransform = pTransform;
	return S_OK;
}

void CA3dRoom::InvalidateScene()
{
	if ( m_pScene )
		m_pScene->m_bValid = FALSE;
}

HRESULT STDMETHODCALLTYPE CA3dRoom::GetPosition( CA3dTransform* pReference, A3DVAL* pPosition )
{
	return m_Transform.GetPosition( pReference, pPosition );
}

HRESULT STDMETHODCALLTYPE CA3dRoom::SetPosition( CA3dTransform* pReference, const A3DVAL* pPosition )
{
	HRESULT hr = m_Transform.SetPosition( pReference, pPosition );
	if ( SUCCEEDED( hr ) )
		InvalidateScene();

	return hr;
}

HRESULT STDMETHODCALLTYPE CA3dRoom::GetOrientation( CA3dTransform* pReference, A3DVAL* pForward, A3DVAL* pUp )
{
	return m_Transform.GetOrientation( pReference, pForward, pUp );
}

HRESULT STDMETHODCALLTYPE CA3dRoom::SetOrientation( CA3dTransform* pReference,
	const A3DVAL* pForward, const A3DVAL* pUp )
{
	HRESULT hr = m_Transform.SetOrientation( pReference, pForward, pUp );
	if ( SUCCEEDED( hr ) )
		InvalidateScene();

	return hr;
}

HRESULT STDMETHODCALLTYPE CA3dRoom::CompatibilityCheck()
{
	return S_OK;
}

void CA3dRoom::ClearConnections()
{
	m_ShellWalls.m_pCurrent = m_ShellWalls.m_pFirst;
	for ( int i = m_ShellWalls.m_nCount; i > 0; i-- )
	{
		CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_ShellWalls.Next();
		pWall->m_AdjacentFaces.Clear();
	}

	m_AdjacentRooms.Clear();
}

BOOL CA3dRoom::ContainsPoint( const A3DVAL* pPoint )
{
	if ( !m_bPointTesting )
		return FALSE;

	A3DVECTOR point;
	A3DVECTOR direction = { 1.0f, 0.0f, 0.0f, 0.0f };
	memcpy( point, pPoint, sizeof( A3DVAL ) * 3 );
	DWORD nIntersections = 0;

	m_ShellWalls.m_pCurrent = m_ShellWalls.m_pFirst;
	for ( int i = m_ShellWalls.m_nCount; i > 0; i-- )
	{
		CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_ShellWalls.Next();
		pWall->m_Polygons.m_pCurrent = pWall->m_Polygons.m_pFirst;
		for ( int j = pWall->m_Polygons.m_nCount; j > 0; j-- )
		{
			a3d_polygon_t* pPolygon = (a3d_polygon_t*)pWall->m_Polygons.Next();
			a3d_polygon_t polygon;
			pWall->m_Transform.TransformPolygon( &polygon, pPolygon );

			BOOL bIntersects;
			if ( pPolygon->m_nVertices == 4 )
				bIntersects = A3dIntersectQuad( point, direction, &polygon, TRUE, TRUE );
			else
				bIntersects = A3dIntersectTriangle( point, direction, &polygon, TRUE, TRUE );

			if ( bIntersects )
				nIntersections++;
		}
	}

	return ( nIntersections & 1 ) ? TRUE : FALSE;
}

BOOL CA3dRoom::HasUnpairedWall()
{
	m_ShellWalls.m_pCurrent = m_ShellWalls.m_pFirst;
	for ( int i = m_ShellWalls.m_nCount; i > 0; i-- )
	{
		CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_ShellWalls.Next();
		if ( !pWall->m_AdjacentFaces.m_nCount )
			return TRUE;
	}

	return FALSE;
}

void CA3dRoom::FindAdjacentRoom( CA3dRoom* pRoom )
{
	m_ShellWalls.m_pCurrent = m_ShellWalls.m_pFirst;
	for ( int i = m_ShellWalls.m_nCount; i > 0; i-- )
	{
		CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_ShellWalls.Next();
		pRoom->m_ShellWalls.m_pCurrent = pRoom->m_ShellWalls.m_pFirst;
		for ( int j = pRoom->m_ShellWalls.m_nCount; j > 0; j-- )
		{
			CA3dGeometryFace* pOtherWall = (CA3dGeometryFace*)pRoom->m_ShellWalls.Next();
			if ( pWall->IsAdjacentTo( pOtherWall ) )
			{
				m_AdjacentRooms.Append( pRoom );
				return;
			}
		}
	}
}

HRESULT CA3dRoom::ConnectRoom( CA3dRoom* pRoom )
{
	m_ShellWalls.m_pCurrent = m_ShellWalls.m_pFirst;
	for ( int i = m_ShellWalls.m_nCount; i > 0; i-- )
	{
		CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_ShellWalls.Next();
		pRoom->m_ShellWalls.m_pCurrent = pRoom->m_ShellWalls.m_pFirst;
		for ( int j = pRoom->m_ShellWalls.m_nCount; j > 0; j-- )
		{
			CA3dGeometryFace* pOtherWall = (CA3dGeometryFace*)pRoom->m_ShellWalls.Next();
			if ( FAILED( pWall->ConnectTo( pOtherWall ) ) )
				return E_FAIL;
		}
	}

	return S_OK;
}

void CA3dRoom::UpdateWalls()
{
	m_Walls.m_pCurrent = m_Walls.m_pFirst;
	for ( int i = m_Walls.m_nCount; i > 0; i-- )
	{
		CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_Walls.Next();
		if ( pWall->m_dwFlags & A3D_SHELL_WALL )
			pWall->RebuildOpenings();
	}
}

void CA3dRoom::Render( CA3d* pA3d )
{
	if ( m_pMaterial )
		pA3d->BindMaterial( m_pMaterial );

	m_Walls.m_pCurrent = m_Walls.m_pFirst;
	for ( int i = m_Walls.m_nCount; i > 0; i-- )
	{
		CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_Walls.Next();
		pWall->Render( pA3d );
		if ( pWall->m_dwFlags & A3D_WALL_SKIP_RENDER_MASK )
			m_bDynamicWalls = TRUE;
	}
}

void CA3dRoom::RebuildRenderList( CA3d* pA3d )
{
	if ( m_pRenderList )
	{
		m_pRenderList->Release();
		m_pRenderList = NULL;
	}

	pA3d->NewList( &m_pRenderList );
	pA3d->Clear();
	pA3d->LoadIdentity();
	m_pRenderList->Begin();
	Render( pA3d );
	m_pRenderList->End();
}

CA3dRoomBuilder::CA3dRoomBuilder( DWORD dwMode )
{
	m_Walls.m_pFirst = NULL;
	m_Walls.m_pLast = NULL;
	m_Walls.m_pCurrent = NULL;
	m_Walls.m_nCount = 0;
	m_Auxiliary.m_pFirst = NULL;
	m_Auxiliary.m_pLast = NULL;
	m_Auxiliary.m_pCurrent = NULL;
	m_Auxiliary.m_nCount = 0;
	m_dwMode = dwMode;
	m_cReferences = 0;
	CA3dName::Format( "RoomBuilder", (int)++g_nRoomBuilderNames );
	m_bPointTesting = FALSE;
}

CA3dRoomBuilder::~CA3dRoomBuilder()
{
	Clear();
	m_Auxiliary.Clear();
	m_Walls.Clear();
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::UnsupportedOperationA( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::UnsupportedOperationB( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::SetMaterial( IA3dMaterial* pMaterial )
{
	return CA3dMaterialReference::Set( pMaterial );
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::GetMaterial( IA3dMaterial** ppMaterial )
{
	return CA3dMaterialReference::Clone( ppMaterial );
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::Clear()
{
	m_Auxiliary.m_pCurrent = m_Auxiliary.m_pFirst;
	for ( int i = m_Auxiliary.m_nCount; i > 0; i-- )
		operator delete( m_Auxiliary.Next() );
	m_Auxiliary.Clear();

	m_Walls.m_pCurrent = m_Walls.m_pFirst;
	for ( int i = m_Walls.m_nCount; i > 0; i-- )
	{
		IUnknown* pWall = (IUnknown*)m_Walls.Next();
		pWall->Release();
	}
	m_Walls.Clear();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::UnsupportedOperationC( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::UnsupportedOperationD( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::UnsupportedOperationE( DWORD, DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::UnsupportedOperationF( DWORD, DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::UnsupportedOperationG( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::SetName( const char* pszName )
{
	return CA3dName::Set( pszName );
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::GetName( char* pszName, DWORD dwSize )
{
	return CA3dName::Get( pszName, dwSize );
}

static DWORD g_dwWallNameId;

CA3dWallBuilder::CA3dWallBuilder( int nSceneType ) :
	CA3dGeometryBuilder( nSceneType ),
	m_ulReferences( 0 ),
	m_Openings{ NULL, NULL, NULL, 0 }
{
	CA3dName::Format( "WallBuilder", (int)++g_dwWallNameId );
}

CA3dWallBuilder::~CA3dWallBuilder()
{
	ClearOpenings();
	m_Openings.Clear();
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::QueryInterface( REFIID, void** ppvObject )
{
	if ( !ppvObject )
		return E_INVALIDARG;

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CA3dWallBuilder::AddRef()
{
	return ++m_ulReferences;
}

ULONG STDMETHODCALLTYPE CA3dWallBuilder::Release()
{
	ULONG ulReferences = --m_ulReferences;
	if ( !ulReferences )
		delete this;

	return ulReferences;
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::SetMaterial( IA3dMaterial* pMaterial )
{
	return CA3dMaterialReference::Set( pMaterial );
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::GetMaterial( IA3dMaterial** ppMaterial )
{
	return CA3dMaterialReference::Clone( ppMaterial );
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::AddOpening( CA3dOpeningBuilder* pOpening,
		const A3DVAL* pPosition, const A3DVAL* pDirection, const A3DVAL* pUp )
{
	if ( FAILED( pOpening->Validate() ) )
		return A3DERROR_OPENING_NOT_VALID;

	A3DVECTOR position = { 0, 0, 0, 1 };
	A3DVECTOR direction = { 0, 0, 1, 0 };
	A3DVECTOR up = { 0, 1, 0, 0 };
	if ( !pPosition )
		pPosition = position;
	if ( !pDirection )
		pDirection = direction;
	if ( !pUp )
		pUp = up;

	double flDot = ( (double)pUp[2] * pDirection[2] + (double)pUp[0] * pDirection[0] ) +
			(double)pDirection[1] * pUp[1];
	if ( fabs( flDot ) > A3D_EPSILON )
		return A3DERROR_DIR_AND_UP_VECTORS_NOT_PERPENDICULAR;

	double flDirectionLength = ( (double)pDirection[1] * pDirection[1] +
			(double)pDirection[2] * pDirection[2] ) + (double)pDirection[0] * pDirection[0];
	if ( !( flDirectionLength >= A3D_EPSILON_SQUARED ) )
		return A3DERROR_DIR_AND_UP_VECTORS_NOT_PERPENDICULAR;

	double flUpLength = ( (double)pUp[1] * pUp[1] + (double)pUp[2] * pUp[2] ) +
			(double)pUp[0] * pUp[0];
	if ( !( flUpLength >= A3D_EPSILON_SQUARED ) )
		return A3DERROR_DIR_AND_UP_VECTORS_NOT_PERPENDICULAR;

	CA3dOpening* pInstance = new ( std::nothrow ) CA3dOpening( pOpening, pPosition, pDirection, pUp );
	pInstance->AddRef();
	m_Openings.Append( pInstance );
	return m_Openings.m_nCount;
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::RemoveOpening( int nOpening )
{
	int nIndex = (int)( (DWORD)nOpening - 1 );
	CA3dListNode* pNode = NULL;
	if ( nIndex >= 0 && nIndex < m_Openings.m_nCount )
	{
		pNode = m_Openings.m_pFirst;
		while ( nIndex-- > 0 )
			pNode = pNode->m_pNext;
	}

	CA3dOpening* pOpening = NULL;
	if ( pNode )
	{
		m_Openings.m_pCurrent = pNode;
		pOpening = (CA3dOpening*)pNode->m_pObject;
	}
	if ( !pOpening )
		return A3DERROR_INVALID_OPENING_INDEX;

	int nCount = m_Openings.m_nCount;
	CA3dListNode* pFirst = m_Openings.m_pFirst;
	pNode = pFirst;
	int nPosition = 0;
	for ( int i = 0; i < nCount; ++i )
	{
		if ( pNode->m_pObject == pOpening )
		{
			m_Openings.m_pCurrent = pNode;
			nPosition = i + 1;
			break;
		}
		pNode = pNode->m_pNext;
	}

	if ( nPosition > 0 )
	{
		nIndex = nPosition - 1;
		pNode = NULL;
		if ( nIndex >= 0 && nIndex < nCount )
		{
			pNode = pFirst;
			while ( nIndex-- > 0 )
				pNode = pNode->m_pNext;
		}

		if ( pNode )
		{
			if ( pFirst == pNode )
				m_Openings.m_pFirst = pNode->m_pNext;
			if ( m_Openings.m_pLast == pNode )
				m_Openings.m_pLast = pNode->m_pPrevious;
			if ( m_Openings.m_pCurrent == pNode )
			{
				m_Openings.m_pCurrent = pNode->m_pPrevious;
				if ( !m_Openings.m_pCurrent )
					m_Openings.m_pCurrent = m_Openings.m_pFirst;
			}
			if ( pNode->m_pNext )
				pNode->m_pNext->m_pPrevious = pNode->m_pPrevious;
			if ( pNode->m_pPrevious )
				pNode->m_pPrevious->m_pNext = pNode->m_pNext;
			delete pNode;
		}
		--m_Openings.m_nCount;
	}

	pOpening->Release();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::GetOpening( int nOpening, CA3dOpening** ppOpening )
{
	if ( !ppOpening )
		return A3DERROR_INVALID_ARGUMENT;

	int nIndex = (int)( (DWORD)nOpening - 1 );
	CA3dListNode* pNode = NULL;
	if ( nIndex >= 0 && nIndex < m_Openings.m_nCount )
	{
		pNode = m_Openings.m_pFirst;
		while ( nIndex-- > 0 )
			pNode = pNode->m_pNext;
	}

	CA3dOpening* pOpening = NULL;
	if ( pNode )
	{
		m_Openings.m_pCurrent = pNode;
		pOpening = (CA3dOpening*)pNode->m_pObject;
	}
	if ( !pOpening )
		return A3DERROR_INVALID_OPENING_INDEX;

	pOpening->AddRef();
	*ppOpening = pOpening;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::Begin( DWORD dwMode )
{
	return CA3dGeometryBuilder::Begin( dwMode );
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::Vertex3f( A3DVAL x, A3DVAL y, A3DVAL z )
{
	return CA3dGeometryBuilder::AddVertex3f( x, y, z );
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::Vertex3fv( const A3DVAL* pVertex )
{
	return CA3dGeometryBuilder::AddVertex3fv( pVertex );
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::End()
{
	return CA3dGeometryBuilder::End();
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::RemovePolygon( int nPolygon )
{
	return CA3dGeometryBuilder::RemovePolygon( nPolygon );
}

int STDMETHODCALLTYPE CA3dWallBuilder::GetPolygonCount()
{
	return CA3dGeometryBuilder::GetPolygonCount();
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::SetVertex( int nPolygon, int nVertex, A3DVAL x, A3DVAL y, A3DVAL z )
{
	return CA3dGeometryBuilder::SetVertex( nPolygon, nVertex, x, y, z );
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::GetVertex( int nPolygon, int nVertex, A3DVAL* pVertex )
{
	return CA3dGeometryBuilder::GetVertex( nPolygon, nVertex, pVertex );
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::Clear()
{
	ClearOpenings();
	return CA3dGeometryBuilder::Clear();
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::Validate()
{
	HRESULT hr = CA3dGeometryBuilder::Validate();
	if ( FAILED( hr ) )
		return hr;

	m_Openings.m_pCurrent = m_Openings.m_pFirst;
	for ( int nRemaining = m_Openings.m_nCount; nRemaining > 0; --nRemaining )
	{
		CA3dOpening* pOpening = (CA3dOpening*)m_Openings.Next();
		if ( !IsOpeningCoplanar( pOpening ) )
			return A3DERROR_OPENING_NOT_WALL_COPLANAR;
	}
	return S_OK;
}

BOOL CA3dWallBuilder::IsOpeningCoplanar( CA3dOpening* pOpening )
{
	pOpening->m_Polygons.m_pCurrent = pOpening->m_Polygons.m_pFirst;
	for ( int nRemaining = pOpening->m_Polygons.m_nCount; nRemaining > 0; --nRemaining )
	{
		a3d_polygon_t* pPolygon = (a3d_polygon_t*)pOpening->m_Polygons.Next();
		int nVertices = pPolygon->m_nVertices;
		a3d_polygon_t polygon;
		pOpening->m_Transform.TransformPolygon( &polygon, pPolygon );
		while ( nVertices-- > 0 )
		{
			double dx = (double)m_pFirstPolygon->m_Vertices[0][0] - polygon.m_Vertices[nVertices][0];
			double dy = (double)m_pFirstPolygon->m_Vertices[0][1] - polygon.m_Vertices[nVertices][1];
			double dz = (double)m_pFirstPolygon->m_Vertices[0][2] - polygon.m_Vertices[nVertices][2];
			double flDistanceSquared = ( dx * dx + dy * dy ) + dz * dz;
			if ( flDistanceSquared >= A3D_EPSILON_SQUARED )
			{
				double flDistance = ( (double)m_pNormal[0] * dx + (double)m_pNormal[1] * dy ) +
						(double)m_pNormal[2] * dz;
				if ( fabs( flDistance ) >= A3D_EPSILON )
					return FALSE;
			}
		}
	}
	return TRUE;
}

void CA3dWallBuilder::ClearOpenings()
{
	m_Openings.m_pCurrent = m_Openings.m_pFirst;
	for ( int nRemaining = m_Openings.m_nCount; nRemaining > 0; --nRemaining )
		( (IUnknown*)m_Openings.Next() )->Release();

	m_Openings.Clear();
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::UnsupportedRequest1( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::UnsupportedRequest2( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::UnsupportedRequest3( DWORD, DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::UnsupportedRequest4( DWORD, DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::UnsupportedRequest5( DWORD )
{
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::SetNameID( const char* pszName )
{
	return CA3dName::Set( pszName );
}

HRESULT STDMETHODCALLTYPE CA3dWallBuilder::GetNameID( char* pszName, DWORD dwSize )
{
	return CA3dName::Get( pszName, dwSize );
}

#define A3D_BACKEND_INITIALIZATION_MODE	1
#define A3D_BACKEND_COMPATIBILITY_ID		0x104
#define A3D_SECONDS_PER_MILLISECOND		0.001f
#define A3D_MAX_FRAME_TIME				5.0
#define A3D_DEFAULT_FRAME_TIME			0.0166f
#define A3D_UNSET_FLUSH_TIME			0xFFFFFFFF

static union
{
	DWORD	dwBits;
	A3DVAL	flMilliseconds;
} s_lastFlush = { A3D_UNSET_FLUSH_TIME };

static A3DVAL s_flLastFrameTime = A3D_DEFAULT_FRAME_TIME;

typedef struct a3d_backend_caps_s
{
	WORD	wCompatibilityId;
	BYTE	ReservedHeader[66];
	DWORD	dwCompatibilityState;
	BYTE	ReservedTail[492];
} a3d_backend_caps_t;

static const IID s_BackendInterfaceId =
{
	0xa3c62f2a, 0x2671, 0x11d2, { 0xbb, 0x48, 0x00, 0x60, 0x08, 0x2f, 0x3c, 0x00 }
};

HRESULT CA3d::SetupBackend( IDirectSound* pDirectSound, IA3d2* pA3d2, IA3d* pA3d )
{
	m_pDirectSound = pDirectSound;
	m_pA3d2 = pA3d2;
	m_pA3d = pA3d;
	m_pBackend = NULL;
	m_pPrimaryBuffer = NULL;
	m_bBackendInitialized = TRUE;
	m_dwBackendState = 0;
	m_pDirectSoundListener = NULL;
	m_dwCooperativeLevel = A3D_CL_NORMAL;
	m_dwRequestedFeatures = 0;
	m_dwFeaturesAvailable = 0;
	m_bCompat1011 = FALSE;
	m_bCompat1010 = FALSE;
	return S_OK;
}

HRESULT CA3d::ShutdownBackend()
{
	int nSources = m_Sources.m_nCount;
	m_Sources.m_pCurrent = m_Sources.m_pFirst;
	for ( int i = 0; i < nSources; i++ )
	{
		CA3dSource* pSource = (CA3dSource*)m_Sources.Next();
		pSource->m_bRegistered = FALSE;
		if ( pSource )
			delete pSource;
	}

	m_Sources.Clear();
	if ( m_pBackend )
	{
		m_pBackend->Release();
		m_pBackend = NULL;
	}
	if ( m_pDirectSoundListener )
	{
		m_pDirectSoundListener->Release();
		m_pDirectSoundListener = NULL;
	}
	if ( m_pA3d )
	{
		m_pA3d->Release();
		m_pA3d = NULL;
	}
	if ( m_pA3d2 )
	{
		m_pA3d2->Release();
		m_pA3d2 = NULL;
	}
	if ( m_pPrimaryBuffer )
	{
		m_pPrimaryBuffer->Release();
		m_pPrimaryBuffer = NULL;
	}
	if ( m_pDirectSound )
	{
		m_pDirectSound->Release();
		m_pDirectSound = NULL;
	}
	m_bBackendInitialized = FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::IsFeatureAvailable( DWORD dwFeatures )
{
	if ( m_dwFeaturesAvailable & dwFeatures )
		return S_OK;
	if ( m_dwRequestedFeatures & dwFeatures )
		return A3DERROR_FEATURE_NOT_SUPPORTED;
	return A3DERROR_FEATURE_NOT_REQUESTED;
}

HRESULT STDMETHODCALLTYPE CA3d::Init( LPGUID pDevice, DWORD dwFeatures, DWORD )
{
	DWORD dwSupportedFeatures = 0;
	m_dwRequestedFeatures = dwFeatures;

	HRESULT hr = m_pDirectSound->QueryInterface( s_BackendInterfaceId, (void**)&m_pBackend );
	if ( FAILED( hr ) )
	{
		if ( FAILED( m_pDirectSound->Initialize( pDevice ) ) )
			return A3DERROR_FAILED_INIT;
	}
	else
	{
		hr = m_pBackend->InitializeEx( (DWORD)pDevice, dwFeatures,
			(void*)A3D_BACKEND_INITIALIZATION_MODE, &dwSupportedFeatures );
		if ( FAILED( hr ) )
			return A3DERROR_FAILED_INIT;

		a3d_backend_caps_t caps;
		hr = m_pBackend->GetA3dCaps( &caps, &pDevice );
		if ( SUCCEEDED( hr ) && caps.wCompatibilityId == A3D_BACKEND_COMPATIBILITY_ID &&
			!caps.dwCompatibilityState )
		{
			Compat( A3D_COMPAT_LEGACY_MARKER, A3D_COMPAT_LEGACY_MARKER_VALUE );
		}
		if ( m_bCompat1011 )
			dwSupportedFeatures &= ~A3D_1ST_REFLECTIONS;
	}

	m_dwFeaturesAvailable = dwSupportedFeatures | A3D_OCCLUSIONS | A3D_DIRECT_PATH_A3D;
	SetResourceManagerMode( A3D_RESOURCE_MODE_DYNAMIC );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::Clear()
{
	if ( !m_bGeometryInitialized )
		return S_OK;

	m_GeometryPools.m_pCurrent = m_GeometryPools.m_pLast;
	int nRemove = m_GeometryPools.m_nCount > 1 ? m_GeometryPools.m_nCount - 1 : 0;
	for ( int i = 0; i < nRemove; i++ )
	{
		CA3dListNode* pNode = m_GeometryPools.m_pCurrent;
		CA3dCommandBuffer* pBuffer = pNode ? (CA3dCommandBuffer*)pNode->m_pObject : NULL;
		if ( !pBuffer->m_iBufferType && pBuffer )
			delete pBuffer;

		pNode = m_GeometryPools.m_pCurrent;
		if ( pNode )
		{
			if ( m_GeometryPools.m_pFirst == pNode )
				m_GeometryPools.m_pFirst = pNode->m_pNext;
			if ( m_GeometryPools.m_pLast == pNode )
				m_GeometryPools.m_pLast = pNode->m_pPrevious;
			if ( m_GeometryPools.m_pCurrent == pNode )
			{
				m_GeometryPools.m_pCurrent = pNode->m_pPrevious;
				if ( !m_GeometryPools.m_pCurrent )
					m_GeometryPools.m_pCurrent = m_GeometryPools.m_pFirst;
			}
			if ( pNode->m_pNext )
				pNode->m_pNext->m_pPrevious = pNode->m_pPrevious;
			if ( pNode->m_pPrevious )
				pNode->m_pPrevious->m_pNext = pNode->m_pNext;
			operator delete( pNode );
			m_GeometryPools.m_nCount = (int)( (DWORD)m_GeometryPools.m_nCount - 1 );
		}
	}

	CA3dCommandBuffer* pBuffer = NULL;
	if ( m_GeometryPools.m_nCount > 0 )
	{
		m_GeometryPools.m_pCurrent = m_GeometryPools.m_pFirst;
		if ( m_GeometryPools.m_pCurrent )
			pBuffer = (CA3dCommandBuffer*)m_GeometryPools.m_pCurrent->m_pObject;
	}
	m_pCommandBuffer = pBuffer;
	pBuffer->m_cCommands = 0;
	pBuffer->m_iCursor = 0;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::Flush()
{
	double flMilliseconds = (double)timeGetTime();
	double flFrameTime = ( flMilliseconds - s_lastFlush.flMilliseconds ) * A3D_SECONDS_PER_MILLISECOND;
	m_flFrameTime = (A3DVAL)flFrameTime;
	if ( !( flFrameTime >= 0.0 ) || flFrameTime > A3D_MAX_FRAME_TIME )
		m_flFrameTime = s_flLastFrameTime;

	s_lastFlush.flMilliseconds = (A3DVAL)flMilliseconds;
	s_flLastFrameTime = m_flFrameTime;
	RenderSources();
	return S_OK;
}

#define A3D_ROOM_MIN_SHELL_WALLS				4
#define A3D_ROOM_MIN_UNIQUE_NORMALS			3
#define A3D_ROOM_DISABLE_POINT_TESTING		1

INT STDMETHODCALLTYPE CA3dRoomBuilder::AddWall( IUnknown* pGeometry, DWORD dwFlags,
	const A3DVAL* pPosition, const A3DVAL* pForward, const A3DVAL* pUp )
{
	CA3dWallBuilder* pBuilder = (CA3dWallBuilder*)pGeometry;
	if ( FAILED( pBuilder->Validate() ) )
		return A3DERROR_INVALID_WALL;

	A3DVECTOR position = { 0.0f, 0.0f, 0.0f, 1.0f };
	A3DVECTOR forward = { 0.0f, 0.0f, 1.0f, 0.0f };
	A3DVECTOR up = { 0.0f, 1.0f, 0.0f, 0.0f };
	if ( !pPosition )
		pPosition = position;
	if ( !pForward )
		pForward = forward;
	if ( !pUp )
		pUp = up;

	if ( fabs( ( (double)pUp[0] * pForward[0] + (double)pForward[2] * pUp[2] ) +
		(double)pForward[1] * pUp[1] ) > A3D_EPSILON ||
		!( ( (double)pForward[1] * pForward[1] + (double)pForward[2] * pForward[2] ) +
		(double)pForward[0] * pForward[0] >= A3D_EPSILON_SQUARED ) ||
		!( ( (double)pUp[1] * pUp[1] + (double)pUp[2] * pUp[2] ) +
		(double)pUp[0] * pUp[0] >= A3D_EPSILON_SQUARED ) )
		return A3DERROR_DIR_AND_UP_VECTORS_NOT_PERPENDICULAR;

	CA3dGeometryFace* pWall = new( std::nothrow ) CA3dGeometryFace( pBuilder, dwFlags,
		pPosition, pForward, pUp );
	pWall->AddRef();
	m_Walls.Append( pWall );
	return m_Walls.m_nCount;
}

BOOL CA3dRoomBuilder::IsUniqueNormal( const A3DVAL* pNormal )
{
	m_Auxiliary.m_pCurrent = m_Auxiliary.m_pFirst;
	for ( int i = m_Auxiliary.m_nCount; i > 0; i-- )
	{
		const A3DVAL* pStored = (const A3DVAL*)m_Auxiliary.Next();
		double flDot = ( (double)pStored[2] * pNormal[2] + (double)pStored[1] * pNormal[1] ) +
			(double)pStored[0] * pNormal[0];
		if ( !( 1.0 - fabs( flDot ) >= A3D_EPSILON ) )
			return FALSE;
	}

	return TRUE;
}

void CA3dRoomBuilder::ClearNormals()
{
	m_Auxiliary.m_pCurrent = m_Auxiliary.m_pFirst;
	for ( int i = m_Auxiliary.m_nCount; i > 0; i-- )
		operator delete( m_Auxiliary.Next() );
	m_Auxiliary.Clear();
}

HRESULT STDMETHODCALLTYPE CA3dRoomBuilder::Build()
{
	int nWalls = m_Walls.m_nCount;
	int nShellWalls = 0;
	m_Walls.m_pCurrent = m_Walls.m_pFirst;
	for ( int i = nWalls; i > 0; i-- )
	{
		CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_Walls.Next();
		if ( pWall->m_dwFlags & A3D_SHELL_WALL )
			nShellWalls++;
	}

	if ( nShellWalls <= 0 )
		return A3DERROR_ROOM_HAS_NO_SHELL_WALLS;
	if ( nShellWalls < A3D_ROOM_MIN_SHELL_WALLS )
		return A3DERROR_ROOM_HAS_LESS_THAN_4SHELL_WALLS;

	ClearNormals();
	m_Walls.m_pCurrent = m_Walls.m_pFirst;
	for ( int i = m_Walls.m_nCount; i > 0; i-- )
	{
		CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_Walls.Next();
		if ( pWall->m_dwFlags & A3D_SHELL_WALL )
		{
			A3DVECTOR normal;
			pWall->TransformPlane( normal );
			if ( IsUniqueNormal( normal ) )
			{
				A3DVECTOR* pNormal = (A3DVECTOR*)operator new( sizeof( A3DVECTOR ), std::nothrow );
				memcpy( pNormal, normal, sizeof( *pNormal ) );
				m_Auxiliary.Append( pNormal );
			}
		}
	}

	if ( m_Auxiliary.m_nCount < A3D_ROOM_MIN_UNIQUE_NORMALS )
		return A3DERROR_ROOM_HAS_LESS_THAN_3UNIQUE_NORMALS;

	m_bPointTesting = TRUE;
	CA3dObjectList walls = {};
	try
	{
		m_Walls.m_pCurrent = m_Walls.m_pFirst;
		walls.Clear();
		for ( int i = m_Walls.m_nCount; i > 0; i-- )
			walls.Append( m_Walls.Next() );

		m_Walls.m_pCurrent = m_Walls.m_pFirst;
		for ( DWORD i = (DWORD)nWalls; i; i-- )
		{
			CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_Walls.Next();
			if ( pWall->m_dwFlags & A3D_SHELL_WALL )
			{
				pWall->ClearEdgeLinks();
				walls.m_pCurrent = walls.m_pFirst;
				for ( DWORD j = (DWORD)walls.m_nCount; j; j-- )
				{
					CA3dGeometryFace* pOtherWall = (CA3dGeometryFace*)walls.Next();
					if ( ( pOtherWall->m_dwFlags & A3D_SHELL_WALL ) && pWall != pOtherWall &&
						pWall->ConnectEdges( pOtherWall ) == A3D_ROOM_DISABLE_POINT_TESTING )
					{
						m_bPointTesting = FALSE;
						walls.Clear();
						return S_OK;
					}
				}
			}
		}

		m_Walls.m_pCurrent = m_Walls.m_pFirst;
		for ( DWORD i = (DWORD)m_Walls.m_nCount; i; i-- )
		{
			CA3dGeometryFace* pWall = (CA3dGeometryFace*)m_Walls.Next();
			if ( ( pWall->m_dwFlags & A3D_SHELL_WALL ) && pWall->HasEdgeConflict() )
			{
				m_bPointTesting = FALSE;
				walls.Clear();
				return S_OK;
			}
		}

		walls.Clear();
	}
	catch ( ... )
	{
		walls.Clear();
		throw;
	}

	return S_OK;
}
