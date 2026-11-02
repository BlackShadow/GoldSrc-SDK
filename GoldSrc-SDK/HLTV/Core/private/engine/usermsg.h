#ifndef USERMSG_H
#define USERMSG_H

#include "HLTV/common/types.h"

struct _UserMsg
{
	int				iMsg;
	int				iSize;
	char			szName[16];

	_UserMsg*		next;
	pfnUserMsgHook	pfn;
};

#endif
