#include "hud.h"
#include "cl_util.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern Vector v_angles;

float VecToYaw( float *vec )
{
	if ( vec[0] == 0 && vec[1] == 0 )
		return 0;
	float yaw = (int)(atan2(vec[1], vec[0]) * 180.0 / M_PI);
	if ( yaw < 0 )
		yaw += 360;
	return yaw;
}

CMortarHud::CMortarHud()
{
	m_TrajectoryList = new CTrajectoryList;
}

CMortarHud::~CMortarHud()
{
	delete m_TrajectoryList;
}

int CMortarHud::Init()
{
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
}

int CMortarHud::VidInit()
{
	return 1;
}

int CMortarHud::Draw( float )
{
	return !gHUD.IsInMortarDeploy();
}

void CMortarHud::CalculateFireAngle( float *pitch, float *yaw )
{
	if ( v_angles.x <= 10 )
	{
		if ( -v_angles.x <= 50 )
			*pitch = (10.0-v_angles.x)/60.0*60.0 + 30.0;
		else
			*pitch = 90;
	}
	else
		*pitch = 30;
	*yaw = v_angles.y;
}

void CMortarHud::DrawPredictedMortarImpactSite()
{
}

CTrajectoryList::CTrajectoryList()
{
	memset(m_Trajectories, 0, sizeof(m_Trajectories));
	InvalidateAllTrajectories();
}

CTrajectoryList::~CTrajectoryList()
{
}

void CTrajectoryList::InvalidateAllTrajectories()
{
	for ( int i = 0; i < 32; ++i )
		m_Trajectories[i].fLastUsedTime = -1;
}

trajectory_t *CTrajectoryList::AddTrajectory( Vector target )
{
	trajectory_t *trajectory = NULL;
	float time = gEngfuncs.GetClientTime();
	for ( int i = 0; i < 32; ++i )
	{
		if ( m_Trajectories[i].fLastUsedTime < 0 || time-m_Trajectories[i].fLastUsedTime > 30 )
		{
			trajectory = &m_Trajectories[i];
			break;
		}
	}
	if ( trajectory != NULL )
	{
		VectorCopy(target, trajectory->vTargetPos);
		trajectory->fLastUsedTime = gEngfuncs.GetClientTime();
		CalculateTrajectory(m_vecLaunchPos, Vector(trajectory->vTargetPos),
			&trajectory->fPitch1, &trajectory->fPitch2, &trajectory->fYaw);
	}
	return trajectory;
}

void CTrajectoryList::GetTrajectory( Vector launch, Vector target, float *pitch1, float *pitch2, float *yaw )
{
	trajectory_t *trajectory = NULL;
	bool cacheEmpty = true;
	for ( int i = 0; i < 32; ++i )
	{
		if ( m_Trajectories[i].fLastUsedTime >= 0 )
		{
			cacheEmpty = false;
			break;
		}
	}
	if ( cacheEmpty )
		m_vecLaunchPos = launch;
	if ( launch == m_vecLaunchPos )
	{
		for ( int i = 0; i < 32; ++i )
		{
			if ( m_Trajectories[i].fLastUsedTime > 0 && target == Vector(m_Trajectories[i].vTargetPos) )
				trajectory = &m_Trajectories[i];
		}
	}
	else
	{
		InvalidateAllTrajectories();
		m_vecLaunchPos = launch;
	}
	if ( trajectory == NULL )
	{
		float time = gEngfuncs.GetClientTime();
		for ( int i = 0; i < 32; ++i )
		{
			if ( m_Trajectories[i].fLastUsedTime < 0 || time-m_Trajectories[i].fLastUsedTime > 30 )
			{
				trajectory = &m_Trajectories[i];
				break;
			}
		}
		if ( trajectory == NULL )
		{
			CalculateTrajectory(m_vecLaunchPos, target, pitch1, pitch2, yaw);
			return;
		}
		VectorCopy(target, trajectory->vTargetPos);
		trajectory->fLastUsedTime = gEngfuncs.GetClientTime();
		CalculateTrajectory(m_vecLaunchPos, Vector(trajectory->vTargetPos),
			&trajectory->fPitch1, &trajectory->fPitch2, &trajectory->fYaw);
	}
	*pitch1 = trajectory->fPitch1;
	*pitch2 = trajectory->fPitch2;
	*yaw = trajectory->fYaw;
}

void CTrajectoryList::CalculateTrajectory( Vector launch, Vector target, float *pitch1, float *pitch2, float *yaw )
{
	Vector delta = target-launch;
	*yaw = VecToYaw(delta);
	double height = launch.z-target.z;
	double distance = sqrt(delta.x*delta.x + delta.y*delta.y);
	double distanceSquared = distance*distance;
	double lengthSquared = height*height + distanceSquared;
	const float gravity = 800.0f*0.6f;
	const double speedSquared = 1000000.0;
	double sum = height*gravity + speedSquared;
	double discriminant = sum*sum - gravity*(lengthSquared*gravity);
	if ( !(discriminant >= 0) )
	{
		*pitch1 = -1;
		*pitch2 = -1;
		return;
	}
	double root = sqrt(discriminant);
	double factor = distanceSquared*speedSquared/(lengthSquared*2000000000000.0);
	double first = (sum+root)*factor;
	double second = (sum-root)*factor;
	if ( first >= 0 )
		*pitch1 = acos(sqrt(first))*57.29578f;
	if ( second >= 0 )
		*pitch2 = acos(sqrt(second))*57.29578f;
}
