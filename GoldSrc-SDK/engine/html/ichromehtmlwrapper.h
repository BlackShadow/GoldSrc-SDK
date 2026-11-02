#pragma once

#include "htmlmessages.h"
#include <cstdint>

class CUtlString;
class IHTMLResponses;
class IHTMLResponses_HL1;
class IHTMLSerializer;

class IHTMLChromeController
{
public:
	virtual ~IHTMLChromeController();
	virtual bool Init( const char* cachePath, const char* cookiePath );
	virtual void Shutdown();
	virtual bool RunFrame();
	virtual void SetWebCookie( const char* host, const char* name, const char* value, const char* path, int expires );
	virtual void GetWebCookiesForURL( CUtlString* cookies, const char* url, const char* host );
	virtual void SetClientBuildID( std::uint64_t buildID );
	virtual bool BHasPendingMessages();
	virtual void CreateBrowser( IHTMLResponses* responses, bool popup, const char* userAgent );
	virtual void RemoveBrowser( IHTMLResponses* responses );
	virtual void WakeThread();
	virtual HTMLCommandBuffer_t* GetFreeCommandBuffer( EHTMLCommands command, int browserID );
	virtual void PushCommand( HTMLCommandBuffer_t* command );
	virtual void SetCefThreadTargetFrameRate( std::uint32_t frameRate );
	virtual IHTMLSerializer* CreateSerializer( IHTMLResponses_HL1* responses );
};
