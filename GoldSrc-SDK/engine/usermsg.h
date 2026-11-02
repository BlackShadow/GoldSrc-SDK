#pragma once

#include "engine_apiproxy.h"

#define USER_MESSAGE_NAME_BYTES 16

typedef struct _UserMsg
{
	int iMsg;
	int iSize;
	char szName[USER_MESSAGE_NAME_BYTES];
	_UserMsg* next;
	pfnUserMsgHook pfn;
} UserMsg;

extern UserMsg* sv_gpNewUserMsgs;
extern UserMsg* sv_gpUserMsgs;

int				RegUserMsg( const char* name, int size );

pfnUserMsgHook	HookServerMsg( const char* name, pfnUserMsgHook hook );
void			CL_ShutDownUsrMessages();
