#pragma once

#include "const.h"

struct ref_params_s;

void		V_Init();
void		V_InitLevel();
void		V_RenderView();
void		V_GetRefParams( ref_params_s* parameters );

void		BuildGammaTable( float gamma );
qboolean	V_CheckGamma();
void		V_UpdatePalette();
void		V_CalcBlend();
int			V_FadeAlpha();

int			V_ScreenShake( const char* name, int size, void* buffer );
int			V_ScreenFade( const char* name, int size, void* buffer );
void		V_CalcShake();
void		V_ApplyShake( float* origin, float* angles, float factor );
