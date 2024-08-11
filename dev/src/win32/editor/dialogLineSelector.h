// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "selectionEditor.h"

/*
Dialog line selector.

Dialog line selector is to be used in Scene Editor only. It allows
the user to choose a dialog line from all lines of current section.
*/
class CDialogLineSelector : public ISelectionEditor
{
public:
	CDialogLineSelector( CPropertyItem* propertyItem  );
	virtual ~CDialogLineSelector() override;

	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

protected:
	virtual void FillChoices() override;
};
