#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "RopeSample.h"

LINK_ENTITY_TO_CLASS( rope_sample, CRopeSample );

TYPEDESCRIPTION CRopeSample::m_SaveData[] =
{
	DEFINE_FIELD( CRopeSample, data.mPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CRopeSample, data.mVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, data.mForce, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, data.mExternalForce, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, data.mApplyExternalForce, FIELD_CHARACTER ),
	DEFINE_FIELD( CRopeSample, data.mMassReciprocal, FIELD_FLOAT ),
	DEFINE_FIELD( CRopeSample, mMasterRope, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CRopeSample, CBaseEntity );

void CRopeSample::Spawn()
{
}

CRopeSample* CRopeSample::CreateSample()
{
	CRopeSample* sample = GetClassPtr( (CRopeSample*)NULL );
	sample->pev->classname = MAKE_STRING( "rope_sample" );
	sample->Spawn();
	return sample;
}
