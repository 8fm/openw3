/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "meshTypePreviewComponent.h"

class CEdMeshEditor;
class CMeshPreviewComponent;

class CVoxelOctree;

class CEdVertexPaintTool;


/// Preview panel that renders mesh preview
class CEdMeshPreviewPanel : public CEdInteractivePreviewPanel
{
	DECLARE_EVENT_TABLE();

	friend class CEdMeshEditor;

protected:
	CEdMeshEditor*					m_meshEditor;		// Mesh Editor
	
	CMeshTypePreviewComponent*		m_meshTypeComponent;

	CEntity*						m_entity;			// Preview entity
	CVoxelOctree*					m_tree;

	TDynArray<Vector>				m_points;
	TDynArray<Vector>				m_normals;
	TDynArray<Uint32>				m_colors;

	Bool							m_showWireframe;
	Bool							m_showCollision;
	Bool							m_showNavObstacles;
	Bool							m_showBoundingBox;
	Bool							m_showTBN;

public:
	CEdMeshPreviewPanel( wxWindow* parent, CEdMeshEditor* editor );
	virtual ~CEdMeshPreviewPanel();
	
	wxString						m_textureArraysDataSize;	//!< Displaying approx texture arrays data size
	wxString						m_textureDataSize;			//!< Displaying approx texture memory size
	wxString						m_meshDataSize;				//!< Displaying approx render data size
	wxString						m_bbDiagInfo;				//!< Displaying half of the bbox diagonal length

	bool m_showPlaneFloor;
	Float m_debugPlaneZ;

	// tool for painting on vertices
	CEdVertexPaintTool*				m_vertexPaintTool;
	
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	virtual Bool OnViewportMouseMove( const CMousePacket& packet );
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	virtual void UpdateBounds();
	virtual void OverrideViewLOD( Int32 lodOverride );
	virtual void ShowNavObstacles( Bool show );
	virtual void Reload();
	virtual void Refresh();
	virtual void ShowWireframe( Bool show );
	virtual void ShowBoundingBox( Bool show );
	virtual void ShowCollision( Bool show );
	virtual void ShowTBN( Bool show );

	void SetTree( CVoxelOctree* tree ){ m_tree = tree; }

	//! Get preview entity
	RED_INLINE CEntity* GetEntity() const { return m_entity; }

	EMeshTypePreviewType GetMeshType();
	CMeshTypePreviewComponent* GetMeshPreviewComponent() { return m_meshTypeComponent; }

public:
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	void UpdateBBoxInfo();
};

