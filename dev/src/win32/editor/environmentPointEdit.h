/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

/// Used to forward events about moving vertices to the environment point editor
class CEdEnvironmentPointEditNotifier : public IEditorNodeMovementHook
{
public:
	class CEdEnvironmentPointEdit*			m_editor;				// The editor
	virtual void OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition );
};

class CEdEnvironmentPointEditPanel : public wxPanel
{
	wxDECLARE_CLASS( CEdEnvironmentPointEditPanel );

	friend class CEdEnvironmentPointEdit;

private:
	class CEdEnvironmentPointEdit*			m_editor;				//!< The editor
	CEdPropertiesPage*						m_properties;

	// Widgets defined in the resource
	wxChoice*								m_pointsList;
	wxChoice*								m_pointType;
	wxChoice*								m_pointBlend;
	wxSlider*								m_blendScale;
	wxSlider*								m_dirAngle1;
	wxSlider*								m_dirAngle2;
	wxSlider*								m_outerRadius;
	wxSlider*								m_innerRadius;
	wxSpinCtrl*								m_scaleX;
	wxSpinCtrl*								m_scaleY;
	wxSpinCtrl*								m_scaleZ;
	wxTextCtrl*								m_envPath;

public:
	CEdEnvironmentPointEditPanel( wxWindow* parent, class CEdEnvironmentPointEdit* editor );

	void UpdateEnabledStates();
	void LoadValues();

protected:
	void CreatePointChoices();
	void UpdateSelectedVertex();
	void OpenAdvancedPropertiesPanel();
	SAreaEnvironmentPoint& GetSelectedPoint();
	void UpdateEnvironmentArea();
	void OnPointListSelected( wxCommandEvent& event );
	void OnAddPointClicked( wxCommandEvent& event );
	void OnClonePointClicked( wxCommandEvent& event );
	void OnRemovePointClicked( wxCommandEvent& event );
	void OnPointTypeSelected( wxCommandEvent& event );
	void OnPointBlendSelected( wxCommandEvent& event );
	void OnBlendScaleChanged( wxCommandEvent& event );
	void OnDirAngle1Scroll( wxCommandEvent& event );
	void OnDirAngle2Scroll( wxCommandEvent& event );
	void OnOuterRadiusScroll( wxCommandEvent& event );
	void OnInnerRadiusScroll( wxCommandEvent& event );
	void OnScaleXChanged( wxCommandEvent& event );
	void OnScaleYChanged( wxCommandEvent& event );
	void OnScaleZChanged( wxCommandEvent& event );
	void OnUseEnvironmentClicked( wxCommandEvent& event );
	void OnAdvancedClicked( wxCommandEvent& event );
	void OnOuterToInnerClicked( wxCommandEvent& event );
	void OnPropertiesModified( wxCommandEvent& event );
};

/// Editor tool for editing environment area points
class CEdEnvironmentPointEdit : public IEditorTool, wxEvtHandler
{
	DECLARE_ENGINE_CLASS( CEdEnvironmentPointEdit, IEditorTool, 0 );

	friend class CEdEnvironmentPointEditPanel;
	friend class CEdEnvironmentPointEditNotifier;

private:
	CEdEnvironmentPointEditPanel*			m_panel;				//!< Editor panel
	class CAreaEnvironmentComponent*		m_areaComponent;		//!< Area environment componnet
	TDynArray< class CVertexEditorEntity* >	m_vertices;				//!< Vertex entities
	CEdEnvironmentPointEditNotifier*		m_notifier;				//!< Component instance that notifies vertex position change
	static Bool								m_advancedOpen;			//!< If the advanced properties are open

	void CreateVertices();
	void DestroyVertices();

	void OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition );

public:
	CEdEnvironmentPointEdit();

	virtual String GetCaption() const;
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();
	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	virtual Bool HandleActionClick( Int32 x, Int32 y );
	virtual Bool OnDelete();
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	virtual Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual Bool OnViewportTrack( const CMousePacket& packet );

	virtual Bool UsableInActiveWorldOnly() const { return true; }
};

BEGIN_CLASS_RTTI( CEdEnvironmentPointEdit );
PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();