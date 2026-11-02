#pragma once

#include "userid.h"

#include <cstddef>

#define MAX_IPFILTERS 32768
#define MAX_USERFILTERS 32768

typedef struct
{
	unsigned int mask;
	unsigned int compare;
	float banEndTime; // 0 for permanent ban
	float banTime;
} ipfilter_t;

typedef struct
{
	USERID_t userid;
	float banEndTime;
	float banTime;
} userfilter_t;

extern ipfilter_t ipfilters[MAX_IPFILTERS];
extern int numipfilters;
extern userfilter_t userfilters[MAX_USERFILTERS];
extern int numuserfilters;

int			StringToFilter( char* address, ipfilter_t* filter );
void		SV_ListId_f();
void		SV_WriteId_f();
void		SV_RemoveId_f();
