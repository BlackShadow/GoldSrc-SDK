//*********** (C) Copyright 2002 Valve, L.L.C. All rights reserved. ***********
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
//*****************************************************************************
//
// Contents:
//
//		SteamUserID types.
//
//		This file is distributed to Steam application developers.
//
// Authors:	
//
// Target restrictions:
//
// Tool restrictions:
//
// Things to do:
//
//		
//
//*****************************************************************************

#pragma once

#include <cstdint>

typedef enum
{
	eSteamErrorNone												= 0,
	eSteamErrorUnknown											= 1,
	eSteamErrorLibraryNotInitialized							= 2,
	eSteamErrorLibraryAlreadyInitialized						= 3,
	eSteamErrorConfig											= 4,
	eSteamErrorContentServerConnect								= 5,
	eSteamErrorBadHandle										= 6,
	eSteamErrorHandlesExhausted									= 7,
	eSteamErrorBadArg											= 8,
	eSteamErrorNotFound											= 9,
	eSteamErrorRead												= 10,
	eSteamErrorEOF												= 11,
	eSteamErrorSeek												= 12,
	eSteamErrorCannotWriteNonUserConfigFile						= 13,
	eSteamErrorCacheOpen										= 14,
	eSteamErrorCacheRead										= 15,
	eSteamErrorCacheCorrupted									= 16,
	eSteamErrorCacheWrite										= 17,
	eSteamErrorCacheSession										= 18,
	eSteamErrorCacheInternal									= 19,
	eSteamErrorCacheBadApp										= 20,
	eSteamErrorCacheVersion										= 21,
	eSteamErrorCacheBadFingerPrint								= 22,
	eSteamErrorNotFinishedProcessing							= 23,
	eSteamErrorNothingToDo										= 24,
	eSteamErrorCorruptEncryptedUserIDTicket						= 25,
	eSteamErrorSocketLibraryNotInitialized						= 26,
	eSteamErrorFailedToConnectToUserIDTicketValidationServer	= 27,
	eSteamErrorBadProtocolVersion								= 28,
	eSteamErrorReplayedUserIDTicketFromClient					= 29,
	eSteamErrorReceiveResultBufferTooSmall						= 30,
	eSteamErrorSendFailed										= 31,
	eSteamErrorReceiveFailed									= 32,
	eSteamErrorReplayedReplyFromUserIDTicketValidationServer	= 33,
	eSteamErrorBadSignatureFromUserIDTicketValidationServer		= 34,
	eSteamErrorValidationStalledSoAborted						= 35,
	eSteamErrorInvalidUserIDTicket								= 36,
	eSteamErrorClientLoginRateTooHigh							= 37,
	eSteamErrorClientWasNeverValidated							= 38,
	eSteamErrorInternalSendBufferTooSmall						= 39,
	eSteamErrorInternalReceiveBufferTooSmall					= 40,
	eSteamErrorUserTicketExpired								= 41,
	eSteamErrorCDKeyAlreadyInUseOnAnotherClient					= 42,

	eSteamErrorNotLoggedIn										= 101,
	eSteamErrorAlreadyExists									= 102,
	eSteamErrorAlreadySubscribed								= 103,
	eSteamErrorNotSubscribed									= 104,
	eSteamErrorAccessDenied										= 105,
	eSteamErrorFailedToCreateCacheFile							= 106,
	eSteamErrorCallStalledSoAborted								= 107,
	eSteamErrorEngineNotRunning									= 108,
	eSteamErrorEngineConnectionLost								= 109,
	eSteamErrorLoginFailed										= 110,
	eSteamErrorAccountPending									= 111,
	eSteamErrorCacheWasMissingRetry								= 112,
	eSteamErrorLocalTimeIncorrect								= 113,
	eSteamErrorCacheNeedsDecryption								= 114,
	eSteamErrorAccountDisabled									= 115,
	eSteamErrorCacheNeedsRepair									= 116,
	eSteamErrorRebootRequired									= 117,

	eSteamErrorNetwork											= 200,
	eSteamErrorOffline											= 201
} ESteamError;

typedef enum
{
	eNoDetailedErrorAvailable	= 0,
	eStandardCerrno				= 1,
	eWin32LastError				= 2,
	eWinSockLastError			= 3,

	eDetailedPlatformErrorCount	= 4
} EDetailedPlatformErrorType;

struct TSteamError
{
	ESteamError					eSteamError;
	EDetailedPlatformErrorType	eDetailedErrorType;
	int							nDetailedErrorCode;
	char						szDesc[255];
};

struct TSteamProgress
{
	int				bValid; // non-zero if call provides progress info
	unsigned int	uPercentDone; // 0 to 100 if bValid
	char			szProgress[255]; // additional progress info
};

typedef unsigned short		SteamInstanceID_t; // MUST be 16 bits
typedef unsigned long long	SteamLocalUserID_t; // MUST be 64 bits

typedef struct
{
	std::uint32_t	Low32bits;
	std::uint32_t	High32bits;
} TSteamSplitLocalUserID;

typedef struct
{
	SteamInstanceID_t m_SteamInstanceID;

	union
	{
		SteamLocalUserID_t		As64bits;
		TSteamSplitLocalUserID	Split;
	} m_SteamLocalUserID;
} TSteamGlobalUserID;
