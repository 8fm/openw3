/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once


// Hook up to a (or many) bitmap button, to toggle between multiple values. Each value has a separate bitmap image.
// Templated so that any type of value can be supported (just needs equality comparison and copy/assignment, but
// should probably be something simple to avoid high cost of copying).
template< typename T >
class CEdMultiValueBitmapButtonControl : public wxEvtHandler
{
protected:
	struct Value
	{
		T			m_value;
		wxBitmap	m_bitmap;
	};

	TDynArray< Value >				m_values;				//!< Values that have been added. These are cycled through in the order they were added.
	TDynArray< wxBitmapButton* >	m_buttons;				//!< All buttons that are watched/changed by this control.
	Uint32							m_currentValueIndex;	//!< Index of current value.

	Bool							m_enabled;				//!< Are buttons enabled?

public:
	CEdMultiValueBitmapButtonControl()
		: m_currentValueIndex( 0 )
		, m_enabled( true )
	{
	}


	//! Set bitmap to be shown on buttons for the given value. Buttons will be set up with the first value added, but no event
	//! is triggered.
	void AddValue( T value, const wxBitmap& bitmap )
	{
		m_values.Grow( 1 );
		m_values.Back().m_value = value;
		m_values.Back().m_bitmap = bitmap;

		// If this was the first, set all buttons to match.
		if ( m_values.Size() == 1 )
		{
			SetValueIndex( 0, true );
		}
	}

	//! Add a button which will be watched and modified by this control. When the button is clicked, the current value
	//! will be cycled. When the value changes, the bitmap on the button will be changed. This function will set the
	//! button up for the current value.
	void AddButton( wxBitmapButton* button )
	{
		if ( m_buttons.PushBackUnique( button ) )
		{
			button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMultiValueBitmapButtonControl::OnButtonClicked ), 0, this );

			// Set icon for current value.
			wxBitmap buttonIcon = GetCurrentBitmap();
			button->SetBitmap( buttonIcon );

			// Enable or disable based on current setting.
			button->Enable( m_enabled );
		}
	}

	//! Set the current value. Setting a value that has not been added has no effect. If the value is changed
	//! and muteEvents is not true, wxEVT_COMMAND_BUTTON_CLICKED is triggered.
	void SetValue( T newValue, bool muteEvents = false )
	{
		// Find index for this value.
		Uint32 valueIndex = m_values.Size();
		for ( Uint32 i = 0; i < m_values.Size(); ++i )
		{
			if ( m_values[i].m_value == newValue )
			{
				valueIndex = i;
				break;
			}
		}
		if ( valueIndex != m_currentValueIndex )
		{
			SetValueIndex( valueIndex, muteEvents );
		}
	}

	//! Get the current value. If there is no current value (i.e. there are no values), T() is returned.
	RED_FORCE_INLINE T GetValue() const
	{
		if ( m_currentValueIndex >= m_values.Size() )
		{
			return T();
		}
		return m_values[m_currentValueIndex].m_value;
	}

	//! Set enabled state on all buttons.
	void SetEnabled( bool enabled )
	{
		m_enabled = enabled;

		for ( Uint32 i = 0; i < m_buttons.Size(); ++i )
		{
			m_buttons[i]->Enable( enabled );
		}
	}

	RED_FORCE_INLINE Bool IsEnabled() const { return m_enabled; }

private:
	//! Set current value by setting bitmap on all buttons and optionally sending out an event.
	void SetValueIndex( Uint32 index, Bool muteEvents )
	{
		// Do nothing if setting out-of-range.
		if ( index >= m_values.Size() )
		{
			return;
		}

		// Update images on all buttons.
		m_currentValueIndex = index;
		wxBitmap buttonIcon = GetCurrentBitmap();
		for ( Uint32 i = 0; i < m_buttons.Size(); ++i )
		{
			m_buttons[i]->SetBitmap( buttonIcon );
		}

		// If we aren't muting events, send out notification.
		if ( !muteEvents )
		{
			wxCommandEvent eventChanged( wxEVT_COMMAND_BUTTON_CLICKED );
			eventChanged.SetEventObject( this );
			ProcessEvent( eventChanged );
		}
	}

	//! Get the bitmap for the current value. If the bitmap is not valid, a dummy 1x1 bitmap will be created.
	wxBitmap GetCurrentBitmap() const
	{
		wxBitmap buttonIcon;
		if ( m_currentValueIndex < m_values.Size() )
		{
			buttonIcon = m_values[ m_currentValueIndex ].m_bitmap;
		}
		// If we couldn't get the bitmap, just create a dummy 1x1 image so we don't end up crashing later.
		if ( !buttonIcon.IsOk() )
		{
			buttonIcon.Create(1, 1);
		}
		return buttonIcon;
	}

	void OnButtonClicked( wxCommandEvent& event )
	{
		// When button is clicked, just cycle through values.
		SetValueIndex( ( m_currentValueIndex + 1 ) % m_values.Size(), false );

		// And let the event pass through in case others are listening in.
		event.Skip();
	}

};
