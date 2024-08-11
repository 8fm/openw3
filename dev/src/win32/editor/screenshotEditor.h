/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdScreenshotEditor : public wxFrame
{
	DECLARE_EVENT_TABLE();
	wxDECLARE_CLASS( CEdScreenshotEditor );

public:
	CEdScreenshotEditor( wxWindow* parent );
	~CEdScreenshotEditor();

	void OnViewportGenerateFragments( IViewport* view, CRenderFrame* frame );

protected:
	void OnGenerate( wxCommandEvent& event );
	void OnFOVChanged( wxScrollEvent& event );
	void OnPresetChanged( wxCommandEvent& event );
	void OnCompositionChanged( wxCommandEvent& event );

	void OnScreenshotTaken( const String& path, const void* buffer, Bool status, const String& msg );

private:
	wxTextCtrl*		m_widthTB;
	wxTextCtrl*		m_heightTB;
	wxChoice*		m_resolutionPresetChoice;

	wxTextCtrl*		m_samplesPerPixelTB;

	wxTextCtrl*		m_fovTB;
	wxSlider*		m_fovSlider;

	wxCheckListBox* m_renderFilterList;
	wxChoice*		m_saveFormatChoice;

	Float			m_preservedEditorCameraFOV;
	Float			m_preservedGameFreeCameraFOV;

	Float			m_fov;

	wxRadioBox*		m_composition;
	wxToggleButton*	m_toggleX;
	wxToggleButton*	m_toggleY;

	Uint32			m_compositionLineWidth;
	Color			m_compositionLineColor;
	Float			m_phi;

private:
	void FillPresets();
	void GenerateFragmentsRuleOfThirds( CRenderFrame* frame, Uint32 width, Uint32 height );
	void GenerateFragmentsGoldenRule( CRenderFrame* frame, Uint32 localWidth, Uint32 localHeight, Uint32 widthMargin, Uint32 heightMargin );
	void GenerateFragmentsFibonacciSpiral( CRenderFrame* frame, Uint32 localWidth, Uint32 localHeight, Uint32 widthMargin, Uint32 heightMargin );
	void GenerateFragmentsDynamicSymmetry( CRenderFrame* frame, Uint32 localWidth, Uint32 localHeight, Uint32 widthMargin, Uint32 heightMargin );
	void GenerateFragmentsDrawRectangle( CRenderFrame* frame, Uint32 x, Uint32 y, Uint32 width, Uint32 height );
};
