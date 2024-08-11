/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CVertexEditorEntity;

/// Editor tool for editing stripes
class CEdStripeEdit : public IEditorTool, public IEdEventListener, public wxEvtHandler
{
	DECLARE_ENGINE_CLASS( CEdStripeEdit, IEditorTool, 0 );

	friend class CEdStripeToolPanel;
	friend class CEdStripeEditPresetsBoxHook;
	friend class CUndoStripePoint;

private:
	class CEditedStripe : public IEditorNodeMovementHook
	{
	public:
		class CEdStripeEdit*						m_editor;
		class CStripeComponent*						m_stripe;
		TDynArray< CVertexEditorEntity* >			m_vertices;
		Matrix										m_lastKnownMatrix;

		CEditedStripe(){}
		virtual void OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition );
		virtual void OnEditorNodeRotated( Int32 vertexIndex, const EulerAngles& oldRotation, const EulerAngles& wishedRotation, EulerAngles& allowedRotation );
	};

	CWorld*									m_world;					//!< World shortcut
	CEdRenderingPanel*						m_viewport;					//!< Viewport shortcut
	TDynArray< CEditedStripe* >				m_stripes;					//!< Edited stripes
	Uint32									m_nearbyVertex;				//!< Vertex nearby the mouse cursor
	CEditedStripe*							m_nearbyStripe;				//!< Stripe that contains the m_nearbyVertex
	Float									m_nearbyVertexDistance;		//!< Distance to the nearby vertex
	Vector									m_contextMenuVector;		//!< Saved collision point from context menu
	TDynArray< CVertexEditorEntity* >		m_allVertexEntities;		//!< Contains all vertex entities across stripes

	class CEdStripeToolPanel*				m_panel;

	void OnStripePointMoved( CEditedStripe* editedStripe, Int32 index, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition );
	void OnStripePointRotated( CEditedStripe* editedStripe, Int32 vertexIndex, const EulerAngles& oldRotation, const EulerAngles& wishedRotation, EulerAngles& allowedRotation );
	void UpdateStripe( CEditedStripe* editedStripe );
	void UpdateAllStripes();
	void DestroyVertexEntities();
	void RebuildVertexEntities();
	void SelectPointByIndex( class CStripeComponent* stripe, Uint32 vertexIndex );
	void DestroyEditedStripes();
	void AddEditedStripe( CStripeComponent* stripeComponent );
	void AlignTerrainToStripe( CEditedStripe* editedStripe, Float falloff, Float heightOffset );
	void AlignTerrainToStripes( Float falloff, Float heightOffset );
	void AlignStripeToTerrain( CEditedStripe* editedStripe, Float blend );
	void AlignStripesToTerrain( Float blend );

	void HandleInvalidObject( CObject* invalidObject );

	void OnCreateStripe( wxCommandEvent& event );
	void OnAlignTerrainToStripes( wxCommandEvent& event );
	void OnAlignStripesToTerrain( wxCommandEvent& event );
	void OnSplitStripe( wxCommandEvent& event );
	void OnCloseStripeEditor( wxCommandEvent& event );

public:
	CEdStripeEdit();
	virtual String GetCaption() const;

	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();	
	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	virtual Bool HandleContextMenu( Int32 x, Int32 y, const Vector& collision );
	virtual Bool HandleActionClick( Int32 x, Int32 y );
	virtual Bool OnDelete();
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
	virtual Bool OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );
	virtual Bool OnViewportTrack( const CMousePacket& packet );
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	virtual Bool UsableInActiveWorldOnly() const { return false; }

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

private:
	void Reset();
};

BEGIN_CLASS_RTTI( CEdStripeEdit );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CEdStripeToolPanel : public wxPanel
{
	wxDECLARE_CLASS( CEdStripeToolPanel );

	friend class CEdStripeEdit;

private:
	CEdStripeEdit*				m_tool;
	wxCheckBox*					m_viewAllCheck;
	wxCheckBox*					m_viewAICheck;
	wxCheckBox*					m_viewBlendMaskCheck;
	wxCheckBox*					m_viewControlsCheck;
	wxToggleButton*				m_regularStripe;
	wxToggleButton*				m_terrainStripe;
	wxToggleButton*				m_aiRoad;
	wxToggleButton*				m_alignTerrainToStripe;
	wxPanel*					m_alignTerrainToStripeControls;
	wxToggleButton*				m_alignStripeToTerrain;
	wxPanel*					m_alignStripeToTerrainControls;
	wxPanel*					m_controlPointPanel;
	wxCheckBox*					m_falloffCheck;
	wxSpinCtrl*					m_falloffRangeSpin;
	wxCheckBox*					m_heightOffsetCheck;
	wxSpinCtrl*					m_heightOffsetSpin;
	wxCheckBox*					m_falloffMapCheck;
	wxStaticBitmap*				m_falloffMapBitmap;
	wxSlider*					m_rotationBlendSlider;
	wxSlider*					m_pointSlider;
	wxStaticText*				m_pointLabel;
	wxPanel*					m_stripePropertiesPanel;
	CEdPropertiesPage*			m_stripePropertiesPage;
	wxPanel*					m_propertiesPanel;
	CEdPropertiesPage*			m_propertiesPage;
	CStripeComponent*			m_stripe;
	Int32						m_controlPointIndex;
	wxImage						m_falloffMap;
	class CEdPresets*			m_presets;
	class IEdPresetsHook*		m_presetsHook;
	class CEdPresetsBox*		m_presetsBox;
	class IEdPresetsBoxHook*	m_presetsBoxHook;

	void FillFromControlPoint( const SStripeControlPoint& cp );
	void UpdateControlPoint( SStripeControlPoint& cp );

	void UpdateStripeToggleButtons();
	void UpdatePointInfo();

	void PointSelected( Int32 pointIndex );
	void PointModified( Int32 pointIndex );
	void PointDeleted( Int32 pointIndex );

	void HandleInvalidObject( CObject* invalidObject );

	void OnViewAllClick( wxCommandEvent& event );
	void OnViewAIClick( wxCommandEvent& event );
	void OnViewBlendMaskClick( wxCommandEvent& event );
	void OnRegularStripeClick( wxCommandEvent& event );
	void OnTerrainStripeClick( wxCommandEvent& event );
	void OnAIRoadClick( wxCommandEvent& event );
	void OnAlignTerrainToStripeClick( wxCommandEvent& event );
	void OnBrowseFalloffMapClick( wxCommandEvent& event );
	void OnApplyTerrainToStripeClick( wxCommandEvent& event );
	void OnAlignStripeToTerrainClick( wxCommandEvent& event );
	void OnApplyStripeToTerrainClick( wxCommandEvent& event );
	void OnPointSliderChange( wxCommandEvent& event );
	void OnPropertyChange( wxCommandEvent& event );

public:
	CEdStripeToolPanel( CEdStripeEdit* tool, wxWindow* parent );
	~CEdStripeToolPanel();

	RED_INLINE CStripeComponent* GetStripe() const { return m_stripe; }
	void SetStripe( CStripeComponent* stripe, Bool forceRefresh=false );

	RED_INLINE Int32 GetControlPointIndex() const { return m_controlPointIndex; }
};
