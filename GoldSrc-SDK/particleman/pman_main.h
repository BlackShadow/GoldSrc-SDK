#pragma once

#include "wrect.h"
#include "cl_dll.h"
#include "particleman.h"

class IParticleMan_Active : public IParticleMan
{
public:
	void SetUp( cl_enginefunc_t* engine ) override;
	void Update() override;
	void SetVariables( float gravity, Vector viewAngles ) override;
	void ResetParticles() override;
	void ApplyForce( Vector origin, Vector direction, float radius, float strength, float duration ) override;
	void AddCustomParticleClassSize( unsigned long size ) override;
	CBaseParticle* CreateParticle( Vector origin, Vector normal, model_s* sprite, float size,
			float brightness, const char* classname ) override;
	char* RequestNewMemBlock( int size ) override;

	void CoreInitializeSprite( CCoreTriangleEffect* particle, Vector origin, Vector normal, model_s* sprite,
			float size, float brightness ) override;
	void CoreThink( CCoreTriangleEffect* particle, float time ) override;
	void CoreDraw( CCoreTriangleEffect* particle ) override;
	void CoreAnimate( CCoreTriangleEffect* particle, float time ) override;
	void CoreAnimateAndDie( CCoreTriangleEffect* particle, float time ) override;
	void CoreExpand( CCoreTriangleEffect* particle, float time ) override;
	void CoreContract( CCoreTriangleEffect* particle, float time ) override;
	void CoreFade( CCoreTriangleEffect* particle, float time ) override;
	void CoreSpin( CCoreTriangleEffect* particle, float time ) override;
	void CoreCalculateVelocity( CCoreTriangleEffect* particle, float time ) override;
	void CoreCheckCollision( CCoreTriangleEffect* particle, float time ) override;
	void CoreTouch( CCoreTriangleEffect* particle, Vector position, Vector normal, int index ) override;
	void CoreDie( CCoreTriangleEffect* particle ) override;
	void CoreForce( CCoreTriangleEffect* particle ) override;
	bool CoreCheckVisibility( CCoreTriangleEffect* particle ) override;
	void SetRender( int mode ) override;
};

extern float	g_flGravity;
extern Vector	g_vViewAngles;
extern int		g_iRenderMode;

extern float	g_flOldTime;
extern long		g_lMaxParticleClassSize;
