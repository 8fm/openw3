/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "selectionEditor.h"

class CEdDialogEditorEnterActorPropertyEditor : public ISelectionEditor
{
public:
	CEdDialogEditorEnterActorPropertyEditor( CPropertyItem* propertyItem );
	virtual ~CEdDialogEditorEnterActorPropertyEditor();

protected:
	virtual void FillChoices();
};

class CEdDialogEditorExitActorPropertyEditor : public ISelectionEditor
{
public:
	CEdDialogEditorExitActorPropertyEditor( CPropertyItem* propertyItem );
	virtual ~CEdDialogEditorExitActorPropertyEditor();

protected:
	virtual void FillChoices();
};
