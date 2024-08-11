/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Floating properties browser
class CEdPropertiesFrame : public wxSmartLayoutFrame
{
protected:
	CEdPropertiesBrowserWithStatusbar*		m_browser;

public:
	CEdPropertiesFrame( wxWindow* parent, const String& caption, CEdUndoManager* undoManager );
	~CEdPropertiesFrame();

	void SetObject( CObject* object );
	void SetObjects( const TDynArray< CObject* > &objects );

	// Save / Load options from config file
	void SaveOptionsToConfig();
	void LoadOptionsFromConfig();
	void SaveSession( CConfigurationManager &config );
	void RestoreSession( CConfigurationManager &config );
};