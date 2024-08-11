/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "selectionEditor.h"

class CEdCutsceneEffectPropertyEditor : public ISelectionEditor
{
public:
	CEdCutsceneEffectPropertyEditor( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
};

class CEdCutsceneActorEffectPropertyEditor : public ISelectionEditor
{
public:
	CEdCutsceneActorEffectPropertyEditor( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
};
