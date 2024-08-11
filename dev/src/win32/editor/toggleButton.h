#pragma once

/// My toggle button
class CEdToggleButton : public wxPanel
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Editor );
	DECLARE_EVENT_TABLE();

protected:
	wxColour	m_color;		//!< Color of button when it's pressed
	Bool		m_isToggled;	//!< True if button is toggled
	Bool		m_isPressed;	//!< True if button is currently pressed
	Bool		m_isMouseOver;	//!< Is the mouse over the button
	Bool		m_isAutoToggle;	//!< Auto toggle button
	wxString	m_caption;		//!< Button caption
	wxFont		m_drawFont;		//!< Drawing font
	wxFont		m_boldFont;		//!< Font used for drawing

public:
	//! Constructor
	CEdToggleButton( wxWindow* parent, const wxPoint &position, const wxSize& size, const wxString& caption );

	//! Is this button toggled ?
	RED_INLINE Bool IsToggled() const { return m_isToggled; }

	//! Set toggle state
	void SetToggle( Bool toggle );

	//! Set auto toggle state
	void SetAutoToggle( Bool autoToggle );

	//! Set caption
	void SetCaption( const wxString& string );

	//! Set font
	void SetDrawFont( const wxFont& font );

	//! Set color
	void SetColor( const wxColor& color );

protected:
	//! Events
	void OnPaint( wxPaintEvent& event );
	void OnSize( wxSizeEvent& event );
	void OnEraseBackground( wxEraseEvent& event );
	void OnMouseClick( wxMouseEvent& event );
	void OnMouseMove( wxMouseEvent& event );
};