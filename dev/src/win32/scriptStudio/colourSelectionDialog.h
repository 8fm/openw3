#pragma once

#include "styleManager.h"
#include "styledDocument.h"

class CSSColourSelectionDialog : public wxFrame
{
public:
	CSSColourSelectionDialog( wxWindow* parent );
	~CSSColourSelectionDialog();

private:
	inline wxWindow* CreateCell( wxFlexGridSizer* grid );
	inline void FormatCell( wxFlexGridSizer* grid, wxWindow* cell, wxSizer* cellSizer );

	void CreateEditingPage( wxListbook* book );
	void CreateHoverInfoPage( wxListbook* book );
	void CreateCaretPage( wxListbook* book );

	void CreateColumn( wxFlexGridSizer* grid, const wxString& label );
	void CreateRow( wxFlexGridSizer* grid, const wxString& label, EStyle style );
	void CreateLabelCell( wxFlexGridSizer* grid, const wxString& label );
	void CreateColourCell( wxFlexGridSizer* grid, EStyle style, ESubStyle subStyle );
	void CreateFontCell( wxFlexGridSizer* grid, EStyle style );
	void CreateBoldCell( wxFlexGridSizer* grid );
	void CreateResetCell( wxFlexGridSizer* grid, EStyle style );

	void OnColourChanged( wxColourPickerEvent& event );
	void OnFontChanged( wxFontPickerEvent& event );
	void OnResetStyle( wxCommandEvent& event );
	void OnClose( wxCloseEvent& event );
	void OnPreviewPainted( wxStyledTextEvent& event );

	void RefreshWidgets();

	void OnCaretColourChanged( wxColourPickerEvent& event );
	void OnCaretThicknessChanged( wxScrollEvent& event );
	void OnCaretBlinkRateChanged( wxScrollEvent& event );
	void OnCaretHighlightChanged( wxCommandEvent& event );
	void OnCaretHighlightColourChanged( wxColourPickerEvent& event );
	void OnWordHighlightColourChanged( wxColourPickerEvent& event );

	void OnResetCaretColour( wxCommandEvent& event );
	void OnResetCaretBlinkRate( wxCommandEvent& event );
	void OnResetCaretThickness( wxCommandEvent& event );
	void OnResetCaretHighlight( wxCommandEvent& event );
	void OnResetWordHighlightColour( wxCommandEvent& event );


private:
	wxBitmap m_resetColoursBitmap;

	CSSStyledDocument* m_preview;

	vector< wxColourPickerCtrl* > m_colourPickers;
	vector< wxFontPickerCtrl* > m_fontPickers;

	struct
	{
		wxColourPickerCtrl* m_colour;
		wxSlider* m_blinkRate;
		wxSlider* m_thickness;
		wxCheckBox* m_highlight;
		wxColourPickerCtrl* m_highlightColour;
		wxColourPickerCtrl* m_wordHighlightColour;
	} m_caret;
};
