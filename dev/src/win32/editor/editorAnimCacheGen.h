/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "smartLayout.h"

class CEdAnimCacheGenerator : public wxSmartLayoutPanel
{
	DECLARE_EVENT_TABLE();

protected:
	wxTextCtrl* m_log;

public:
	CEdAnimCacheGenerator( wxWindow* parent );

protected:
	void Log( wxString str );

protected:
	void OnGenerate( wxCommandEvent& event );
	void OnLoad( wxCommandEvent& event );
	void OnUnloadAnims( wxCommandEvent& event );
	void OnLoadAnims( wxCommandEvent& event );
	void OnStreamAnims( wxCommandEvent& event );
	void OnReport( wxCommandEvent& event );
};
