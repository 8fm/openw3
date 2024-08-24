//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <AK/SoundFrame/SFObjects.h>
#include "afxwin.h"

using namespace AK;
using namespace SoundFrame;

class CSFTestPositioningDlg;

// List control that is used to display Game Object
// It keep a reference to all IGameObject that is displayed in it.
class SFGameObjectListCtrl : public CListCtrl
{
public:
	SFGameObjectListCtrl();

	void Init( CSFTestPositioningDlg* in_pPositionDlg );

	void ClearList();

	IGameObject* GetObject( int in_idx );
	IGameObject* GetSelectedObject();
	int AddObject( IGameObject * in_pObject );
	int AddGlobalObject();
	void AddObjects( IGameObjectList * in_pObjectList );

	// Return true if the object was at least once in the list
	bool RemoveObject( AkGameObjectID in_uiGameObject );

private:

	CSFTestPositioningDlg* m_pPositionDlg;
};