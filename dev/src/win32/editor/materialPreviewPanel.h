/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Preview panel that renders material preview
class CEdMaterialPreviewPanel : public CEdPreviewPanel
{
	DECLARE_EVENT_TABLE();

public:
	enum EPreviewShape
	{
		SHAPE_Box,
		SHAPE_Sphere,
		SHAPE_Cylinder,
		SHAPE_Plane,

		SHAPE_Max
	};

protected:
	IMaterial*			m_material;
	CMeshComponent*		m_component;

public:
	CEdMaterialPreviewPanel( wxWindow* parent );

	// Set the material to be shown. Can be an instance, or graph. Setting to NULL will hide the mesh from the preview.
	virtual void SetMaterial( IMaterial* material );
	IMaterial* GetMaterial() const;

	// Set which shape the material is shown on.
	void SetShape( EPreviewShape shape );

	void RefreshPreviewVisibility( Bool visible );

protected:
	virtual void HandleContextMenu( Int32 x, Int32 y );

	void OnMeshSelected( wxCommandEvent& event );

	virtual void OnViewportGenerateFragments( IViewport* view, CRenderFrame* frame );
};
