#pragma once

#include "selectionEditor.h"

class CEncounterPhasesEditor : public ISelectionEditor
{
public:
	CEncounterPhasesEditor( CPropertyItem* item )
		: ISelectionEditor( item )													{}
protected:
	void FillChoices() override;
	Bool IsTextEditable() const override;
};