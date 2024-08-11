#pragma once

#include "animationPreviewGhost.h"

wxDECLARE_EVENT( wxEVT_GHOSTCONFIGUPDATED, wxCommandEvent );

class CEdGhostConfigDialog : public wxSmartLayoutDialog
{
	wxDECLARE_CLASS( CEdGhostConfigDialog );

public:
	CEdGhostConfigDialog( wxWindow* parent, Uint32 count = 10, PreviewGhostContainer::EGhostType type = PreviewGhostContainer::GhostType_Entity );
	~CEdGhostConfigDialog();

	Uint32 GetCount() const { return m_numberOfInstances; }
	PreviewGhostContainer::EGhostType GetType() const { return m_type; }

private:
	void OnNumberOfInstancesChanged( wxSpinEvent& event );
	void OnTypeSelected( wxCommandEvent& event );
	void OnOK( wxCommandEvent& event );


private:
	Uint32 m_numberOfInstances;
	PreviewGhostContainer::EGhostType m_type;
};
