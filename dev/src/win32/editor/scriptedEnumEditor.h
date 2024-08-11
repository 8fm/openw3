/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdScriptedEnumPropertyEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	CEdChoice	*m_ctrlChoice;

public:
	CEdScriptedEnumPropertyEditor( CPropertyItem* item );
	virtual ~CEdScriptedEnumPropertyEditor(){ };

	virtual void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual void CloseControls() override;

	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;

protected:
	virtual void OnChoiceChanged( wxCommandEvent &event );
};
