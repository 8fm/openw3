#pragma once

#include "selectionEditor.h"

class CEdHardAttachmentBonePicker : public ISelectionEditor
{
public:
	CEdHardAttachmentBonePicker( CPropertyItem* item );

protected:
	virtual void FillChoices() override;
};