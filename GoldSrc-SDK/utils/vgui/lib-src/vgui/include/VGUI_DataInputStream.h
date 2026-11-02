#ifndef VGUI_DATAINPUTSTREAM_H
#define VGUI_DATAINPUTSTREAM_H

#include "VGUI.h"
#include "VGUI_InputStream.h"

namespace vgui
{

class VGUIAPI DataInputStream : public virtual InputStream
{
public:
	DataInputStream( InputStream* is );

	virtual void seekStart( bool& success ) override;
	virtual void seekRelative( int count, bool& success ) override;
	virtual void seekEnd( bool& success ) override;
	virtual int getAvailable( bool& success ) override;

	virtual uchar readUChar( bool& success ) override;
	virtual void readUChar( uchar* buf, int count, bool& success ) override;

	virtual void close( bool& success ) override;
	virtual void close();

	virtual bool readBool( bool& success );
	virtual char readChar( bool& success );
	virtual short readShort( bool& success );
	virtual ushort readUShort( bool& success );
	virtual int readInt( bool& success );
	virtual uint readUInt( bool& success );
	virtual long readLong( bool& success );
	virtual ulong readULong( bool& success );
	virtual float readFloat( bool& success );
	virtual double readDouble( bool& success );
	virtual void readLine( char* buf, int bufLen, bool& success );

private:
	InputStream*	_is;

public:
	DataInputStream& operator=( const DataInputStream& ) = default;
};

}

#endif
