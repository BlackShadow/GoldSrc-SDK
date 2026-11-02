#ifndef ELECTRIFIEDWIRE_H
#define ELECTRIFIEDWIRE_H

#include "rope.h"

class CElectrifiedWire : public CRope
{
public:
	CElectrifiedWire();
	void Spawn() override;
	void Think() override;
	void KeyValue( KeyValueData* pkvd ) override;
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	int getBodyModel() const override;
	int getEndingModel() const override;

	bool isActive() const;
	bool shouldDoEffect( int frequency ) const;
	void doSpark( int segment, bool exertForce );
	void doLightning();
	void addUninsulatedSegment( int segment );

	bool	mIsActive;
	int		mTipSparkFrequency;
	int		mBodySparkFrequency;
	int		mLightningFrequency;
	int		mXJoltForce;
	int		mYJoltForce;
	int		mZJoltForce;
	int		mNumUninsulatedSegments;
	int		mUninsulatedSegments[MAX_ROPE_SEGMENTS];
	int		mLightningSprite;
	float	mLastSparkTime;
};

#endif
