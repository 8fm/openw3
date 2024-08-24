//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "afxwin.h"
#include "resource.h"

template<class TObject, class TList>
class CarSimAssignmentComboBox : public CComboBox
{
public:
	void ClearCombo()
	{
		ResetContent();
	}

	AkUniqueID GetSelectedObject()
	{
		int idx = GetCurSel();

		if( idx == CB_ERR )
			return NULL;

		return (AkUniqueID) GetItemDataPtr( idx );
	}

	void AddObjectAndDisable( AkUniqueID in_Object )
	{
		if( in_Object != AK_INVALID_UNIQUE_ID )
		{
			int idx = AddString( L"Unknown" );
			SetItemDataPtr( idx, (void *) in_Object );
			SetCurSel( idx );
		}

		EnableWindow( FALSE );
	}

	int AddObject( TObject* in_pObject )
	{
		int idx = AddString( in_pObject->GetName() );
		SetItemDataPtr( idx, (void *) in_pObject->GetID() );
		return idx;
	}

	void FillFromList( TList * in_pObjectList, AkUniqueID in_ObjectToSelect )
	{
		ClearCombo();

		CString csNone;
		csNone.LoadString( IDS_ASSIGNMENT_NONE );
		
		AddString( csNone );
		SetItemDataPtr( 0, NULL );

		if( in_pObjectList )
		{
			while ( TObject * pObject = in_pObjectList->Next() )
			{
				AddObject( pObject );
			}
		}

		if( ! SelectObject( in_ObjectToSelect ) )
			SetCurSel( 0 );

		if( in_pObjectList )
			in_pObjectList->Reset();
	}

	bool SelectObject( AkUniqueID in_ObjectID )
	{
		int cObjects = GetCount();
		for ( int i = 0; i < cObjects; i++ )
		{
			AkUniqueID object = (AkUniqueID) GetItemDataPtr( i );
			if ( object == in_ObjectID )
			{
				SetCurSel( i );
				return true;
			}
		}

		return false;
	}

	// Return true if the object was in the list
	bool RemoveObject( AkUniqueID in_ObjectID )
	{
		int cObjects = GetCount();
		int iSel = GetCurSel();
		for ( int i = (cObjects - 1); i >= 0; --i )
		{
			AkUniqueID object = (AkUniqueID) GetItemDataPtr( i );
			if ( object == in_ObjectID )
			{
				DeleteString( i );

				if( iSel == i )
					SetCurSel( 0 );

				return true;
			}
		}

		return false;
	}
};