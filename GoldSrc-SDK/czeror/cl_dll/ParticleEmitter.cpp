#include "hud.h"
#include "buildslave_assert.h"
#include "cl_util.h"
#include "r_efx.h"
#include "parsemsg.h"
#include "particle.h"
#include "triangleeffect.h"
#include <map>

std::map<int, ParticleEmitter> g_ParticleEmitList;

DECLARE_MESSAGE(m_Particles, Particles)
DECLARE_MESSAGE(m_Particles, PartState)
DECLARE_MESSAGE(m_Particles, TmpSprite)

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
ParticleEmitter::ParticleEmitter()
{
	state = 0;
	m_flNextEmitTime = 0.0f;
}

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\cl_dll\\ParticleEmitter.cpp" )

bool IsInLocalVisSet(const int index)
{
	cl_entity_t *pPlayer = gEngfuncs.GetLocalPlayer();
	ASSERT_AT( pPlayer && "Particles: IsInLocalVisSet bad player", 344 );
	cl_entity_t *entity = gEngfuncs.GetEntityByIndex(index);
	return entity && entity->curstate.messagenum >= pPlayer->curstate.messagenum;
}

int CHudParticles::Init()
{
	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;
	HOOK_MESSAGE(Particles);
	HOOK_MESSAGE(PartState);
	HOOK_MESSAGE(TmpSprite);
	return 1;
}

void CHudParticles::Reset()
{
	m_iFlags = HUD_ACTIVE;
}

int CHudParticles::VidInit()
{
	m_iFlags = HUD_ACTIVE;
	g_ParticleEmitList.clear();
	return 1;
}

int CHudParticles::MsgFunc_PartState(const char *, int size, void *buffer)
{
	BEGIN_READ(buffer, size);
	int id = READ_SHORT();
	int state = READ_BYTE();
	std::map<int, ParticleEmitter>::iterator emitter = g_ParticleEmitList.find( id );
	if ( emitter != g_ParticleEmitList.end() )
		emitter->second.state = state;
	return 1;
}

int CHudParticles::MsgFunc_TmpSprite(const char *, int size, void *buffer)
{
	BEGIN_READ(buffer, size);
	Vector origin;
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	int texture = READ_SHORT();
	float scale = static_cast<float>(READ_SHORT());
	int life = READ_SHORT();
	int brightness = READ_BYTE();
	int renderMode = READ_BYTE();
	if (scale != 0.0f)
	{
		model_s *sprite = gEngfuncs.hudGetModelByIndex(texture);
		if (sprite)
		{
			CBaseParticle *particle = g_pParticleMan->CreateParticle(origin, Vector(0, 0, 0), sprite,
				scale, static_cast<float>(brightness), "particle_sprite");
			if (particle)
			{
				particle->SetRenderFlag(RENDER_FACEPLAYER);
				particle->m_iRendermode = renderMode;
				particle->m_flDieTime = gEngfuncs.GetClientTime() + life / 10.0f;
			}
		}
	}
	return 1;
}

int CHudParticles::MsgFunc_BeamPts(const char *, int size, void *buffer)
{
	BEGIN_READ(buffer, size);
	Vector origin, end;
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	end.x = READ_COORD();
	end.y = READ_COORD();
	end.z = READ_COORD();
	int texture = READ_SHORT();
	READ_SHORT();
	READ_SHORT();
	READ_SHORT();
	READ_BYTE();
	int speed = READ_SHORT();
	int red = READ_BYTE();
	int green = READ_BYTE();
	int blue = READ_BYTE();
	gEngfuncs.pEfxAPI->R_BeamPoints(origin, end, texture, 100.0f, 100.0f, 5.0f, 255.0f,
		speed / 10.0f, 0, 15.0f, static_cast<float>(red), static_cast<float>(green), static_cast<float>(blue));
	return 1;
}

int CHudParticles::MsgFunc_Particles(const char *, int size, void *buffer)
{
	BEGIN_READ(buffer, size);
	int texture = READ_SHORT();
	int count = READ_BYTE();
	int speed = READ_SHORT();
	int noise = READ_BYTE();
	int renderMode = READ_BYTE();
	float gravity = static_cast<float>(READ_SHORT());
	float scale = static_cast<float>(READ_SHORT());
	float scaleSpeed = static_cast<float>(READ_BYTE());
	int brightness = READ_BYTE();
	float fadeSpeed = static_cast<float>(READ_BYTE());
	float life = static_cast<float>(READ_SHORT());
	int type = READ_BYTE();
	int flags = READ_BYTE();
	Vector angles, angularVelocity, color;
	angles.x = READ_COORD();
	angles.y = READ_COORD();
	angles.z = READ_COORD();
	angularVelocity.x = READ_COORD();
	angularVelocity.y = READ_COORD();
	angularVelocity.z = READ_COORD();
	color.x = static_cast<float>(READ_BYTE());
	color.y = static_cast<float>(READ_BYTE());
	color.z = static_cast<float>(READ_BYTE());
	int state = READ_BYTE();
	float frequency = READ_SHORT() / 10.0f;
	int countRandom = READ_BYTE();
	int targetEntity = READ_SHORT();
	int sourceEntity = READ_SHORT();
	int id = READ_SHORT();
	int visibleEntity = READ_SHORT();
	int attachment = READ_BYTE();

	ParticleEmitter &emitter = g_ParticleEmitList[id];
	emitter.texture = texture;
	emitter.count = count;
	emitter.speed = speed;
	emitter.noise = noise / 10;
	emitter.rendermode = renderMode;
	emitter.gravity = gravity / 10.0f;
	emitter.scale = scale;
	emitter.scalespeed = scaleSpeed;
	emitter.brightness = static_cast<float>(brightness);
	emitter.fadespeed = fadeSpeed;
	emitter.life = life / 10.0f;
	emitter.type = type;
	emitter.flags = flags;
	emitter.angles = angles;
	emitter.avelocity = angularVelocity;
	emitter.color = color;
	emitter.state = state;
	emitter.frequency = frequency;
	emitter.countRand = countRandom;
	emitter.targetEnt = targetEntity;
	emitter.sourceEnt = sourceEntity;
	emitter.visEnt = visibleEntity;
	emitter.iAttachment = attachment;
	emitter.state = 1;
	return 1;
}

int CHudParticles::Draw(float time)
{
	for (std::map<int, ParticleEmitter>::iterator it = g_ParticleEmitList.begin(); it != g_ParticleEmitList.end(); ++it)
	{
		ParticleEmitter &emitter = it->second;
		if (emitter.state != 0 && emitter.m_flNextEmitTime <= time &&
			(emitter.visEnt == 0 || IsInLocalVisSet(emitter.visEnt)))
		{
			emitter.m_flNextEmitTime = time + emitter.frequency;
			if (emitter.frequency < 0.01f)
				emitter.state = 0;
			cl_entity_t *target = NULL;
			cl_entity_t *source = gEngfuncs.GetEntityByIndex(emitter.sourceEnt);
			if (emitter.targetEnt)
				target = gEngfuncs.GetEntityByIndex(emitter.targetEnt);
			if (emitter.targetEnt == 1)
				target = gEngfuncs.GetLocalPlayer();
			if (emitter.sourceEnt == 1)
				source = gEngfuncs.GetLocalPlayer();
			if (source)
			{
				Vector origin = source->curstate.origin;
				if (emitter.iAttachment && emitter.iAttachment <= (int)(sizeof(source->attachment) / sizeof(source->attachment[0])))
					origin = source->attachment[emitter.iAttachment - 1];
				Vector direction;
				if (target)
					direction = (target->curstate.origin - origin).Normalize();
				else
					gEngfuncs.pfnAngleVectors(emitter.angles, direction, NULL, NULL);
				int count = emitter.count + gEngfuncs.pfnRandomLong(-emitter.countRand, emitter.countRand);
				CreateParticles(origin, direction, count, static_cast<float>(emitter.speed), emitter.noise,
					emitter.rendermode, emitter.gravity, emitter.scale, emitter.scalespeed, emitter.brightness,
					emitter.fadespeed, emitter.life, emitter.type, emitter.texture, emitter.flags,
					emitter.angles, emitter.avelocity, emitter.color);
			}
		}
	}
	return 0;
}
