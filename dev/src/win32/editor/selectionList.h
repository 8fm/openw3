/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Class list selection
class CListSelection : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	CEdChoice	*m_ctrlChoice;
	wxButton    *m_clearButton;
	Bool		m_isCleared;

public:
	CListSelection( CPropertyItem* item );
	virtual ~CListSelection(){ };

	virtual void CloseControls() override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;
	virtual void ClearValue();

protected:
	virtual void OnChoiceChanged( wxCommandEvent &event );

public:
	void OnClearClicked( wxCommandEvent &event );
};
