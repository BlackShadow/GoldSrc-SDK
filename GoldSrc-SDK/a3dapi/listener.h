#pragma once

#include <windows.h>
#include <unknwn.h>

class CA3dPrivateUnknown : public IUnknown
{
public:
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
};
