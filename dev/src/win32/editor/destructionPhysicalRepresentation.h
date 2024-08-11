/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "wx/choice.h"
#include "wx/textctrl.h"

class CEdDestructionPhysicalRepresentation : public wxEvtHandler
{
protected:
	wxWindow*											m_parent;
	class CEdMeshPreviewPanel*							m_preview;
	class CMeshTypePreviewPhysicsDestructionComponent*	m_previewComponent;
	CPhysicsDestructionResource*						m_mesh;
	wxChoice*											m_simTypeList;
	wxChoice*											m_shapeList;
	class CEdPropertiesBrowserWithStatusbar*			m_properties;	
	CEdUndoManager*										m_undoManager;

public:
	CEdDestructionPhysicalRepresentation( wxWindow* parent, CEdMeshPreviewPanel* preview, CPhysicsDestructionResource* mesh, CEdUndoManager* undoManager );
	~CEdDestructionPhysicalRepresentation();

	void												Refresh();

	void												OnSelectedShapeChanged( wxCommandEvent& event );
	void												OnSimulationTypeChanged( wxCommandEvent& event );
	void												OnPropertyModified( wxCommandEvent& event );

protected:
	void												UpdatePhysXDestructionResource();

	/// Get the index of the currently selected collision shape. -1 if nothing selected (when there are no shapes).
	Int32												GetSelectedShape() const;
	Int32												GetSelectedSimType();

};

