#ifndef PARTICLE_H
#define PARTICLE_H

class ParticleEmitter
{
public:
	ParticleEmitter();
	int texture;
	int count;
	int countRand;
	int speed;
	int noise;
	int rendermode;
	float gravity;
	float scale;
	float scalespeed;
	float brightness;
	float fadespeed;
	float life;
	int type;
	int flags;
	Vector angles;
	Vector avelocity;
	Vector color;
	int state;
	float frequency;
	int targetEnt;
	int sourceEnt;
	float m_flNextEmitTime;
	int visEnt;
	int iAttachment;
};

#endif
