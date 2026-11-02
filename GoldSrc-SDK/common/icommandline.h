//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================
#ifndef ICOMMANDLINE_H
#define ICOMMANDLINE_H

//-----------------------------------------------------------------------------
// Purpose: Interface to engine command line
//-----------------------------------------------------------------------------
class ICommandLine
{
public:
	virtual void CreateCmdLine( const char* commandline ) = 0;
#if !defined( _WIN32 )
	virtual void CreateCmdLine( int argc, const char** argv ) = 0;
#endif
	virtual const char* GetCmdLine() = 0;
	virtual char* CheckParm( const char* psz, char** ppszValue ) = 0;

	virtual void RemoveParm( const char* pszParm ) = 0;
	virtual void AppendParm( const char* pszParm, const char* pszValues ) = 0;
	virtual void SetParm( const char* pszParm, const char* pszValues ) = 0;
	virtual void SetParm( const char* pszParm, int iValue ) = 0;
};

extern ICommandLine* cmdline;

#endif
