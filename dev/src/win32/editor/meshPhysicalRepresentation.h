/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "wx/choice.h"
#include "wx/textctrl.h"

/// List of mesh LODs
class CEdMeshPhysicalRepresentation : public wxEvtHandler, public IEdEventListener
{
protected:
	wxWindow*					m_parent;
	class CEdMeshPreviewPanel*	m_preview;
	CMesh*						m_mesh;
	wxChoice*					m_physicalMaterial;
	wxChoice*					m_shapeList;

	wxTextCtrl*					m_densityScaler;

	wxCheckBox*					m_soundOcclusionEnabled;
	wxTextCtrl*					m_soundOcclusionAttenuation;
	wxTextCtrl*					m_soundOcclusionDiagonalLimit;

	wxChoice*					m_rotationAxis;


public:
	CEdMeshPhysicalRepresentation( wxWindow* parent, CEdMeshPreviewPanel* preview, CMesh* mesh );
	~CEdMeshPhysicalRepresentation();

	void DispatchEditorEvent( const CName& name, IEdEventData* data );

	void Refresh();

	void OnSelectedShapeChanged( wxCommandEvent& event );
	void OnPhysicalMaterialChoicebookPageChanged( wxCommandEvent& event );

	void OnMeshCollisionRemoveAll( wxCommandEvent& event );
	void OnMeshCollisionRemove( wxCommandEvent& event );
	void OnMeshCollisionAdd( wxCommandEvent& event );

	void OnDensityScaler( wxCommandEvent& event );
	void OnFillDensityScaler( wxCommandEvent& event );

	void OnRotationAxisChanged( wxCommandEvent& event );

	void OnSoundAttenuationSwitch( wxCommandEvent& event );
	void OnSoundAttenuationParametersChanged( wxCommandEvent& event );

protected:
	void UpdateMaterials();

	/// Get the index of the currently selected collision shape. -1 if nothing selected (when there are no shapes).
	Int32 GetSelectedShape() const;
};


