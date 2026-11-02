#ifndef IBASESYSTEM_H
#define IBASESYSTEM_H

#include "ISystemModule.h"

#ifdef _WIN32
namespace vgui
{
	class Panel;
}
#endif

class IFileSystem;
class IVGuiModule;
class ObjectList;

class IBaseSystem : public virtual ISystemModule
{
public:
	~IBaseSystem() override = default;

	virtual double GetTime() = 0;
	virtual unsigned int GetTick() = 0;
	virtual void SetFPS( float fps ) = 0;

	virtual void Printf( char* format, ... ) = 0;
	virtual void DPrintf( char* format, ... ) = 0;
	virtual void RedirectOutput( char* buffer, int size ) = 0;

	virtual IFileSystem* GetFileSystem() = 0;
	virtual unsigned char* LoadFile( const char* name, int* length ) = 0;
	virtual void FreeFile( unsigned char* buffer ) = 0;

	virtual void SetTitle( char* title ) = 0;
	virtual void SetStatusLine( char* status ) = 0;
	virtual void ShowConsole( bool visible ) = 0;
	virtual void LogConsole( char* filename ) = 0;

	virtual bool InitVGUI( IVGuiModule* module ) = 0;
#ifdef _WIN32
	virtual vgui::Panel* GetPanel() = 0;
#endif

	virtual bool RegisterCommand( char* name, ISystemModule* module, int commandID ) = 0;
	virtual void GetCommandMatches( char* prefix, ObjectList* matches ) = 0;
	virtual void ExecuteString( char* commands ) = 0;
	virtual void ExecuteFile( char* filename ) = 0;
	virtual void Errorf( char* format, ... ) = 0;
	virtual char* CheckParam( char* parameter ) = 0;

	virtual bool AddModule( ISystemModule* module, char* name ) = 0;
	virtual ISystemModule* GetModule( char* interfaceName, char* libraryName, char* instanceName ) = 0;
	virtual bool RemoveModule( ISystemModule* module ) = 0;

	virtual void Stop() = 0;
	virtual char* COM_GetBaseDir() = 0;
};

#endif
