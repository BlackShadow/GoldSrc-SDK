#ifndef ROPESAMPLE_H
#define ROPESAMPLE_H

class CRope;

struct RopeSampleData
{
	Vector	mPosition;
	Vector	mVelocity;
	Vector	mForce;
	Vector	mExternalForce;
	bool	mApplyExternalForce;
	float	mMassReciprocal;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CRopeSample : public CBaseEntity
{
public:
	void Spawn() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static CRopeSample* CreateSample();
	static TYPEDESCRIPTION m_SaveData[];

	RopeSampleData	data;
	CRope*			mMasterRope;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

#endif
