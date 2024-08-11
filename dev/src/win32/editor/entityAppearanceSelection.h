/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "selectionEditor.h"

class CEntityAppearanceSelection : public ISelectionEditor
{
public:
	CEntityAppearanceSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
};
