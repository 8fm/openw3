/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Resource preview panel
class CEdPreviewPanel : public CEdRenderingPanel,
						public ISavableToConfig
{
protected:
	CWorld*			m_previewWorld;			// Preview world
	wxSlider*		m_lightPositionSlider;
	wxCheckBox*		m_shadowsCheckbox;
	wxSpinCtrl*		m_fovSpin;
	wxCheckBox*		m_lockCheckbox;
		
	wxChoice*		m_envChoice;	
	TDynArray< CEnvironmentDefinition* > m_envDefs;

    CFont*			m_font;

	Float			m_windIndicatorScale;

	CSelectionManager*	   m_selectionManager;
	CNodeTransformManager* m_transformManager;

public:
	CEdPreviewPanel( wxWindow* parent, Bool allowRenderOptionsChange = false, Bool allowLocking = false );
	virtual ~CEdPreviewPanel();

	// Get world used for previewing
	RED_INLINE CWorld* GetPreviewWorld() const { return m_previewWorld; }

	// Get lock state
	RED_INLINE Bool GetLockState() const { return m_lockCheckbox != nullptr ? m_lockCheckbox->GetValue() : false; }

	void SetShadowsEnabled( bool enable );
	Bool GetShadowsEnabled();
	void SetLightPosition( Int32 pos );
	Int32 GetLightPosition();

	void SetEnableFOVControl( Bool value );

	virtual void Reload() {}

	virtual void		SaveSession( CConfigurationManager &config );
	virtual void		RestoreSession( CConfigurationManager &config );

	/// Set a scalar applied to the arrow in the wind indicator (shown when viewing "Wind" debug info in the preview panel).
	/// A value of 1 gives the arrow the same length as the indicator's radius (head of the arrow is on the sphere).
	void SetWindIndicatorScale( Float scale ) { m_windIndicatorScale = scale; }
	Float GetWindIndicatorScale() { return m_windIndicatorScale; }
	void ShowZoomExtents( const Box& b );

	virtual CWorld* GetWorld() override;
	virtual CSelectionManager* GetSelectionManager() override;
	virtual CNodeTransformManager* GetTransformManager() override;

protected:
	//! Generate debug viewport fragments
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame );

	//! Create rendering frame for viewport
	virtual CRenderFrame *OnViewportCreateFrame( IViewport *view );

	//! Render viewport, should issue a render command
	virtual void OnViewportRenderFrame( IViewport *view, CRenderFrame *frame );

	//! External viewport tick
	virtual void OnViewportTick( IViewport* view, Float timeDelta );

	//! Inputs
	virtual Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );

	void OnLightPosChanged( wxCommandEvent &event );
	void OnShadowsChanged( wxCommandEvent &event );
	void OnFovChanged( wxCommandEvent& event );
	void ChangeEnvironment( Int32 newSelection );
	void OnChoiceChanged( wxCommandEvent &event );
};