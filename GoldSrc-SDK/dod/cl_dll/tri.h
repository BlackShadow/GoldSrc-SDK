#pragma once

#include "particleman.h"

enum
{
	DOD_PARTICLE_SMOKE = 0x201,
	DOD_PARTICLE_FIXED_ANGLES = 0x40,
	DOD_PARTICLE_FIXED_PITCH = 0x80,
	DOD_PARTICLE_SMOKE_RING = 0x200,
	DOD_PARTICLE_WATER_RIPPLE = 0x400,
	DOD_PARTICLE_SNOW = 0x800,
	DOD_PARTICLE_SHRINK = 0x1000,
	DOD_PARTICLE_RAIN_ANGLE = 0x2000,
	DOD_PARTICLE_WATER_SPLASH = 0x4000
};

class CBaseDoDParticle : public CBaseParticle
{
public:
	void Think( float time ) override { g_pParticleMan->CoreThink( this, time ); }
	void Draw() override { g_pParticleMan->CoreDraw( this ); }
	void Animate( float time ) override { g_pParticleMan->CoreAnimate( this, time ); }
	void AnimateAndDie( float time ) override { g_pParticleMan->CoreAnimateAndDie( this, time ); }
	void Expand( float time ) override { g_pParticleMan->CoreExpand( this, time ); }
	void Contract( float time ) override { g_pParticleMan->CoreContract( this, time ); }
	void Fade( float time ) override { g_pParticleMan->CoreFade( this, time ); }
	void Spin( float time ) override { g_pParticleMan->CoreSpin( this, time ); }
	void CalculateVelocity( float time ) override { g_pParticleMan->CoreCalculateVelocity( this, time ); }
	void CheckCollision( float time ) override { g_pParticleMan->CoreCheckCollision( this, time ); }
	void Touch( Vector pos, Vector normal, int index ) override { g_pParticleMan->CoreTouch( this, pos, normal, index ); }
	void Die() override { g_pParticleMan->CoreDie( this ); }
	void InitializeSprite( Vector org, Vector normal, model_s* sprite, float size, float brightness ) override
	{
		g_pParticleMan->CoreInitializeSprite( this, org, normal, sprite, size, brightness );
	}
	void Force() override { g_pParticleMan->CoreForce( this ); }
};

class CDoDParticle : public CBaseDoDParticle
{
public:
	void Think( float time ) override;
	void Touch( Vector pos, Vector normal, int index ) override;
	void Die() override;
	void Force() override;
	static CDoDParticle* Create( Vector org, Vector normal, model_s* sprite, float size, float brightness,
			const char* classname, bool distCull );
	static void SetGlobalWind( float* wind );
	void AddGlobalWind();

	bool	m_bInsideSmoke;
	bool	m_bSpawnInside;
	bool	m_bDampMod;
	Vector	m_vRingOrigin;
	bool	m_bAffectedByForce;
	int		m_iPFlags;

	static Vector m_vGlobalWind;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class TriangleWallPuff : public CBaseParticle
{
public:
	void Think( float time ) override;
	float m_flActivateTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CDoDRocketTrail : public CBaseDoDParticle
{
public:
	void Think( float time ) override;
	static CDoDRocketTrail* Create( Vector org, Vector normal, model_s* sprite, float size, float brightness, const char* classname );
	bool m_bRocketTrail;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CDoDDirtExploDust : public CBaseDoDParticle
{
public:
	void Think( float time ) override;
	static CDoDDirtExploDust* Create( Vector org, Vector normal, model_s* sprite, float size, float brightness, const char* classname );
	bool	m_bFire;
	float	m_flActivateTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CDoDRainDrop : public CDoDParticle
{
public:
	void Think( float time ) override;
};

class CDoDSnowFlake : public CDoDParticle
{
public:
	void Think( float time ) override;
	void Touch( Vector pos, Vector normal, int index ) override;
	static CDoDSnowFlake* Create( Vector org, Vector normal, model_s* sprite, float size, float brightness, const char* classname );

	bool	m_bSpiral;
	bool	m_bTouched;
	float	m_flOldTime;
	float	m_flFadeOutTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

extern float	g_flWeatherTime;
extern int		g_iWeatherType;
extern cvar_t*	cl_weatherdis;

Vector	GetViewAngles();
void	CreateDebrisWallPuff( Vector origin, Vector velocity, Vector color, int puff );
void	CreateExplosionSmoke( Vector origin, Vector velocity, bool insideSmoke, bool spawnInside, bool blowable );
void	CreateExplosionSmokeInside( Vector origin );
void	UpdateRain();
void	UpdateSnow();

void	RenderDoDFog();
