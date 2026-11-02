#pragma once

#include "const.h"
#include "../external/steamworks/isteamapps.h"
#include "../external/steamworks/isteamfriends.h"
#include "../external/steamworks/isteamgameserver.h"
#include "../external/steamworks/isteamuser.h"
#include "../external/steamworks/steamnetworkingfakeip.h"

typedef struct client_s client_t;

class CSteam3
{
public:
	CSteam3();
	virtual ~CSteam3();

	virtual void Shutdown() = 0;

	void GSSendUserStatusResponse( CSteamID& steamID, int status, int result );

	friend qboolean Steam_GSBLoggedOn();

protected:
	bool InitModule();

	bool m_bLoggedOn;
	bool m_bLogOnResult;
	HSteamPipe m_hSteamPipe;
};

class CSteam3Server : public CSteam3
{
public:
	CSteam3Server();
	~CSteam3Server() override;

	void Activate();
	void RunFrame();
	void SetServerType();
	void Shutdown() override;

	client_t* ClientFindFromSteamID( CSteamID& steamID );
	bool NotifyClientConnect( client_t* client, const void* authentication, uint32 bytes );
	void NotifyClientDisconnect( client_t* client );
	bool NotifyBotConnect( client_t* client );
	void NotifyOfLevelChange( bool active );
	void SetSpawnCount( int count );

	uint64 GetSteamID();
	bool BSecure();
	bool BLanOnly();
	bool BWantsSecure();
	bool BLoggedOn();

	void OnGSClientApprove( GSClientApprove_t* result );
	void OnGSClientDeny( GSClientDeny_t* result );
	void OnGSClientKick( GSClientKick_t* result );
	void OnGSPolicyResponse( GSPolicyResponse_t* result );
	void OnLogonSuccess( SteamServersConnected_t* result );
	void OnLogonFailure( SteamServerConnectFailure_t* result );
	void OnFakeIPResult( SteamNetworkingFakeIPResult_t* result );

	friend qboolean Steam_GSBSecurePreference();

private:
	CCallback<CSteam3Server, GSClientApprove_t, true> m_CallbackGSClientApprove;
	CCallback<CSteam3Server, GSClientDeny_t, true> m_CallbackGSClientDeny;
	CCallback<CSteam3Server, GSClientKick_t, true> m_CallbackGSClientKick;
	CCallback<CSteam3Server, GSPolicyResponse_t, true> m_CallbackGSPolicyResponse;
	CCallback<CSteam3Server, SteamServersConnected_t, true> m_CallbackLogonSuccess;
	CCallback<CSteam3Server, SteamServerConnectFailure_t, true> m_CallbackLogonFailure;
	CCallback<CSteam3Server, SteamNetworkingFakeIPResult_t, true> m_CallbackFakeIPResult;

protected:
	void OnGSClientDenyHelper( client_t* client, EDenyReason reason, const char* text );
	void SendUpdatedServerDetails();

	bool m_bHasActivePlayers;
	bool m_bWantToBeSecure;
	bool m_bLanOnly;
	CSteamID m_SteamIDGS;
};

class CSteam3Client : public CSteam3
{
public:
	CSteam3Client();
	~CSteam3Client() override;

	void Shutdown() override;
	void RunFrame();
	void InitClient();

	void ActivateInputSet( bool inMenu );
	void ShowOnScreenKeyboard( int fieldX, int fieldY );

	int InitiateGameConnection( void* data, int maxData, uint64 steamID, uint32 serverIP, uint16 serverPort,
		bool secure );
	void TerminateConnection();

	void UpdateRichPresence();
	void SetRichPresenceOverride( const char* text );
	void SetRichPresenceGameMode( const char* text );
	void SetRichPresenceInfo( const char* mapName );

	void OnClientGameServerDeny( ClientGameServerDeny_t* result );
	void OnGameServerChangeRequested( GameServerChangeRequested_t* result );
	void OnGameOverlayActivated( GameOverlayActivated_t* result );
	void OnNewUrlLaunchParameters( NewUrlLaunchParameters_t* result );
	void OnGameRichPresenceJoinRequested( GameRichPresenceJoinRequested_t* result );

private:
	CCallback<CSteam3Client, ClientGameServerDeny_t, false> m_CallbackClientGameServerDeny;
	CCallback<CSteam3Client, GameServerChangeRequested_t, false> m_CallbackGameServerChangeRequested;
	CCallback<CSteam3Client, GameOverlayActivated_t, false> m_CallbackGameOverlayActivated;

	struct CCallbackInternal_OnNewUrlLaunchParameters : private CCallbackImpl<sizeof( NewUrlLaunchParameters_t )>
	{
		CCallbackInternal_OnNewUrlLaunchParameters()
		{
			SteamAPI_RegisterCallback( this, NewUrlLaunchParameters_t::k_iCallback );
		}

		~CCallbackInternal_OnNewUrlLaunchParameters() override;

	private:
		void Run( void* pvParam ) override;
	};

	struct CCallbackInternal_OnGameRichPresenceJoinRequested
		: private CCallbackImpl<sizeof( GameRichPresenceJoinRequested_t )>
	{
		CCallbackInternal_OnGameRichPresenceJoinRequested()
		{
			SteamAPI_RegisterCallback( this, GameRichPresenceJoinRequested_t::k_iCallback );
		}

		~CCallbackInternal_OnGameRichPresenceJoinRequested() override;

	private:
		void Run( void* pvParam ) override;
	};

	CCallbackInternal_OnNewUrlLaunchParameters m_steamcallback_OnNewUrlLaunchParameters;
	CCallbackInternal_OnGameRichPresenceJoinRequested m_steamcallback_OnGameRichPresenceJoinRequested;
};

CSteam3Server*	Steam3Server();
CSteam3Client*	Steam3Client();

qboolean		Steam_GSBLoggedOn();
qboolean		Steam_GSBSecurePreference();
