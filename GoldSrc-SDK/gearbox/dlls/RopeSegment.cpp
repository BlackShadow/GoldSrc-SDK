#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "player.h"
#include "RopeSegment.h"
#include "rope.h"

LINK_ENTITY_TO_CLASS( rope_segment, CRopeSegment );

TYPEDESCRIPTION CRopeSegment::m_SaveData[] =
{
	DEFINE_FIELD( CRopeSegment, m_Sample, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRopeSegment, mDefaultMass, FIELD_FLOAT ),
	DEFINE_FIELD( CRopeSegment, mCauseDamage, FIELD_CHARACTER ),
	DEFINE_FIELD( CRopeSegment, mCanBeGrabbed, FIELD_CHARACTER ),
	DEFINE_FIELD( CRopeSegment, mModelName, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CRopeSegment, CBaseAnimating );

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CRopeSegment::CRopeSegment()
{
	mModelName = ALLOC_STRING( "models/rope16.mdl" );
}

void CRopeSegment::Precache()
{
	PRECACHE_MODEL( (char*)STRING( mModelName ) );
	PRECACHE_SOUND( "items/grab_rope.wav" );
}

void CRopeSegment::Think()
{
}

void CRopeSegment::Spawn()
{
	Precache();
	SET_MODEL( edict(), STRING( mModelName ) );
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_TRIGGER;
	pev->flags |= FL_ALWAYSTHINK;
	pev->effects = EF_NODRAW;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -30, -30, -30 ), Vector( 30, 30, 30 ) );
	pev->nextthink = gpGlobals->time + 0.5;
}

void CRopeSegment::Touch( CBaseEntity* pOther )
{
	if ( !this )
	{
		ALERT( at_console, "CRopeSegment::Touch() this == NULL" );
		return;
	}
	if ( !pOther->IsPlayer() )
		return;

	if ( mCauseDamage )
		pOther->TakeDamage( pev, pev, 1, DMG_SHOCK );

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;
	if ( !m_Sample->mMasterRope->isAcceptingAttachment() || pPlayer->isOnRope() )
		return;

	if ( !mCanBeGrabbed )
	{
		CRope* rope = m_Sample->mMasterRope;
		CRopeSegment* segment;
		if ( rope->m_iSegments <= 4 )
		{
			segment = rope->seg[rope->m_iSegments - 1];
			segment->mCanBeGrabbed = true;
		}
		else
		{
			segment = rope->seg[4];
		}
		segment->Touch( pOther );
		return;
	}

	pOther->pev->origin = m_Sample->data.mPosition;
	pPlayer->setOnRopeState( true );
	pPlayer->setRope( m_Sample->mMasterRope );
	m_Sample->mMasterRope->attachObjectToSegment( this );
	if ( pOther->pev->velocity.Length() > 0.5 )
	{
		m_Sample->data.mApplyExternalForce = true;
		m_Sample->data.mExternalForce = pOther->pev->velocity * 750 + m_Sample->data.mExternalForce;
	}
	if ( m_Sample->mMasterRope->isSoundAllowed() )
		EMIT_SOUND( edict(), CHAN_BODY, "items/grab_rope.wav", 1, ATTN_NORM );
}

CRopeSegment* CRopeSegment::CreateSegment( CRopeSample* sample, int modelName )
{
	CRopeSegment* segment = GetClassPtr( (CRopeSegment*)NULL );
	segment->pev->classname = MAKE_STRING( "rope_segment" );
	segment->mModelName = modelName;
	segment->Spawn();
	segment->m_Sample = sample;
	segment->mDefaultMass = sample->data.mMassReciprocal;
	segment->mCauseDamage = false;
	segment->mCanBeGrabbed = true;
	return segment;
}

void CRopeSegment::applyExternalForce( const Vector& force )
{
	if ( !this )
	{
		ALERT( at_console, "CRopeSegment::applyExternalForce() this == NULL" );
		return;
	}
	m_Sample->data.mApplyExternalForce = true;
	m_Sample->data.mExternalForce = m_Sample->data.mExternalForce + force;
}

void CRopeSegment::setMassToDefault()
{
	m_Sample->data.mMassReciprocal = mDefaultMass;
}

void CRopeSegment::setDefaultMass( float defaultMass )
{
	mDefaultMass = defaultMass;
}

void CRopeSegment::setMass( float mass )
{
	m_Sample->data.mMassReciprocal = mass;
}

void CRopeSegment::setCauseDamageOnTouch( bool causeDamage )
{
	mCauseDamage = causeDamage;
}

void CRopeSegment::setCanBeGrabbed( bool canBeGrabbed )
{
	mCanBeGrabbed = canBeGrabbed;
}
