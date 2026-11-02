#ifndef E_EFFECTCLASSES_H
#define E_EFFECTCLASSES_H

#include "particleman.h"
#include "../ev_hldm.h"

class CTriGlass : public CBaseParticle
{
public:
	void Think(float time) override;
	void Touch(Vector position, Vector normal, int index) override;
	float m_flActivateTime;
	float m_flActivatePhysicsTime;
	int m_iXpos;
	int m_iYpos;
	int m_iXDecay;
	int m_iYDecay;
	int m_iGlassWidth;
	int m_iGlassHeight;
	int m_iUser1;
	int m_iUser2;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class GlassHandle
{
public:
	GlassHandle() { ClearAll(); }
	void ClearAll()
	{
		for (int x = 0; x < 40; ++x)
		{
			for (int y = 0; y < 40; ++y)
				particles[x][y] = NULL;
		}
	}
	CTriGlass *particles[40][40];
};

class CSnowFlake : public CBaseParticle
{
public:
	void Touch(Vector position, Vector normal, int index) override;
	void Think(float time) override;
	bool m_bSpiral;
	bool m_bTouched;
	float m_flSpiralTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CRainDrop : public CBaseParticle
{
public:
	void Think(float time) override;
	void Touch(Vector position, Vector normal, int index) override;
	bool m_bTouched;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CGrassPiece : public CBaseParticle
{
public:
	void Think(float time) override;
};

class CTriangleDust : public CBaseParticle
{
public:
	void Think(float time) override;
	bool m_bMod;
	float m_flActivateTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CPartSmokeGrenade : public CBaseParticle
{
public:
	void Think(float time) override;
	Vector m_vRingOrigin;
	bool m_bInsideSmoke;
	bool m_bSpawnInside;
	bool m_bDampMod;
	bool m_bAffectedByForce;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CPoisonSmoke : public CBaseParticle
{
public:
	void Think(float time) override;
	Vector m_vRingOrigin;
	bool m_bInsideSmoke;
	bool m_bSpawnInside;
	bool m_bDampMod;
	bool m_bAffectedByForce;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTriangleWallPuff : public CBaseParticle
{
public:
	void Think(float time) override;
	float m_flActivateTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTriangleCustomGib : public CBaseParticle
{
public:
	void Think(float time) override;
	void Touch(Vector position, Vector normal, int index) override;
	float m_flActivateTime = 0.0f;
};

class CHEGrenadeSmoke : public CBaseParticle
{
public:
	void Think(float time) override;
	void Touch(Vector position, Vector normal, int index) override;
	float m_flActivateTime;
	bool m_bInsideSmoke;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CPartWind : public CBaseParticle
{
public:
	void Think(float time) override;
	void Touch(Vector position, Vector normal, int index) override;
};

class CPartBlowTorchPilot : public CBaseParticle
{
public:
	void Think(float time) override;
	float m_flForward;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CPartBlowTorchGlow : public CBaseParticle
{
public:
	void Think(float time) override;
};

extern GlassHandle g_GlassPanes[40];

void CreateTriangle(Vector position, Vector normal, float width, float height, int index);
void DecayGlass(float xDecay, float yDecay, int size, int index, float xDecay2, float yDecay2, Vector direction);
void CreateSpark(Vector origin, Vector normal, const char *spriteName);
void CreateCustomGib(Vector origin, Vector normal, const char *spriteName, float velocityModifier, materials_t *material);
void CreateDebrisWallPuff(Vector origin, Vector velocity, Vector color, int puff, bool blood);
void CreateDustRing(Vector origin, Vector velocity, float scale);
void CreateGasSmoke(Vector origin, Vector velocity, bool insideSmoke, bool spawnInside, bool blowable);
void CreatePoisonSmoke(Vector origin, Vector velocity, bool insideSmoke, bool spawnInside, bool blowable);
void CreateGasInside(Vector origin);
void CreatePoisonInside(Vector origin);
void CreateGrenadeSmoke(Vector origin, Vector velocity, bool insideSmoke);
void CreateHEGrenadeFlash(Vector origin);
void CreateHEGrenadeFlashGlow(Vector origin, int size);
void CreatePilotFlame(Vector origin);
void CreateBlowTorchGlow();
void UpdateBlowTorchEffect();
void SetBlowTorchAttachment(struct cl_entity_s *entity);

#endif
