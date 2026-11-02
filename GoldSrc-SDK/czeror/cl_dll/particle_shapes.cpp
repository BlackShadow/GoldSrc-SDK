#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"
#include "triangleeffect.h"

static Vector gGlobePoints[75];
static Vector gGlobeTexPoints[75];
static int gGlobeIndices[433];
static bool gGlobeInitialized;

void R_BeamSphere(model_s *sprite, const Vector &origin, float radius, float,
	const Vector &, float, int renderMode, int frame)
{
	if (!(radius > 0.0f))
		return;
	gEngfuncs.pTriAPI->RenderMode(renderMode);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->SpriteTexture(sprite, frame);
	gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);
	const float sphereColor = 100.0f / 255.0f;
	gEngfuncs.pTriAPI->Color4fRendermode(sphereColor, sphereColor, sphereColor, sphereColor, renderMode);
	for (int triangle = 0; triangle < 432; triangle += 3)
	{
		for (int point = 0; point < 3; ++point)
		{
			int index = gGlobeIndices[triangle + point];
			gEngfuncs.pTriAPI->TexCoord2f(gGlobeTexPoints[index].x, gGlobeTexPoints[index].z);
			Vector position = gGlobePoints[index] * radius + origin;
			gEngfuncs.pTriAPI->Vertex3fv(position);
		}
	}
	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

void TriSphere::InitializeSphere(Vector origin, model_s *sprite, float size, float brightness)
{
	m_vOrigin = origin;
	m_vPrevOrigin = origin;
	m_flOriginalSize = size;
	m_flSize = size;
	m_flOriginalBrightness = brightness;
	m_flBrightness = brightness;
	m_pTexture = sprite;
	m_iFrame = 0;
}

void TriSphere::Draw()
{
	if (!gGlobeInitialized)
	{
		gGlobePoints[0] = Vector(0, 1, 0);
		gGlobePoints[1] = Vector(0, -1, 0);
		const float latitudeStep = 3.1415927f / 7.0f;
		const float longitudeStep = 3.1415927f / 6.0f;
		for (int latitude = 1; latitude < 7; ++latitude)
		{
			double polar = static_cast<double>(latitude) * latitudeStep;
			double radius = sin(polar);
			for (int longitude = 0; longitude < 12; ++longitude)
			{
				double angle = static_cast<float>(longitude * longitudeStep);
				int index = 2 + (latitude - 1) * 12 + longitude;
				Vector &point = gGlobePoints[index];
				point.x = static_cast<float>(sin(angle) * radius);
				point.y = static_cast<float>(cos(polar));
				point.z = static_cast<float>(cos(angle) * radius);
				gGlobeTexPoints[index].x = 1.0f - (point.x + 1.0f) * 0.5f;
				gGlobeTexPoints[index].y = 1.0f - (point.y + 1.0f) * 0.5f;
				gGlobeTexPoints[index].z = 1.0f - (point.z + 1.0f) * 0.5f;
			}
		}
		int nextIndex = 0;
		for (int longitude = 0; longitude < 12; ++longitude)
		{
			gGlobeIndices[nextIndex++] = 0;
			gGlobeIndices[nextIndex++] = 2 + longitude;
			gGlobeIndices[nextIndex++] = 2 + (longitude + 1) % 12;
		}
		for (int latitude = 0; latitude < 5; ++latitude)
		{
			for (int longitude = 0; longitude < 12; ++longitude)
			{
				int upper = 2 + latitude * 12 + longitude;
				int lower = upper + 12;
				int nextUpper = 2 + latitude * 12 + (longitude + 1) % 12;
				int nextLower = nextUpper + 12;
				gGlobeIndices[nextIndex++] = upper;
				gGlobeIndices[nextIndex++] = lower;
				gGlobeIndices[nextIndex++] = nextUpper;
				gGlobeIndices[nextIndex++] = lower;
				gGlobeIndices[nextIndex++] = nextLower;
				gGlobeIndices[nextIndex++] = nextUpper;
			}
		}
		for (int longitude = 0; longitude < 12; ++longitude)
		{
			gGlobeIndices[nextIndex++] = 1;
			gGlobeIndices[nextIndex++] = 62 + (longitude + 1) % 12;
			gGlobeIndices[nextIndex++] = 62 + longitude;
		}
		gGlobeInitialized = true;
	}
	if (m_flSize > 0.0f)
		R_BeamSphere(m_pTexture, m_vOrigin, m_flSize, m_vAngles.z, m_vColor, m_flBrightness, m_iRendermode, m_iFrame);
}

void Draw_BeamCylinder(Vector &origin, Vector &angles, float width, float height, int segments,
	float brightness, model_s *sprite, int renderMode, int frame, Vector color)
{
	if (segments <= 2)
		return;
	gEngfuncs.pTriAPI->RenderMode(renderMode);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->SpriteTexture(sprite, frame);
	gEngfuncs.pTriAPI->Begin(TRI_QUAD_STRIP);
	gEngfuncs.pTriAPI->Color4fRendermode(color.x / 255.0f, color.y / 255.0f, color.z / 255.0f, brightness / 255.0f, renderMode);
	for (int point = 0; point <= segments; ++point)
	{
		float segmentSize = 1.0f / segments;
		float fraction = point * segmentSize;
		Vector rotatedAngles = angles;
		rotatedAngles.z = static_cast<float>(fmod(fraction * 360.0 + angles.z, 360.0));
		Vector forward, right, up;
		gEngfuncs.pfnAngleVectors(rotatedAngles, forward, right, up);
		Vector lower(origin.x - up.x * width, origin.y - up.y * width, origin.z + up.z * width);
		Vector upper(lower.x + forward.x * height, lower.y + forward.y * height, lower.z - forward.z * height);
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, fraction);
		gEngfuncs.pTriAPI->Vertex3fv(upper);
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, fraction);
		gEngfuncs.pTriAPI->Vertex3fv(lower);
	}
	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

void Draw_BeamDisk(Vector &origin, Vector &angles, float width, int segments, float brightness,
	model_s *sprite, int renderMode, int frame, Vector color)
{
	if (segments <= 1)
		return;
	gEngfuncs.pTriAPI->RenderMode(renderMode);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->SpriteTexture(sprite, frame);
	gEngfuncs.pTriAPI->Begin(TRI_TRIANGLE_FAN);
	gEngfuncs.pTriAPI->Color4fRendermode(color.x / 255.0f, color.y / 255.0f, color.z / 255.0f, brightness / 255.0f, renderMode);
	Vector lastPosition;
	float lastFraction = 0.0f;
	for (int point = 0; point <= segments; ++point)
	{
		float segmentSize = 1.0f / segments;
		float fraction = point * segmentSize;
		Vector rotatedAngles = angles;
		rotatedAngles.z = static_cast<float>(fmod(fraction * 360.0 + angles.z, 360.0));
		Vector forward, right, up;
		gEngfuncs.pfnAngleVectors(rotatedAngles, forward, right, up);
		Vector position = origin + up * width;
		if (point != 0)
		{
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, fraction);
			gEngfuncs.pTriAPI->Vertex3fv(origin);
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, lastFraction);
			gEngfuncs.pTriAPI->Vertex3fv(lastPosition);
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, fraction);
			gEngfuncs.pTriAPI->Vertex3fv(position);
		}
		lastPosition = position;
		lastFraction = fraction;
	}
	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

void Draw_BeamCone(Vector &origin, Vector &angles, float width, float depth, int segments,
	float brightness, model_s *sprite, int renderMode, int frame, Vector color)
{
	if (segments <= 1)
		return;
	gEngfuncs.pTriAPI->RenderMode(renderMode);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->SpriteTexture(sprite, frame);
	gEngfuncs.pTriAPI->Begin(TRI_TRIANGLE_FAN);
	gEngfuncs.pTriAPI->Color4fRendermode(color.x / 255.0f, color.y / 255.0f, color.z / 255.0f, brightness / 255.0f, renderMode);
	Vector lastPosition;
	float lastFraction = 0.0f;
	for (int point = 0; point <= segments; ++point)
	{
		float segmentSize = 1.0f / segments;
		float fraction = point * segmentSize;
		Vector rotatedAngles = angles;
		rotatedAngles.z = static_cast<float>(fmod(fraction * 360.0 + angles.z, 360.0));
		Vector forward, right, up;
		gEngfuncs.pfnAngleVectors(rotatedAngles, forward, right, up);
		Vector position = origin + up * width;
		Vector tip = origin + forward * depth;
		if (point != 0)
		{
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, fraction);
			gEngfuncs.pTriAPI->Vertex3fv(tip);
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, lastFraction);
			gEngfuncs.pTriAPI->Vertex3fv(lastPosition);
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, fraction);
			gEngfuncs.pTriAPI->Vertex3fv(position);
		}
		lastPosition = position;
		lastFraction = fraction;
	}
	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

void TriCylinder::Draw()
{
	Draw_BeamCylinder(m_vOrigin, m_vAngles, m_flSize, m_flSize * 0.5f, 12, m_flBrightness,
		m_pTexture, m_iRendermode, m_iFrame, m_vColor);
}

void TriDisk::Draw()
{
	Draw_BeamDisk(m_vOrigin, m_vAngles, m_flSize, 12, m_flBrightness, m_pTexture, m_iRendermode, m_iFrame, m_vColor);
}

void TriCone::Draw()
{
	Draw_BeamCone(m_vOrigin, m_vAngles, m_flSize, m_flSize, 12, m_flBrightness,
		m_pTexture, m_iRendermode, m_iFrame, m_vColor);
}
