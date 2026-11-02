#ifndef MD5_H
#define MD5_H

extern "C"
{
	char*	MD5_Print( unsigned char* hash );
	char*	MD5_GetCDKeyHash( const char* key );
	void	MD5_Hash_Mem( unsigned char* digest, unsigned char* mem, int size );
}

#endif
