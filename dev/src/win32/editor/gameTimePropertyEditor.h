#pragma once

class CGameTimePropertyEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	wxBitmap			m_icon;
	Bool				m_dayPeriodOnly;

public:
	CGameTimePropertyEditor( CPropertyItem* item, Bool dayPeriodOnly = false );

	//! Property item got selected
	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;

	void OnSpawnGameTimeEditor( wxCommandEvent &event );

	Bool GrabValue( String& displayValue );
};
