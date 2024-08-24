//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SFAddGameObjectDialog.h"

#include <AK/SoundFrame/SFObjects.h>

using namespace AK;
using namespace SoundFrame;

SFAddGameObjectDialog::SFAddGameObjectDialog( CWnd* pParent )
:	CDialog( SFAddGameObjectDialog::IDD, pParent )
,	m_uiGameObjectID( IGameObject::s_InvalidGameObject )
{
}
	
SFAddGameObjectDialog::~SFAddGameObjectDialog()
{
}

UINT32 SFAddGameObjectDialog::GetGameObjectID() const
{
	return m_uiGameObjectID;
}
	
const CString& SFAddGameObjectDialog::GetGameObjectName() const
{
	return m_csGameObjectName;
}

void SFAddGameObjectDialog::OnOK( )
{
	CString csGameObjectID;
	GetDlgItemText( IDC_GAME_OBJECT_ID_EDIT, csGameObjectID );

	GetDlgItemText( IDC_GAME_OBJECT_NAME_EDIT, m_csGameObjectName );

	m_uiGameObjectID = _ttol( csGameObjectID );

	if( csGameObjectID.IsEmpty() || m_uiGameObjectID == IGameObject::s_InvalidGameObject )
	{
		MessageBox( L"Enter a valid Game Object ID." );
		return;
	}

	CDialog::OnOK(); 
}