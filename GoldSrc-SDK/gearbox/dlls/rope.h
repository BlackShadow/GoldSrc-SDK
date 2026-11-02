#ifndef ROPE_H
#define ROPE_H

#include "RopeSegment.h"

#define MAX_ROPE_SEGMENTS	63
#define MAX_ROPE_SAMPLES		64

struct Spring
{
	int		p1;
	int		p2;
	float	restLength;
	float	hookConstant;
	float	springDampning;
};

void	getAlignmentAngles( const Vector& top, const Vector& bot, Vector* out );
void	truncateEpsilon( Vector& vec );

class CRope : public CBaseDelay
{
public:
	CRope();
	void Spawn() override;
	void Precache() override;
	void Think() override;
	void Touch( CBaseEntity* pOther ) override;
	void KeyValue( KeyValueData* pkvd ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	virtual int getBodyModel() const;
	virtual int getEndingModel() const;

	void initializeRopeSim();
	void ComputeForces( RopeSampleData* system );
	void ComputeForces( CRopeSample** system );
	void RK4Integrate( float DeltaTime, CRopeSample** sampleSource, CRopeSample** sampleTarget );
	void runSimOnSamples();
	float getSegmentLength( int segmentIndex ) const;
	bool moveUp( float deltaTime );
	bool moveDown( float deltaTime );
	Vector getAttachedObjectsVelocity() const;
	void applyForceFromPlayer( const Vector& force );
	void applyForceToSegment( const Vector& force, int segment );
	void attachObjectToSegment( CRopeSegment* segment );
	void detachObject();
	bool isObjectAttached() const;
	bool isAcceptingAttachment() const;
	float getRopeLength() const;
	Vector getRopeOrigin() const;
	void initializeSprings( int numSprings );
	bool isValidSegmentIndex( int segment ) const;
	Vector getSegmentOrigin( int segment ) const;
	Vector getSegmentAttachmentPoint( int segment ) const;
	bool isSoundAllowed() const;
	void setAttachedObjectsSegment( CRopeSegment* segment );
	Vector getSegmentDirFromOrigin( int segmentIndex ) const;
	Vector getAttachedObjectsPosition() const;
	bool shouldCreak() const;
	void creak() const;
	void traceModels( CRopeSegment** primarySegs, CRopeSegment** hiddenSegs );
	void setRopeSegments( int numSegs, CRopeSegment** primarySegs, CRopeSegment** hiddenSegs );

	int				m_iSegments;
	CRopeSegment*	seg[MAX_ROPE_SEGMENTS];
	CRopeSegment*	altseg[MAX_ROPE_SEGMENTS];
	bool			m_bToggle;
	bool			m_InitialDeltaTime;
	float			mLastTime;
	Vector			m_LastEndPos;
	Vector			m_Gravity;
	float			m_HookConstant;
	float			m_SpringDampning;
	CRopeSample*	m_CurrentSys[MAX_ROPE_SAMPLES];
	CRopeSample*	m_TargetSys[MAX_ROPE_SAMPLES];
	RopeSampleData*	m_TempSys[5];
	int				m_NumSamples;
	Spring*			m_Spring;
	int				m_SpringCnt;
	bool			mSpringsInitialized;
	int				m_BeamOffset;
	bool			mObjectAttached;
	int				mAttachedObjectsSegment;
	float			mAttachedObjectsOffset;
	float			detachTime;
	int				mBodyModel;
	int				mEndingModel;
	int				mDisallowPlayerAttachment;
	int				mMakeSound;
};

#endif
