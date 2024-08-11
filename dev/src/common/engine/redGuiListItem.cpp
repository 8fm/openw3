/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiListItem.h"

namespace RedGui
{
	CRedGuiListItem::CRedGuiListItem( const String& text/* = String::EMPTY*/, RedGuiAny userData /*= nullptr*/, 
		const Color& textColor /*= Color::WHITE*/, const Color& backgroundColor /*= Color( 45,45,45,255 )*/ )
		: CRedGuiUserData( userData )
		, m_backgroundColor( backgroundColor )
		, m_textColor( textColor )
		, m_selected( false )
	{
		m_texts.PushBack( text );
	}	

	CRedGuiListItem::~CRedGuiListItem()
	{
		/* intentionally empty */
	}

	const String& CRedGuiListItem::GetText( Uint32 column /*= 0 */ ) const
	{
		return ( column < m_texts.Size() ) ? m_texts[column] : String::EMPTY;
	}

	void CRedGuiListItem::SetText( const String& text, Uint32 column /*= 0 */ )
	{
		if( column < m_texts.Size() )
		{
			m_texts[column] = text;
		}
		else
		{
			m_texts.PushBack( text );
		}

		EventTextChanged( NULL, column );
	}

	Color CRedGuiListItem::GetBackgroundColor() const
	{
		return m_backgroundColor;
	}

	void CRedGuiListItem::SetBackgroundColor( const Color& color )
	{
		m_backgroundColor = color;
	}

	const Color& CRedGuiListItem::GetTextColor() const
	{
		return m_textColor;
	}

	void CRedGuiListItem::SetTextColor( const Color& color )
	{
		m_textColor = color;
	}

	Bool CRedGuiListItem::GetSelected() const
	{
		return m_selected;
	}

	void CRedGuiListItem::SetSelected( Bool value )
	{
		m_selected = value;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
