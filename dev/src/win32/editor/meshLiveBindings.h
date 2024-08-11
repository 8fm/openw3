/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once


/// I hate that this doesn't work for member functions. C++ func ptrs are annoying...
typedef void (*BoundControlCallback)( void*, class CBoundControl* );


/// Base class for an object used to control live-updates of a property in the mesh editor. Subclasses are instantiated to provide controllers for
/// specific types of widgets (or combinations of widgets).
class CBoundControl : public wxEvtHandler
{
protected:
	BoundControlCallback m_callbackFunc;
	void* m_callbackContext;

	void NotifyChanged();

public:
	CBoundControl();
	virtual ~CBoundControl() { }

	virtual void UpdateFromSource() = 0;

	void SetCallback( BoundControlCallback function, void* context );
};


/// Hook a wxSlider and wxTextCtrl up to a Float value. The slider's values are rescaled to fit some floating-point bounds (wxSlider only supports integer values).
/// The label displays the Float value.
class CBoundRangedSlider : public CBoundControl
{
protected:
	wxSlider* m_slider;
	wxTextCtrl* m_label;
	Float m_minValue, m_maxValue;
	Float* m_source;

	Float m_actualValue;
	Bool m_liveUpdate;

	void UpdateActualValue();
	void OnSliderChange( wxScrollEvent& event );
	void OnSliderTrack( wxScrollEvent& event );
	void OnTextBoxEdited( wxCommandEvent& event );

public:
	CBoundRangedSlider( wxSlider* slider, wxTextCtrl* label, Float minValue, Float maxValue, Float* source, Bool live );
	virtual ~CBoundRangedSlider() { }

	virtual void UpdateFromSource();
};

/// Hook a wxCheckBox to a Bool. The checkbox's state may optionally be inverted from that of the Bool (ie. if the Bool is true, the box is unchecked).
class CBoundToggleControl : public CBoundControl
{
protected:
	wxCheckBox* m_checkbox;
	Bool* m_source;

	Bool m_invert;

	void OnCheckboxToggled( wxCommandEvent& event );

public:
	CBoundToggleControl( wxCheckBox* checkbox, Bool* source, Bool invert = false );
	virtual ~CBoundToggleControl() { }

	virtual void UpdateFromSource();
};

/// Hook a wxTextCtrl to a numeric value. Optional bounds may be given, so that the final value is clamped to a specific range.
/// To simplify implementation, the text control's value is internally parsed at a Double, and then cast to the appropriate type.
/// This extra conversion may introduce issues with very large numbers, where Double loses precision, but those sorts of values
/// are probably unlikely to be needed.
template<typename T>
class CBoundNumericTextBoxControl : public CBoundControl
{
protected:
	wxTextCtrl* m_textbox;
	T* m_source;

	T m_rangeMin, m_rangeMax;
	Bool m_useRange;

	void OnTextBoxEdited( wxCommandEvent& event );

public:
	CBoundNumericTextBoxControl( wxTextCtrl* textbox, T* source );
	CBoundNumericTextBoxControl( wxTextCtrl* textbox, T minValue, T maxValue, T* source );
	virtual ~CBoundNumericTextBoxControl() { }

	virtual void UpdateFromSource();
};




class CEdBindingsBuilder
{
protected:
	wxPanel*	m_panel;

	wxBoxSizer*	m_mainSizer;
	wxBoxSizer*	m_column;

	TDynArray< CBoundControl* >	m_bindings;
	TDynArray< wxSizer* >		m_mainSizers;	//!< We keep a list of the main sizers we create, so we can destroy them when we clear.

	void*					m_callbackContext;
	BoundControlCallback	m_callbackFunc;


	CBoundControl* AddBoundControl( CBoundControl* boundCtrl );

	wxTextCtrl* AddNumericText( const wxString& text, const wxString& subtext, const wxString& tooltip );

public:
	CEdBindingsBuilder();
	~CEdBindingsBuilder();

	void Clear();

	/// Update controls from their bound sources.
	void RefreshControls();

	/// Set the callback used by all further controls.
	void SetCallback( BoundControlCallback function, void* context );

	void Begin( wxPanel* panel );
	void End();

	void NewColumn();

	void AddSlider( const wxString& text, Float minValue, Float maxValue, Float step, Float* source, const wxString& tooltip );
	void AddSlider( const wxString& text, const wxString& subtext, Float minValue, Float maxValue, Float step, Float* source, const wxString& tooltip );
	void AddLiveSlider( const wxString& text, Float minValue, Float maxValue, Float step, Float* source, const wxString& tooltip );
	void AddLiveSlider( const wxString& text, const wxString& subtext, Float minValue, Float maxValue, Float step, Float* source, const wxString& tooltip );

	void AddNumericTextFloat( const wxString& text, Float* source, const wxString& tooltip );
	void AddNumericTextFloat( const wxString& text, const wxString& subtext, Float* source, const wxString& tooltip );
	void AddNumericTextMinFloat( const wxString& text, Float minValue, Float* source, const wxString& tooltip );
	void AddNumericTextMinFloat( const wxString& text, const wxString& subtext, Float minValue, Float* source, const wxString& tooltip );
	void AddNumericTextMaxFloat( const wxString& text, Float maxValue, Float* source, const wxString& tooltip );
	void AddNumericTextMaxFloat( const wxString& text, const wxString& subtext, Float maxValue, Float* source, const wxString& tooltip );
	void AddNumericTextMinMaxFloat( const wxString& text, Float minValue, Float maxValue, Float* source, const wxString& tooltip );
	void AddNumericTextMinMaxFloat( const wxString& text, const wxString& subtext, Float minValue, Float maxValue, Float* source, const wxString& tooltip );

	void AddNumericTextU32( const wxString& text, Uint32* source, const wxString& tooltip );
	void AddNumericTextU32( const wxString& text, const wxString& subtext, Uint32* source, const wxString& tooltip );
	void AddNumericTextMinU32( const wxString& text, Uint32 minValue, Uint32* source, const wxString& tooltip );
	void AddNumericTextMinU32( const wxString& text, const wxString& subtext, Uint32 minValue, Uint32* source, const wxString& tooltip );
	void AddNumericTextMinMaxU32( const wxString& text, Uint32 minValue, Uint32 maxValue, Uint32* source, const wxString& tooltip );
	void AddNumericTextMinMaxU32( const wxString& text, const wxString& subtext, Uint32 minValue, Uint32 maxValue, Uint32* source, const wxString& tooltip );

	void AddCheckbox( const wxString& text, Bool* source, const wxString& tooltip );
	void AddCheckboxInverted( const wxString& text, Bool* source, const wxString& tooltip );

	void AddSeparator();

	void AddLabel( const wxString& label, const wxString& tooltip="" );
};


#include "meshLiveBindings.inl"
