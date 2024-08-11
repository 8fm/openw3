/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "propertyItemArray.h"
#include "propertyItemClass.h"
#include "propertyItemEnum.h"
#include "propertyItemPointer.h"
#include "propertyItemSoftHandle.h"
#include "propertyItemBitfield.h"
#include "propertyItemEngineTransform.h"
#include "propertyItemEngineQsTransform.h"
#include "../../common/core/engineQsTransform.h"
#include "../../common/engine/environmentComponentArea.h"
#include "../../common/engine/materialInstance.h"
#include "entityTagGroupItem.h"

CBasePropItem::CBasePropItem( CEdPropertiesPage* page, CBasePropItem* parent )
	: m_page( page )
	, m_parent( parent )
	, m_isExpandable( false )
	, m_buttonsWidth( 0 )
{
	if ( m_parent )
	{
		m_parent->m_children.PushBack( this );
	}
}

CBasePropItem::~CBasePropItem()
{
    DestroyButtons();
	DestroyChildren();
}

void CBasePropItem::OnObjectDiscarded( CObject* object )
{
	for ( Uint32 i=0; i<m_children.Size(); i++ )
	{
		m_children[i]->OnObjectDiscarded( object );
	}
}

void CBasePropItem::DestroyButtons()
{
	for ( Uint32 i=0; i<m_buttons.Size(); i++ )
	{
        if ( m_page->GetActiveButton() == m_buttons[i] )
            m_page->ClearActiveButton();
		delete m_buttons[i];
	}

	m_buttons.Clear();
}

void CBasePropItem::DestroyChildren()
{
	for ( Uint32 i=0; i<m_children.Size(); i++ )
	{
		m_page->DiscardItem( m_children[i] );
	}

	m_children.Clear();
}

void CBasePropItem::DrawButtons( wxDC& dc )
{
	for ( Uint32 i=0; i<m_buttons.Size(); i++ )
	{
		m_buttons[i]->DrawLayout( dc );
	}
}

void CBasePropItem::DrawChildren( wxDC& dc )
{
	for ( Uint32 i=0; i<m_children.Size(); i++ )
	{
		m_children[i]->DrawLayout( dc );
	}
}

void CBasePropItem::DrawIcons( wxDC& dc )
{
	if ( m_isExpandable )
	{
		const wxRect rect = CalcTreeIconRect();

		// Draw icon
		if ( IsExpanded() )
		{
			dc.DrawBitmap( m_page->GetStyle().m_iconTreeCollapse, rect.x, rect.y );
		}
		else
		{
			dc.DrawBitmap( m_page->GetStyle().m_iconTreeExpand, rect.x, rect.y );
		}
	}
}

CBasePropItem* CBasePropItem::GetItemAtPoint( wxPoint point )
{
	// Test children
	for ( Uint32 i=0; i<m_children.Size(); i++ )
	{
		CBasePropItem* item = m_children[i]->GetItemAtPoint( point );
		if ( item )
		{
			return item;
		}
	}

	// Test this item
	if ( m_rect.Contains( point ) )
	{
		return this;
	}

	// No collision
	return NULL;
}

CPropertyButton* CBasePropItem::GetButtonAtPoint( wxPoint point )
{
	// Test buttons
	for ( Uint32 i=0; i<m_buttons.Size(); i++ )
	{
		if ( m_buttons[i]->GetRect().Contains( point ) )
		{
			return m_buttons[i];
		}
	}

	return NULL;
}

Bool CBasePropItem::IsSelected() const
{
	return m_page->GetActiveItem() == this;
}

Bool CBasePropItem::IsExpanded() const
{
	return m_children.Size() > 0;
}

void CBasePropItem::Linearize( TDynArray< CBasePropItem* >& items )
{
	items.PushBack( this );

	for ( Uint32 i=0; i<m_children.Size(); i++ )
	{
		m_children[i]->Linearize( items );
	}
}

Int32 CBasePropItem::GetIndent() const
{
	return 0;
}

Int32 CBasePropItem::GetHeight() const
{
	return 0;
}

Int32 CBasePropItem::GetLocalIndent() const
{
	assert( m_parent );
	return m_parent->GetLocalIndent();
}

void CBasePropItem::UpdateButtonsLayout()
{
	// Move buttons
	Int32 xOffset = m_rect.x + m_rect.width;
	for ( Uint32 i=0; i<m_buttons.Size(); i++ )
	{
		m_buttons[i]->UpdateLayout( xOffset, m_rect.y, m_rect.height );			
	}

	// Calculate width of button area
	m_buttonsWidth = m_rect.x + m_rect.width - xOffset;
}

void CBasePropItem::UpdateLayout( Int32& yOffset, Int32 x, Int32 width )
{
	m_rect.x = x;
	m_rect.y = yOffset;
	m_rect.width = width;
	m_rect.height = GetHeight();

	// Advance
	yOffset += m_rect.height + 1;
	x += GetIndent();
	width -= GetIndent();

	// Add child properties
	for ( Uint32 i=0; i<m_children.Size(); i++ )
	{
		m_children[i]->UpdateLayout( yOffset, x, width );
	}

	// Update button layout
	UpdateButtonsLayout();
}

void CBasePropItem::DrawLayout( wxDC& dc )
{
	// Indent
	{
		const wxColour backh( 192, 192, 192 );

		// Calculate title rect size
		wxRect titleRect = m_rect;
		titleRect.x = 0;
		titleRect.width = m_rect.x;
		titleRect.height += 1;

		// Draw colored bar
		dc.SetPen( wxPen( backh ) );
		dc.SetBrush( wxBrush( backh ) );
		dc.DrawRectangle( titleRect );
	}

	// Calculate title rect size
	wxRect titleRect = m_rect;
	titleRect.width = m_page->GetSplitterPos() - titleRect.x;

	// Get style and setup background, if provided
	const SEdPropertiesPagePropertyStyle* style = GetPropertyStyle();
	if ( style )
	{
		// Draw background if provided
		if ( style->m_backgroundColor.Alpha() > 0 )
		{
			dc.SetPen( wxPen( style->m_backgroundColor ) );
			dc.SetBrush( wxBrush( style->m_backgroundColor ) );
			dc.DrawRectangle( titleRect );
		}
	}

	// Selection background
	if ( IsSelected() )
	{
		const wxColour backh( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );

		// Draw colored bar
		dc.SetPen( wxPen( backh ) );
		dc.SetBrush( wxBrush( backh ) );
		dc.DrawRectangle( titleRect );
	}

	// Draw style icons
	if ( style && !style->m_icons.Empty() )
	{
		Int32 x = m_page->GetSplitterPos() - titleRect.x;
		for ( auto it=style->m_icons.Begin(); it != style->m_icons.End(); ++it )
		{
			x -= (*it).GetWidth();
		}
		for ( auto it=style->m_icons.Begin(); it != style->m_icons.End(); ++it )
		{
			dc.DrawBitmap( *it, x, titleRect.y + ( titleRect.height - (*it).GetHeight() )/2 );
			x += (*it).GetWidth();
		}
	}

	// Separation line
	{
		const wxColour separator( 192, 192, 192 );
		dc.SetPen( wxPen( separator ) );
		dc.DrawLine( m_rect.x, m_rect.y + m_rect.height, m_rect.x + m_rect.width, m_rect.y + m_rect.height );
	}

	// Draw splitter line
	{
		const wxColour separator( 192, 192, 192 );
		dc.SetPen( wxPen( separator ) );
		dc.DrawLine( m_page->GetSplitterPos(), m_rect.y, m_page->GetSplitterPos(), m_rect.y + m_rect.height );
	}

	// Draw property icons
	DrawIcons( dc );
}

void CBasePropItem::Expand()
{
	if ( IsSelected() )
	{
		m_page->SelectItem( this );
	}
	m_page->Refresh( false );
}

void CBasePropItem::Collapse()
{
	DestroyButtons();
	DestroyChildren();
	m_page->Refresh( false );
	if ( IsSelected() )
	{
		m_page->SelectItem( this );
	}
}

void CBasePropItem::ToggleExpand()
{
	if ( !m_children.Size() && m_isExpandable )
	{
		Expand();
	}
	else if ( m_children.Size() )
	{
		Collapse();
	}
}

void CBasePropItem::CreateControls()
{

}

void CBasePropItem::CloseControls()
{

}

void CBasePropItem::RecursiveCloseControls()
{
	CloseControls();
	for ( Uint32 i=0; i<m_children.Size(); i++ )
	{
		m_children[ i ]->RecursiveCloseControls();
	}
}

CPropertyButton* CBasePropItem::AddButton( const wxBitmap& bitmap, wxObjectEventFunction eventHandler, wxEvtHandler *eventSink /* = NULL */ )
{
	if ( !IsReadOnly() )
	{
		CPropertyButton* button = new CPropertyButton( this, bitmap, 17 );
		m_buttons.PushBack( button );
		button->Connect( wxEVT_COMMAND_BUTTON_CLICKED, eventHandler, NULL, eventSink ? eventSink : this );
		UpdateButtonsLayout();
		return button;
	}
	return nullptr;
};

Bool CBasePropItem::WriteProperty( CProperty *property, void* buffer, Int32 objectIndex )
{
	ASSERT( m_parent );
	return m_parent->WriteProperty( property, buffer, objectIndex );
}

Bool CBasePropItem::ReadProperty( CProperty *property, void* buffer, Int32 objectIndex )
{
	ASSERT( m_parent );
	return m_parent->ReadProperty( property, buffer, objectIndex );
}

Int32 CBasePropItem::GetNumObjects() const
{
	ASSERT( m_parent );
	return m_parent->GetNumObjects();
}

STypedObject CBasePropItem::GetRootObject( Int32 objectIndex ) const
{
	ASSERT( m_parent );
	return m_parent->GetRootObject( objectIndex );
}

STypedObject CBasePropItem::GetParentObject( Int32 objectIndex ) const
{
	ASSERT( m_parent );
	return m_parent->GetParentObject( objectIndex );
}

wxRect CBasePropItem::CalcTreeIconRect() const
{
	const INT size = 9;
	const Int32 indent = m_parent->GetLocalIndent();

	// Setup defaults
	wxRect checkRect;
	checkRect.x = m_rect.x + 3 + indent;
	checkRect.width = size;

	// Center image vertically
	INT space = ( GetHeight() - size ) / 2;
	checkRect.y = m_rect.y + space + 1;
	checkRect.height = size;
	return checkRect;
}


void CBasePropItem::OnBrowserMouseEvent( wxMouseEvent& event )
{
	// Double click, if expandable then expand, if expanded then collapse
	if ( event.LeftDClick() )
	{
		ToggleExpand();
		return;
	}

	// Left click on expand button
	if ( event.LeftDown() && CalcTreeIconRect().Contains( event.GetPosition() ) )
	{
		ToggleExpand();
	}

	if ( event.RightDown() && m_isExpandable )
	{
		wxMenu menu;

		menu.Append( 1, TXT( "Expand all" ) );
		menu.Connect( 1, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CBasePropItem::OnExpandAll ), NULL, this );

		menu.Append( 2, TXT( "Copy" ) );
		menu.Connect( 2, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdPropertiesPage::OnEditCopy ), NULL, m_page );

		// Only allow for copy-via-RTTI for non-CObjects (e.g. structs)
		// Note: copying some CObjects via CVariant is problematic, e.g. when copying CMaterialInstance (being a CResource) should it really copy *all* properties (including unique GUID) around or not? -> most likely not
		{
			STypedObject object = GetParentObject( 0 );
			if ( object.m_class && !object.m_class->IsObject() )
			{
				menu.Append( 3, TXT( "Copy via RTTI" ) );
				menu.Connect( 3, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdPropertiesPage::OnEditCopyViaRTTI ), NULL, m_page );
			}
		}

		menu.Append( 4, TXT( "Paste" ) );
		menu.Connect( 4, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdPropertiesPage::OnEditPaste ), NULL, m_page );

		wxTheFrame->PopupMenu( &menu );
	}
}

void CBasePropItem::OnBrowserKeyDown( wxKeyEvent& event )
{
	// Enter
	if ( event.GetKeyCode() == WXK_RETURN )
	{
		// Expand property
		if ( m_isExpandable )
		{
			ToggleExpand();
		}
	}
}

void CBasePropItem::OnExpandAll( wxCommandEvent& event )
{
	if( !IsExpanded() )
		Expand();
	for( TDynArray< CBasePropItem* >::iterator it=m_children.Begin(); it!=m_children.End(); it++ )
		( *it )->OnExpandAll( event );
}

Bool CBasePropItem::IsReadOnly() const
{
	ASSERT( m_parent );
	return m_parent->IsReadOnly();
}

Bool CBasePropItem::IsInlined() const
{
	ASSERT( m_parent );
	return m_parent->IsInlined();
}

String CBasePropItem::GetName() const
{
	ASSERT( m_parent );
	return m_parent->GetName();
}

String CBasePropItem::GetCustomEditorType() const
{
	ASSERT( m_parent );
	return m_parent->GetCustomEditorType();
}

const SEdPropertiesPagePropertyStyle* CBasePropItem::GetPropertyStyle() const
{
	return NULL;
}

Bool CBasePropItem::ReadImp( class CPropertyItem* childItem, void *buffer, Int32 objectIndex /*= 0*/ )
{
	ASSERT( m_parent );
	return m_parent->ReadImp( childItem, buffer, objectIndex );
}

Bool CBasePropItem::WriteImp( class CPropertyItem* childItem, void *buffer, Int32 objectIndex /*= 0*/ )
{
	ASSERT( m_parent );
	return m_parent->WriteImp( childItem, buffer, objectIndex );
}

CBaseGroupItem* CBasePropItem::CreateGroupItem( CEdPropertiesPage *page, CBasePropItem *parent, CClass *classItem )
{
	if ( classItem->IsA< CEntity >() )
	{
		// tags
		if ( classItem == ClassID< CEntity >() )
		{
			new CEntityTagGroupItem( m_page, this, classItem );
		}

		// normal props
		return new CClassGroupItem( page, parent, classItem );
	}
	else if ( classItem->IsA< CMaterialInstance >() )
	{
		if ( GetNumObjects() == 1 )
		{
			CMaterialInstance *instance = classItem->CastTo< CMaterialInstance >( GetParentObject( 0 ).m_object );
			return new CMaterialInstanceGroupItem ( m_page, this, instance );
		}

		return new CClassGroupItem( page, parent, classItem );
	}
	else if ( classItem->IsA< IMaterial >() )
	{
		if ( GetNumObjects() == 1 )
		{
			IMaterial *material = classItem->CastTo< IMaterial >( GetParentObject( 0 ).m_object );
			new CMaterialGroupItem( m_page, this, material );
			return new CClassGroupItem( page, parent, classItem );
		}		

		return new CClassGroupItem( page, parent, classItem );
	}
	else if ( classItem->IsA< CComponent >() )
	{
		if ( !classItem->IsA< CAreaEnvironmentComponent >() )	// ace_hack
		{
			TDynArray< CComponent* > components;
			for ( Int32 i = 0; i < GetNumObjects(); ++i )
			{
				CComponent *component = classItem->CastTo< CComponent >( GetParentObject( i ).m_object );
				components.PushBack(component);
			}
			CComponentGroupItem* cgi = new CComponentGroupItem( m_page, this, classItem );
			cgi->SetObjects(components);
			return cgi;
		}

		return new CClassGroupItem( page, parent, classItem );
	}
	else
	{
		return new CClassGroupItem( page, parent, classItem );
	}
}

CPropertyItem* CBasePropItem::InternalCreatePropertyItem( CEdPropertiesPage *page, CBasePropItem *parent, IRTTIType* type )
{
	ERTTITypeType typeType = type->GetType();
	CPropertyItem* newPropertyItem = nullptr;

	// Special simple types
	if ( typeType == RT_Simple && type->GetName() == GetTypeName< EngineTransform >() )
	{
		newPropertyItem = new CPropertyItemEngineTransform( page, parent );
	}
	else if ( typeType == RT_Simple && type->GetName() == GetTypeName< EngineQsTransform >() )
	{
		newPropertyItem = new CPropertyItemEngineQsTransform( page, parent );
	}
	else
	{
		switch( typeType )
		{
		case RT_Simple:
		case RT_Fundamental:
			newPropertyItem = new CPropertyItem( page, parent );
			break;
		case RT_Enum:
			newPropertyItem = new CPropertyItemEnum( page, parent );
			break;
		case RT_BitField:
			newPropertyItem = new CPropertyItemBitField( page, parent );
			break;
		case RT_Class:
			newPropertyItem = new CPropertyItemClass( page, parent );
			break;
		case RT_Array: 
		case RT_StaticArray: 
		case RT_NativeArray: 
			newPropertyItem = new CPropertyItemArray( page, parent );
			break;
		case RT_Handle:
		case RT_Pointer:
			newPropertyItem = new CPropertyItemPointer( page, parent );
			break;
		case RT_SoftHandle:
			newPropertyItem = new CPropertyItemSoftHandle( page, parent );
			break;
		}
	}

	ASSERT( newPropertyItem, TXT("Unsupported property type") );
	return newPropertyItem;
}


CPropertyItem* CBasePropItem::CreatePropertyItem( CEdPropertiesPage *page, CBasePropItem *parent, CProperty* property )
{
	CPropertyItem* newPropertyItem = InternalCreatePropertyItem( page, parent, property->GetType() );
	newPropertyItem->Init( property );
	return newPropertyItem;
}

CPropertyItem* CBasePropItem::CreatePropertyItem( CEdPropertiesPage *page, CBasePropItem *parent, IRTTIType* type, Int32 arrayIndex /*= -1*/ )
{
	CPropertyItem* newPropertyItem = InternalCreatePropertyItem( page, parent, type );
	newPropertyItem->Init( type, arrayIndex );
	return newPropertyItem;
}

Bool CBasePropItem::SerializeXML( IXMLFile& file )
{
    ASSERT( !TXT("Unable to serialize BasePropItem object.") );
    return false;
}

Uint32 CBasePropItem::GetChildCount( Bool onlyExpanded ) const
{
	Uint32 count = m_children.Size();
	for( Uint32 i = 0; i < m_children.Size(); ++i )
	{
		if( !onlyExpanded || m_children[ i ]->IsExpanded() )
		{
			count += m_children[ i ]->GetChildCount( onlyExpanded );
		}
	}

	return count;
}
