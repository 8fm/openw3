/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "selectionEditor.h"


class CQuestBehaviorGraphSelection : public ISelectionEditor
{
public:
	CQuestBehaviorGraphSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};

	Bool GrabValue( String& displayValue ) override;
	Bool SaveValue() override;

protected:
	virtual void FillChoices();
	virtual void OnChoiceChanged( wxCommandEvent &event );
};

class CQuestBehaviorTagsSelection : public ISelectionEditor
{
public:
	CQuestBehaviorTagsSelection( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices() override;
	virtual void OnChoiceChanged( wxCommandEvent &event ) override;
};
