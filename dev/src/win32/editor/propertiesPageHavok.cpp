/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "propertiesPageHavok.h"

BEGIN_EVENT_TABLE( CEdHavokPropertiesPage, wxScrolledWindow )
END_EVENT_TABLE()
#ifdef USE_HAVOK_ANIMATION
CEdHavokPropertiesPage::CEdHavokPropertiesPage( wxWindow* parent )
	: wxScrolledWindow( parent )
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxSplitterWindow* splitter1 = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	wxPanel* panel1 = new wxPanel( splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	m_tree = new wxTreeCtrl( panel1, -1, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE );
	m_tree->Connect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( CEdHavokPropertiesPage::OnItemSelected ), NULL, this );
	bSizer2->Add( m_tree, 1, wxALL|wxEXPAND, 5 );

	panel1->SetSizer( bSizer2 );
	panel1->Layout();
	bSizer2->Fit( panel1 );

	wxPanel* panel2 = new wxPanel( splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_text = new wxTextCtrl( panel2, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE | wxTE_RICH | wxTE_DONTWRAP );
	bSizer3->Add( m_text, 1, wxALL|wxEXPAND, 5 );

	panel2->SetSizer( bSizer3 );
	panel2->Layout();
	bSizer3->Fit( panel2 );
	splitter1->SplitHorizontally( panel1, panel2, 0 );
	bSizer1->Add( splitter1, 1, wxEXPAND, 5 );

	SetSizer( bSizer1 );
	Layout();

	splitter1->SetSashPosition( 100 );

	SetNoObject();
}

void CEdHavokPropertiesPage::RefreshValues()
{
	m_tree->Freeze();
	m_tree->DeleteAllItems();

	m_items.clear();

	if ( m_objects.Size() > 0 )
	{
		wxTreeItemId treeRoot = m_tree->AddRoot( wxT("Objects") );

		for ( Uint32 i=0; i<m_objects.Size(); ++i )
		{
			const HavokDataBuffer* buff = m_objects[ i ];

			const hkVariant& root = m_parser.SetRoot( buff );

			FillTree( root, treeRoot );

			if ( i == 0 )
			{
				DisplayItemData( root );
			}
		}
	}
	else
	{
		m_tree->AddRoot( wxT("<Empty>") );
	}

	m_tree->Thaw();
}

void CEdHavokPropertiesPage::SetObject( const HavokDataBuffer* object )
{
	m_objects.Resize( 1 );
	m_objects[ 0 ] = object;
	m_parser.Clear();
	RefreshValues();
}

void CEdHavokPropertiesPage::SetObject( TDynArray< const HavokDataBuffer* >& objects )
{
	m_objects = objects;
	m_parser.Clear();
	RefreshValues();
}

void CEdHavokPropertiesPage::SetNoObject()
{
	m_objects.Clear();
	m_parser.Clear();
	RefreshValues();
}

void CEdHavokPropertiesPage::FillTree( const hkVariant& hkRoot, wxTreeItemId& treeRoot )
{
	hkString name;
	hkArray< hkVariant > ch;
	m_parser.GetItemData( hkRoot, name, ch );

	wxString nameStr = ANSI_TO_UNICODE( name.cString() );

	m_items.pushBack( hkRoot );
	wxTreeItemId newItem = m_tree->AppendItem( treeRoot, nameStr, -1, -1, new TreeItem( m_items.getSize() - 1 ) );

	for ( int i=0; i<ch.getSize(); ++i )
	{
		const hkVariant& child = ch[ i ];

		FillTree( child, newItem );
	}
}

struct ViewXmlNameFromAddress : public hkXmlObjectWriter::NameFromAddress
{
	/*virtual*/ int nameFromAddress( const void* addr, char* buf, int bufSize )
	{
		return hkString::snprintf(buf, bufSize, "0x%x", addr);
	}
};

void CEdHavokPropertiesPage::DisplayItemData( const hkVariant& var )
{
	hkString str;
	str.printf("<!-- %s @ 0x%x -->\n", var.m_class->getName(), var.m_object );

	hkArray<char> buf;
	hkArrayStreamWriter asw(&buf, hkArrayStreamWriter::ARRAY_BORROW);
	ViewXmlNameFromAddress namer;
	hkXmlObjectWriter xml(namer);
	xml.writeObjectWithElement(&asw, var.m_object, *(var.m_class), HK_NULL);
	str += buf.begin();

	wxString dataStr = ANSI_TO_UNICODE( str.cString() );
	m_text->SetLabel( dataStr );
}

void CEdHavokPropertiesPage::OnItemSelected( wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();

	TreeItem* data = static_cast< TreeItem* >( m_tree->GetItemData( item ) );

	if ( data )
	{
		if ( m_items.getSize() > data->m_num )
		{
			DisplayItemData( m_items[ data->m_num ] );
		}
		else
		{
			ASSERT( m_items.getSize() > data->m_num );
		}
	}
	else
	{
		m_text->SetLabel( wxT("") );
	}
}
#endif