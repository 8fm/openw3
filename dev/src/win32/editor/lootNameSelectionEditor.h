/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "selectionEditor.h"

class CLootNameSelectionEditor : public ISelectionEditor
{
public:
	CLootNameSelectionEditor( CPropertyItem* item )
		: ISelectionEditor( item ) {}

protected:
	virtual void FillChoices() override;
	virtual Bool IsTextEditable() const override { return false; }
};
