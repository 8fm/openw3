#pragma once

#include "selectionEditor.h"

class CEdChooseRewardXMLCustomEditor : public ISelectionEditor
{
public:
	CEdChooseRewardXMLCustomEditor( CPropertyItem* item );

protected:
	virtual void FillChoices();
};
