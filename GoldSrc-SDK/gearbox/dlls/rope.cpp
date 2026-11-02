#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "rope.h"

LINK_ENTITY_TO_CLASS( env_rope, CRope );

static const char* CREAK_SOUNDS[] =
{
	"items/rope1.wav",
	"items/rope2.wav",
	"items/rope3.wav",
};

TYPEDESCRIPTION CRope::m_SaveData[] =
{
	DEFINE_FIELD( CRope, m_iSegments, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, m_bToggle, FIELD_CHARACTER ),
	DEFINE_FIELD( CRope, m_InitialDeltaTime, FIELD_CHARACTER ),
	DEFINE_FIELD( CRope, mLastTime, FIELD_TIME ),
	DEFINE_FIELD( CRope, m_LastEndPos, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CRope, m_Gravity, FIELD_VECTOR ),
	DEFINE_FIELD( CRope, m_HookConstant, FIELD_FLOAT ),
	DEFINE_FIELD( CRope, m_SpringDampning, FIELD_FLOAT ),
	DEFINE_FIELD( CRope, m_NumSamples, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, m_SpringCnt, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, mObjectAttached, FIELD_CHARACTER ),
	DEFINE_FIELD( CRope, mAttachedObjectsSegment, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, detachTime, FIELD_TIME ),
	DEFINE_ARRAY( CRope, seg, FIELD_CLASSPTR, MAX_ROPE_SEGMENTS ),
	DEFINE_ARRAY( CRope, altseg, FIELD_CLASSPTR, MAX_ROPE_SEGMENTS ),
	DEFINE_ARRAY( CRope, m_CurrentSys, FIELD_CLASSPTR, MAX_ROPE_SAMPLES ),
	DEFINE_ARRAY( CRope, m_TargetSys, FIELD_CLASSPTR, MAX_ROPE_SAMPLES ),
	DEFINE_FIELD( CRope, mDisallowPlayerAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, mBodyModel, FIELD_STRING ),
	DEFINE_FIELD( CRope, mEndingModel, FIELD_STRING ),
	DEFINE_FIELD( CRope, mAttachedObjectsOffset, FIELD_FLOAT ),
	DEFINE_FIELD( CRope, mMakeSound, FIELD_INTEGER ),
};

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CRope::CRope()
{
	mBodyModel = ALLOC_STRING( "models/rope16.mdl" );
	mEndingModel = ALLOC_STRING( "models/rope16.mdl" );
}

void CRope::Touch( CBaseEntity* )
{
}

int CRope::getBodyModel() const
{
	return mBodyModel;
}

int CRope::getEndingModel() const
{
	return mEndingModel;
}

void CRope::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "segments" ) || FStrEq( pkvd->szKeyName, "Segments" ) )
	{
		pkvd->fHandled = TRUE;
		m_iSegments = atoi( pkvd->szValue );
		if ( m_iSegments < 2 || m_iSegments > 62 )
			m_iSegments = 62;
	}
	if ( FStrEq( pkvd->szKeyName, "bodymodel" ) )
	{
		pkvd->fHandled = TRUE;
		mBodyModel = ALLOC_STRING( pkvd->szValue );
	}
	if ( FStrEq( pkvd->szKeyName, "endingmodel" ) )
	{
		pkvd->fHandled = TRUE;
		mEndingModel = ALLOC_STRING( pkvd->szValue );
	}
	if ( FStrEq( pkvd->szKeyName, "disable" ) )
	{
		pkvd->fHandled = TRUE;
		mDisallowPlayerAttachment = atoi( pkvd->szValue );
	}
}

void CRope::Precache()
{
	UTIL_PrecacheOther( "rope_segment" );
	UTIL_PrecacheOther( "rope_sample" );
	PRECACHE_SOUND( "items/rope1.wav" );
	PRECACHE_SOUND( "items/rope2.wav" );
	PRECACHE_SOUND( "items/rope3.wav" );
}

int CRope::Save( CSave& save )
{
	if ( !CBaseDelay::Save( save ) )
		return 0;
	return save.WriteFields( "CRope", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
}

int CRope::Restore( CRestore& restore )
{
	if ( !CBaseDelay::Restore( restore ) )
		return 0;

	int result = restore.ReadFields( "CRope", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
	if ( !result || m_iSegments < 2 || m_iSegments >= MAX_ROPE_SEGMENTS || m_NumSamples != m_iSegments + 1 )
		return 0;
	for ( int i = 0; i < ARRAYSIZE( m_TempSys ); i++ )
	{
		m_TempSys[i] = (RopeSampleData*)malloc( sizeof( RopeSampleData ) * m_NumSamples );
		if ( m_TempSys[i] )
		{
			for ( int sample = 0; sample < m_NumSamples; sample++ )
			{
				m_TempSys[i][sample].mApplyExternalForce = false;
				m_TempSys[i][sample].mExternalForce = g_vecZero;
			}
		}
	}
	mSpringsInitialized = false;
	m_InitialDeltaTime = true;
	return result;
}

void CRope::Spawn()
{
	pev->classname = MAKE_STRING( "env_rope" );
	if ( m_iSegments < 2 || m_iSegments >= MAX_ROPE_SEGMENTS )
		m_iSegments = MAX_ROPE_SEGMENTS - 1;
	mMakeSound = 1;
	Precache();
	mSpringsInitialized = false;
	m_Gravity = Vector( 0, 0, -50 );
	mObjectAttached = false;
	pev->flags |= FL_ALWAYSTHINK;
	m_NumSamples = m_iSegments + 1;

	int i;
	for ( i = 0; i <= m_iSegments; i++ )
	{
		m_CurrentSys[i] = CRopeSample::CreateSample();
		m_CurrentSys[i]->mMasterRope = this;
	}
	for ( ; i < MAX_ROPE_SAMPLES; i++ )
		m_CurrentSys[i] = NULL;

	CRopeSample* sample = m_CurrentSys[0];
	seg[0] = CRopeSegment::CreateSegment( sample, getBodyModel() );
	UTIL_SetOrigin( seg[0]->pev, pev->origin );
	altseg[0] = CRopeSegment::CreateSegment( sample, getBodyModel() );
	UTIL_SetOrigin( altseg[0]->pev, pev->origin );

	Vector pos, angles;
	seg[0]->GetAttachment( 0, pos, angles );
	Vector initialDir = m_Gravity.Normalize();
	for ( i = 1; i < m_iSegments - 1; i++ )
	{
		sample = m_CurrentSys[i];
		seg[i] = CRopeSegment::CreateSegment( sample, getBodyModel() );
		altseg[i] = CRopeSegment::CreateSegment( sample, getBodyModel() );
		seg[i - 1]->GetAttachment( 0, pos, angles );
		float length = (pos - seg[i - 1]->pev->origin).Length();
		pos = initialDir * length + seg[i - 1]->pev->origin;
		UTIL_SetOrigin( seg[i]->pev, pos );
		UTIL_SetOrigin( altseg[i]->pev, pos );
	}

	sample = m_CurrentSys[i];
	seg[i] = CRopeSegment::CreateSegment( sample, getEndingModel() );
	altseg[i] = CRopeSegment::CreateSegment( sample, getEndingModel() );
	seg[i - 1]->GetAttachment( 0, pos, angles );
	float length = (pos - seg[i - 1]->pev->origin).Length();
	pos = initialDir * length + seg[i - 1]->pev->origin;
	UTIL_SetOrigin( seg[i]->pev, pos );
	UTIL_SetOrigin( altseg[i]->pev, pos );
	for ( i = m_iSegments; i < MAX_ROPE_SEGMENTS; i++ )
	{
		seg[i] = NULL;
		altseg[i] = NULL;
	}
	for ( i = 0; i < ARRAYSIZE( m_TempSys ); i++ )
		m_TempSys[i] = NULL;
	m_SpringCnt = 0;
	m_InitialDeltaTime = true;
	m_HookConstant = 2500;
	m_SpringDampning = 0.1f;
	initializeRopeSim();
	pev->nextthink = gpGlobals->time + 0.01;
}

void CRope::initializeRopeSim()
{
	if ( !this )
	{
		ALERT( at_console, "CRope::initializeRopeSim() this == NULL" );
		return;
	}
	if ( m_iSegments < 2 || m_iSegments >= MAX_ROPE_SEGMENTS || m_NumSamples != m_iSegments + 1 )
		return;
	for ( int i = 0; i < ARRAYSIZE( m_TempSys ); i++ )
	{
		if ( m_TempSys[i] )
			free( m_TempSys[i] );
	}

	int i;
	for ( i = 0; i <= m_iSegments; i++ )
	{
		m_TargetSys[i] = CRopeSample::CreateSample();
		m_TargetSys[i]->mMasterRope = this;
	}
	for ( ; i < MAX_ROPE_SAMPLES; i++ )
		m_TargetSys[i] = NULL;
	for ( i = 0; i < ARRAYSIZE( m_TempSys ); i++ )
	{
		m_TempSys[i] = (RopeSampleData*)malloc( sizeof( RopeSampleData ) * m_NumSamples );
		if ( m_TempSys[i] )
		{
			for ( int sample = 0; sample < m_NumSamples; sample++ )
			{
				m_TempSys[i][sample].mApplyExternalForce = false;
				m_TempSys[i][sample].mExternalForce = g_vecZero;
			}
		}
	}

	RopeSampleData& first = m_CurrentSys[0]->data;
	first.mPosition = seg[0]->pev->origin;
	first.mMassReciprocal = 0;
	first.mVelocity = g_vecZero;
	first.mForce = g_vecZero;
	first.mApplyExternalForce = false;
	first.mExternalForce = g_vecZero;
	seg[0]->setDefaultMass( first.mMassReciprocal );
	for ( i = 1; i < m_NumSamples - 1; i++ )
	{
		RopeSampleData& data = m_CurrentSys[i]->data;
		data.mPosition = seg[i]->pev->origin;
		data.mVelocity = g_vecZero;
		data.mForce = g_vecZero;
		data.mMassReciprocal = 1;
		data.mApplyExternalForce = false;
		data.mExternalForce = g_vecZero;
		seg[i]->setDefaultMass( data.mMassReciprocal );
	}

	Vector pos, angles;
	seg[m_iSegments - 1]->GetAttachment( 0, pos, angles );
	float length = (pos - seg[m_iSegments - 1]->pev->origin).Length();
	pos = m_Gravity.Normalize() * length + seg[m_iSegments - 1]->pev->origin;
	RopeSampleData& end = m_CurrentSys[i]->data;
	end.mPosition = pos;
	m_LastEndPos = end.mPosition;
	end.mVelocity = g_vecZero;
	end.mForce = g_vecZero;
	end.mMassReciprocal = 0.2f;
	end.mApplyExternalForce = false;
	end.mExternalForce = g_vecZero;

	int fixedSegments = min( 4, m_iSegments );
	for ( i = 0; i < fixedSegments; i++ )
	{
		seg[i]->setCanBeGrabbed( false );
		altseg[i]->setCanBeGrabbed( false );
	}
}

void CRope::initializeSprings( int numSprings )
{
	m_SpringCnt = numSprings;
	m_Spring = (Spring*)malloc( sizeof( Spring ) * numSprings );
	if ( !m_Spring )
	{
		m_SpringCnt = 0;
		return;
	}
	for ( int i = 0; i < m_SpringCnt; i++ )
	{
		m_Spring[i].p1 = i;
		m_Spring[i].p2 = i + 1;
		m_Spring[i].restLength = getSegmentLength( i );
		m_Spring[i].hookConstant = m_HookConstant;
		m_Spring[i].springDampning = m_SpringDampning;
	}
	mSpringsInitialized = true;
}

float CRope::getSegmentLength( int segmentIndex ) const
{
	if ( !isValidSegmentIndex( segmentIndex ) )
		return 0;
	Vector origin = seg[segmentIndex]->pev->origin;
	Vector attachment, angles;
	seg[segmentIndex]->GetAttachment( 0, attachment, angles );
	return (origin - attachment).Length();
}

bool CRope::moveUp( float deltaTime )
{
	if ( !this )
	{
		ALERT( at_console, "CRope::moveUp() this == NULL" );
		return true;
	}
	if ( mAttachedObjectsSegment <= 4 )
		return true;

	float distance = deltaTime * 128;
	while ( distance > 0 && mAttachedObjectsSegment > 3 )
	{
		float remaining = distance;
		distance = 0;
		if ( remaining > mAttachedObjectsOffset )
		{
			distance = remaining - mAttachedObjectsOffset;
			mAttachedObjectsSegment--;
			mAttachedObjectsOffset = getSegmentLength( mAttachedObjectsSegment );
		}
		else
		{
			mAttachedObjectsOffset -= remaining;
		}
	}
	return true;
}

bool CRope::moveDown( float deltaTime )
{
	if ( !this )
	{
		ALERT( at_console, "CRope::moveDown() this == NULL" );
		return true;
	}
	bool attached = true;
	if ( mObjectAttached )
	{
		float distance = deltaTime * 128;
		while ( distance > 0 && attached )
		{
			float remaining = getSegmentLength( mAttachedObjectsSegment ) - mAttachedObjectsOffset;
			if ( distance <= remaining )
			{
				mAttachedObjectsOffset += distance;
				distance = 0;
			}
			else
			{
				distance -= remaining;
				if ( mAttachedObjectsSegment + 1 == m_iSegments )
					attached = false;
				else
					mAttachedObjectsSegment++;
				mAttachedObjectsOffset = 0;
			}
		}
	}
	return attached;
}

Vector CRope::getAttachedObjectsVelocity() const
{
	if ( !this )
	{
		ALERT( at_console, "CRope::getAttachedObjectsVelocity() this == NULL" );
		return g_vecZero;
	}
	if ( mObjectAttached )
		return seg[mAttachedObjectsSegment]->m_Sample->data.mVelocity;
	return g_vecZero;
}

void CRope::applyForceFromPlayer( const Vector& force )
{
	if ( !this )
	{
		ALERT( at_console, "CRope::applyForceFromPlayer() this == NULL" );
		return;
	}
	if ( !mObjectAttached )
		return;

	double scale = 20000;
	if ( m_iSegments < 26 )
		scale *= m_iSegments / 26.0;
	Vector scaledForce( (float)(force.x * scale), (float)(force.y * scale), (float)(force.z * scale) );
	applyForceToSegment( scaledForce, mAttachedObjectsSegment );
}

void CRope::applyForceToSegment( const Vector& force, int segment )
{
	if ( !this )
	{
		ALERT( at_console, "CRope::applyForceToSegment() this == NULL" );
		return;
	}
	if ( isValidSegmentIndex( segment ) )
		seg[segment]->applyExternalForce( force );
	if ( segment == m_iSegments )
	{
		m_CurrentSys[segment]->data.mExternalForce = m_CurrentSys[segment]->data.mExternalForce + force;
		m_CurrentSys[segment]->data.mApplyExternalForce = true;
	}
}

void CRope::attachObjectToSegment( CRopeSegment* segment )
{
	if ( !this )
	{
		ALERT( at_console, "CRope::attachObject() this == NULL" );
		return;
	}
	mObjectAttached = true;
	detachTime = 0;
	setAttachedObjectsSegment( segment );
	mAttachedObjectsOffset = 0;
}

void CRope::detachObject()
{
	if ( !this )
	{
		ALERT( at_console, "CRope::detachObject() this == NULL" );
		return;
	}
	mObjectAttached = false;
	detachTime = gpGlobals->time;
}

bool CRope::isObjectAttached() const
{
	if ( !this )
	{
		ALERT( at_console, "CRope::isObjectAttached() this == NULL" );
		return false;
	}
	return mObjectAttached;
}

bool CRope::isAcceptingAttachment() const
{
	if ( !this )
	{
		ALERT( at_console, "CRope::isAcceptingAttachment() this == NULL" );
		return false;
	}
	return gpGlobals->time - detachTime > 2 && !mObjectAttached && mDisallowPlayerAttachment != 1;
}

float CRope::getRopeLength() const
{
	float length = 0;
	for ( int i = 0; i < m_iSegments; i++ )
		length += getSegmentLength( i );
	return length;
}

Vector CRope::getRopeOrigin() const
{
	return m_CurrentSys[0]->data.mPosition;
}

bool CRope::isValidSegmentIndex( int segment ) const
{
	return segment >= 0 && segment < m_iSegments;
}

Vector CRope::getSegmentOrigin( int segment ) const
{
	Vector origin;
	if ( isValidSegmentIndex( segment ) )
		origin = m_CurrentSys[segment]->data.mPosition;
	return origin;
}

Vector CRope::getSegmentAttachmentPoint( int segment ) const
{
	Vector origin, angles;
	if ( isValidSegmentIndex( segment ) )
	{
		CRopeSegment* ropeSegment = m_bToggle ? altseg[segment] : seg[segment];
		ropeSegment->GetAttachment( 0, origin, angles );
	}
	return origin;
}

bool CRope::isSoundAllowed() const
{
	return mMakeSound == 1;
}

void CRope::setAttachedObjectsSegment( CRopeSegment* segment )
{
	if ( !this )
	{
		ALERT( at_console, "CRope::setAttachedObjectsSegment() this == NULL" );
		return;
	}
	for ( int i = 0; i < m_iSegments; i++ )
	{
		if ( seg[i] == segment || altseg[i] == segment )
		{
			mAttachedObjectsSegment = i;
			return;
		}
	}
}

Vector CRope::getSegmentDirFromOrigin( int segmentIndex ) const
{
	if ( !isValidSegmentIndex( segmentIndex ) )
		return g_vecZero;
	return (m_CurrentSys[segmentIndex + 1]->data.mPosition - m_CurrentSys[segmentIndex]->data.mPosition).Normalize();
}

Vector CRope::getAttachedObjectsPosition() const
{
	if ( !this )
	{
		ALERT( at_console, "CRope::getAttachedObjectsPosition() this == NULL" );
		return g_vecZero;
	}
	if ( !mObjectAttached )
		return g_vecZero;
	Vector position = getSegmentOrigin( mAttachedObjectsSegment );
	Vector direction = getSegmentDirFromOrigin( mAttachedObjectsSegment );
	return direction * mAttachedObjectsOffset + position;
}

bool CRope::shouldCreak() const
{
	if ( !this )
	{
		ALERT( at_console, "CRope::isObjectAttached() this == NULL" );
		return false;
	}
	return mObjectAttached && mMakeSound == 1
		&& seg[mAttachedObjectsSegment]->m_Sample->data.mVelocity.Length() > 20
		&& RANDOM_LONG( 1, 5 ) == 1;
}

void CRope::creak() const
{
	EMIT_SOUND( ENT( pev ), CHAN_BODY, CREAK_SOUNDS[RANDOM_LONG( 0, 2 )], 1, ATTN_NORM );
}

void getAlignmentAngles( const Vector& top, const Vector& bot, Vector* out )
{
	static Vector down( 0, 0, -1 );
	static Vector right( 0, 1, 0 );
	Vector direction = (bot - top).Normalize();
	float angle = (float)(acos( (double)DotProduct( direction, right ) ) * 57.29578f);
	out->z = -angle;
	direction = bot - top;
	direction.y = 0;
	direction = direction.Normalize();
	angle = (float)(acos( (double)DotProduct( direction, down ) ) * 57.29578f);
	out->x = direction.x < 0 ? -angle : angle;
	out->y = 0;
}

void truncateEpsilon( Vector& vec )
{
	vec = vec * 10;
	vec.x = ceil( vec.x + 0.5 );
	vec.y = ceil( vec.y + 0.5 );
	vec.z = ceil( vec.z + 0.5 );
	vec = vec / 10;
}

void CRope::ComputeForces( RopeSampleData* system )
{
	if ( !this )
	{
		ALERT( at_console, "CRope::ComputeForces() this == NULL" );
		return;
	}
	for ( int i = 0; i < m_NumSamples; i++ )
	{
		RopeSampleData& data = system[i];
		data.mForce = g_vecZero;
		if ( data.mMassReciprocal != 0 )
			data.mForce = data.mForce + m_Gravity / data.mMassReciprocal;
		if ( data.mApplyExternalForce )
		{
			data.mForce = data.mForce + data.mExternalForce;
			data.mExternalForce = g_vecZero;
			data.mApplyExternalForce = false;
		}
		if ( DotProduct( m_Gravity, data.mVelocity ) >= 0 )
			data.mForce = data.mForce + data.mVelocity * -0.04f;
		else
			data.mForce = data.mForce - data.mVelocity;
	}
	for ( int i = 0; i < m_SpringCnt; i++ )
	{
		Spring& spring = m_Spring[i];
		RopeSampleData& first = system[spring.p1];
		RopeSampleData& second = system[spring.p2];
		Vector deltaPosition = first.mPosition - second.mPosition;
		Vector deltaVelocity = first.mVelocity - second.mVelocity;
		double length = deltaPosition.Length();
		double extension = (length - spring.restLength) * spring.hookConstant;
		double damping = DotProduct( deltaVelocity, deltaPosition ) * spring.springDampning;
		double scale = -(damping / length + extension);
		double inverseLength = 1.0 / length;
		for ( int axis = 0; axis < 3; axis++ )
		{
			double force = deltaPosition[axis] * inverseLength * scale;
			first.mForce[axis] = (float)(first.mForce[axis] + force);
			second.mForce[axis] = (float)(second.mForce[axis] - force);
		}
	}
}

void CRope::ComputeForces( CRopeSample** system )
{
	if ( !this )
	{
		ALERT( at_console, "CRope::ComputeForces() this == NULL" );
		return;
	}
	for ( int i = 0; i < m_NumSamples; i++ )
	{
		RopeSampleData& data = system[i]->data;
		data.mForce = g_vecZero;
		if ( data.mMassReciprocal != 0 )
			data.mForce = data.mForce + m_Gravity / data.mMassReciprocal;
		if ( data.mApplyExternalForce )
		{
			data.mForce = data.mForce + data.mExternalForce;
			data.mApplyExternalForce = false;
			data.mExternalForce = g_vecZero;
		}
		if ( DotProduct( m_Gravity, data.mVelocity ) >= 0 )
			data.mForce = data.mForce + data.mVelocity * -0.04f;
		else
			data.mForce = data.mForce - data.mVelocity;
	}
	for ( int i = 0; i < m_SpringCnt; i++ )
	{
		Spring& spring = m_Spring[i];
		RopeSampleData& first = system[spring.p1]->data;
		RopeSampleData& second = system[spring.p2]->data;
		Vector deltaPosition = first.mPosition - second.mPosition;
		Vector deltaVelocity = first.mVelocity - second.mVelocity;
		double length = deltaPosition.Length();
		double extension = (length - spring.restLength) * spring.hookConstant;
		double damping = DotProduct( deltaVelocity, deltaPosition ) * spring.springDampning;
		double scale = -(damping / length + extension);
		double inverseLength = 1.0 / length;
		for ( int axis = 0; axis < 3; axis++ )
		{
			double force = deltaPosition[axis] * inverseLength * scale;
			first.mForce[axis] = (float)(first.mForce[axis] + force);
			second.mForce[axis] = (float)(second.mForce[axis] - force);
		}
	}
}

void CRope::RK4Integrate( float DeltaTime, CRopeSample** sampleSource, CRopeSample** sampleTarget )
{
	RopeSampleData* system = m_TempSys[0];
	RopeSampleData* accum1 = m_TempSys[1];
	RopeSampleData* accum2 = m_TempSys[2];
	RopeSampleData* accum3 = m_TempSys[3];
	RopeSampleData* accum4 = m_TempSys[4];
	float halfDeltaTime = (float)(DeltaTime * 0.5);

	for ( int i = 0; i < m_NumSamples; i++ )
	{
		RopeSampleData& source = sampleSource[i]->data;
		for ( int axis = 0; axis < 3; axis++ )
		{
			double deltaVelocity = (double)source.mForce[axis] * halfDeltaTime * source.mMassReciprocal;
			accum1[i].mForce[axis] = (float)deltaVelocity;
			accum1[i].mVelocity[axis] = source.mVelocity[axis] * halfDeltaTime;
			system[i].mVelocity[axis] = (float)(deltaVelocity + source.mVelocity[axis]);
			system[i].mPosition[axis] = source.mPosition[axis] + accum1[i].mVelocity[axis];
		}
		system[i].mMassReciprocal = source.mMassReciprocal;
	}
	ComputeForces( system );

	for ( int i = 0; i < m_NumSamples; i++ )
	{
		RopeSampleData& source = sampleSource[i]->data;
		for ( int axis = 0; axis < 3; axis++ )
		{
			double deltaVelocity = (double)system[i].mForce[axis] * halfDeltaTime * source.mMassReciprocal;
			accum2[i].mForce[axis] = (float)deltaVelocity;
			accum2[i].mVelocity[axis] = system[i].mVelocity[axis] * halfDeltaTime;
			system[i].mVelocity[axis] = (float)(deltaVelocity + source.mVelocity[axis]);
			system[i].mPosition[axis] = source.mPosition[axis] + accum2[i].mVelocity[axis];
		}
		system[i].mMassReciprocal = source.mMassReciprocal;
	}
	ComputeForces( system );

	for ( int i = 0; i < m_NumSamples; i++ )
	{
		RopeSampleData& source = sampleSource[i]->data;
		for ( int axis = 0; axis < 3; axis++ )
		{
			double deltaVelocity = (double)system[i].mForce[axis] * DeltaTime * source.mMassReciprocal;
			accum3[i].mForce[axis] = (float)deltaVelocity;
			accum3[i].mVelocity[axis] = system[i].mVelocity[axis] * DeltaTime;
			system[i].mVelocity[axis] = (float)(deltaVelocity + source.mVelocity[axis]);
			system[i].mPosition[axis] = source.mPosition[axis] + accum3[i].mVelocity[axis];
		}
		system[i].mMassReciprocal = source.mMassReciprocal;
	}
	ComputeForces( system );

	for ( int i = 0; i < m_NumSamples; i++ )
	{
		RopeSampleData& source = sampleSource[i]->data;
		for ( int axis = 0; axis < 3; axis++ )
		{
			accum4[i].mForce[axis] = (float)((double)system[i].mForce[axis] * DeltaTime * source.mMassReciprocal);
			accum4[i].mVelocity[axis] = system[i].mVelocity[axis] * DeltaTime;
		}
	}
	for ( int i = 0; i < m_NumSamples; i++ )
	{
		RopeSampleData& source = sampleSource[i]->data;
		RopeSampleData& target = sampleTarget[i]->data;
		for ( int axis = 0; axis < 3; axis++ )
		{
			double middleForce = (double)accum2[i].mForce[axis] + accum3[i].mForce[axis];
			target.mVelocity[axis] = (float)((middleForce + middleForce + accum1[i].mForce[axis]
				+ accum4[i].mForce[axis]) * 0.16666667f + source.mVelocity[axis]);
			double middleVelocity = (double)accum2[i].mVelocity[axis] + accum3[i].mVelocity[axis];
			target.mPosition[axis] = (float)((middleVelocity + middleVelocity + accum1[i].mVelocity[axis]
				+ accum4[i].mVelocity[axis]) * 0.16666667f + source.mPosition[axis]);
		}
	}
}

void CRope::runSimOnSamples()
{
	if ( !this )
	{
		ALERT( at_console, "CRope::runSimOnSamples() this == NULL" );
		return;
	}
	CRopeSample** system = m_CurrentSys;
	CRopeSample** target = m_TargetSys;
	float deltaTime = 0.025f;
	if ( m_InitialDeltaTime )
	{
		m_InitialDeltaTime = false;
		mLastTime = gpGlobals->time;
		deltaTime = 0;
	}

	int iterations = 0;
	for ( ;; )
	{
		iterations++;
		ComputeForces( system );
		RK4Integrate( deltaTime, system, target );
		mLastTime = (float)(mLastTime + 0.007);
		if ( gpGlobals->time <= mLastTime && (iterations & 1) )
			break;
		CRopeSample** swap = target;
		target = system;
		system = swap;
	}
	mLastTime = gpGlobals->time;
}

void CRope::traceModels( CRopeSegment** primarySegs, CRopeSegment** hiddenSegs )
{
	if ( m_iSegments > 1 )
		getAlignmentAngles( m_CurrentSys[0]->data.mPosition, m_CurrentSys[1]->data.mPosition,
			&primarySegs[0]->pev->angles );

	TraceResult trace;
	for ( int i = 1; i < m_iSegments; i++ )
	{
		if ( mObjectAttached )
		{
			int distance = (unsigned int)(i - mAttachedObjectsSegment + 2) < 5 ? 50 : 10;
			Vector extension = (m_CurrentSys[i]->data.mPosition - hiddenSegs[i]->pev->origin).Normalize() * distance;
			Vector dest = m_CurrentSys[i]->data.mPosition + extension;
			UTIL_TraceLine( hiddenSegs[i]->pev->origin, dest, ignore_monsters, edict(), &trace );
			if ( trace.flFraction != 1 || trace.fAllSolid || trace.fStartSolid || !trace.fInOpen )
			{
				primarySegs[i]->pev->origin = trace.vecEndPos - extension;
				truncateEpsilon( primarySegs[i]->pev->origin );
				RopeSampleData& data = primarySegs[i]->m_Sample->data;
				data.mApplyExternalForce = true;
				data.mExternalForce = trace.vecPlaneNormal.Normalize() * 20000;
				data.mVelocity = g_vecZero;
			}
			else
			{
				primarySegs[i]->pev->origin = m_CurrentSys[i]->data.mPosition;
				truncateEpsilon( primarySegs[i]->pev->origin );
			}
		}
		else
		{
			UTIL_TraceLine( hiddenSegs[i]->pev->origin, m_CurrentSys[i]->data.mPosition,
				ignore_monsters, edict(), &trace );
			if ( trace.flFraction == 1 )
			{
				primarySegs[i]->pev->origin = m_CurrentSys[i]->data.mPosition;
				truncateEpsilon( primarySegs[i]->pev->origin );
			}
			else
			{
				Vector normal = trace.vecPlaneNormal.Normalize();
				primarySegs[i]->pev->origin = normal * 10 + trace.vecEndPos;
				truncateEpsilon( primarySegs[i]->pev->origin );
				RopeSampleData& data = primarySegs[i]->m_Sample->data;
				data.mApplyExternalForce = true;
				data.mExternalForce = normal * 40000;
			}
		}
	}

	for ( int i = 1; i < m_iSegments; i++ )
		getAlignmentAngles( primarySegs[i - 1]->pev->origin, primarySegs[i]->pev->origin,
			&primarySegs[i - 1]->pev->angles );
	if ( m_iSegments > 1 )
	{
		UTIL_TraceLine( m_LastEndPos, m_CurrentSys[m_NumSamples - 1]->data.mPosition,
			ignore_monsters, edict(), &trace );
		if ( trace.flFraction == 1 )
		{
			m_LastEndPos = m_CurrentSys[m_iSegments]->data.mPosition;
		}
		else
		{
			m_LastEndPos = trace.vecEndPos;
			RopeSampleData& data = m_CurrentSys[m_iSegments]->data;
			data.mApplyExternalForce = true;
			data.mExternalForce = trace.vecPlaneNormal.Normalize() * 40000;
		}
		getAlignmentAngles( primarySegs[m_iSegments - 1]->pev->origin, m_LastEndPos,
			&primarySegs[m_iSegments - 1]->pev->angles );
	}
}

void CRope::setRopeSegments( int numSegs, CRopeSegment** primarySegs, CRopeSegment** hiddenSegs )
{
	if ( numSegs <= 0 )
		return;
	traceModels( primarySegs, hiddenSegs );
	for ( int i = 0; i < numSegs; i++ )
	{
		primarySegs[i]->pev->solid = SOLID_TRIGGER;
		primarySegs[i]->pev->effects = 0;
		hiddenSegs[i]->pev->effects = EF_NODRAW;
		hiddenSegs[i]->pev->solid = SOLID_NOT;
		if ( i )
			hiddenSegs[i]->pev->origin = primarySegs[i]->pev->origin + Vector( 10, 10, 0 );
	}
}

void CRope::Think()
{
	if ( !this )
	{
		ALERT( at_console, "CRope::Think() this == NULL" );
		return;
	}
	if ( !mSpringsInitialized )
		initializeSprings( m_iSegments );
	m_bToggle = !m_bToggle;
	runSimOnSamples();
	if ( m_bToggle )
		setRopeSegments( m_iSegments, altseg, seg );
	else
		setRopeSegments( m_iSegments, seg, altseg );
	if ( shouldCreak() )
		creak();
	pev->nextthink = gpGlobals->time + 0.001;
}
