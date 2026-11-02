#ifndef TRI_H
#define TRI_H

#include "particleman.h"
#include "fog.h"

class CPartSmokeGrenade : public CBaseParticle
{
public:
	bool m_bInsideSmoke = false;
	bool m_bSpawnInside = false;
	bool m_bDampMod = false;
	Vector m_vRingOrigin;
	bool m_bAffectedByForce = false;
	int bdsg;
	int dsgads;

	void Think( float time );
	void Force( void );
	static CPartSmokeGrenade* Create( Vector origin, Vector normal, model_s* sprite,
		float size, float brightness, const char* classname );
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CPartRainDrop : public CBaseParticle
{
public:
	bool m_bTouched;
	void Think( float time );
	void Touch( Vector position, Vector normal, int index );
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CPartSnowFlake : public CBaseParticle
{
public:
	bool m_bSpiral;
	bool m_bTouched;
	float m_flSpiralTime;
	void Think( float time );
	void Touch( Vector position, Vector normal, int index );
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CPartGrassPiece : public CBaseParticle
{
public:
	void Think( float time );
};

class CPartWind : public CBaseParticle
{
public:
	void Think( float time );
};

void CreateGasSmoke( Vector origin, Vector velocity, bool insideSmoke, bool spawnInside, bool blowable );
void CreateGasInside( Vector origin );

#endif //TRI_H
