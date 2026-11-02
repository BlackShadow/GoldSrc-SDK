#ifndef TRIANGLEEFFECT_H
#define TRIANGLEEFFECT_H

#include "particleman.h"

class TriSphere : public CBaseParticle
{
public:
	virtual void InitializeSphere(Vector origin, model_s *sprite, float size, float brightness);
	void Draw() override;
};

class TriCylinder : public CBaseParticle
{
public:
	void Draw() override;
};

class TriDisk : public CBaseParticle
{
public:
	void Draw() override;
};

class TriCone : public CBaseParticle
{
public:
	void Draw() override;
};

enum particle_type
{
	PARTICLE_SPRITE,
	PARTICLE_PLANE,
	PARTICLE_SPHERE,
	PARTICLE_CYLINDER,
	PARTICLE_DISK,
	PARTICLE_CONE,
	PARTICLE_WATERDROP
};

void CreateParticles(Vector origin, Vector direction, int count, float speed, int noise, int renderMode,
	float gravity, float scale, float scaleSpeed, float brightness, float fadeSpeed, float life,
	int type, int texture, int flags, Vector angles, Vector angularVelocity, Vector color);

void R_BeamSphere(model_s *sprite, const Vector &origin, float radius, float rotation,
	const Vector &color, float brightness, int renderMode, int frame);
void Draw_BeamCylinder(Vector &origin, Vector &angles, float radius, float height, int segments,
	float brightness, model_s *sprite, int renderMode, int frame, Vector color);
void Draw_BeamDisk(Vector &origin, Vector &angles, float radius, int segments, float brightness,
	model_s *sprite, int renderMode, int frame, Vector color);
void Draw_BeamCone(Vector &origin, Vector &angles, float radius, float height, int segments,
	float brightness, model_s *sprite, int renderMode, int frame, Vector color);

#endif
