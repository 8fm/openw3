/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiGridLayout.h"

namespace RedGui
{
	CRedGuiGridLayout::CRedGuiGridLayout(Uint32 x, Uint32 y, Uint32 width, Uint32 height)
		: CRedGuiControl(x, y, width, height)
		, m_dimensions( 2.0f, 2.0f )
	{ 
	}

	CRedGuiGridLayout::~CRedGuiGridLayout()
	{
		/*intentionally empty*/
	}

	void CRedGuiGridLayout::SetDimensions( Int32 columnCount, Int32 rowCount )
	{
		m_dimensions.Set( (Float)columnCount, (Float)rowCount );
	}

	Vector2 CRedGuiGridLayout::GetDimensions() const
	{
		return m_dimensions;
	}

	void CRedGuiGridLayout::UpdateControl()
	{
		CRedGuiControl::UpdateControl();

		const Uint32 cellWidth = (Uint32)( GetWidth() / m_dimensions.X );
		const Uint32 cellHeight = (Uint32)( GetHeight() / m_dimensions.Y );
		const Uint32 cellCount = (Uint32)( m_dimensions.X * m_dimensions.Y );

		const Uint32 childCount = GetChildCount();
		for( Uint32 i=0; i<childCount && i<cellCount; ++i )
		{
			CRedGuiControl* child = GetChildAt( i );
			child->SetAlign( IA_None );
			child->SetAnchor( ANCHOR_None );
			child->SetDock( DOCK_None );

			Uint32 y = ( i/(Uint32)m_dimensions.X );
			Uint32 x = i - ( y*( (Uint32)m_dimensions.X ) );

			child->SetPosition( Vector2( (Float)( x*cellWidth + child->GetMargin().Min.X ), (Float)( y*cellHeight + child->GetMargin().Min.Y ) ) );
			Uint32 horizontalMargin = (Uint32)( child->GetMargin().Min.X + child->GetMargin().Max.X );
			Uint32 verticalMargin = (Uint32)( child->GetMargin().Min.Y + child->GetMargin().Max.Y );
			child->SetSize( Vector2( (Float)cellWidth -horizontalMargin, (Float)cellHeight - verticalMargin ) );
		}

		if( childCount > cellCount )
		{
			Uint32 start = childCount - cellCount;
			for( Uint32 i=start; i<childCount; ++i )
			{
				CRedGuiControl* child = GetChildAt( i );
				child->SetVisible( false );
			}
		}
	}

	void CRedGuiGridLayout::Draw()
	{
		/* intentionally empty */
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
