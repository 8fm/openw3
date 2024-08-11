/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include <wx/listctrl.h>

wxDEFINE_EVENT( wxEVT_CHOICE_CHANGED, CChoiceChangedEvent );

// CEdChoiceComboPopup
class CEdChoiceComboPopup : public wxListView, public wxComboPopup
{
	friend class CEdChoice;
	CEdChoice* choice;

	void SetValue( int n, bool updateText = true )
	{
		m_value = n;
		if ( updateText )
		{
			choice->SetText( n > -1 ? GetItemText( n ) : wxEmptyString );
		}
	}

public:
	virtual void Init()
	{
		m_value = -1;
	}

	virtual bool Create( wxWindow* parent )
	{
		return wxListView::Create( parent, 1, wxPoint(0, 0), wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_NO_HEADER);
	}

	virtual wxWindow* GetControl()
	{
		return this;
	}

	virtual void Select( long n, bool updateText = true, bool on = true )
	{
		if ( n == -1 )
		{
			int currentSelection = GetFirstSelected();
			if ( currentSelection != -1 )
			{
				wxListView::Select( currentSelection, false );
			}
			SetStringValue( wxEmptyString );
		}
		else
		{
			wxListView::Select( n, on );
			SetValue( GetFirstSelected(), updateText );
			wxListView::EnsureVisible(n);
		}
	}

	virtual void SetStringValue( const wxString& s )
	{
		int n = wxListView::FindItem( -1, s );
		if ( n >= 0 && n < wxListView::GetItemCount() )
		{
			Select( n );
		}
	}

	virtual wxString GetStringValue() const
	{
		return m_value >= 0 ? wxListView::GetItemText( m_value ) : wxEmptyString;
	}

	virtual wxSize GetAdjustedSize( int minWidth, int prefHeight, int maxHeight )
	{
		wxSize size;
		int notExtras = 8;

		SetColumnWidth( 0, wxLIST_AUTOSIZE );

		for ( int i=0; i<GetItemCount(); i++ )
		{
			wxRect rect;
			GetItemRect( i, rect );
			if ( size.GetWidth() < rect.GetWidth() )
				size.SetWidth( rect.GetWidth() );
			size.SetHeight( size.GetHeight() + rect.GetHeight() );
		}

		size.SetWidth( size.GetWidth() + 8 );
		size.SetHeight( size.GetHeight() + 4 );

		if ( size.GetHeight() > maxHeight ) {
			size.SetHeight( maxHeight );
			size.SetWidth( size.GetWidth() + wxSystemSettings::GetMetric( wxSYS_VSCROLL_X ) );
			notExtras += wxSystemSettings::GetMetric( wxSYS_VSCROLL_X );
		}
		if ( size.GetWidth() < minWidth )
			size.SetWidth( minWidth );

		SetColumnWidth( 0, size.GetWidth() - notExtras );

		return size;
	}

	virtual void OnPopup()
	{
		wxCommandEvent cmd( wxEVT_COMMAND_COMBOBOX_DROPDOWN );
		cmd.SetEventObject( choice );
		choice->ProcessCommand( cmd );
	}

	virtual void OnDismiss()
	{
		wxCommandEvent cmd( wxEVT_COMMAND_COMBOBOX_CLOSEUP );
		cmd.SetEventObject( choice );
		choice->ProcessCommand( cmd );

		CChoiceChangedEvent* choiceChangedEvent = new CChoiceChangedEvent( wxEVT_CHOICE_CHANGED, GetId() );
		choiceChangedEvent->SetEventObject( choice );
		QueueEvent( choiceChangedEvent );
	}

	void OnMouseMove( wxMouseEvent& event )
	{
		wxPoint pos = ScreenToClient( wxGetMousePosition() );
		for ( int i=0; i<GetItemCount(); i++ )
		{
			wxRect rect;
			if ( GetItemRect( i, rect ) )
			{
				if ( rect.Contains( pos ) )
				{
					Select( i, false );
					break;
				}
			}
		}
	}

	void OnMouseClick( wxMouseEvent& event )
	{
		int item = wxListView::GetFirstSelected();
		SetValue( item );

		/* Consume the "left button up" event so that it won't reach any
		 * other window when the popup closes and accidentally invoke
		 * unexpected functionality. */
		{
			MSG msg;
			GetMessage( &msg, 0, WM_LBUTTONUP, WM_LBUTTONUP );
		}

		wxCommandEvent cmdChoice( wxEVT_COMMAND_CHOICE_SELECTED );
		cmdChoice.SetEventObject( choice );
		cmdChoice.SetInt( item );
		cmdChoice.SetString( item > -1 ? GetItemText( item ) : wxEmptyString );
		choice->ProcessCommand( cmdChoice );

		wxCommandEvent cmdCB( wxEVT_COMMAND_COMBOBOX_SELECTED );
		cmdCB.SetEventObject( choice );
		cmdCB.SetInt( item );
		cmdCB.SetString( item > -1 ? GetItemText( item ) : wxEmptyString );
		choice->ProcessCommand( cmdCB );

		Dismiss();
	}

protected:
	int			m_value;

private:
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CEdChoiceComboPopup, wxListView)
    EVT_MOTION(CEdChoiceComboPopup::OnMouseMove)
	EVT_LEFT_DOWN(CEdChoiceComboPopup::OnMouseClick)
END_EVENT_TABLE()


// CEdChoice
CEdChoice::CEdChoice( wxWindow* parent, const wxPoint& pos, const wxSize& size, bool editableText, long style )
	: wxComboCtrl( parent, wxID_ANY, wxEmptyString, pos, size, style | wxCC_STD_BUTTON )
	, m_editableText( editableText )
{
	m_comboPopup = new CEdChoiceComboPopup();
	((CEdChoiceComboPopup*)m_comboPopup)->choice = this;
	GetTextCtrl()->SetEditable( editableText );
	if ( !editableText )
	{
		GetTextCtrl()->SetBackgroundColour( GetBackgroundColour() );
	}
	SetPopupControl( m_comboPopup );
	Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CEdChoice::OnKeyDown ), NULL, this );
	//Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( CEdChoice::OnMouseWheel ), NULL, this );
}

CEdChoice::~CEdChoice()
{
}

int CEdChoice::DoInsertItems(const wxArrayStringsAdapter & items,
                             unsigned int pos,
                             void **clientData,
                             wxClientDataType type)
{
	CEdChoiceComboPopup* popup = (CEdChoiceComboPopup*)m_comboPopup;
	int lastIndex = wxNOT_FOUND;

	popup->Freeze();
	if ( popup->GetColumnCount() < 1 )
	{
		popup->InsertColumn( 0, TXT(""), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE );
	}

	for ( unsigned int i = 0; i < items.GetCount(); i++ )
	{
		wxListItem item;
		item.SetId( (long)( i + pos ) );
		item.SetColumn( 0 );
		item.SetText( items[i] );
		if ( clientData )
		{
			item.SetData( clientData[ i ] );
		}
		lastIndex = (int)popup->InsertItem( item );
	}
	SetClientDataType( type );
	popup->Thaw();
	
	return lastIndex;
}

void CEdChoice::DoSetItemClientData(unsigned int n, void *clientData)
{
	CEdChoiceComboPopup* popup = (CEdChoiceComboPopup*)m_comboPopup;

	if ( n >= 0 && (signed int)n < popup->GetItemCount() )
	{
		popup->SetItemPtrData( n, (wxUIntPtr)clientData );
	}
}

void *CEdChoice::DoGetItemClientData(unsigned int n) const
{
	CEdChoiceComboPopup* popup = (CEdChoiceComboPopup*)m_comboPopup;

	if ( n >= 0 && (signed int)n < popup->GetItemCount() )
	{
		return (void*)popup->GetItemData( n );
	}

	return NULL;
}

void CEdChoice::DoClear()
{
	((CEdChoiceComboPopup*)m_comboPopup)->ClearAll();
}

void CEdChoice::DoDeleteOneItem(unsigned int pos)
{
	((CEdChoiceComboPopup*)m_comboPopup)->DeleteItem( pos );
}

bool CEdChoice::SetBackgroundColour( const wxColour& colour )
{
	return wxComboCtrl::SetBackgroundColour( colour );
}

unsigned int CEdChoice::GetCount() const
{
	return (unsigned int)((CEdChoiceComboPopup*)m_popup)->GetItemCount();
}

void CEdChoice::SetSelection(int n)
{
	((CEdChoiceComboPopup*)m_popup)->Select( n );

	// Send events not being sent by wx2.9 anymore
	n = GetSelection();
	wxString str = n == -1 ? wxEmptyString : GetString( n );

	// wxEVT_COMMAND_CHOICE_SELECTED
	{
		wxCommandEvent cmdCB( wxEVT_COMMAND_CHOICE_SELECTED );
		cmdCB.SetEventObject( this );
		cmdCB.SetInt( n );
		cmdCB.SetString( n > -1 ? str : wxEmptyString );
		ProcessCommand( cmdCB );
	}

	// wxEVT_COMMAND_COMBOBOX_SELECTED
	{
		wxCommandEvent cmdCB( wxEVT_COMMAND_COMBOBOX_SELECTED );
		cmdCB.SetEventObject( this );
		cmdCB.SetInt( n );
		cmdCB.SetString( n > -1 ? str : wxEmptyString );
		ProcessCommand( cmdCB );
	}
}

int CEdChoice::GetSelection() const
{
	return (int)((CEdChoiceComboPopup*)m_popup)->GetFirstSelected();
}

wxString CEdChoice::GetString(unsigned int n) const
{
	return ((CEdChoiceComboPopup*)m_popup)->GetItemText( (long)n );
}

void CEdChoice::SetString(unsigned int n, const wxString& s)
{
	((CEdChoiceComboPopup*)m_popup)->SetItemText( (long)n, s );
}

wxClientData *CEdChoice::GetClientObject( unsigned int n ) const
{
	return wxItemContainer::GetClientObject( n );
}

void CEdChoice::SetClientObject( unsigned int n, wxClientData* data )
{
	wxItemContainer::SetClientObject( n, data );
}

void CEdChoice::SetClientDataType( wxClientDataType clientDataItemsType )
{
	wxItemContainer::SetClientDataType( clientDataItemsType );
}

wxClientDataType CEdChoice::GetClientDataType() const
{
	return wxItemContainer::GetClientDataType();
}

void CEdChoice::OnKeyDown( wxKeyEvent& event )
{
	int code = event.GetKeyCode();
	switch ( code )
	{
	case WXK_UP:
		if ( GetSelection() > 0 )
		{
			SetSelection( GetSelection() - 1 );
		}
		break;
	case WXK_DOWN:
		if ( GetSelection() < static_cast<int>( GetCount() ) - 1 )
		{
			SetSelection( GetSelection() + 1 );
		}
		break;
	default:
		if ( !m_editableText && ( code >= 32 ) )
		{
			wxString codeStr = wxString::Format( wxT("%c"), (wxChar)code );
			long index = ((wxListCtrl*)m_popup)->FindItem( GetSelection() + 1, codeStr, true );
			if ( index == -1 )
			{
				index = ((wxListCtrl*)m_popup)->FindItem( 0, codeStr, true );
			}
			if ( index != -1 )
			{
				SetSelection( index );
			}
		}

		event.Skip();
	}
}

void CEdChoice::OnMouseWheel( wxMouseEvent& event )
{
	if ( event.GetWheelRotation() > 0 && GetSelection() < static_cast<int>( GetCount() ) - 1 )
	{
		SetSelection( GetSelection() + 1 );
	}
	else if ( event.GetWheelRotation() < 0 && GetSelection() > 0 )
	{
		SetSelection( GetSelection() - 1 );
	}
	else
	{
		event.Skip();
	}
}
