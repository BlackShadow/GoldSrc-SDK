//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#pragma once

#include "ObjectPage.h"
#include "resource.h"

class COP_Flags : public CObjectPage
{
	DECLARE_DYNCREATE( COP_Flags )

public:
	COP_Flags();
	virtual bool SaveData();
	virtual void UpdateData( int mode, PVOID data );
	void UpdateForClass( LPCTSTR className );

	enum { IDD = IDD_OBJPAGE_FLAGS };

protected:
	enum { SaveFlagsData = 4 };
	afx_msg void OnClearFlags();

	DECLARE_MESSAGE_MAP()
};
