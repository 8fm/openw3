#pragma once 

enum EBrushCSGType : CEnum::TValueType;

/// Panel for the whole brush edition
class CEdBrushPanel : public CEdDraggablePanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE();

protected:
	CEdPropertiesPage*		m_builderProperties;
	CBrushBuilder*			m_brushBuilder;

public:
	CEdBrushPanel( wxWindow* parent );
	~CEdBrushPanel();

protected:
	//! Get the existing builder brush
	CBrushComponent* GetBuilderBrush( Bool allowToCreate = false );

	//! Get the selected layer
	CLayer* GetSelectedLayer();

	//! Internal event dispatch
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

	//! Update the list of layers
	void UpdateLayersList();

	//! Update controls that depends on selected layer
	void UpdateLayerDependentControls();

	//! Initialize editor brush using given builder class
	void InitializeBuildBrush( CClass* builderClass );

	//! Build new edit brush shape using current builder
	void BuildBrush();

	//! Add brush to selected layer
	void CreateBrush( EBrushCSGType brushType );

protected:
	void OnBuildCube( wxCommandEvent& event );
	void OnBuildCylinder( wxCommandEvent& event );
	void OnBuildCone( wxCommandEvent& event );
	void OnBuildSphere( wxCommandEvent& event );
	void OnBuildLinearStairs( wxCommandEvent& event );
	void OnBuildCurvedStairs( wxCommandEvent& event );
	void OnBuildSpiralStairs( wxCommandEvent& event );
	void OnCSGAdd( wxCommandEvent& event );
	void OnCSGSubtract( wxCommandEvent& event );
	void OnCSGIntersect( wxCommandEvent& event );
	void OnCSGDeintersect( wxCommandEvent& event );
	void OnSelectBuilderBrush( wxCommandEvent& event );
	void OnRebuildGeometry( wxCommandEvent& event );
};