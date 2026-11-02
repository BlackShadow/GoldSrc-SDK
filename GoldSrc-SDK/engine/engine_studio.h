#pragma once

#include "studio.h"
#include "../common/r_studioint.h"

#include <cstddef>

#define IDSTUDIOHEADER ( ( 'T' << 24 ) | ( 'S' << 16 ) | ( 'D' << 8 ) | 'I' )
#define STUDIO_VERSION 10

#define STUDIO_LIGHT_SCALE	0xC0FF

extern engine_studio_api_t engine_studio_api;

void		R_ResetStudio();
