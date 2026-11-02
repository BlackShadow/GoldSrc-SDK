#ifndef CL_DEMO_H
#define CL_DEMO_H

#include "HLTV/common/types.h"
#include "common/ref_params.h"
#include "common/usercmd.h"
#include "pm_shared/pm_movevars.h"

enum DemoFileCommand
{
	DEMO_CMD_CONSOLE		= 3,
	DEMO_CMD_CLIENTDATA		= 4,
	DEMO_CMD_EVENT			= 6,
	DEMO_CMD_WEAPONANIM		= 7,
	DEMO_CMD_SOUND			= 8,
	DEMO_CMD_CLIENTBUFFER	= 9
};

typedef struct demo_info_s
{
	float			timestamp;
	ref_params_t	rp;
	usercmd_t		cmd;
	movevars_s		movevars;
	vec3_t			view;
	int				viewmodel;
} demo_info_t;

#endif
