//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "afxwin.h"

// List box that will contain Sound Frame Objects
// It keeps a referece to all objects in the list.
template<class TObject, class TList>
class SFTestListBox : public CListBox
{
public:
	void ClearList()
	{
		int cObjects = GetCount();
		for ( int i = 0; i < cObjects; i++ )
		{
			TObject * pObject = (TObject *) GetItemDataPtr( i );
			if ( pObject )
				pObject->Release();
		}

		ResetContent();
	}

	TObject* GetObject( int in_idx )
	{
		return (TObject*) GetItemDataPtr( in_idx );
	}

	int AddObject( TObject * in_pObject )
	{
		int idx = AddString( in_pObject->GetName() );
		SetItemDataPtr( idx, in_pObject );

		in_pObject->AddRef();

		return idx;
	}

	void AddObjects( TList * in_pObjectList )
	{
		while ( TObject * pObject = in_pObjectList->Next() )
		{
			AddObject( pObject );
		}

		in_pObjectList->Reset();
	}

	// Return true if the object was at least once in the list
	bool RemoveObject( AkUniqueID in_ObjectID, bool& out_bWasSelected )
	{
		bool retVal = false;
		out_bWasSelected = false;

		int cObjects = GetCount();
		for ( int i = (cObjects - 1); i >= 0; --i )
		{
			TObject * pObject = (TObject *) GetItemDataPtr( i );
			if ( pObject && pObject->GetID() == in_ObjectID )
			{
				out_bWasSelected = out_bWasSelected || GetSel( i ) != FALSE;
				retVal = true;

				pObject->Release();
				DeleteString( i );
			}
		}

		return retVal;
	}
};