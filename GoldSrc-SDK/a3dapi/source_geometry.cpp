#include "A3d3.h"
#include "A3dSource.h"
#include "ChunkPage.h"
#include "matrix.h"
#include "matrix_inverse.h"
#include "vector_math.h"

#include <string.h>

#define A3D_INTERSECTION_EDGE_LIMIT			1.000001f
#define A3D_MATERIAL_TRANSMITTANCE_INDEX	2

void CA3d::RenderSources()
{
	if ( m_Sources.m_nCount < 1 )
		return;

	int nReflectionSources = 0;
	int nSources = m_Sources.m_nCount;
	m_Sources.m_pCurrent = m_Sources.m_pFirst;
	for ( int i = 0; i < nSources; i++ )
	{
		CA3dSource* pSource = (CA3dSource*)m_Sources.Next();
		if ( pSource->m_nPlaybackState && pSource->m_bWithinReflectionRange &&
			( pSource->m_dwRenderMode & A3DSOURCE_RENDERMODE_1ST_REFLECTIONS ) )
		{
			++nReflectionSources;
		}
	}

	A3DMATRIX leftListener;
	A3DMATRIX centerListener;
	A3DMATRIX rightListener;
	A3DMATRIX inverseLeft;
	A3DMATRIX inverseCenter;
	A3DMATRIX inverseRight;
	A3DVAL listenerVelocity[3];
	GetListenerMatrices( leftListener, centerListener, rightListener );
	Matrix_Inverse( leftListener, inverseLeft );
	Matrix_Inverse( centerListener, inverseCenter );
	Matrix_Inverse( rightListener, inverseRight );
	Matrix_GetTranslation( centerListener, m_vWorldListenerPosition );
	GetListenerVelocity( listenerVelocity );
	DWORD dwTick = timeGetTime();
	BOOL bUpdate = dwTick - m_dwLastUpdateTick >= m_dwUpdateTickPeriod;
	if ( bUpdate )
		m_dwLastUpdateTick = dwTick;

	nSources = m_Sources.m_nCount;
	m_Sources.m_pCurrent = m_Sources.m_pFirst;
	for ( int i = 0; i < nSources; i++ )
	{
		CA3dSource* pSource = (CA3dSource*)m_Sources.Next();
		if ( !pSource->m_nPlaybackState )
			continue;

		A3DMATRIX sourceWorld;
		A3DMATRIX sourceCenter;
		A3DMATRIX sourceLeft;
		A3DMATRIX sourceRight;
		A3DVAL centerPosition[3];
		A3DVAL leftPosition[3];
		A3DVAL rightPosition[3];
		A3DVAL leftSpherical[3];
		A3DVAL rightSpherical[3];
		A3DVAL centerSpherical[3];
		A3DVAL sourceVelocity[3];
		pSource->GetWorldTransform( sourceWorld );
		Matrix_Multiply( inverseCenter, sourceWorld, sourceCenter );
		Matrix_Multiply( inverseLeft, sourceWorld, sourceLeft );
		Matrix_Multiply( inverseRight, sourceWorld, sourceRight );
		Matrix_GetTranslation( sourceWorld, pSource->m_vRenderPosition );
		Matrix_GetTranslation( sourceCenter, centerPosition );
		Matrix_GetTranslation( sourceLeft, leftPosition );
		Matrix_GetTranslation( sourceRight, rightPosition );
		Vector_ToSphericalRadians( leftPosition, leftSpherical );
		Vector_ToSphericalRadians( rightPosition, rightSpherical );
		for ( int axis = 0; axis < 3; axis++ )
		{
			centerSpherical[axis] = (A3DVAL)( ( (double)rightSpherical[axis] + leftSpherical[axis] ) * 0.5f );
			m_vSourceToListener[axis] = pSource->m_vRenderPosition[axis] - m_vWorldListenerPosition[axis];
		}
		pSource->UpdateStereoSpread( centerSpherical );
		pSource->GetWorldVelocity( sourceVelocity );
		pSource->UpdateDoppler( sourceVelocity, listenerVelocity, m_vSourceToListener, centerSpherical[2] );
		pSource->UpdateConeGain( m_vSourceToListener );
		pSource->UpdateDistanceModel( leftSpherical[2], rightSpherical[2] );

		BOOL bUpdateOcclusion = FALSE;
		if ( bUpdate )
		{
			pSource->m_iUpdateGroup = pSource->m_nPlaybackCounter % m_dwReflectionUpdateModulus == 0;
			++pSource->m_nPlaybackCounter;
			bUpdateOcclusion = pSource->m_nPlaybackCounter % m_dwOcclusionUpdateModulus == 0;
		}
		else
		{
			pSource->m_iUpdateGroup = 0;
		}
		if ( pSource->m_iUpdateGroup )
			pSource->BeginReflectionFrame();

		DWORD dwRenderMode = pSource->m_dwRenderMode;
		if ( !pSource->m_bWithinReflectionRange )
			pSource->m_dwRenderMode &= ~A3DSOURCE_RENDERMODE_1ST_REFLECTIONS;

		if ( m_bGeometryInitialized )
		{
			m_flOcclusionFactor = 0;
			m_vOcclusionGain[0] = 1;
			m_vOcclusionGain[1] = 1;
			if ( ( m_dwEnabledFeatures & A3D_OCCLUSIONS ) &&
				( pSource->m_dwRenderMode & A3DSOURCE_RENDERMODE_OCCLUSIONS ) )
			{
				if ( bUpdateOcclusion )
				{
					m_nRemainingGeometryPools = m_GeometryPools.m_nCount;
					m_GeometryPools.m_pCurrent = m_GeometryPools.m_pFirst;
					m_pCommandBuffer = (CA3dCommandBuffer*)m_GeometryPools.Next();
					m_pCommandBuffer->m_iCursor = 0;
					m_dwPolygonTestCount = 0;
					m_nRemainingGeometryPools = (LONG)( (DWORD)m_nRemainingGeometryPools - 1 );
					do
					{
						m_pCurrentCommand = m_pCommandBuffer->ReadCommand();
						if ( !m_pCurrentCommand )
						{
							LONG nRemaining = m_nRemainingGeometryPools;
							m_nRemainingGeometryPools = (LONG)( (DWORD)nRemaining - 1 );
							if ( nRemaining > 0 )
							{
								m_pCommandBuffer = (CA3dCommandBuffer*)m_GeometryPools.Next();
								m_pCommandBuffer->m_iCursor = 0;
								m_pCurrentCommand = m_pCommandBuffer->ReadCommand();
							}
						}
						if ( !m_pCurrentCommand )
							break;

						m_flClosedFraction = 1;
						if ( m_pCurrentCommand->m_dwOpcode == A3D_MATERIAL )
						{
							memcpy( m_MaterialProperties, m_pCurrentCommand->m_Payload.m_MaterialProperties,
								sizeof( m_MaterialProperties ) );
							continue;
						}

						BOOL bHit = FALSE;
						if ( m_dwOcclusionMode == A3D_FASTEST )
						{
							if ( m_pCurrentCommand->m_dwOpcode == A3D_TRIANGLES )
								bHit = A3dIntersectTriangle( m_vWorldListenerPosition, m_vSourceToListener,
									&m_pCurrentCommand->m_Payload.m_Primitive, TRUE, FALSE );
							else if ( m_pCurrentCommand->m_dwOpcode == A3D_QUADS )
								bHit = A3dIntersectQuad( m_vWorldListenerPosition, m_vSourceToListener,
									&m_pCurrentCommand->m_Payload.m_Primitive, TRUE, FALSE );
						}
						BOOL bSolidWall = FALSE;
						if ( bHit && !( m_pCurrentCommand->m_Payload.m_Primitive.m_flDistance >=
							m_flMaxOcclusionRayFraction ) )
						{
							for ( ;; )
							{
								m_pCurrentCommand = m_pCommandBuffer->ReadCommand();
								if ( !m_pCurrentCommand )
								{
									LONG nRemaining = m_nRemainingGeometryPools;
									m_nRemainingGeometryPools = (LONG)( (DWORD)nRemaining - 1 );
									if ( nRemaining > 0 )
									{
										m_pCommandBuffer = (CA3dCommandBuffer*)m_GeometryPools.Next();
										m_pCommandBuffer->m_iCursor = 0;
										m_pCurrentCommand = m_pCommandBuffer->ReadCommand();
									}
								}
								if ( !m_pCurrentCommand ||
									( m_pCurrentCommand->m_dwOpcode != A3D_SUB_TRIANGLES &&
									m_pCurrentCommand->m_dwOpcode != A3D_SUB_QUADS ) )
								{
									m_flOcclusionFactor = 1;
									memcpy( m_vOcclusionGain, &m_MaterialProperties[A3D_MATERIAL_TRANSMITTANCE_INDEX],
										sizeof( m_vOcclusionGain ) );
									bSolidWall = TRUE;
									break;
								}

								if ( m_pCurrentCommand->m_dwOpcode == A3D_SUB_TRIANGLES )
									bHit = A3dIntersectTriangle( m_vWorldListenerPosition, m_vSourceToListener,
										&m_pCurrentCommand->m_Payload.m_Primitive, TRUE, FALSE );
								else
									bHit = A3dIntersectQuad( m_vWorldListenerPosition, m_vSourceToListener,
										&m_pCurrentCommand->m_Payload.m_Primitive, TRUE, FALSE );
								if ( bHit )
								{
									A3DVAL flOpening = m_pCurrentCommand->m_pOpeningFactor ?
										*m_pCurrentCommand->m_pOpeningFactor : m_pCurrentCommand->m_flOpeningFactor;
									m_flClosedFraction = 1.0f - flOpening;
									if ( !( flOpening >= 1 ) )
									{
										m_flOcclusionFactor = m_flClosedFraction;
										m_vOcclusionGain[0] = m_flClosedFraction;
										m_vOcclusionGain[1] = m_flClosedFraction;
										break;
									}
								}
							}
						}
						if ( bSolidWall )
							break;
						++m_dwPolygonTestCount;
					}
					while ( m_dwPolygonTestCount <= m_dwPolygonTestLimit );
					pSource->m_flCachedOcclusionFactor = m_flOcclusionFactor;
					memcpy( pSource->m_flCachedOcclusionGain, m_vOcclusionGain, sizeof( m_vOcclusionGain ) );
				}
				else
				{
					m_flOcclusionFactor = pSource->m_flCachedOcclusionFactor;
					memcpy( m_vOcclusionGain, pSource->m_flCachedOcclusionGain, sizeof( m_vOcclusionGain ) );
				}
			}
			pSource->UpdateOcclusion( m_flOcclusionFactor, m_vOcclusionGain );
			if ( ( m_dwEnabledFeatures & A3D_1ST_REFLECTIONS ) && pSource->m_iUpdateGroup &&
				( pSource->m_dwRenderMode & A3DSOURCE_RENDERMODE_1ST_REFLECTIONS ) )
			{
				RenderSourceGeometry( pSource );
				pSource->PruneReflections( nReflectionSources );
			}
		}

		pSource->UpdateRenderState( leftSpherical, rightSpherical );
		if ( ( m_dwEnabledFeatures & A3D_1ST_REFLECTIONS ) &&
			( pSource->m_dwRenderMode & A3DSOURCE_RENDERMODE_1ST_REFLECTIONS ) )
		{
			if ( pSource->m_iUpdateGroup )
				pSource->UpdateReflections( nReflectionSources );
			else
				pSource->ReuseReflections();
		}
		else
		{
			pSource->DisableReflectionChannels();
		}
		pSource->m_iUpdateGroup = pSource->m_iUpdateGroup == 0;
		pSource->SubmitRenderState();
		pSource->StartPendingPlayback();
		double flDistanceSquared = ( (double)m_vSourceToListener[0] * m_vSourceToListener[0] +
			(double)m_vSourceToListener[1] * m_vSourceToListener[1] ) +
			(double)m_vSourceToListener[2] * m_vSourceToListener[2];
		pSource->m_bWithinReflectionRange = !( flDistanceSquared >= m_flMaxReflectionDistanceSquared );
		pSource->m_dwRenderMode = dwRenderMode;
	}
}

void CA3d::RenderSourceGeometry( CA3dSource* pSource )
{
	m_nRemainingGeometryPools = m_GeometryPools.m_nCount;
	m_GeometryPools.m_pCurrent = m_GeometryPools.m_pFirst;
	m_pCommandBuffer = (CA3dCommandBuffer*)m_GeometryPools.Next();
	m_pCommandBuffer->m_iCursor = 0;
	m_dwPolygonTestCount = 0;
	m_nRemainingGeometryPools = (LONG)( (DWORD)m_nRemainingGeometryPools - 1 );

	for ( ;; )
	{
		m_pCurrentCommand = m_pCommandBuffer->ReadCommand();
		if ( !m_pCurrentCommand )
		{
			LONG nRemaining = m_nRemainingGeometryPools;
			m_nRemainingGeometryPools = (LONG)( (DWORD)nRemaining - 1 );
			if ( nRemaining > 0 )
			{
				m_pCommandBuffer = (CA3dCommandBuffer*)m_GeometryPools.Next();
				m_pCommandBuffer->m_iCursor = 0;
				m_pCurrentCommand = m_pCommandBuffer->ReadCommand();
			}
		}
		if ( !m_pCurrentCommand )
			return;

		if ( m_pCurrentCommand->m_dwOpcode == A3D_MATERIAL )
		{
			memcpy( m_MaterialProperties, m_pCurrentCommand->m_Payload.m_MaterialProperties,
				sizeof( m_MaterialProperties ) );
			continue;
		}

		if ( m_pCurrentCommand->m_dwOpcode == A3D_TRIANGLES ||
			m_pCurrentCommand->m_dwOpcode == A3D_QUADS )
		{
			a3d_primitive_t* pPrimitive = &m_pCurrentCommand->m_Payload.m_Primitive;
			const A3DVAL* pFirst = pPrimitive->m_Vertices[0];
			A3DVAL* pNormal = pPrimitive->m_Normal;
			A3DVAL reflected[3];
			A3DVAL scaled[3];
			A3DVAL direction[3];
			A3DVAL reflectedDirection[3];
			double flPlaneDistance;
			BOOL bVisible = FALSE;

			if ( m_pCurrentCommand->m_dwOpcode == A3D_TRIANGLES )
			{
				flPlaneDistance = ( ( (double)pSource->m_vRenderPosition[2] - pFirst[2] ) * pNormal[2] +
					( (double)pSource->m_vRenderPosition[1] - pFirst[1] ) * pNormal[1] ) +
					( (double)pSource->m_vRenderPosition[0] - pFirst[0] ) * pNormal[0];
				double flTwiceDistance = flPlaneDistance + flPlaneDistance;
				A3DVAL flRoundedTwiceDistance = (A3DVAL)flTwiceDistance;
				double flScaledDistance = m_flReflectionPlaneScale * flPlaneDistance;
				A3DVAL flRoundedScaledDistance = (A3DVAL)flScaledDistance;
				reflected[0] = (A3DVAL)( pSource->m_vRenderPosition[0] - flTwiceDistance * pNormal[0] );
				reflected[1] = (A3DVAL)( pSource->m_vRenderPosition[1] - (double)flRoundedTwiceDistance * pNormal[1] );
				reflected[2] = (A3DVAL)( pSource->m_vRenderPosition[2] - (double)flRoundedTwiceDistance * pNormal[2] );
				scaled[0] = (A3DVAL)( pSource->m_vRenderPosition[0] - flScaledDistance * pNormal[0] );
				scaled[1] = (A3DVAL)( pSource->m_vRenderPosition[1] - (double)flRoundedScaledDistance * pNormal[1] );
				scaled[2] = (A3DVAL)( pSource->m_vRenderPosition[2] - (double)flRoundedScaledDistance * pNormal[2] );
			}
			else
			{
				flPlaneDistance = (A3DVAL)ReflectPoint( pSource->m_vRenderPosition, pNormal, pFirst,
					reflected, scaled );
			}

			for ( int i = 0; i < 3; i++ )
			{
				reflectedDirection[i] = reflected[i] - m_vWorldListenerPosition[i];
				direction[i] = scaled[i] - m_vWorldListenerPosition[i];
			}

			if ( pPrimitive->m_dwEdgeFlags & A3D_PRIMITIVE_COMPAT1009 )
			{
				double flListenerDistance =
					( ( (double)m_vWorldListenerPosition[2] - pFirst[2] ) * pNormal[2] +
					( (double)m_vWorldListenerPosition[1] - pFirst[1] ) * pNormal[1] ) +
					( (double)m_vWorldListenerPosition[0] - pFirst[0] ) * pNormal[0];
				bVisible = flPlaneDistance * flListenerDistance > 0;
			}
			else if ( m_pCurrentCommand->m_dwOpcode == A3D_QUADS )
			{
				bVisible = A3dIntersectQuad( m_vWorldListenerPosition, direction, pPrimitive, TRUE, FALSE );
			}
			else
			{
				double flDirectionDot = ( (double)direction[2] * pNormal[2] +
					(double)direction[1] * pNormal[1] ) + (double)direction[0] * pNormal[0];
				double flAbsDot = flDirectionDot;
				if ( !( flDirectionDot >= 0 ) )
					flAbsDot = -flDirectionDot;
				if ( flAbsDot >= A3D_EPSILON )
				{
					double flFraction =
						( ( ( (double)pNormal[0] * pFirst[0] + (double)pNormal[1] * pFirst[1] ) +
						(double)pNormal[2] * pFirst[2] ) -
						( ( (double)m_vWorldListenerPosition[0] * pNormal[0] +
						(double)pNormal[1] * m_vWorldListenerPosition[1] ) +
						(double)pNormal[2] * m_vWorldListenerPosition[2] ) ) / flDirectionDot;
					if ( !( flFraction >= 0 ) )
					{
						A3DVAL flNormalX = -pNormal[0];
						A3DVAL flNormalY = -pNormal[1];
						A3DVAL flNormalZ = -pNormal[2];
						pNormal[0] = flNormalX;
						pNormal[1] = flNormalY;
						pNormal[2] = flNormalZ;
						double flDot = ( (double)direction[2] * flNormalZ +
							(double)direction[0] * flNormalX ) + (double)direction[1] * flNormalY;
						flFraction =
							( ( ( (double)flNormalX * pFirst[0] + (double)flNormalY * pFirst[1] ) +
							(double)flNormalZ * pFirst[2] ) -
							( ( (double)m_vWorldListenerPosition[0] * flNormalX +
							(double)flNormalY * m_vWorldListenerPosition[1] ) +
							(double)flNormalZ * m_vWorldListenerPosition[2] ) ) / flDot;
					}
					pPrimitive->m_flDistance = (A3DVAL)flFraction;
					if ( flFraction >= 0 && !( flFraction > 1 ) )
					{
						double flPointX = flFraction * direction[0] + m_vWorldListenerPosition[0];
						A3DVAL flPointY = (A3DVAL)( flFraction * direction[1] + m_vWorldListenerPosition[1] );
						A3DVAL flPointZ = (A3DVAL)( flFraction * direction[2] + m_vWorldListenerPosition[2] );
						A3DVAL flAbsX = pNormal[0] < 0 ? -pNormal[0] : pNormal[0];
						double flAbsY = pNormal[1] < 0 ? -pNormal[1] : pNormal[1];
						double flAbsZ = pNormal[2] < 0 ? -pNormal[2] : pNormal[2];
						int u = 0;
						int v = 1;
						if ( flAbsX > flAbsY && flAbsX > flAbsZ )
						{
							u = 1;
							v = 2;
						}
						else if ( flAbsY > flAbsX && flAbsY > flAbsZ )
						{
							v = 2;
						}

						double flPointU = u ? (double)flPointY - pFirst[u] : flPointX - pFirst[u];
						A3DVAL flPointV = ( v == 1 ? flPointY : flPointZ ) - pFirst[v];
						double flEdgeU = (double)pPrimitive->m_Vertices[1][u] - pFirst[u];
						A3DVAL flOtherEdgeU = pPrimitive->m_Vertices[2][u] - pFirst[u];
						A3DVAL flEdgeV = pPrimitive->m_Vertices[1][v] - pFirst[v];
						A3DVAL flOtherEdgeV = pPrimitive->m_Vertices[2][v] - pFirst[v];
						A3DVAL flBeta = 0;
						double flAlpha = 0;
						BOOL bCoordinates = FALSE;
						if ( !( flEdgeU < 0 || flEdgeU > 0 ) )
						{
							if ( flOtherEdgeU < 0 || flOtherEdgeU > 0 )
							{
								double flBetaWide = flPointU / flOtherEdgeU;
								flBeta = (A3DVAL)flBetaWide;
								if ( flBetaWide > 0 && flBeta < 1 && ( flEdgeV < 0 || flEdgeV > 0 ) )
								{
									flAlpha = ( (double)flPointV - (double)flBeta * flOtherEdgeV ) / flEdgeV;
									bCoordinates = TRUE;
								}
							}
						}
						else
						{
							double flDeterminant = (double)flOtherEdgeV * flEdgeU - (double)flEdgeV * flOtherEdgeU;
							if ( flDeterminant < 0 || flDeterminant > 0 )
							{
								flBeta = (A3DVAL)( ( (double)flPointV * flEdgeU -
									(double)flEdgeV * flPointU ) / flDeterminant );
								if ( flBeta > 0 && flBeta < 1 )
								{
									flAlpha = ( flPointU - (double)flBeta * flOtherEdgeU ) / flEdgeU;
									bCoordinates = TRUE;
								}
							}
						}
						bVisible = bCoordinates && flAlpha >= -A3D_EPSILON &&
							flAlpha + flBeta <= A3D_INTERSECTION_EDGE_LIMIT;
					}
				}
			}

			if ( bVisible )
				pSource->AddReflection( pPrimitive->m_dwTag, reflectedDirection, 1.0f, m_MaterialProperties );
		}

		++m_dwPolygonTestCount;
		if ( m_dwPolygonTestCount > m_dwPolygonTestLimit )
			return;
	}
}
