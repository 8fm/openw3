/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "selectionEditor.h"


class CEdWorldSelectionEditor : public ISelectionEditor
{
public:
	CEdWorldSelectionEditor( CPropertyItem* item );

	virtual void FillChoices();
};

//////////////////////////////////////////////////////////////////////////

class CEdWorldSelectionQuestBindingEditor : public CEdWorldSelectionEditor
{
public:
	CEdWorldSelectionQuestBindingEditor( CPropertyItem* item );

	virtual void FillChoices();
};


//////////////////////////////////////////////////////////////////////////

class CEdCSVWorldSelectionEditor : public ISelectionEditor
{
public:
	CEdCSVWorldSelectionEditor( CPropertyItem* item );

	virtual void FillChoices();
};