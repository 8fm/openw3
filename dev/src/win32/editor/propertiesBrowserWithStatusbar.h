/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//! Wrapper for CEdPropertiesPage, adding a status bar. 
//! This class exists to keep backwards compatibility (propertyItem->GetParent() has to be CEdPropertiesPage)
class CEdPropertiesBrowserWithStatusbar : public wxPanel
{
public:
	CEdPropertiesBrowserWithStatusbar( wxWindow* parent, const PropertiesPageSettings& settings, CEdUndoManager* undoManager );

	// Access enclosed properties page
	CEdPropertiesPage& Get() { return *m_propertiesBrowser; }
	const CEdPropertiesPage& Get() const { return *m_propertiesBrowser; }

protected:
	CEdPropertiesPage*	m_propertiesBrowser;
	wxStaticText*		m_statusBar;

private:
	// Hide Connect and Bind to prevent from accidentally connecting to the wrong window
	void Connect( int, wxObjectEventFunction, wxObject*, wxEvtHandler* );
	template < typename E, typename C, typename A, typename H > void Bind( const E&, void (C::*)(A&), H*, int, int, wxObject* );
};
