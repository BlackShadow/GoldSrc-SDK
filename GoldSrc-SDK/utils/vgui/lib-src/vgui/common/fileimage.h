#ifndef VGUI_FILEIMAGE_H
#define VGUI_FILEIMAGE_H

#include <stdio.h>

class FileImageStream
{
public:
	virtual void Read( void* output, int length ) = 0;
	virtual bool ErrorStatus() = 0;
};

class FileImageStream_Memory : public FileImageStream
{
public:
	FileImageStream_Memory( void* data, int length );

	virtual void Read( void* output, int length ) override;
	virtual bool ErrorStatus() override;

private:
	const unsigned char*	_data;
	int						_length;
	int						_position;
	bool					_error;
};

class FileImage
{
public:
	FileImage() : m_Width( 0 ), m_Height( 0 ), m_pData( NULL )
	{
	}

	~FileImage()
	{
		delete[] m_pData;
	}

	void Clear();

	int				m_Width;
	int				m_Height;
	unsigned char*	m_pData;

private:
	FileImage( const FileImage& ) = delete;
	FileImage& operator=( const FileImage& ) = delete;
};

bool	Load32BitTGA( FileImageStream* stream, FileImage* image );
void	Save32BitTGA( FILE* file, FileImage* image );

#endif
