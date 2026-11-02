#pragma once

#include "interface.h"

struct cl_enginefuncs_s;
class IBaseSystem;

#ifdef _MSC_VER
class __declspec( novtable ) IGameUI : public IBaseInterface
#else
class IGameUI : public IBaseInterface
#endif
{
public:
	virtual void Initialize( CreateInterfaceFn* factories, int count );
	virtual void Start( cl_enginefuncs_s* engineFunctions, int interfaceVersion, IBaseSystem* system );
	virtual void Shutdown();

	virtual int ActivateGameUI();
	virtual int ActivateDemoUI();
	virtual int HasExclusiveInput();
	virtual void RunFrame();

	virtual void ConnectToServer( const char* game, int address, int port );
	virtual void DisconnectFromServer();

	virtual void HideGameUI();
	virtual bool IsGameUIActive();

	virtual void LoadingStarted( const char* resourceType, const char* resourceName );
	virtual void LoadingFinished( const char* resourceType, const char* resourceName );
	virtual void StartProgressBar( const char* progressType, int steps );
	virtual int ContinueProgressBar( int point, float fraction );
	virtual void StopProgressBar( bool error, const char* failureReason, const char* extendedReason );
	virtual int SetProgressBarStatusText( const char* statusText );
	virtual void SetSecondaryProgressBar( float progress );
	virtual void SetSecondaryProgressBarText( const char* statusText );

	virtual void ValidateCDKey( bool force, bool connect );
	virtual void OnDisconnectFromServer( int failureCode, const char* username );

	virtual void SetIsRunningOnSteamDeck( bool running );
	virtual bool IsRunningOnSteamDeck() const;
};
