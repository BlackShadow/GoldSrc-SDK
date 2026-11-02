#pragma once

#include "const.h"
#include "filesystem.h"

void		VGuiWrap_Startup();
void		VGuiWrap_Shutdown();
int			VGuiWrap_CallEngineSurfaceAppHandler( void* event, void* userData );
void		VGuiWrap_ReleaseMouse();
void		VGuiWrap_GetMouse();
void		VGuiWrap_SetVisible( int state );
void		VGuiWrap_Paint( int paintAll );
void		VGuiWrap_SetRootPanelSize();
void*		VGuiWrap_GetPanel();

int			VGui_LoadBMP( FileHandle_t file, byte* buffer, int bufferSize, int* width, int* height );
int			VGui_Key_Event( int down, int keynum, const char* binding );
int			VGui_GameUIKeyPressed();

void		VGui_ViewportPaintBackground( int* extents );
