#pragma once

#include "const.h"
#include "mathlib.h"
#include "../common/entity_state.h"

#define MAX_EXTRA_BASELINES 64

struct extra_baselines_s
{
	int number;
	int classname[MAX_EXTRA_BASELINES];
	entity_state_t baseline[MAX_EXTRA_BASELINES];
};
