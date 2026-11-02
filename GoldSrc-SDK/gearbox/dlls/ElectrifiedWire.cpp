#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "ElectrifiedWire.h"

LINK_ENTITY_TO_CLASS( env_electrified_wire, CElectrifiedWire );

TYPEDESCRIPTION CElectrifiedWire::m_SaveData[] =
{
	DEFINE_FIELD( CElectrifiedWire, mIsActive, FIELD_CHARACTER ),
	DEFINE_FIELD( CElectrifiedWire, mTipSparkFrequency, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, mBodySparkFrequency, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, mLightningFrequency, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, mXJoltForce, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, mYJoltForce, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, mXJoltForce, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, mNumUninsulatedSegments, FIELD_INTEGER ),
	DEFINE_ARRAY( CElectrifiedWire, mUninsulatedSegments, FIELD_INTEGER, MAX_ROPE_SEGMENTS ),
	DEFINE_FIELD( CElectrifiedWire, mLightningSprite, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, mLastSparkTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CElectrifiedWire, CRope );

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CElectrifiedWire::CElectrifiedWire()
{
	mIsActive = true;
	mTipSparkFrequency = 3;
	mBodySparkFrequency = 100;
	mLightningFrequency = 150;
	mXJoltForce = 0;
	mYJoltForce = 0;
	mZJoltForce = 0;
	mNumUninsulatedSegments = 0;
}

int CElectrifiedWire::getBodyModel() const
{
	return mBodyModel;
}

int CElectrifiedWire::getEndingModel() const
{
	return mEndingModel;
}

void CElectrifiedWire::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	mIsActive = !mIsActive;
	for ( int i = 0; i < mNumUninsulatedSegments; i++ )
	{
		seg[mUninsulatedSegments[i]]->setCauseDamageOnTouch( mIsActive );
		altseg[mUninsulatedSegments[i]]->setCauseDamageOnTouch( mIsActive );
	}
	if ( mTipSparkFrequency > 0 )
	{
		seg[m_iSegments - 1]->setCauseDamageOnTouch( mIsActive );
		altseg[m_iSegments - 1]->setCauseDamageOnTouch( mIsActive );
	}
}

void CElectrifiedWire::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "sparkfrequency" ) )
	{
		mTipSparkFrequency = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "bodysparkfrequency" ) )
	{
		mBodySparkFrequency = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "lightningfrequency" ) )
	{
		mLightningFrequency = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "xforce" ) )
	{
		mXJoltForce = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "yforce" ) )
	{
		mYJoltForce = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "zforce" ) )
	{
		mZJoltForce = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( !pkvd->fHandled )
	{
		CRope::KeyValue( pkvd );
	}
}

void CElectrifiedWire::Spawn()
{
	mLightningSprite = PRECACHE_MODEL( "sprites/lgtning.spr" );
	mNumUninsulatedSegments = 0;
	CRope::Spawn();
	pev->classname = MAKE_STRING( "env_electrified_wire" );
	mIsActive = true;
	if ( mBodySparkFrequency > 0 )
	{
		for ( int i = 0; i < m_iSegments - 1; i++ )
			addUninsulatedSegment( i );
	}
	for ( int i = 0; i < mNumUninsulatedSegments; i++ )
	{
		seg[mUninsulatedSegments[i]]->setCauseDamageOnTouch( mIsActive );
		altseg[mUninsulatedSegments[i]]->setCauseDamageOnTouch( mIsActive );
	}
	if ( mTipSparkFrequency > 0 )
	{
		seg[m_iSegments - 1]->setCauseDamageOnTouch( mIsActive );
		altseg[m_iSegments - 1]->setCauseDamageOnTouch( mIsActive );
	}
	mLastSparkTime = gpGlobals->time;
	mMakeSound = 0;
}

bool CElectrifiedWire::isActive() const
{
	return mIsActive;
}

bool CElectrifiedWire::shouldDoEffect( int frequency ) const
{
	return frequency > 0 && mIsActive && RANDOM_LONG( 1, frequency ) == 1;
}

void CElectrifiedWire::doSpark( int segment, bool exertForce )
{
	Vector position = getSegmentAttachmentPoint( segment );
	UTIL_Sparks( position );
	if ( exertForce )
	{
		Vector sparkForce;
		sparkForce.x = RANDOM_FLOAT( (float)-mXJoltForce, (float)mXJoltForce );
		sparkForce.y = RANDOM_FLOAT( (float)-mYJoltForce, (float)mYJoltForce );
		sparkForce.z = RANDOM_FLOAT( (float)-mZJoltForce, (float)mZJoltForce );
		applyForceToSegment( sparkForce, segment );
	}
}

void CElectrifiedWire::doLightning()
{
	int first = RANDOM_LONG( 0, m_iSegments - 1 );
	int second;
	int attempts = 0;
	do
	{
		attempts++;
		second = RANDOM_LONG( 0, m_iSegments - 1 );
	}
	while ( attempts <= 9 && first == second );
	if ( attempts > 9 )
		return;

	CRopeSegment** segments = m_bToggle ? altseg : seg;
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMENTS );
		WRITE_SHORT( ENTINDEX( segments[first]->edict() ) );
		WRITE_SHORT( ENTINDEX( segments[second]->edict() ) );
		WRITE_SHORT( mLightningSprite );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 1 );
		WRITE_BYTE( 10 );
		WRITE_BYTE( 80 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
	MESSAGE_END();
}

void CElectrifiedWire::Think()
{
	if ( gpGlobals->time - mLastSparkTime > 0.1f )
	{
		mLastSparkTime = gpGlobals->time;
		for ( int i = 0; i < mNumUninsulatedSegments; i++ )
		{
			if ( shouldDoEffect( mBodySparkFrequency ) )
				doSpark( mUninsulatedSegments[i], false );
		}
		if ( shouldDoEffect( mTipSparkFrequency ) )
			doSpark( m_iSegments - 1, true );
		if ( shouldDoEffect( mLightningFrequency ) )
			doLightning();
	}
	CRope::Think();
}

void CElectrifiedWire::addUninsulatedSegment( int segment )
{
	if ( isValidSegmentIndex( segment ) )
		mUninsulatedSegments[mNumUninsulatedSegments++] = segment;
}
