#ifndef FOG_H
#define FOG_H

struct FogParameters
{
	float color[3];
	float density;
	bool affectsSkyBox;
};

extern FogParameters g_FogParameters;

void RenderFog( void );

#endif
