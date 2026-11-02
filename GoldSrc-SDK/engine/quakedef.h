#pragma once

#include "const.h"
#include "mathlib.h"

#define MAX_OSPATH 260 // max length of a filesystem pathname

#ifndef CACHE_USER
#define CACHE_USER
typedef struct cache_user_s
{
	void* data;
} cache_user_t;
#endif // QUAKEDEF_H
