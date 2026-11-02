#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "com_model.h"
#include "tri.h"

extern cvar_t* cl_particlefx;

DECLARE_MESSAGE( m_PShooter, PShoot )

int CParticleShooter::Init()
{
	m_iFlags |= HUD_ACTIVE;
	HOOK_MESSAGE( PShoot );
	memset( m_sShooters, 0, sizeof( m_sShooters ) );
	gHUD.AddHudElem( this );
	m_iNumShooters = 0;
	return 1;
}

int CParticleShooter::VidInit()
{
	m_iNumShooters = 0;
	memset( m_sShooters, 0, sizeof( m_sShooters ) );
	return 1;
}

void CParticleShooter::AddParticleSystem( particle_shooter_t* shooter )
{
	if ( m_iNumShooters >= 64 )
		return;
	particle_shooter_t* system = &m_sShooters[m_iNumShooters];
	*system = *shooter;
	system->pSprite = (model_s*)gEngfuncs.GetSpritePointer( gEngfuncs.pfnSPR_Load( system->szSprite ) );
	system->fNextShootTime = -1;
	system->iParticlesRemaining = 0;
	++m_iNumShooters;
}

int CParticleShooter::MsgFunc_PShoot( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int id = READ_BYTE();
	int state = READ_BYTE();
	if ( state )
	{
		Vector velocity;
		velocity.x = READ_COORD();
		velocity.y = READ_COORD();
		velocity.z = READ_COORD();
		for ( int i = 0; i < 64; ++i )
		{
			particle_shooter_t* system = &m_sShooters[i];
			if ( system->id == id )
			{
				system->iState = state;
				system->vVelocity = velocity;
				system->fNextShootTime = gHUD.m_flTime;
				system->iParticlesRemaining = system->iNumParticles;
			}
		}
	}
	else
	{
		for ( int i = 0; i < 64; ++i )
		{
			if ( m_sShooters[i].id == id )
			{
				m_sShooters[i].iState = 0;
				m_sShooters[i].iParticlesRemaining = 0;
			}
		}
	}
	return 1;
}

void CParticleShooter::Think()
{
	float time = gEngfuncs.GetClientTime();
	if ( !( cl_particlefx->value >= 2 ) )
		return;
	for ( int i = 0; i < m_iNumShooters; ++i )
	{
		particle_shooter_t* system = &m_sShooters[i];
		if ( !system->iState || ( system->iParticlesRemaining <= 0 && system->iNumParticles != -1 ) )
			continue;
		if ( time <= system->fNextShootTime || system->fNextShootTime == 0 )
			continue;
		if ( system->iParticlesRemaining > 0 )
			--system->iParticlesRemaining;
		system->fNextShootTime = time + system->fFireDelay;
		if ( !system->pSprite )
		{
			gEngfuncs.Con_DPrintf( "Bad sprite in particle_shooter: %s\n", system->szSprite );
			continue;
		}
		CDoDParticle* particle = CDoDParticle::Create( system->vOrigin, Vector( 0, 0, 1 ), system->pSprite,
			system->fSize, system->fBrightness, "dod_particle", true );
		if ( !particle )
			return;
		for ( int axis = 0; axis < 3; ++axis )
			particle->m_vVelocity[axis] = gEngfuncs.pfnRandomFloat( -1, 1 ) * system->fVariance + system->vVelocity[axis];
		particle->m_iFrame = 0;
		particle->m_flDieTime = time + system->fParticleLife;
		particle->m_iRendermode = system->iRenderMode;
		particle->m_iPFlags = 0;
		int collisionFlags = 0;
		if ( system->iFlags & 2 )
			collisionFlags |= TRI_SPIRAL;
		if ( system->iFlags & 4 )
			collisionFlags |= TRI_COLLIDEWORLD;
		if ( system->iFlags & 8 )
			particle->m_bAffectedByForce = false;
		if ( system->iFlags & 0x10 )
		{
			particle->m_iFramerate = system->iFramerate;
			particle->m_iNumFrames = system->pSprite->numframes;
		}
		else
			particle->m_iFramerate = 0;
		if ( system->iFlags & 0x40 )
			collisionFlags |= TRI_COLLIDEKILL;
		if ( system->iFlags & 0x80 )
		{
			particle->m_iPFlags |= DOD_PARTICLE_WATER_RIPPLE;
			collisionFlags |= TRI_WATERTRACE;
		}
		if ( system->iFlags & 0x100 )
			particle->AddGlobalWind();
		particle->SetLightFlag( LIGHT_NONE );
		particle->SetCullFlag( CULL_FRUSTUM_PLANE | CULL_PVS );
		particle->m_flMass = gEngfuncs.pfnRandomFloat( 1, 1.5f );
		particle->m_flFadeSpeed = system->fFadeSpeed;
		particle->m_flScaleSpeed = system->fScaleSpeed;
		particle->m_flGravity = system->fGravity;
		particle->SetCollisionFlags( collisionFlags );
		particle->m_vAVelocity = Vector( 0, 0, 0 );
		particle->m_flDampingTime = system->fDampingTime;
		particle->m_flDampingVelocity = system->fDampingVel;
		particle->m_vColor = Vector( system->iColour[0], system->iColour[1], system->iColour[2] );
	}
}
