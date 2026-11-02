#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <unknwn.h>

#include "ia3dapi.h"
#include "listener.h"
#include "LinkList.h"

#define A3D_PRIMITIVE_COMPAT1009	0x10
#define A3D_NAME_LENGTH			256

class CA3d;
class CA3dCommandBuffer;
class CA3dGeometryEdge;
class CA3dGeometryFace;
class CA3dOpening;
class CA3dOpeningBuilder;
class CA3dRoom;
class CA3dRoomBuilder;
class CA3dWallBuilder;
struct IA3dMaterial;

typedef struct a3d_primitive_s
{
	DWORD		m_dwTag;
	BYTE		m_nVertices;
	BYTE		m_Padding[3];
	A3DVERTEX	m_Vertices[4];
	A3DVECTOR	m_Normal;
	A3DVAL		m_flDistance;
	DWORD		m_dwEdgeFlags;
} a3d_primitive_t;

typedef union a3d_geometry_payload_u
{
	a3d_primitive_t	m_Primitive;
	A3DVAL			m_MaterialProperties[6];
	void*			m_pObject;
} a3d_geometry_payload_t;

typedef struct a3d_geometry_command_s
{
	DWORD					m_dwOpcode;
	a3d_geometry_payload_t	m_Payload;
	LPA3DVAL				m_pOpeningFactor;
	A3DVAL					m_flOpeningFactor;
} a3d_geometry_command_t;

typedef a3d_primitive_t a3d_polygon_t;

typedef struct a3d_edge_s
{
	A3DVERTEX	vertices[2];
} a3d_edge_t;

struct CPlex
{
	CPlex*	pNext;

	static CPlex* __stdcall Create( CPlex** ppBlocks, DWORD nItems, DWORD dwItemSize );
	void FreeDataChain();
};

enum
{
	TRIANGLE_POINT_BOUNDARY	= 0,
	TRIANGLE_POINT_INTERIOR	= 1,
	TRIANGLE_POINT_OUTSIDE	= 2
};

class CA3dTransform : public CA3dPrivateUnknown
{
public:
	CA3dTransform();

	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE LoadMatrix( const A3DVAL* pMatrix );
	virtual HRESULT STDMETHODCALLTYPE GetMatrix( A3DVAL* pMatrix );
	virtual HRESULT STDMETHODCALLTYPE Translate( const A3DVAL* pOffset );
	virtual HRESULT STDMETHODCALLTYPE Rotate( A3DVAL* pAxis, A3DVAL angle );
	virtual HRESULT STDMETHODCALLTYPE Scale( A3DVAL x, A3DVAL y, A3DVAL z );
	virtual ~CA3dTransform();

	void SetOwner( void* pOwner );
	void SetFrame( void* pOwner, const A3DVAL* pPosition, const A3DVAL* pForward, const A3DVAL* pUp );
	CA3dTransform* CopyFrom( void* pOwner, const CA3dTransform* pTransform );
	void CopyMatrixFrom( const CA3dTransform* pTransform );

	HRESULT GetOrientation( CA3dTransform* pReference, A3DVAL* pForward, A3DVAL* pUp );
	HRESULT SetOrientation( CA3dTransform* pReference, const A3DVAL* pForward, const A3DVAL* pUp );
	HRESULT SetPosition( CA3dTransform* pReference, const A3DVAL* pPosition );
	HRESULT GetPosition( CA3dTransform* pReference, A3DVAL* pPosition );

	void TransformVector( A3DVECTOR vector );
	void TransformVector( A3DVECTOR out, const A3DVECTOR in );
	void TransformPolygon( a3d_polygon_t* pOut, const a3d_polygon_t* pPolygon );
	void TransformLine( A3DVERTEX* pOut, const A3DVERTEX* pLine );

private:
	friend class CA3dOpening;
	friend class CA3dGeometryFace;

	void AccumulateParentMatrices( A3DMATRIX matrix );
	void TransformToLocal( A3DVECTOR out, const A3DVECTOR in );
	void GetWorldPosition( A3DVECTOR out );

	ULONG			m_cReferences;
	A3DMATRIX		m_Matrix;
	void*			m_pOwner;
	CA3dTransform*	m_pParent;
};

class CA3dMaterialReference
{
public:
	CA3dMaterialReference();
	virtual ~CA3dMaterialReference();

	HRESULT Set( IA3dMaterial* pMaterial );
	HRESULT Clone( IA3dMaterial** ppMaterial );
	HRESULT Copy( const CA3dMaterialReference* pReference );

protected:
	IA3dMaterial*	m_pMaterial;
};

class CA3dName
{
public:
	CA3dName();
	virtual ~CA3dName();

	HRESULT Format( const char* pszName, int nId );
	HRESULT Set( const char* pszName );
	HRESULT Get( char* pszName, DWORD dwSize );

private:
	char	m_szName[A3D_NAME_LENGTH];
};

class CA3dGeometryInstance
{
public:
	CA3dGeometryInstance();
	virtual ~CA3dGeometryInstance();

	void Clear();
	void TransformPolygons( CA3dObjectList* pPolygons, CA3dTransform* pTransform, const A3DVAL* pMatrix );
	void CopyPolygons( CA3dObjectList* pPolygons );

protected:
	CA3dTransform	m_Transform;
	CA3dObjectList	m_Polygons;
	A3DVECTOR		m_Plane;

	friend class CA3dWallBuilder;
	friend class CA3dOpening;
	friend class CA3dOpeningBuilder;
	friend class CA3dRoom;
};

class CA3dGeometryBuilder
{
public:
	CA3dGeometryBuilder( int nSceneType );
	virtual ~CA3dGeometryBuilder();

	HRESULT Begin( DWORD dwMode );
	HRESULT End();
	HRESULT AddVertex3f( A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT AddVertex3fv( const A3DVAL* pVertex );
	HRESULT RemovePolygon( int nPolygon );
	int GetPolygonCount();
	HRESULT SetVertex( int nPolygon, int nVertex, A3DVAL x, A3DVAL y, A3DVAL z );
	HRESULT GetVertex( int nPolygon, int nVertex, A3DVAL* pVertex );
	HRESULT Clear();
	HRESULT Validate();

protected:
	void FinishPolygon();
	HRESULT CheckCoplanarity();
	void SetEdgeFlag( a3d_polygon_t* pPolygon, DWORD dwEdge, BOOL bSet );
	void AddPolygonEdges( a3d_polygon_t* pPolygon );
	void AddEdge( const a3d_polygon_t* pPolygon, int nStart, int nEnd );
	void ClearEdges();
	void ClearPolygons();
	void CheckEdgeIntersections();
	void CheckEdgeIntersection( const a3d_polygon_t* pPolygon, int nStart, int nEnd,
			const a3d_edge_t* pEdge );

	CA3dObjectList	m_Polygons;
	CA3dObjectList	m_Edges;
	a3d_polygon_t	m_Polygon;
	int				m_nVertex;
	BOOL			m_bValidated;
	int				m_nSceneType;
	A3DVAL*			m_pNormal;
	a3d_polygon_t*	m_pFirstPolygon;

	friend class CA3dOpening;
	friend class CA3dWallBuilder;
	friend class CA3dOpeningBuilder;
};

class CA3dGeometryFace : public CA3dPrivateUnknown, public CA3dGeometryInstance, public CA3dMaterialReference
{
public:
	CA3dGeometryFace( CA3dWallBuilder* pBuilder, DWORD dwFlags, const A3DVAL* pPosition,
			const A3DVAL* pForward, const A3DVAL* pUp );
	CA3dGeometryFace( CA3dRoom* pRoom, CA3dGeometryFace* pFace );
	~CA3dGeometryFace();

	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE GetParent( IUnknown** ppParent );
	virtual HRESULT STDMETHODCALLTYPE SetMaterial( IA3dMaterial* pMaterial );
	virtual HRESULT STDMETHODCALLTYPE GetMaterial( IA3dMaterial** ppMaterial );
	virtual HRESULT STDMETHODCALLTYPE GetOpening( int nOpening, IUnknown** ppOpening );
	virtual HRESULT STDMETHODCALLTYPE SetPosition( CA3dTransform* pFrame, const A3DVAL* pPosition );
	virtual HRESULT STDMETHODCALLTYPE GetPosition( CA3dTransform* pFrame, A3DVAL* pPosition );
	virtual HRESULT STDMETHODCALLTYPE SetOrientation( CA3dTransform* pFrame, const A3DVAL* pDirection,
			const A3DVAL* pUp );
	virtual HRESULT STDMETHODCALLTYPE GetOrientation( CA3dTransform* pFrame, A3DVAL* pDirection, A3DVAL* pUp );
	virtual HRESULT STDMETHODCALLTYPE SetTransform( CA3dTransform* pTransform );
	virtual HRESULT STDMETHODCALLTYPE GetTransform( CA3dTransform** ppTransform );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperation( DWORD argument );

	void TransformPlane( A3DVAL* pPlane );
	void ClearOpenings();
	void ClearEdges();
	void ClearEdgeLinks();
	void CopyEdges( CA3dObjectList* pEdges );
	void CopyOpenings( CA3dObjectList* pOpenings );
	BOOL IsAdjacentTo( CA3dGeometryFace* pFace );
	HRESULT ConnectTo( CA3dGeometryFace* pFace );
	BOOL HasEdgeConflict();
	int ConnectEdges( CA3dGeometryFace* pFace );
	void Render( CA3d* pA3d );
	void RebuildOpenings();

private:
	friend class CA3dGeometryEdge;
	friend class CA3dOpening;
	friend class CA3dRoom;
	friend class CA3dRoomBuilder;

	BOOL HasPolygonWithin( CA3dGeometryFace* pFace );
	BOOL IsWithin( CA3dGeometryFace* pFace );
	BOOL ContainsOpening( CA3dOpening* pOpening );
	BOOL IntersectsSegment( const A3DVERTEX* pEndpoints );

	ULONG				m_cReferences;
	CA3dObjectList		m_Edges;
	CA3dObjectList		m_Openings;
	CA3dObjectList		m_AdjacentFaces;
	DWORD				m_dwFlags;
	CA3dRoom*			m_pRoom;
};

class IA3dOpeningTarget : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetParent( IUnknown** ppParent ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOpeningFactor( A3DVAL flFactor ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetOpeningFactor( A3DVAL* pFactor ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPosition( CA3dTransform* pReference, const A3DVAL* pPosition ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPosition( CA3dTransform* pReference, A3DVAL* pPosition ) = 0;
};

class CA3dOpening : public IA3dOpeningTarget, public CA3dGeometryInstance
{
public:
	CA3dOpening( CA3dOpeningBuilder* pBuilder, const A3DVAL* pPosition, const A3DVAL* pForward,
			const A3DVAL* pUp );
	CA3dOpening( CA3dOpening* pOpening, CA3dGeometryFace* pFace );
	CA3dOpening( CA3dOpening* pOpening, CA3dGeometryFace* pFace, CA3dGeometryFace* pOtherFace );
	~CA3dOpening();

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE GetParent( IUnknown** ppParent );
	HRESULT STDMETHODCALLTYPE SetOpeningFactor( A3DVAL flFactor );
	HRESULT STDMETHODCALLTYPE GetOpeningFactor( A3DVAL* pFactor );
	HRESULT STDMETHODCALLTYPE SetPosition( CA3dTransform* pReference, const A3DVAL* pPosition );
	HRESULT STDMETHODCALLTYPE GetPosition( CA3dTransform* pReference, A3DVAL* pPosition );

	void Render( CA3d* pA3d );

private:
	friend class CA3dGeometryFace;
	friend class CA3dWallBuilder;

	ULONG			m_cReferences;
	IUnknown*		m_pParent;
	DWORD			m_dwInstanceMode;
	CA3dOpening*	m_pLinkedOpening;
	A3DVAL			m_flOpeningFactor;
};

BOOL	A3dIntersectTriangle( const A3DVAL* pStart, const A3DVAL* pDirection, a3d_primitive_t* pPrimitive,
		BOOL bExpandEdges, BOOL bRay );
BOOL	A3dIntersectQuad( const A3DVAL* pStart, const A3DVAL* pDirection, a3d_primitive_t* pPrimitive,
		BOOL bExpandEdges, BOOL bRay );
int	__cdecl Geometry_ClassifyTrianglePoint( const A3DVAL* pPoint, const A3DVAL* pNormal,
			const A3DVERTEX* pVertices );
BOOL	__cdecl Geometry_PointOnSegment( const A3DVAL* pPoint, const A3DVERTEX* pEndpoints );
BOOL	__cdecl Geometry_PointOnSegmentProjection( const A3DVAL* pPoint, int firstComponent,
			int secondComponent, const A3DVERTEX* pEndpoints );
BOOL	__cdecl Geometry_SameSegment( const A3DVERTEX* pFirst, const A3DVERTEX* pSecond );
