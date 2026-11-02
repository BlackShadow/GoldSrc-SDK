#ifndef ROPESEGMENT_H
#define ROPESEGMENT_H

#include "RopeSample.h"

class CRopeSegment : public CBaseAnimating
{
public:
	CRopeSegment();
	void Spawn() override;
	void Precache() override;
	void Think() override;
	void Touch( CBaseEntity* pOther ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static CRopeSegment* CreateSegment( CRopeSample* sample, int modelName );
	static TYPEDESCRIPTION m_SaveData[];

	void applyExternalForce( const Vector& force );
	void setMassToDefault();
	void setDefaultMass( float defaultMass );
	void setMass( float mass );
	void setCauseDamageOnTouch( bool causeDamage );
	void setCanBeGrabbed( bool canBeGrabbed );

	CRopeSample*	m_Sample;
	int				mModelName;
	float			mDefaultMass;
	bool			mCauseDamage;
	bool			mCanBeGrabbed;
};

#endif
