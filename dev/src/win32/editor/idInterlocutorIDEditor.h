/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CIDInterlocutorIDEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	CEdChoice*	m_choice;

public:
	CIDInterlocutorIDEditor( CPropertyItem* item );
	virtual ~CIDInterlocutorIDEditor(){ };

	virtual void CreateControls		( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	virtual void CloseControls	( ) override;

	virtual Bool GrabValue	( String& displayValue ) override;
	virtual Bool SaveValue	( ) override;
	virtual Bool DrawValue	( wxDC& dc, const wxRect &valueRect, const wxColour& textColour ) override;

protected:
	virtual void OnChoiceChanged( wxCommandEvent &event );
};