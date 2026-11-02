#ifndef CL_DEMO_H
#define CL_DEMO_H

#include "HLTV/common/types.h"
#include "common/ref_params.h"
#include "common/usercmd.h"
#include "pm_shared/pm_movevars.h"

typedef struct demoheader_s
{
	char		szFileStamp[6];
	int			nDemoProtocol;
	int			nNetProtocolVersion;

	char		szMapName[260];
	char		szDllDir[260];
	CRC32_t		mapCRC;
	int			nDirectoryOffset;
} demoheader_t;

typedef struct demoentry_s
{
	int			nEntryType;
	char		szDescription[64];
	int			nFlags;
	int			nCDTrack;

	float		fTrackTime;
	int			nFrames;
	int			nOffset;
	int			nFileLength;
} demoentry_t;

typedef struct demo_info_s
{
	float			timestamp;
	ref_params_t	rp;
	usercmd_t		cmd;
	movevars_t		movevars;
	vec3_t			view;
	int				viewmodel;
} demo_info_t;

#endif
