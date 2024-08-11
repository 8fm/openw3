/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "selectionEditor.h"

class CEdDialogEditorSettingPropertyEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	CEdChoice* m_choice;

public:
	CEdDialogEditorSettingPropertyEditor( CPropertyItem* propertyItem );
	~CEdDialogEditorSettingPropertyEditor();

	//! Property item got selected
	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	//! Close property item editors
	virtual void CloseControls() override;

	//! Draw property, return true if you handle this message
	virtual Bool DrawValue( wxDC& dc, const wxRect &valueRect, const wxColour& textColour ) override;

	//! Save property value, return true if you process this message
	virtual Bool SaveValue() override;

	//! Grab property value, return true if you process this message
	virtual Bool GrabValue( String& displayValue ) override;


protected:
	void OnChoiceChanged( wxCommandEvent& event );
};


class CEdSceneInputSelector : public ISelectionEditor
{
public:
	CEdSceneInputSelector( CPropertyItem* item ) : ISelectionEditor( item ) {};
protected:
	virtual void FillChoices();
};