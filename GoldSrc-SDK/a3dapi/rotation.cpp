#include "rotation.h"
#include "matrix.h"
#include "vector_math.h"
#include "A3d3.h"
#include "WallEdge.h"
#include "A3dGeom.h"

#include <math.h>
#include <string.h>

#define DEGREES_TO_RADIANS	0.017453292f

void __cdecl Matrix_Rotate( float angle, float axis[3], float matrix[16] )
{
	double lengthSquared = ( (double)axis[1] * axis[1] + (double)axis[2] * axis[2] ) +
		(double)axis[0] * axis[0];
	double radians = (double)angle * DEGREES_TO_RADIANS;
	double axisLength = sqrt( lengthSquared );
	float sine = RoundToFloat( sin( radians ) );
	float cosine = RoundToFloat( cos( (double)RoundToFloat( radians ) ) );
	float length = RoundToFloat( axisLength );

	if ( !( axisLength > 0.0 ) )
		return;

	double normalizedZ = (double)axis[2] / length;
	double normalizedY = (double)axis[1] / length;
	double normalizedX = (double)axis[0] / length;
	float z = RoundToFloat( normalizedZ );
	float y = RoundToFloat( normalizedY );
	float yz = RoundToFloat( normalizedZ * y );
	float x = RoundToFloat( normalizedX );
	float xy = RoundToFloat( y * x );
	float xx = RoundToFloat( x * x );
	float yy = RoundToFloat( y * y );
	double zz = (double)z * z;
	float sineX = RoundToFloat( sine * x );
	float sineY = RoundToFloat( sine * y );
	float sineZ = RoundToFloat( sine * z );
	double oneMinusCosine = 1.0 - cosine;
	float oneMinusCosineXY = RoundToFloat( oneMinusCosine * xy );
	float oneMinusCosineYZ = RoundToFloat( oneMinusCosine * yz );
	float rotation[16];

	axis[0] = x;
	axis[1] = y;
	axis[2] = z;

	rotation[0] = (float)( oneMinusCosine * xx + cosine );
	rotation[1] = (float)( (double)sine * z + oneMinusCosineXY );
	rotation[2] = oneMinusCosineYZ - sineY;
	rotation[3] = 0.0f;
	rotation[4] = (float)( oneMinusCosine * xy - sineZ );
	rotation[5] = (float)( oneMinusCosine * yy + cosine );
	rotation[6] = sineX + oneMinusCosineYZ;
	rotation[7] = 0.0f;
	rotation[8] = (float)( (double)sine * y + oneMinusCosineYZ );
	rotation[9] = (float)( oneMinusCosine * yz - sineX );
	rotation[10] = (float)( oneMinusCosine * zz + cosine );
	rotation[11] = 0.0f;
	rotation[12] = 0.0f;
	rotation[13] = 0.0f;
	rotation[14] = 0.0f;
	rotation[15] = 1.0f;

	Matrix_Multiply( matrix, rotation, matrix );
}

HRESULT STDMETHODCALLTYPE CA3d::Rotate3fv( A3DVAL flAngle, LPA3DVAL pAxis )
{
	A3DMATRIX rotation =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	double radians = (double)flAngle * DEGREES_TO_RADIANS;
	float sine = RoundToFloat( sin( radians ) );
	double cosine = cos( radians );
	const A3DVAL one = 1.0f;

	if ( !memcmp( &pAxis[0], &one, sizeof( one ) ) &&
		!( pAxis[1] < 0.0f || pAxis[1] > 0.0f ) &&
		!( pAxis[2] < 0.0f || pAxis[2] > 0.0f ) )
	{
		rotation[5] = (float)cosine;
		rotation[6] = sine;
		rotation[9] = -sine;
		rotation[10] = (float)cosine;
	}
	else if ( !( pAxis[0] < 0.0f || pAxis[0] > 0.0f ) &&
		!memcmp( &pAxis[1], &one, sizeof( one ) ) &&
		!( pAxis[2] < 0.0f || pAxis[2] > 0.0f ) )
	{
		rotation[0] = (float)cosine;
		rotation[2] = -sine;
		rotation[8] = sine;
		rotation[10] = (float)cosine;
	}
	else if ( !( pAxis[0] < 0.0f || pAxis[0] > 0.0f ) &&
		!( pAxis[1] < 0.0f || pAxis[1] > 0.0f ) &&
		!memcmp( &pAxis[2], &one, sizeof( one ) ) )
	{
		rotation[0] = (float)cosine;
		rotation[1] = sine;
		rotation[4] = -sine;
		rotation[5] = (float)cosine;
	}
	else
	{
		double lengthSquared = ( (double)pAxis[1] * pAxis[1] + (double)pAxis[2] * pAxis[2] ) +
			(double)pAxis[0] * pAxis[0];
		double axisLength = sqrt( lengthSquared );
		float length = RoundToFloat( axisLength );

		if ( !( axisLength > 0.0 ) )
			return E_FAIL;

		double normalizedZ = (double)pAxis[2] / length;
		double normalizedY = (double)pAxis[1] / length;
		double normalizedX = (double)pAxis[0] / length;
		float z = RoundToFloat( normalizedZ );
		float y = RoundToFloat( normalizedY );
		float yz = RoundToFloat( normalizedZ * y );
		float x = RoundToFloat( normalizedX );
		double oneMinusCosine = 1.0 - cosine;
		float roundedOneMinusCosine = RoundToFloat( oneMinusCosine );
		float xy = RoundToFloat( y * x );
		float xx = RoundToFloat( x * x );
		float yy = RoundToFloat( y * y );
		float zz = RoundToFloat( z * z );
		float sineX = RoundToFloat( sine * x );
		float sineY = RoundToFloat( sine * y );
		float sineZ = RoundToFloat( sine * z );
		float oneMinusCosineXY = RoundToFloat( oneMinusCosine * xy );
		float oneMinusCosineYZ = RoundToFloat( roundedOneMinusCosine * yz );

		pAxis[0] = x;
		pAxis[1] = y;
		pAxis[2] = z;

		rotation[0] = (float)( (double)roundedOneMinusCosine * xx + cosine );
		rotation[1] = (float)( (double)sine * z + oneMinusCosineXY );
		rotation[2] = oneMinusCosineYZ - sineY;
		rotation[4] = (float)( oneMinusCosine * xy - sineZ );
		rotation[5] = (float)( (double)roundedOneMinusCosine * yy + cosine );
		rotation[6] = sineX + oneMinusCosineYZ;
		rotation[8] = (float)( (double)sine * y + oneMinusCosineYZ );
		rotation[9] = (float)( (double)roundedOneMinusCosine * yz - sineX );
		rotation[10] = (float)( (double)roundedOneMinusCosine * zz + cosine );
	}

	Matrix_Multiply( m_Matrix[m_nMatrix], rotation, m_Matrix[m_nMatrix] );
	return S_OK;
}

void CA3dGeometryEdge::TransformEndpoints( A3DVERTEX* pOut )
{
	if ( m_pFace && pOut )
		m_pFace->m_Transform.TransformLine( pOut, m_EndPoints );
}

BOOL CA3dGeometryEdge::HasConflict()
{
	CA3dObjectList edges = {};
	A3DVAL flCoveredLength = 0;
	BOOL bConflict = FALSE;
	A3DVERTEX current[2];
	A3DVERTEX other[2];
	int nEdgesToCopy = m_AdjacentEdges.m_nCount;
	m_AdjacentEdges.m_pCurrent = m_AdjacentEdges.m_pFirst;

	try
	{
		edges.Clear();
		for ( int i = nEdgesToCopy; i > 0; i-- )
			edges.Append( m_AdjacentEdges.Next() );

		m_AdjacentEdges.m_pCurrent = m_AdjacentEdges.m_pFirst;
		for ( int i = m_AdjacentEdges.m_nCount; i > 0; i-- )
		{
			CA3dGeometryEdge* pEdge = (CA3dGeometryEdge*)m_AdjacentEdges.Next();
			pEdge->TransformEndpoints( current );
			edges.m_pCurrent = edges.m_pFirst;

			for ( int j = edges.m_nCount; j > 0; j-- )
			{
				CA3dGeometryEdge* pOther = (CA3dGeometryEdge*)edges.Next();
				if ( pOther == pEdge )
					continue;

				pOther->TransformEndpoints( other );
				if ( Geometry_SameSegment( current, other ) )
					continue;

				BOOL bOtherStart = Geometry_PointOnSegment( other[0], current );
				BOOL bOtherEnd = Geometry_PointOnSegment( other[1], current );
				if ( bOtherStart && bOtherEnd )
					continue;

				BOOL bCurrentStart = Geometry_PointOnSegment( current[0], other );
				BOOL bCurrentEnd = Geometry_PointOnSegment( current[1], other );
				if ( ( bCurrentStart && bCurrentEnd ) || ( bOtherStart && bCurrentStart ) ||
					( bOtherEnd && bCurrentEnd ) )
					continue;

				bConflict = TRUE;
				break;
			}

			if ( bConflict )
				break;

			double flDeltaX = (double)current[0][0] - current[1][0];
			double flDeltaY = (double)current[0][1] - current[1][1];
			double flDeltaZ = (double)current[0][2] - current[1][2];
			double flLengthSquared = ( flDeltaX * RoundToFloat( flDeltaX ) + flDeltaY * RoundToFloat( flDeltaY ) ) +
				flDeltaZ * flDeltaZ;
			flCoveredLength = (A3DVAL)( sqrt( flLengthSquared ) + flCoveredLength );
		}

		if ( !bConflict )
		{
			double flDeltaX = (double)m_EndPoints[0][0] - m_EndPoints[1][0];
			double flDeltaY = (double)m_EndPoints[0][1] - m_EndPoints[1][1];
			double flDeltaZ = (double)m_EndPoints[0][2] - m_EndPoints[1][2];
			double flLengthSquared = ( flDeltaX * RoundToFloat( flDeltaX ) + flDeltaY * RoundToFloat( flDeltaY ) ) +
				flDeltaZ * flDeltaZ;
			bConflict = fabs( flCoveredLength - sqrt( flLengthSquared ) ) > A3D_EPSILON;
		}
	}
	catch ( ... )
	{
		edges.Clear();
		throw;
	}

	edges.Clear();
	return bConflict;
}
