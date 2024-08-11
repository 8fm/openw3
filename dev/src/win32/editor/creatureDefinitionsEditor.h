#pragma once

#include "selectionEditor.h"

class CCreatureDefinitionsEditor : public ISelectionEditor
{
public:
	CCreatureDefinitionsEditor( CPropertyItem* item )
		: ISelectionEditor( item ) {}

protected:
	virtual void FillChoices() override;
	virtual Bool IsTextEditable() const override { return true; }
};
