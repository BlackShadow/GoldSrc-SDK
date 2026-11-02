#include "A3dSource.h"
#include "A3d3.h"
#include "A3dGeom.h"
#include "matrix.h"
#include "matrix_inverse.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <new>

#define A3D_DISTANCE_EQ_RANGE_SCALE	17.0f

#pragma intrinsic( pow )

void CA3dSource::UpdateOcclusion( float flFactor, const float gainEq[2] )
{
	if ( !( m_pA3d->m_dwEnabledFeatures & A3D_OCCLUSIONS ) ||
		!( m_dwRenderMode & A3DSOURCE_RENDERMODE_OCCLUSIONS ) )
	{
		m_flOcclusionStep[0] = 0.0f;
		m_flOcclusionStep[1] = 0.0f;
		m_flOcclusionFactor = 0.0f;
		m_flTargetOcclusion[0] = 1.0f;
		m_flOcclusion[0] = 1.0f;
		m_flTargetOcclusion[1] = 1.0f;
		m_flOcclusion[1] = 1.0f;
		m_flOcclusionFactor = flFactor;
		return;
	}

	if ( m_nPlaybackState == 1 )
	{
		m_flOcclusionStep[0] = 0.0f;
		m_flOcclusionStep[1] = 0.0f;
		for ( int i = 0; i < 2; i++ )
		{
			m_flTargetOcclusion[i] = gainEq[i];
			m_flOcclusion[i] = gainEq[i];
		}
		m_flOcclusionFactor = flFactor;
		return;
	}

	for ( int i = 0; i < 2; i++ )
	{
		if ( m_flTargetOcclusion[i] < gainEq[i] || m_flTargetOcclusion[i] > gainEq[i] )
		{
			double flTarget = gainEq[i];
			m_flTargetOcclusion[i] = (float)flTarget;
			double flStep = ( flTarget - m_flOcclusion[i] ) * m_pA3d->m_flFrameTime;
			m_flOcclusionStep[i] = (float)( flStep + flStep );
		}

		if ( !( m_flOcclusionStep[i] < 0.0f || m_flOcclusionStep[i] > 0.0f ) )
			continue;

		double flCurrent = (double)m_flOcclusionStep[i] + m_flOcclusion[i];
		m_flOcclusion[i] = (float)flCurrent;
		if ( m_flOcclusionStep[i] > 0.0f ? flCurrent > m_flTargetOcclusion[i] :
			!( flCurrent >= m_flTargetOcclusion[i] ) )
		{
			m_flOcclusion[i] = m_flTargetOcclusion[i];
			m_flOcclusionStep[i] = 0.0f;
		}
	}
	m_flOcclusionFactor = flFactor;
}

void CA3dSource::ComputeDistanceGain( float flDistance, float* pGain )
{
	double flScaledDistance = (double)flDistance * m_flDistanceModelScale;
	if ( !( flScaledDistance > m_flMinDistance ) )
	{
		*pGain = 1.0f;
		return;
	}

	if ( flScaledDistance > m_flMaxDistance )
	{
		if ( m_dwMaxDistanceMute )
		{
			*pGain = 0.0f;
			return;
		}
		flDistance = m_flMaxDistance;
	}

	double flGain = pow( 1.0f / ( (double)flDistance * m_flDistanceModelScale ), 1.0 );
	double flReferenceGain = pow( 1.0f / (double)m_flMinDistance, 1.0 );
	*pGain = (float)( flGain / flReferenceGain );
}

void CA3dSource::ComputeDistanceEq( float flDistance, float* pEq )
{
	if ( (double)m_flMinDistance * m_flDistanceModelScale > flDistance ||
		!( m_flDistanceModelScale < 0.0f || m_flDistanceModelScale > 0.0f ) )
	{
		*pEq = 1.0f;
		return;
	}

	if ( (double)flDistance * m_flDistanceModelScale > m_flMaxDistance )
	{
		if ( m_dwMaxDistanceMute )
		{
			*pEq = 0.0f;
			return;
		}
		flDistance = m_flMaxDistance;
	}

	double flRange = ( (double)m_flMinDistance * A3D_DISTANCE_EQ_RANGE_SCALE ) / m_flDistanceModelScale;
	double flOffset = (double)flDistance - m_flMinDistance;
	double flDenominator = ( flRange + flOffset ) + m_flMinDistance;
	if ( flDenominator < 0.0 || flDenominator > 0.0 )
		*pEq = (float)( 1.0f - flOffset / flDenominator );
}

void CA3dSource::UpdateDistanceModel( float flLeftDistance, float flRightDistance )
{
	m_flEarDistance[1] = flRightDistance;
	m_flEarDistance[0] = flLeftDistance;
	ComputeDistanceGain( flLeftDistance, &m_flDistanceGain[0] );
	ComputeDistanceGain( flRightDistance, &m_flDistanceGain[1] );
	float flAverageDistance = (float)( ( (double)flRightDistance + flLeftDistance ) * 0.5f );
	ComputeDistanceEq( flAverageDistance, &m_flDistanceEq );
}

void CA3dSource::UpdateDoppler( const float sourceVelocity[3], const float listenerVelocity[3],
		const float relativeDirection[3], float flDistance )
{
	if ( ( !( sourceVelocity[0] < listenerVelocity[0] || sourceVelocity[0] > listenerVelocity[0] ) &&
		!( sourceVelocity[1] < listenerVelocity[1] || sourceVelocity[1] > listenerVelocity[1] ) &&
		!( sourceVelocity[2] < listenerVelocity[2] || sourceVelocity[2] > listenerVelocity[2] ) ) ||
		!( m_flDopplerScale < 0.0f || m_flDopplerScale > 0.0f ) ||
		!( flDistance < 0.0f || flDistance > 0.0f ) )
	{
		m_flDopplerFactor = 1.0f;
		return;
	}

	double flSpeed = (double)A3D_SPEED_OF_SOUND / m_flDopplerScale;
	double flSourceProjection = ( (double)sourceVelocity[2] * relativeDirection[2] +
		(double)sourceVelocity[1] * relativeDirection[1] ) + (double)sourceVelocity[0] * relativeDirection[0];
	double flListenerProjection = ( (double)listenerVelocity[2] * relativeDirection[2] +
		(double)listenerVelocity[1] * relativeDirection[1] ) + (double)listenerVelocity[0] * relativeDirection[0];
	double flSourceRadial = -( flSourceProjection / flDistance );
	float flRoundedSourceRadial = (float)flSourceRadial;
	float flUpperSpeed = (float)( flSpeed - 1.0f );
	double flListenerRadial = -( flListenerProjection / flDistance );

	if ( flSourceRadial > flUpperSpeed )
		flRoundedSourceRadial = flUpperSpeed;
	else if ( !( flRoundedSourceRadial >= 1.0f - flSpeed ) )
		flRoundedSourceRadial = (float)( 1.0f - flSpeed );

	if ( flListenerRadial > flUpperSpeed )
		flListenerRadial = flUpperSpeed;
	else
	{
		float flLowerSpeed = (float)( 1.0f - flSpeed );
		if ( !( flListenerRadial >= flLowerSpeed ) )
			flListenerRadial = flLowerSpeed;
	}

	if ( !( flSpeed < flRoundedSourceRadial || flSpeed > flRoundedSourceRadial ) )
	{
		m_flDopplerFactor = 1.0f;
		return;
	}

	m_flDopplerFactor = (float)( ( flSpeed - flListenerRadial ) / ( flSpeed - flRoundedSourceRadial ) );
}

#define A3D_RENDER_MODE_SPATIAL			1
#define A3D_RENDER_MODE_MONO				2
#define A3D_RENDER_RESERVED_MODE_FLAG		0x00000002
#define A3D_RENDER_NEAR_DISTANCE			0.071f
#define A3D_RENDER_NEAR_GAIN				0.667f
#define A3D_MAX_EQ_ATTENUATION			0.99f
#define HALF_PI							1.5707964f

static const IID IID_A3dRenderBuffer =
{
	0xe04814d8, 0x26fc, 0x11d2, { 0xbb, 0x48, 0x00, 0x60, 0x08, 0x2f, 0x3c, 0x00 }
};

CA3dSource::CA3dSource( const CA3dSource& source, HRESULT* pStatus )
{
	memcpy( (a3d_source_state_t*)this, (const a3d_source_state_t*)&source, sizeof( a3d_source_state_t ) );
	m_pReflections[0] = NULL;
	m_pReflections[1] = NULL;
	m_nReflections[0] = 0;
	m_nReflections[1] = 0;
	m_iReflectionBuffer = 0;
	m_pDirectSound->AddRef();

	if ( FAILED( m_pDirectSound->DuplicateSoundBuffer( source.m_pBuffer, &m_pBuffer ) ) )
	{
		*pStatus = A3DERROR_FAILED_DUPLICATION;
		return;
	}

	if ( FAILED( m_pBuffer->QueryInterface( IID_IDirectSound3DBuffer, (void**)&m_pBuffer3d ) ) )
	{
		*pStatus = A3DERROR_FAILED_DUPLICATION;
		return;
	}

	if ( FAILED( m_pBuffer->QueryInterface( IID_A3dRenderBuffer, (void**)&m_pSourceBackend ) ) )
		m_pSourceBackend = NULL;

	m_pRenderStates[0] = (A3DCTRL_SRC_SUPER*)calloc( 1, sizeof( A3DCTRL_SRC_SUPER ) );
	m_pRenderStates[1] = (A3DCTRL_SRC_SUPER*)calloc( 1, sizeof( A3DCTRL_SRC_SUPER ) );
	m_iRenderState = 0;
	m_pRenderState = m_pRenderStates[0];
	InitializeRenderState();
	m_cReferences = 1;
	m_nPlaybackState = 0;
	m_iUpdateGroup = g_iSourceUpdateGroup;
	g_iSourceUpdateGroup = g_iSourceUpdateGroup == 0;
	*pStatus = S_OK;
}

void CA3dSource::InitializeRenderState()
{
	m_pRenderState->dwFlags = 0;
	m_pRenderState->dwMode = A3DSOURCE_RENDERMODE_DEFAULT;
	m_pRenderState->dwEnabled = 1;
	m_pRenderState->dwReserved = 0;
	m_pRenderState->flEq = 0.0f;
	m_pRenderState->flPitch = 1.0f;
	m_pRenderState->flMono = 0.0f;
	m_pRenderState->Left.flAzimuth = 0.0f;
	m_pRenderState->Left.flElevation = 0.0f;
	m_pRenderState->Left.flGain = 1.0f;
	m_pRenderState->Left.flDelay = 0.0f;
	m_pRenderState->Right.flAzimuth = 0.0f;
	m_pRenderState->Right.flElevation = 0.0f;
	m_pRenderState->Right.flGain = 1.0f;
	m_pRenderState->Right.flDelay = 0.0f;
	m_pRenderState->flReservedFactor = 1.0f;
	m_pRenderState->flAudibility = 1.0f;
	m_pRenderState->flDistance = 0.0f;
	m_pRenderState->dwReservedDistance = 0;
	SubmitRenderState();
}

void CA3dSource::SetRenderPosition( const float left[3], const float right[3] )
{
	if ( ( m_dwRenderMode & A3DSOURCE_RENDERMODE_MONO ) ||
		m_dwType == A3DSOURCE_INITIAL_RENDERMODE_NATIVE )
	{
		m_pRenderState->flMono = 1.0f;
		return;
	}

	m_pRenderState->flMono = 0.0f;
	m_pRenderState->flDistance = (float)( ( (double)right[2] + left[2] ) * 0.5f );
	m_pRenderState->Left.flAzimuth = left[0];
	m_pRenderState->Left.flElevation = left[1];
	m_pRenderState->Right.flAzimuth = right[0];
	m_pRenderState->Right.flElevation = right[1];
}

void CA3dSource::SetRenderDelays()
{
	if ( ( m_dwRenderMode & A3DSOURCE_RENDERMODE_MONO ) ||
		m_dwType == A3DSOURCE_INITIAL_RENDERMODE_NATIVE )
	{
		m_pRenderState->Left.flDelay = 0.0f;
		m_pRenderState->Right.flDelay = 0.0f;
		return;
	}

	m_pRenderState->Left.flDelay = (float)-( (double)m_flStereoSpread * 0.5f );
	m_pRenderState->Right.flDelay = (float)( (double)m_flStereoSpread * 0.5f );
}

void CA3dSource::SetRenderGain()
{
	float flOutputGain;
	m_pA3d->GetOutputGain( &flOutputGain );
	if ( ( m_dwRenderMode & A3DSOURCE_RENDERMODE_MONO ) ||
		m_dwType == A3DSOURCE_INITIAL_RENDERMODE_NATIVE )
	{
		m_pRenderState->Left.flGain = (float)( (double)flOutputGain * m_flGain );
		m_pRenderState->Right.flGain = (float)( (double)flOutputGain * m_flGain );
		return;
	}

	m_pRenderState->Left.flGain = (float)( ( ( ( (double)m_flDistanceGain[0] * flOutputGain ) *
		m_flGain ) * m_flConeGain ) * m_flOcclusion[0] );
	m_pRenderState->Right.flGain = (float)( ( ( ( (double)m_flDistanceGain[1] * flOutputGain ) *
		m_flGain ) * m_flConeGain ) * m_flOcclusion[0] );
}

void CA3dSource::SetRenderEq()
{
	if ( ( m_dwRenderMode & A3DSOURCE_RENDERMODE_MONO ) ||
		m_dwType == A3DSOURCE_INITIAL_RENDERMODE_NATIVE )
	{
		m_pRenderState->flEq = (float)( 1.0f - (double)m_flEqFactor );
	}
	else
	{
		m_pRenderState->flEq = (float)( 1.0f - ( (double)m_flOcclusion[1] * m_flDistanceEq ) *
			m_flEqFactor );
	}

	if ( m_pRenderState->flEq > A3D_MAX_EQ_ATTENUATION )
		m_pRenderState->flEq = A3D_MAX_EQ_ATTENUATION;
	else if ( !( m_pRenderState->flEq >= 0.0f ) )
		m_pRenderState->flEq = 0.0f;
}

void CA3dSource::SetRenderAudibility()
{
	m_pRenderState->flAudibility = (float)( ( (double)m_pRenderState->Right.flGain +
		m_pRenderState->Left.flGain ) * 0.25f + (double)m_pRenderState->flEq * 0.5f );
}

void CA3dSource::SetRenderPitch()
{
	if ( ( m_dwRenderMode & A3DSOURCE_RENDERMODE_MONO ) ||
		m_dwType == A3DSOURCE_INITIAL_RENDERMODE_NATIVE )
	{
		m_pRenderState->flPitch = (float)( (double)m_flPitch * m_flPlaybackPitch );
		return;
	}

	m_pRenderState->flPitch = (float)( ( (double)m_flDopplerFactor * m_flPitch ) * m_flPlaybackPitch );
}

void CA3dSource::UpdateRenderState( const float left[3], const float right[3] )
{
	SetRenderPosition( left, right );
	SetRenderDelays();
	SetRenderGain();
	SetRenderEq();
	SetRenderAudibility();
	SetRenderPitch();
}

void CA3dSource::SubmitRenderState()
{
	m_pRenderState->dwMode = A3D_RENDER_MODE_SPATIAL;
	if ( m_dwRenderMode & A3DSOURCE_RENDERMODE_MONO )
		m_pRenderState->dwMode = A3D_RENDER_MODE_MONO;
	else if ( m_dwRenderMode & A3D_RENDER_RESERVED_MODE_FLAG )
		m_pRenderState->dwMode = A3D_RENDER_MODE_SPATIAL;

	if ( !( m_pRenderState->flDistance >= A3D_RENDER_NEAR_DISTANCE ) && !m_pA3d->m_bCompat1010 )
	{
		float flOutputGain;
		m_pA3d->GetOutputGain( &flOutputGain );
		m_pRenderState->Left.flGain = (float)( ( (double)flOutputGain * m_flGain ) * A3D_RENDER_NEAR_GAIN );
		m_pRenderState->Right.flGain = (float)( ( (double)flOutputGain * m_flGain ) * A3D_RENDER_NEAR_GAIN );
		m_pRenderState->flEq = (float)( 1.0f - (double)m_flEqFactor );
		m_pRenderState->flMono = 1.0f;
		m_pRenderState->Left.flAzimuth = HALF_PI;
		m_pRenderState->Left.flElevation = 0.0f;
		m_pRenderState->Left.flDelay = 0.0f;
		m_pRenderState->Right.flAzimuth = -HALF_PI;
		m_pRenderState->Right.flElevation = 0.0f;
		m_pRenderState->Right.flDelay = 0.0f;
	}

	if ( m_pSourceBackend )
		m_pSourceBackend->SetA3dSuperCtrl( m_pRenderState, sizeof( A3DCTRL_SRC_SUPER ) );
	else
		ApplyDirectSoundState();

	m_iRenderState = m_iRenderState == 0;
	m_pRenderState = m_pRenderStates[m_iRenderState];
}

void CA3dSource::ResetReflectionChannels()
{
	for ( int i = 0; i < MAX_A3D_SOURCE_REFLECTIONS; i++ )
	{
		a3d_source_reflection_state_t* pReflection = &m_pRenderState->Reflections[i];
		pReflection->dwNewSlot = 1;
		pReflection->flAudibility = 0.0f;
		pReflection->flEq = 0.0f;
		pReflection->Left.flAzimuth = 0.0f;
		pReflection->Left.flElevation = 0.0f;
		pReflection->Left.flGain = 0.0f;
		pReflection->Left.flDelay = 0.0f;
		pReflection->Right.flAzimuth = 0.0f;
		pReflection->Right.flElevation = 0.0f;
		pReflection->Right.flGain = 0.0f;
		pReflection->Right.flDelay = 0.0f;
	}
}

void CA3dSource::DisableReflectionChannels()
{
	for ( int i = 0; i < MAX_A3D_SOURCE_REFLECTIONS; i++ )
	{
		m_pRenderState->Reflections[i].dwEnabled = 0;
		m_pRenderState->Reflections[i].dwActive = 0;
	}
}

CA3dTransform::CA3dTransform()
{
	A3DVECTOR position = { 0.0f, 0.0f, 0.0f, 1.0f };
	A3DVECTOR forward = { 0.0f, 0.0f, 1.0f, 0.0f };
	A3DVECTOR up = { 0.0f, 1.0f, 0.0f, 0.0f };

	m_pParent = NULL;
	SetFrame( NULL, position, forward, up );
	m_cReferences = 0;
}

void CA3dTransform::SetFrame( void* pOwner, const A3DVAL* pPosition,
		const A3DVAL* pForward, const A3DVAL* pUp )
{
	double rightY = (double)pForward[0] * pUp[2] - (double)pUp[0] * pForward[2];
	A3DVAL rightZ = (A3DVAL)( (double)pUp[0] * pForward[1] - (double)pForward[0] * pUp[1] );
	double rightX = (double)pUp[1] * pForward[2] - (double)pForward[1] * pUp[2];

	m_Matrix[2] = rightZ;
	m_Matrix[3] = 0.0f;
	m_Matrix[0] = (A3DVAL)rightX;
	m_Matrix[1] = (A3DVAL)rightY;
	for ( int i = 0; i < 3; i++ )
		m_Matrix[i + 4] = pUp[i];
	m_Matrix[7] = 0.0f;
	for ( int i = 0; i < 3; i++ )
		m_Matrix[i + 8] = pForward[i];
	m_Matrix[11] = 0.0f;
	for ( int i = 0; i < 3; i++ )
		m_Matrix[i + 12] = pPosition[i];
	m_Matrix[15] = 1.0f;
	if ( pOwner )
		m_pOwner = pOwner;
}

void CA3dTransform::AccumulateParentMatrices( A3DMATRIX matrix )
{
	for ( CA3dTransform* pTransform = this; pTransform->m_pParent; pTransform = pTransform->m_pParent )
	{
		A3DMATRIX input;
		memcpy( input, matrix, sizeof( input ) );
		const A3DVAL* pMatrix = pTransform->m_Matrix;

		for ( int i = 0; i < 16; i += 4 )
		{
			matrix[i] = (A3DVAL)( ( ( (double)input[i] * pMatrix[0] +
				(double)input[i + 3] * pMatrix[12] ) + (double)input[i + 1] * pMatrix[4] ) +
				(double)input[i + 2] * pMatrix[8] );
			matrix[i + 1] = (A3DVAL)( ( ( (double)input[i] * pMatrix[1] +
				(double)input[i + 1] * pMatrix[5] ) + (double)input[i + 2] * pMatrix[9] ) +
				(double)input[i + 3] * pMatrix[13] );
			matrix[i + 2] = (A3DVAL)( ( ( (double)input[i + 1] * pMatrix[6] +
				(double)input[i + 2] * pMatrix[10] ) + (double)input[i] * pMatrix[2] ) +
				(double)input[i + 3] * pMatrix[14] );
			matrix[i + 3] = (A3DVAL)( ( ( (double)input[i] * pMatrix[3] +
				(double)input[i + 2] * pMatrix[11] ) + (double)input[i + 3] * pMatrix[15] ) +
				(double)input[i + 1] * pMatrix[7] );
		}
	}
}

void CA3dTransform::TransformToLocal( A3DVECTOR out, const A3DVECTOR in )
{
	A3DMATRIX inverse;
	Matrix_Identity( inverse );
	AccumulateParentMatrices( inverse );
	Matrix_Inverse( inverse, inverse );

	double z = ( ( (double)inverse[14] * in[3] + (double)inverse[10] * in[2] ) +
		(double)inverse[6] * in[1] ) + (double)in[0] * inverse[2];
	double y = ( ( (double)inverse[13] * in[3] + (double)inverse[9] * in[2] ) +
		(double)inverse[5] * in[1] ) + (double)in[0] * inverse[1];
	double w = ( ( (double)inverse[15] * in[3] + (double)inverse[11] * in[2] ) +
		(double)inverse[7] * in[1] ) + (double)in[0] * inverse[3];
	double x = ( ( (double)inverse[12] * in[3] + (double)inverse[8] * in[2] ) +
		(double)inverse[4] * in[1] ) + (double)in[0] * inverse[0];

	out[1] = (A3DVAL)y;
	out[2] = (A3DVAL)z;
	out[0] = (A3DVAL)x;
	out[3] = (A3DVAL)w;
}

void CA3dTransform::TransformVector( A3DVECTOR vector )
{
	double y = ( ( (double)m_Matrix[13] * vector[3] + (double)m_Matrix[9] * vector[2] ) +
		(double)m_Matrix[5] * vector[1] ) + (double)m_Matrix[1] * vector[0];
	double z = ( ( (double)m_Matrix[14] * vector[3] + (double)m_Matrix[6] * vector[1] ) +
		(double)m_Matrix[10] * vector[2] ) + (double)m_Matrix[2] * vector[0];
	double w = ( ( (double)m_Matrix[15] * vector[3] + (double)m_Matrix[7] * vector[1] ) +
		(double)m_Matrix[11] * vector[2] ) + (double)m_Matrix[3] * vector[0];
	double x = ( ( (double)m_Matrix[12] * vector[3] + (double)m_Matrix[8] * vector[2] ) +
		(double)m_Matrix[4] * vector[1] ) + (double)m_Matrix[0] * vector[0];

	vector[1] = (A3DVAL)y;
	vector[2] = (A3DVAL)z;
	vector[3] = (A3DVAL)w;
	vector[0] = (A3DVAL)x;

	for ( CA3dTransform* pParent = m_pParent; pParent; pParent = pParent->m_pParent )
	{
		const A3DVAL* pMatrix = pParent->m_Matrix;
		double parentY = ( ( (double)pMatrix[1] * vector[0] + (double)pMatrix[13] * vector[3] ) +
			(double)pMatrix[9] * vector[2] ) + pMatrix[5] * y;
		double parentZ = ( ( (double)pMatrix[6] * vector[1] + (double)pMatrix[2] * vector[0] ) +
			(double)pMatrix[14] * vector[3] ) + (double)pMatrix[10] * vector[2];
		double parentW = ( ( (double)pMatrix[7] * vector[1] + (double)pMatrix[11] * vector[2] ) +
			(double)pMatrix[3] * vector[0] ) + (double)pMatrix[15] * vector[3];
		double parentX = ( ( (double)pMatrix[12] * vector[3] + (double)pMatrix[8] * vector[2] ) +
			(double)pMatrix[4] * vector[1] ) + (double)pMatrix[0] * vector[0];
		vector[1] = (A3DVAL)parentY;
		vector[2] = (A3DVAL)parentZ;
		vector[3] = (A3DVAL)parentW;
		vector[0] = (A3DVAL)parentX;
		y = parentY;
	}
}

void CA3dTransform::TransformVector( A3DVECTOR out, const A3DVECTOR in )
{
	memcpy( out, in, sizeof( A3DVECTOR ) );
	TransformVector( out );
}

void CA3dTransform::TransformPolygon( a3d_polygon_t* pOut, const a3d_polygon_t* pPolygon )
{
	memcpy( pOut, pPolygon, sizeof( *pOut ) );

	TransformVector( pOut->m_Vertices[0] );
	TransformVector( pOut->m_Vertices[1] );
	TransformVector( pOut->m_Vertices[2] );
	if ( pOut->m_nVertices == 4 )
		TransformVector( pOut->m_Vertices[3] );
	TransformVector( pOut->m_Normal );
}

void CA3dTransform::TransformLine( A3DVERTEX* pOut, const A3DVERTEX* pLine )
{
	memcpy( pOut, pLine, sizeof( A3DVERTEX ) * 2 );
	TransformVector( pOut[0] );
	TransformVector( pOut[1] );
}

void CA3dTransform::GetWorldPosition( A3DVECTOR out )
{
	A3DVECTOR origin = { 0.0f, 0.0f, 0.0f, 1.0f };
	TransformVector( out, origin );
}

HRESULT CA3dTransform::SetPosition( CA3dTransform* pReference, const A3DVAL* pPosition )
{
	if ( !pPosition )
		return E_INVALIDARG;

	A3DVECTOR referencePosition;
	if ( pReference )
		pReference->GetWorldPosition( referencePosition );
	else
	{
		referencePosition[0] = 0.0f;
		referencePosition[1] = 0.0f;
		referencePosition[2] = 0.0f;
		referencePosition[3] = 1.0f;
	}

	A3DVAL worldX = pPosition[0] + referencePosition[0];
	A3DVAL worldY = pPosition[1] + referencePosition[1];
	A3DVAL worldZ = pPosition[2] + referencePosition[2];
	A3DVECTOR parentPosition;
	if ( m_pParent )
		m_pParent->GetWorldPosition( parentPosition );
	else
	{
		parentPosition[0] = 0.0f;
		parentPosition[1] = 0.0f;
		parentPosition[2] = 0.0f;
	}

	m_Matrix[12] = worldX - parentPosition[0];
	m_Matrix[13] = worldY - parentPosition[1];
	m_Matrix[14] = worldZ - parentPosition[2];
	return S_OK;
}

HRESULT CA3dTransform::GetPosition( CA3dTransform* pReference, A3DVAL* pPosition )
{
	if ( !pPosition )
		return E_INVALIDARG;

	pPosition[3] = 1.0f;
	if ( !pReference )
	{
		GetWorldPosition( pPosition );
		return S_OK;
	}

	A3DVECTOR referencePosition;
	A3DVECTOR worldPosition;
	pReference->GetWorldPosition( referencePosition );
	GetWorldPosition( worldPosition );
	for ( int i = 0; i < 3; i++ )
		pPosition[i] = worldPosition[i] - referencePosition[i];
	return S_OK;
}

HRESULT CA3dTransform::GetOrientation( CA3dTransform* pReference, A3DVAL* pForward, A3DVAL* pUp )
{
	if ( !pReference )
	{
		for ( int i = 0; i < 3; i++ )
		{
			pUp[i] = m_Matrix[i + 4];
			pForward[i] = m_Matrix[i + 8];
		}
		return S_OK;
	}

	A3DVECTOR forward = { 0.0f, 0.0f, 1.0f, 0.0f };
	A3DVECTOR up = { 0.0f, 1.0f, 0.0f, 0.0f };
	TransformVector( forward );
	TransformVector( up );
	pReference->TransformToLocal( pForward, forward );
	pReference->TransformToLocal( pUp, up );
	return S_OK;
}

HRESULT CA3dTransform::SetOrientation( CA3dTransform* pReference, const A3DVAL* pForward,
		const A3DVAL* pUp )
{
	A3DVECTOR forward;
	A3DVECTOR up;
	const A3DVAL* pLocalForward = pForward;
	const A3DVAL* pLocalUp = pUp;
	A3DVECTOR parentForward;
	A3DVECTOR parentUp;

	if ( pReference )
	{
		memcpy( forward, pForward, sizeof( forward ) );
		forward[3] = 0.0f;
		memcpy( up, pUp, sizeof( up ) );
		up[3] = 0.0f;
		pReference->TransformVector( forward );
		pReference->TransformVector( up );
		pForward = forward;
		pUp = up;
		pLocalForward = forward;
		pLocalUp = up;
		if ( m_pParent )
		{
			m_pParent->TransformToLocal( parentForward, forward );
			m_pParent->TransformToLocal( parentUp, up );
			pLocalForward = parentForward;
			pLocalUp = parentUp;
		}
	}

	double rightX = (double)pUp[1] * pForward[2] - (double)pForward[1] * pUp[2];
	double rightY = (double)pForward[0] * pUp[2] - (double)pUp[0] * pForward[2];
	double rightZ = (double)pUp[0] * pForward[1] - (double)pForward[0] * pUp[1];
	m_Matrix[0] = (A3DVAL)rightX;
	if ( !pReference )
	{
		m_Matrix[4] = pUp[0];
		A3DVAL forwardX = pForward[0];
		m_Matrix[1] = (A3DVAL)rightY;
		m_Matrix[8] = forwardX;
		m_Matrix[5] = pUp[1];
		A3DVAL forwardY = pForward[1];
		m_Matrix[2] = (A3DVAL)rightZ;
		m_Matrix[9] = forwardY;
		m_Matrix[6] = pUp[2];
		m_Matrix[10] = pForward[2];
	}
	else
	{
		m_Matrix[4] = pLocalUp[0];
		m_Matrix[8] = pLocalForward[0];
		m_Matrix[5] = pLocalUp[1];
		m_Matrix[9] = pLocalForward[1];
		m_Matrix[1] = (A3DVAL)rightY;
		m_Matrix[6] = pLocalUp[2];
		m_Matrix[10] = pLocalForward[2];
		m_Matrix[2] = (A3DVAL)rightZ;
	}
	return S_OK;
}

void CA3dGeometryInstance::TransformPolygons( CA3dObjectList* pPolygons, CA3dTransform* pTransform,
		const A3DVAL* pMatrix )
{
	pPolygons->m_pCurrent = pPolygons->m_pFirst;
	for ( int remaining = pPolygons->m_nCount; remaining > 0; remaining-- )
	{
		a3d_polygon_t* pPolygon = NULL;
		if ( pPolygons->m_pCurrent )
		{
			pPolygon = (a3d_polygon_t*)pPolygons->m_pCurrent->m_pObject;
			pPolygons->m_pCurrent = pPolygons->m_pCurrent->m_pNext;
		}

		a3d_polygon_t transformed;
		pTransform->TransformPolygon( &transformed, pPolygon );
		a3d_polygon_t* pCopy = new ( std::nothrow ) a3d_polygon_t;
		memcpy( pCopy, pPolygon, sizeof( *pCopy ) );

		for ( int i = 0; i < pCopy->m_nVertices; i++ )
		{
			const A3DVAL* pVertex = pPolygon->m_Vertices[i];
			pCopy->m_Vertices[i][0] = (A3DVAL)( ( ( (double)pMatrix[0] * pVertex[0] +
				(double)pVertex[1] * pMatrix[4] ) + (double)pVertex[2] * pMatrix[8] ) +
				(double)pVertex[3] * pMatrix[12] );
			for ( int j = 1; j < 4; j++ )
			{
				pCopy->m_Vertices[i][j] = (A3DVAL)( ( ( (double)pVertex[0] * pMatrix[j] +
					(double)pVertex[1] * pMatrix[j + 4] ) + (double)pVertex[2] * pMatrix[j + 8] ) +
					(double)pVertex[3] * pMatrix[j + 12] );
			}
		}

		const A3DVAL* pNormal = pPolygon->m_Normal;
		pCopy->m_Normal[0] = (A3DVAL)( ( ( (double)pMatrix[0] * pNormal[0] +
			(double)pNormal[2] * pMatrix[8] ) + (double)pNormal[3] * pMatrix[12] ) +
			(double)pNormal[1] * pMatrix[4] );
		pCopy->m_Normal[1] = (A3DVAL)( ( ( (double)pNormal[2] * pMatrix[9] +
			(double)pNormal[0] * pMatrix[1] ) + (double)pNormal[3] * pMatrix[13] ) +
			(double)pNormal[1] * pMatrix[5] );
		pCopy->m_Normal[2] = (A3DVAL)( ( ( (double)pNormal[2] * pMatrix[10] +
			(double)pNormal[0] * pMatrix[2] ) + (double)pNormal[3] * pMatrix[14] ) +
			(double)pMatrix[6] * pNormal[1] );
		pCopy->m_Normal[3] = (A3DVAL)( ( ( (double)pNormal[3] * pMatrix[15] +
			(double)pNormal[2] * pMatrix[11] ) + (double)pNormal[0] * pMatrix[3] ) +
			(double)pNormal[1] * pMatrix[7] );
		m_Polygons.Append( pCopy );
	}
}

#define A3D_STEREO_SPREAD_SCALE		0.001f
#define A3D_CONE_ANGLE_SCALE			57.29578f
#define A3D_VERTICAL_CONE_ANGLE		1.5707964f
#define A3D_LOG_VOLUME_SCALE			0.2f

#pragma intrinsic( sin, cos, sqrt, atan2, log10, fabs )

static const double stereoSpreadCurve[4] =
{
	-0.0429,
	-0.0195,
	-0.0371,
	-0.5729
};

#pragma float_control( push )
#pragma fenv_access( off )
#pragma float_control( except, off )
#pragma float_control( precise, off )

void CA3dSource::UpdateStereoSpread( const float spherical[3] )
{
	double flHeading = spherical[0];
	double flCosine2 = cos( flHeading + flHeading );
	double flCosine3 = cos( flHeading * 3.0f );
	double flCosine4 = cos( flHeading * 4.0f );
	double flHarmonic2 = flCosine2 * stereoSpreadCurve[0];
	double flCosineElevation = cos( (double)spherical[1] );
	double flHarmonic3 = flCosine3 * stereoSpreadCurve[1];
	double flCurve = flHarmonic2 - flHarmonic3;
	double flSineHeading = sin( flHeading );
	double flHarmonic4 = flCosine4 * stereoSpreadCurve[2];
	double flProjection = flCosineElevation * flSineHeading;
	flCurve = flCurve - flHarmonic4;
	flCurve = flCurve - stereoSpreadCurve[3];
	m_flStereoSpread = (float)( ( flCurve * flProjection ) * A3D_STEREO_SPREAD_SCALE );
}

void CA3dSource::UpdateConeGain( const float relativeDirection[3] )
{
	if ( m_flOuterConeGain >= 1.0f || !( m_flOuterConeAngle > 0.0f ) ||
		!( m_flOuterConeAngle < m_flInnerConeAngle || m_flOuterConeAngle > m_flInnerConeAngle ) )
	{
		m_flConeGain = 1.0f;
		return;
	}

	double flHorizontalLength = sqrt( (double)relativeDirection[1] * relativeDirection[1] +
		(double)relativeDirection[0] * relativeDirection[0] );
	double flAngle;
	if ( !( relativeDirection[0] < 0.0f || relativeDirection[0] > 0.0f ) &&
		!( relativeDirection[2] < 0.0f || relativeDirection[2] > 0.0f ) )
	{
		if ( !( flHorizontalLength < 0.0 || flHorizontalLength > 0.0 ) )
			flAngle = 0.0;
		else
			flAngle = A3D_VERTICAL_CONE_ANGLE;
	}
	else
	{
		flAngle = fabs( atan2( flHorizontalLength, -(double)relativeDirection[2] ) * A3D_CONE_ANGLE_SCALE );
	}

	if ( flAngle > m_flOuterConeAngle )
	{
		m_flConeGain = m_flOuterConeGain;
		return;
	}

	if ( flAngle > m_flInnerConeAngle )
	{
		double flFraction = ( (double)m_flOuterConeAngle - flAngle ) /
			( (double)m_flOuterConeAngle - m_flInnerConeAngle );
		m_flConeGain = (float)( flFraction * ( 1.0f - (double)m_flOuterConeGain ) + m_flOuterConeGain );
		return;
	}

	m_flConeGain = 1.0f;
}

void CA3dSource::ApplyDirectSoundState()
{
	A3DCTRL_SRC_SUPER* pState = m_pRenderState;
	DS3DBUFFER parameters;
	memset( &parameters, 0, sizeof( parameters ) );
	double flSine = sin( (double)pState->Left.flAzimuth );
	double flNegativeSine = -flSine;
	double flCosine = cos( (double)pState->Left.flAzimuth );
	double flElevation = sin( (double)pState->Left.flElevation );
	volatile float flGain = (float)( ( (double)pState->Left.flGain + pState->Right.flGain ) * 0.5f );

	volatile double flTwiceNegativeSine = flNegativeSine + flNegativeSine;
	volatile double flTwiceElevation = flElevation + flElevation;
	volatile double flTwiceCosine = flCosine + flCosine;
	parameters.dwSize = sizeof( parameters );
	parameters.vPosition.x = (float)( flTwiceNegativeSine * 0.5f );
	parameters.vPosition.y = (float)( flTwiceElevation * 0.5f );
	parameters.vPosition.z = (float)( flTwiceCosine * 0.5f );
	parameters.flMinDistance = 0.5f;
	parameters.flMaxDistance = 2.0f;
	parameters.dwMode = DS3DMODE_HEADRELATIVE;
	m_pBuffer3d->SetAllParameters( &parameters, DS3D_IMMEDIATE );

	double flVolume;
	if ( !( flGain < 0.0f || flGain > 0.0f ) )
	{
		flVolume = DSBVOLUME_MIN;
	}
	else
	{
		volatile double flLevel = log10( (double)flGain ) * A3D_LOG_VOLUME_SCALE;
		flVolume = flLevel * -(double)DSBVOLUME_MIN;
	}
	m_pBuffer->SetVolume( (LONG)(LONGLONG)flVolume );
	double flFrequency = (double)m_nSamplesPerSec * pState->flPitch;
	m_pBuffer->SetFrequency( (DWORD)(LONGLONG)flFrequency );
}

#pragma float_control( pop )
#if defined( _M_FP_STRICT )
#pragma fenv_access( on )
#endif
