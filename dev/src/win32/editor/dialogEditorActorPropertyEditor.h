/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "selectionEditor.h"
#include "..\..\common\redSystem\utility.h"

class CEdDialogEditorActorPropertyEditor : public ISelectionEditor
{
protected:
	Int32		m_propertyType;

public:
	CEdDialogEditorActorPropertyEditor( CPropertyItem* propertyItem, Int32 propertyType );

protected:
	virtual void FillChoices();
};

class CEdDialogEditorActorVoiceTagEditor : public ISelectionEditor
{
public:
	CEdDialogEditorActorVoiceTagEditor( CPropertyItem* propertyItem );

protected:
	virtual void FillChoices();
};
