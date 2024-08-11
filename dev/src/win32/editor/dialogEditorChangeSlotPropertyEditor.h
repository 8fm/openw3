/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "selectionEditor.h"

class CEdSceneEditor;

class CEdDialogEditorChangeSlotPropertyEditor : public ISelectionEditor
{
public:
	CEdDialogEditorChangeSlotPropertyEditor( CPropertyItem* propertyItem );

protected:
	virtual void FillChoices();
	virtual void OnChoiceChanged( wxCommandEvent &event );

private:
	Bool m_isTargetSlot;
};
