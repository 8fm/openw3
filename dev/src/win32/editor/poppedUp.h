/**
 * Copyright © CD Projekt Red. All Rights Reserved.
 */
#pragma once

template < typename T >
class CEdPoppedUpBase : public T, public ISmartLayoutWindow
{
public:
	CEdPoppedUpBase(
		wxWindow*			parent, 
        const wxPoint&		pos			= wxDefaultPosition,
        const wxSize&		size		= wxDefaultSize,
		CEdSizeConstaints	constraints	= CEdSizeConstaints::EMPTY,
		const String&		title		= TXT("")
		)
		: ISmartLayoutWindow( nullptr/*will set it after creation*/ )
	{
		CreateDialog( parent, pos, size, constraints, title );
	}

	int ShowModal()
	{
		return m_dlg->ShowModal();
	}

    void EndModal( int retCode )
	{
		return m_dlg->EndModal( retCode );
	}

protected:
	wxDialog* m_dlg;

private:
	void CreateDialog( wxWindow* parent, const wxPoint&	pos, const wxSize& size, CEdSizeConstaints constraints, const String& title )
	{
		long dlgStyle = wxCLIP_CHILDREN;
		
		if ( !constraints.IsEmpty() ) dlgStyle |= wxRESIZE_BORDER;
		if ( !title.Empty() ) dlgStyle |= wxCAPTION;

		m_dlg = new wxDialog( parent, wxID_ANY, title.AsChar(), pos, size, dlgStyle, wxDialogNameStr );
		m_dlg->SetSizer( new wxBoxSizer( wxVERTICAL ) );
 		m_dlg->GetSizer()->Add( this, 1, wxEXPAND|wxALL, 0 );
		m_dlg->Bind( wxEVT_CHAR_HOOK, &CEdPoppedUpBase::OnCharHook, this );

		if ( !constraints.IsEmpty() )
		{
			m_dlg->SetMinSize( constraints.m_min );
			m_dlg->SetMaxSize( constraints.m_max );
		}

		m_this = m_dlg; // initialize ISmartLayoutWindow
		SmartSetSize( pos.x, pos.y, size.x, size.y );
	}

	void OnCharHook( wxKeyEvent& event )
	{
		switch ( event.GetKeyCode() )
		{
		case WXK_ESCAPE:
			EndModal( wxID_ABORT );
			break;

		case WXK_RETURN:
			EndModal( wxID_OK );
			break;

		default:
			event.DoAllowNextEvent();
			break;
		}
	}
};

//! Turns (almost) every control into a popup-menu like modal window
template < typename T >
class CEdPoppedUp : public CEdPoppedUpBase< T >
{
public:
	CEdPoppedUp(
		wxWindow*			parent, 
        const wxPoint&		pos   = wxDefaultPosition,
        const wxSize&		size  = wxDefaultSize,
        long				style = 0,
		CEdSizeConstaints	constraints	= CEdSizeConstaints::EMPTY,
		const String&		title		= TXT("")
		)
		: CEdPoppedUpBase( parent, pos, size, constraints, title )
	{
		T::Create( m_dlg, wxID_ANY, pos, size, style|wxCLIP_CHILDREN );
	}
};

//! A specialization for wxListBox to handle the custom order of Create method parameters.
//! You may add specializations for more controls if necessary.
template <>
class CEdPoppedUp< wxListBox > : public CEdPoppedUpBase< wxListBox >
{
public:
	CEdPoppedUp(
		wxWindow*			parent, 
        const wxPoint&		pos			= wxDefaultPosition,
        const wxSize&		size		= wxDefaultSize,
        long				style		= 0,
		CEdSizeConstaints	constraints	= CEdSizeConstaints::EMPTY,
		const String&		title		= TXT("")
		)
		: CEdPoppedUpBase( parent, pos, size, constraints, title )
	{
		wxArrayString ch;
		wxListBox::Create( m_dlg, wxID_ANY, pos, size, ch, style|wxCLIP_CHILDREN );

		m_dlg->Bind( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, &CEdPoppedUp< wxListBox >::OnListClick, this );
	}

protected:
	void OnListClick( wxCommandEvent& event )
	{
		EndModal( wxID_OK );
	}
};
