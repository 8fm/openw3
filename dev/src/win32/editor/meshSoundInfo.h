/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "wx/choice.h"
#include "wx/textctrl.h"

/// List of mesh LODs
class CEdMeshSoundInfo : public wxEvtHandler
{
protected:
	wxWindow*					m_parent;
	class CEdMeshPreviewPanel*	m_preview;
	CMesh*						m_mesh;

	wxChoice*					m_soundBoneMappingInfo;
	wxTextCtrl*					m_soundTypeIdentification;
	wxTextCtrl*					m_soundSizeIdentification;

	wxButton*					m_createSoundInfo;
	wxButton*					m_removeSoundInfo;

private:
	CName						m_default;

public:
	CEdMeshSoundInfo( wxWindow* parent, CEdMeshPreviewPanel* preview, CMesh* mesh );
	~CEdMeshSoundInfo();

	void Refresh();

	void OnSoundAdditionalInfoChanged( wxCommandEvent& event );
	void OnMeshSoundTypeIdentificationChanged( wxCommandEvent& event );
	void OnMeshSoundSizeIdentificationChanged( wxCommandEvent& event );

	void OnMeshSoundInfoRemove( wxCommandEvent& event );
	void OnMeshSoundInfoAdd( wxCommandEvent& event );

protected:
	void UpdateMaterials();

};