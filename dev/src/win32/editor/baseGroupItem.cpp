/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/xmlFile.h"

CBaseGroupItem::CBaseGroupItem( CEdPropertiesPage* page, CBasePropItem* parent )
	: CBasePropItem( page, parent )
{
	m_isExpandable = true;
}

Int32 CBaseGroupItem::GetHeight() const
{
	return 18;
}

Int32 CBaseGroupItem::GetLocalIndent() const
{
	return 0;
}

void CBaseGroupItem::DrawLayout( wxDC& dc )
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

	if ( m_parent )
	{
		wxRect titleRect = m_rect;
		titleRect.height = GetHeight();

		// Draw header background
		{
			const wxColour back( 192, 192, 192 );
			dc.SetPen( wxPen( back ) );
			dc.SetBrush( wxBrush( back ) );
			dc.DrawRectangle( titleRect );
		}

		// Draw header text
		{
			// Calculate placement
			String caption = GetCaption();
			wxSize extents = dc.GetTextExtent( caption.AsChar() );
			const INT yCenter = ( GetHeight() - extents.y ) / 2;

			// Draw caption text
			const wxColour text( 128, 128, 128 );
			dc.SetFont( m_page->GetStyle().m_boldFont );
			dc.SetTextForeground( text );
			dc.DrawText( caption.AsChar(), m_rect.x + 15 + m_parent->GetLocalIndent(), m_rect.y + yCenter );
			dc.SetFont( m_page->GetStyle().m_drawFont );
		}
	}

	// Draw children
	DrawChildren( dc );

	// Draw item icons
	if ( m_parent )
	{
		DrawIcons( dc );
	}
}

void CBaseGroupItem::UpdateLayout( Int32& yOffset, Int32 x, Int32 width )
{
	m_rect.x = x;
	m_rect.y = yOffset;
	m_rect.width = width;

	// Top and left margin/indent
	yOffset += GetHeight();
	x += GetHeight() - 5;
	width -= GetHeight() - 5;

	for ( Uint32 i=0; i<m_children.Size(); i++ )
	{
		m_children[i]->UpdateLayout( yOffset, x, width );
	}

	m_rect.height = yOffset - m_rect.y;
}

Bool CBaseGroupItem::SerializeXML( IXMLFile& file )
{
    if ( !file.BeginNode( TXT("group") ) )
    {
        return false;
    }

    String name = GetCaption();
    if ( !file.Attribute( TXT("name"), name ) )
    {
        return false;
    }

    if ( name != GetCaption() )
    {
        return false;
    }
    
    const TDynArray< CBasePropItem * > &children = GetChildren();
    for ( Uint32 i = 0; i < children.Size(); ++i )
    {
        if ( !children[i]->SerializeXML( file ) )
        {
            return false;
        }
    }

    file.EndNode();
    return true;
}
