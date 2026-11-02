#ifndef VGUI_FILEINPUTSTREAM_H
#define VGUI_FILEINPUTSTREAM_H

#include "VGUI.h"
#include "VGUI_InputStream.h"

#include <stdio.h>

namespace vgui
{

class VGUIAPI FileInputStream : public InputStream
{
public:
	FileInputStream( const char* fileName, bool textMode );

	virtual void seekStart( bool& success ) override;
	virtual void seekRelative( int count, bool& success ) override;
	virtual void seekEnd( bool& success ) override;
	virtual int getAvailable( bool& success ) override;

	virtual uchar readUChar( bool& success ) override;
	virtual void readUChar( uchar* buf, int count, bool& success ) override;

	virtual void close( bool& success ) override;
	virtual void close();

private:
	FILE*	_fp;

public:
	FileInputStream& operator=( const FileInputStream& ) = default;
};

}

#endif
