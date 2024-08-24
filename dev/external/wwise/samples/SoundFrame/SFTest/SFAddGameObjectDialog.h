//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"

// Small dialog that will get the new Game object ID and name
class SFAddGameObjectDialog : public CDialog
{
public:
	SFAddGameObjectDialog(CWnd* pParent = NULL);	// standard constructor
	~SFAddGameObjectDialog();

	enum { IDD = IDD_ADD_GAME_OBJECT_DIALOG };

	UINT32 GetGameObjectID() const;
	const CString& GetGameObjectName() const;

protected:
	virtual void OnOK( );

private:

	UINT32 m_uiGameObjectID;
	CString m_csGameObjectName;
};