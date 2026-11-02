#include "hostage/hostage_improv.h"
#include "animation.h"

void HostageAnimateState::Reset()
{
	m_sequenceCount = 0;
	m_performance = None;
}

void HostageAnimateState::OnEnter( CHostageImprov * ) {}
void HostageAnimateState::OnExit( CHostageImprov * ) {}

void HostageAnimateState::StartSequence( CHostageImprov *improv, const SeqInfo *sequence )
{
	if ( sequence->seqID >= 0 )
	{
		CHostage *hostage = improv->GetEntity();
		hostage->pev->sequence = sequence->seqID;
		hostage->ResetSequenceInfo();
		hostage->m_fSequenceLoops = sequence->holdTime > 0;
		hostage->pev->frame = sequence->rate >= 0 ? 0 : 255;
		hostage->pev->framerate *= sequence->rate;
	}
	m_isHolding = false;
}

void HostageAnimateState::AddSequence( CHostageImprov *improv, const char *name, float holdTime, float rate )
{
	if ( m_sequenceCount > 7 )
		return;
	m_sequence[m_sequenceCount].seqID = name ? improv->GetEntity()->LookupSequence( name ) : -1;
	m_sequence[m_sequenceCount].holdTime = holdTime;
	m_sequence[m_sequenceCount].rate = rate;
	++m_sequenceCount;
	m_currentSequence = 0;
	StartSequence( improv, &m_sequence[0] );
}

void HostageAnimateState::AddSequence( CHostageImprov *improv, int activity, float holdTime, float rate )
{
	CHostage *hostage = improv->GetEntity();
	void *model = GET_MODEL_PTR( hostage->edict() );
	if ( !model )
		return;
	m_sequence[m_sequenceCount].seqID = LookupActivity( model, hostage->pev, activity );
	m_sequence[m_sequenceCount].holdTime = holdTime;
	m_sequence[m_sequenceCount].rate = rate;
	++m_sequenceCount;
	m_currentSequence = 0;
	StartSequence( improv, &m_sequence[0] );
}

bool HostageAnimateState::IsDoneHolding()
{
	float holdTime = m_sequence[m_currentSequence].holdTime;
	if ( holdTime < 0 )
		return true;
	if ( m_isHolding )
		return m_holdTimer.IsElapsed();
	m_isHolding = true;
	m_holdTimer.Start( holdTime );
	return false;
}

void HostageAnimateState::OnUpdate( CHostageImprov *improv )
{
	if ( !m_sequenceCount )
		return;
	if ( !improv->GetEntity()->m_fSequenceFinished && m_sequence[m_currentSequence].seqID >= 0 )
		return;
	if ( !IsDoneHolding() )
		return;
	++m_currentSequence;
	if ( m_currentSequence == m_sequenceCount )
		Reset();
	else
		StartSequence( improv, &m_sequence[m_currentSequence] );
}

bool HostageAnimateState::IsPlaying( CHostageImprov *improv, const char *name ) const
{
	return m_sequenceCount && m_sequence[m_currentSequence].seqID == improv->GetEntity()->LookupSequence( name );
}
