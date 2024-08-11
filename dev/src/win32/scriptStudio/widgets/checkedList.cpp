/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "checkedList.h"

#include "../app.h"

wxIMPLEMENT_CLASS( CSSCheckListCtrl, wxListCtrl );

CSSCheckListCtrl::CSSCheckListCtrl( wxWindow* parent, int flags )
:	wxListCtrl( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, flags | wxCLIP_CHILDREN )
{
	wxImageList* images = new wxImageList( CHECKBOX_ICON_SIZE, CHECKBOX_ICON_SIZE, false, Checked_Max );

	wxBitmap on = wxTheSSApp->LoadBitmap( wxT( "IMG_CHECK_ON" ) );
	wxBitmap off = wxTheSSApp->LoadBitmap( wxT( "IMG_CHECK_OFF" ) );

	// This must match the order of the enumeration
	images->Add( on );
	images->Add( off );

	AssignImageList( images, wxIMAGE_LIST_SMALL );

	//The first column must contain the images
	InsertColumn( 0, wxEmptyString );
	SetColumnWidth( 0, CHECKBOX_ICON_SIZE );

	Bind( wxEVT_LEFT_DOWN, &CSSCheckListCtrl::OnLeftDown, this );
}

CSSCheckListCtrl::~CSSCheckListCtrl()
{

}

void CSSCheckListCtrl::OnLeftDown( wxMouseEvent& event )
{
	if ( event.LeftDown() )
	{
		int flags;
		long item = HitTest( event.GetPosition(), flags );

		if ( item > -1 && ( flags & wxLIST_HITTEST_ONITEMICON ) )
		{
			wxListItem info;
			info.SetId( item );
			info.SetMask( wxLIST_MASK_IMAGE );
			if( GetItem( info ) )
			{
				int imageIndex = info.GetImage();

				EChecked newState = static_cast< EChecked >( ( Checked_Max - 1 ) - imageIndex );

				SetItemImage( item, newState );

				OnStateChange( item, newState );
			}
		}
		else
		{
			event.Skip();
		}
	}
	else
	{
		event.Skip();
	}
}
