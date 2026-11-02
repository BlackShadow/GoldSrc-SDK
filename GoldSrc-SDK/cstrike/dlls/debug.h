#pragma once

enum DebugOutputType
{
	DEBUG_NONE		= 0,
	DEBUG_BOT		= 1,
	DEBUG_CAREER	= 2,
	DEBUG_TUTOR		= 4,
	DEBUG_STATS		= 8,
	DEBUG_HOSTAGE	= 16,
	DEBUG_ALL		= 0xFFFFFFFF
};

void UTIL_DPrintf( DebugOutputType outputType, char* format, ... );
void UTIL_DPrintf( char* format, ... );
void UTIL_BotDPrintf( char* format, ... );
void UTIL_CareerDPrintf( char* format, ... );
void UTIL_TutorDPrintf( char* format, ... );
void UTIL_StatsDPrintf( char* format, ... );
void UTIL_HostageDPrintf( char* format, ... );
void PrintDebugFlag( const char* flag );
void UTIL_SetDprintfFlags( const char* flag );
