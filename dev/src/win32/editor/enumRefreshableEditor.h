#pragma once

#include "selectionEditor.h"


class CEdEnumRefreshableEditor : public ISelectionEditor
{
public:
	CEdEnumRefreshableEditor( CPropertyItem* item );

protected:
	virtual void FillChoices() override;
	virtual Bool SaveValue() override;
	virtual Bool GrabValue( String& displayValue ) override;
};
