/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CBehaviorBoneMultiSelection : public wxEvtHandler,
									public ICustomPropertyEditor

{
	wxBitmap	m_icon;
	Bool		m_withWeight;

public:
	CBehaviorBoneMultiSelection( CPropertyItem* propertyItem, Bool withWeight = false );

	void CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	void CloseControls() override;

	Bool GrabValue( String& displayData ) override;

	Bool SaveValue() override;

protected:
	void OnSelectionDialog( wxCommandEvent &event );
};
