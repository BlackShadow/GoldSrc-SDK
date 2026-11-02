#include "hostage/hostage_improv.h"

void HostageRetreatState::OnEnter( CHostageImprov *improv )
{
	improv->Walk();
	improv->MoveTo( improv->GetEntity()->GetHomePosition() );
}

void HostageRetreatState::OnExit( CHostageImprov * ) {}

void HostageRetreatState::OnUpdate( CHostageImprov *improv )
{
	if ( improv->IsAtHome() )
	{
		improv->Stop();
		improv->Idle();
		return;
	}
	CBasePlayer *player = improv->GetClosestVisiblePlayer( 0 );
	if ( player )
	{
		Vector delta = player->pev->origin - improv->GetCentroid();
		if ( player->m_iTeam == CT && (DotProduct( delta, delta ) <= 160000 || !improv->IsScared()) )
		{
			improv->Stop();
			improv->Idle();
			return;
		}
	}
	if ( improv->IsScared() && improv->GetScareIntensity() == CHostageImprov::TERRIFIED )
		improv->Run();
	else
		improv->Walk();
}
