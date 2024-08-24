//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SFGameObjectListCtrl.h"
#include "SFTestPositioningDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

SFGameObjectListCtrl::SFGameObjectListCtrl()
	: m_pPositionDlg( NULL )
{}

void SFGameObjectListCtrl::Init( CSFTestPositioningDlg* in_pPositionDlg )
{
	m_pPositionDlg = in_pPositionDlg;

	InsertColumn( 0, L"ID", LVCFMT_LEFT, 30 );
	InsertColumn( 1, L"Name", LVCFMT_LEFT, 150 );
	InsertColumn( 2, L"Pos.X", LVCFMT_LEFT, 45 );
	InsertColumn( 3, L"Pos.Z", LVCFMT_LEFT, 45 );
	InsertColumn( 4, L"Ori.X", LVCFMT_LEFT, 45 );
	InsertColumn( 5, L"Ori.Z", LVCFMT_LEFT, 45 );

	SetExtendedStyle( GetExtendedStyle() | LVS_EX_FULLROWSELECT );
}

void SFGameObjectListCtrl::ClearList()
{
	int cObjects = GetItemCount();
	for ( int i = 0; i < cObjects; i++ )
	{
		IGameObject * pObject = (IGameObject *) GetItemData( i );
		if ( pObject )
			pObject->Release();
	}

	DeleteAllItems();
}

IGameObject* SFGameObjectListCtrl::GetObject( int in_idx )
{
	return (IGameObject*) GetItemData( in_idx );
}

IGameObject* SFGameObjectListCtrl::GetSelectedObject()
{
	IGameObject* pReturn = NULL;

	POSITION pos = GetFirstSelectedItemPosition();
	if ( pos )
	{
		pReturn = GetObject( GetNextSelectedItem(pos) );
	}
	return pReturn;
}

int SFGameObjectListCtrl::AddObject( IGameObject * in_pObject )
{
	CString csTemp;
	csTemp.Format( _T("%d"), in_pObject->GetID() );

	int idx = InsertItem( GetItemCount(), csTemp );
	SetItemText( idx, 1, in_pObject->GetName() );

	AkSoundPosition gameObjectPosition = {0};
	m_pPositionDlg->GetGameObjectPosition( in_pObject->GetID(), gameObjectPosition );

	csTemp.Format( _T("%.0f"), gameObjectPosition.Position.X );
	SetItemText( idx, 2, csTemp );
	csTemp.Format( _T("%.0f"), gameObjectPosition.Position.Z );
	SetItemText( idx, 3, csTemp );
	csTemp.Format( _T("%.2f"), gameObjectPosition.Orientation.X );
	SetItemText( idx, 4, csTemp );
	csTemp.Format( _T("%.2f"), gameObjectPosition.Orientation.Z );
	SetItemText( idx, 5, csTemp );

	SetItemData( idx, (DWORD_PTR)in_pObject );

	in_pObject->AddRef();

	return idx;
}

int SFGameObjectListCtrl::AddGlobalObject()
{
	int idx = InsertItem( GetItemCount(), _T("-") );// no ID for global object
	SetItemText( idx, 1, _T("Global") );

	SetItemText( idx, 2, _T("none") );
	SetItemText( idx, 3, _T("none") );
	SetItemText( idx, 4, _T("none") );
	SetItemText( idx, 5, _T("none") );

	SetItemData( idx, NULL );

	return idx;
}

void SFGameObjectListCtrl::AddObjects( IGameObjectList * in_pObjectList )
{
	while ( IGameObject * pObject = in_pObjectList->Next() )
	{
		AddObject( pObject );
	}
}

bool SFGameObjectListCtrl::RemoveObject( AkGameObjectID in_uiGameObject )
{
	bool retVal = false;

	int cObjects = GetItemCount();
	for ( int i = (cObjects - 1); i >= 0; --i )
	{
		IGameObject* pObject = GetObject( i );
		if ( pObject && pObject->GetID() == in_uiGameObject )
		{
			pObject->Release();
			DeleteItem( i );

			retVal = true;
		}
	}

	return retVal;
}