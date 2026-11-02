#pragma once

#include <cstdint>

typedef struct USERID_s
{
	int idtype;
	uint64_t m_SteamID;
	unsigned int clientip;
} USERID_t;
