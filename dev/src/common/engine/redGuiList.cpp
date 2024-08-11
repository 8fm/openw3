/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiManager.h"
#include "redguiScrollBar.h"
#include "redGuiButton.h"
#include "redGuiPanel.h"
#include "redGuiListItem.h"
#include "redGuiList.h"
#include "fonts.h"

namespace RedGui
{
	namespace
	{
		const Uint32 GDefaultListItemHeight = 18;
		const Uint32 GScrollPageSize = GDefaultListItemHeight;
		const Uint32 GScrollWheelValue = 50;
		const Color GHighlight( 255, 170, 0, 100 );
		const Color GPushed( 255, 150, 0, 100 );
		const Color GNormal( 45, 45, 45,255 );
		const Color GBorder( 112, 112, 112, 255 );
		const Color GLocked( 255, 0, 0, 255 );
	}

	CRedGuiList::CRedGuiList( Uint32 x, Uint32 y, Uint32 width, Uint32 height )
		: CRedGuiControl( x, y, width, height )
		, m_croppClient( nullptr )
		, m_horizontalBar( nullptr )
		, m_verticalBar( nullptr )
		, m_pushedIndex( -1 )
		, m_highlightIndex( -1 )
		, m_firstSelectedIndex( -1 )
		, m_textAlign( IA_MiddleCenter )
		, m_mode( SM_Single )
		, m_sorting( false )
		, m_sortMode( ST_Descending )
		, m_sortedColumn( -1 )
		, m_dirty( false )
		, m_firstVisibleItem( -1 )
		, m_lastVisibleItem( -1 )
		, m_horizontalRange( 0 )
		, m_verticalRange( 0 )
		, m_maxItemWidth( 0 )
		, m_maxVerticalItemCount( 0 )
	{
		SetNeedKeyFocus(true);

		// create panel for columns labels
		m_colLabelsPanel = new CRedGuiPanel( 0, 0, width, GDefaultListItemHeight );
		m_colLabelsPanel->SetBackgroundColor( Color::CLEAR );
		m_colLabelsPanel->SetDock( DOCK_Top );
		AddChild( m_colLabelsPanel );

		// create scrollbars
		m_horizontalBar = new CRedGuiScrollBar( 0, height, width, 20, false );
		m_horizontalBar->EventScrollChangePosition.Bind( this, &CRedGuiList::NotifyScrollChangePosition );
		m_horizontalBar->SetMinTrackSize( 20 );
		m_horizontalBar->SetDock( DOCK_Bottom );
		m_horizontalBar->SetVisible( false );
		AddChild( m_horizontalBar );

		m_verticalBar = new CRedGuiScrollBar( width, 0, 20, height );
		m_verticalBar->EventScrollChangePosition.Bind( this, &CRedGuiList::NotifyScrollChangePosition );
		m_verticalBar->SetMinTrackSize( 20 );
		m_verticalBar->SetDock( DOCK_Right );
		m_verticalBar->SetVisible( false );
		AddChild( m_verticalBar );

		// create cropped client
		m_croppClient = new CRedGuiPanel( 0, 0, width, height );
		m_croppClient->SetBorderVisible( false );
		m_croppClient->SetBackgroundColor( Color::CLEAR );
		m_croppClient->SetForegroundColor( Color::CLEAR );
		m_croppClient->EventMouseWheel.Bind( this, &CRedGuiList::NotifyMouseWheel );
		m_croppClient->EventMouseButtonDoubleClick.Bind( this, &CRedGuiList::NotifyMouseButtonDoubleClick );
		m_croppClient->EventMouseMove.Bind( this, &CRedGuiList::NotifyMouseMove );
		m_croppClient->EventMouseLostFocus.Bind( this, &CRedGuiList::NotifyClientMouseLostFocus );
		m_croppClient->EventSizeChanged.Bind( this, &CRedGuiList::NotifyClientSizeChanged );
		m_croppClient->EventMouseButtonPressed.Bind( this, &CRedGuiList::NotifyClientMouseButtonPressed );
		m_croppClient->EventMouseButtonReleased.Bind( this, &CRedGuiList::NotifyClientMouseButtonReleased );
		m_croppClient->SetDock(DOCK_Fill);
		AddChild( m_croppClient );

		m_maxItemWidth = (Uint32)( m_croppClient->GetSize().X );
		m_maxVerticalItemCount = (Uint32)( m_croppClient->GetHeight() / GDefaultListItemHeight );
		m_firstItemPosition = Vector2( 0.0f, 0.0f );
	}

	CRedGuiList::~CRedGuiList()
	{
		
	}

	void CRedGuiList::OnPendingDestruction()
	{
		m_horizontalBar->EventScrollChangePosition.Unbind( this, &CRedGuiList::NotifyScrollChangePosition );
		m_verticalBar->EventScrollChangePosition.Unbind( this, &CRedGuiList::NotifyScrollChangePosition );
		m_croppClient->EventMouseWheel.Unbind( this, &CRedGuiList::NotifyMouseWheel );
		m_croppClient->EventMouseButtonDoubleClick.Unbind( this, &CRedGuiList::NotifyMouseButtonDoubleClick );
		m_croppClient->EventMouseMove.Unbind( this, &CRedGuiList::NotifyMouseMove );
		m_croppClient->EventMouseLostFocus.Unbind( this, &CRedGuiList::NotifyClientMouseLostFocus );
		m_croppClient->EventSizeChanged.Unbind( this, &CRedGuiList::NotifyClientSizeChanged );

		for( Uint32 index = 0, end = m_rows.Size(); index != end; ++index )
		{
			m_rows[ index ]->EventTextChanged.Unbind( this, &CRedGuiList::NotifyItemTextChanged );
		}

		m_lockedItems.ClearFast();
		m_displayOrder.ClearFast();
		m_rows.ClearPtrFast();
	}

	Uint32 CRedGuiList::AddItem( CRedGuiListItem* item )
	{
		Uint32 index = m_rows.Size();
		m_rows.PushBack( item );
		m_displayOrder.PushBack( index );
		RecalculateMaxWidth( item->GetText() );

		item->EventTextChanged.Bind( this, &CRedGuiList::NotifyItemTextChanged );

		UpdateView();
		m_dirty = true;

		return index;
	}

	Uint32 CRedGuiList::AddItem( const String& item, const Color& textColor /*= Color::WHITE*/, RedGuiAny userData /*= nullptr*/ )
	{
		return AddItem( new CRedGuiListItem( item, userData, textColor ) );
	}

	void CRedGuiList::AddItems( const TDynArray< String >& items )
	{
		const Uint32 itemCount = items.Size();
		for( Uint32 i=0; i<itemCount; ++i )
		{
			AddItem( new CRedGuiListItem( items[i] ) );
		}
		UpdateView();
	}

	void CRedGuiList::RemoveAllItems()
	{
		for( Uint32 index = 0, end = m_rows.Size(); index != end; ++index )
		{
			m_rows[ index ]->EventTextChanged.Unbind( this, &CRedGuiList::NotifyItemTextChanged );
		}

		m_rows.ClearPtrFast();
		
		
		m_displayOrder.Clear();
		m_lockedItems.Clear();

		m_maxItemWidth = (Uint32)( m_croppClient->GetSize().X );
		UpdateView();
	}

	Uint32 CRedGuiList::GetItemCount() const
	{
		return m_rows.Size();
	}

	void CRedGuiList::LockItem( Uint32 index, Bool value )
	{
		if ( index < m_rows.Size() )
		{
			if ( value )
			{
				m_lockedItems.PushBack( index );
			}
			else
			{
				m_lockedItems.Remove( index );
			}
			m_dirty = true;
			UpdateView();
		}
	}

	ESelectionMode CRedGuiList::GetSelectionMode() const
	{
		return m_mode;
	}

	void CRedGuiList::SetSelectionMode( ESelectionMode value )
	{
		m_mode = value;
	}

	void CRedGuiList::DeselectAll()
	{
		const Uint32 rowCount = m_rows.Size();
		for( Uint32 i=0; i<rowCount; ++i )
		{
			m_rows[i]->SetSelected( false );
		}
	}

	void CRedGuiList::Deselect( Uint32 index )
	{
		if( index < m_rows.Size() )
		{
			m_rows[index]->SetSelected( false );
		}
	}

	Int32 CRedGuiList::GetSelection() const
	{
		const Uint32 rowCount = m_rows.Size();
		for( Uint32 i=0; i<rowCount; ++i )
		{
			if( m_rows[i]->GetSelected() == true )
			{
				return i;
			}
		}
		return -1;
	}

	Uint32 CRedGuiList::GetSelections( TDynArray< Uint32 >& selections ) const
	{
		Uint32 selectedItemCount = 0;

		const Uint32 rowCount = m_rows.Size();
		for( Uint32 i=0; i<rowCount; ++i )
		{
			if( m_rows[i]->GetSelected() == true )
			{
				selections.PushBack( i );
				++selectedItemCount;
			}
		}

		return selectedItemCount;
	}

	void CRedGuiList::SetSelection( Uint32 index, Bool value /*= true*/ )
	{
		if( index < m_rows.Size() )
		{
			if( value == true )
			{
				UpdateSelection( index );
			}
			else
			{
				m_rows[index]->SetSelected( value );
			}
		}
	}

	void CRedGuiList::SetSelection( const String& item, Bool value /*= true*/ )
	{
		Int32 foundItemIndex = Find( item );

		if( foundItemIndex != -1 )
		{
			if( value == true )
			{
				UpdateSelection( foundItemIndex );
			}
			else
			{
				m_rows[foundItemIndex]->SetSelected( value );
			}
		}
	}

	void CRedGuiList::SetSelection( const TDynArray< Uint32 >& selections, Bool value /*= true*/ )
	{
		const Uint32 itemCount = m_rows.Size();
		const Uint32 selectionCount = selections.Size();

		for( Uint32 i=0; i<selectionCount; ++i )
		{
			const Uint32 selectItemIndex = selections[i];
			if( selectItemIndex < itemCount )
			{
				if( value == true )
				{
					UpdateSelection( selectItemIndex );
				}
				else
				{
					m_rows[selectItemIndex]->SetSelected( value );
				}
			}
		}
	}

	Bool CRedGuiList::IsSelected( Uint32 index )
	{
		if( index < m_rows.Size() )
		{
			return m_rows[index]->GetSelected();
		}
		return false;
	}

	const String CRedGuiList::GetItemText( Uint32 rowIndex, Uint32 columnIndex /*= 0*/ )
	{
		if( rowIndex < m_rows.Size() )
		{
			return m_rows[rowIndex]->GetText( columnIndex );
		}
		return String::EMPTY;
	}

	const Color& CRedGuiList::GetItemColor( Uint32 index )
	{
		if( index < m_rows.Size() )
		{
			return m_rows[index]->GetTextColor();
		}
		return Color::WHITE;
	}

	void CRedGuiList::SetItemColor( Uint32 index, const Color& color )
	{
		if( index < m_rows.Size() )
		{
			return m_rows[index]->SetTextColor( color );
		}
	}

	void CRedGuiList::SetAllItemColors( const Color& color )
	{
		for( Uint32 i = 0; i < m_rows.Size(); ++i )
		{
			m_rows[ i ]->SetTextColor( color );
		}
	}

	RedGui::RedGuiAny CRedGuiList::GetItemUserData( Uint32 index )
	{
		if( index < m_rows.Size() )
		{
			return m_rows[index]->GetUserData< void >();
		}
		return nullptr;
	}

	void CRedGuiList::SetItem( Uint32 index, const String& item, const Color& textColor /*= Color::WHITE*/, RedGuiAny userData /*= nullptr*/ )
	{
		if( index < m_rows.Size() )
		{
			m_rows[index]->SetText( item );
			m_rows[index]->SetTextColor( textColor );
			m_rows[index]->SetUserData( userData );

			if( m_sortedColumn == 0 )
			{
				m_dirty = true;
			}
		}
	}

	Int32 CRedGuiList::Find( const String& item, Uint32 column /*= 0*/, Bool matchCase /*= true*/ )
	{
		if( column > m_columnCount )
		{
			return -1;
		}

		Int32 foundItemIndex = -1;

		const Uint32 rowCount = m_rows.Size();
		for( Uint32 i=0; i<rowCount; ++i )
		{
			if( matchCase == true )
			{
				if( m_rows[i]->GetText( column ) == item )
				{
					foundItemIndex = i;
				}
			}
			else
			{
				if( m_rows[i]->GetText( column ).ToLowerUnicode() == item.ToLowerUnicode() )
				{
					foundItemIndex = i;
				}
			}
		}

		return foundItemIndex;
	}

	void CRedGuiList::SetColLabelsVisible( Bool value )
	{
		m_colLabelsPanel->SetVisible( value );
	}

	Bool CRedGuiList::GetColLabelsVisible() const
	{
		return m_colLabelsPanel->GetVisible();
	}

	Bool CRedGuiList::IsSorting() const
	{
		return m_sorting;
	}

	void CRedGuiList::SetSorting( Bool value )
	{
		m_sorting = value;
	}

	void CRedGuiList::SetFirstItem( Uint32 index )
	{
		const Uint32 size = m_displayOrder.Size();
		if( index < size )
		{
			if( size <= m_maxVerticalItemCount )
			{
				m_firstItemPosition.Y = 0.0f;
			}
			else
			{
				if( index < ( size - m_maxVerticalItemCount ) )
				{
					m_firstItemPosition.Y = -( (Float)( index * GDefaultListItemHeight ) );
				}
				else
				{
					m_firstItemPosition.Y = -( (Float)( ( size - m_maxVerticalItemCount ) * GDefaultListItemHeight ) );
				}
			}
		}

		UpdateView();
	}

	void CRedGuiList::SetFirstItem( const String& item )
	{
		Int32 itemIndexFindString = Find( item );
		SetFirstItem( itemIndexFindString );
	}

	void CRedGuiList::SetTextAlign( EInternalAlign align )
	{
		m_textAlign = align;
	}

	EInternalAlign CRedGuiList::GetTextAlign() const
	{
		return m_textAlign;
	}

	void CRedGuiList::Draw()
	{
		if( m_dirty && m_sortedColumn != -1 )
		{
			SortItems( m_sortedColumn );
			m_dirty = false;
		}

		GetTheme()->DrawPanel(this);

		if( m_firstVisibleItem != -1 && m_lastVisibleItem != -1 )
		{
			GetTheme()->SetCroppedParent( m_croppClient );

			const Box2& padding = m_croppClient->GetPadding();
			const Uint32 rowWidth = (Uint32)( m_croppClient->GetWidth() - ( padding.Min.X + padding.Max.X ) );
			Float x = m_croppClient->GetAbsoluteLeft() + padding.Min.X + m_firstItemPosition.X;
			Vector2 size( Vector2( (Float)m_maxItemWidth, (Float)GDefaultListItemHeight ) );

			Uint32 index = 0;
			for ( ; index < m_lockedItems.Size(); ++index )
			{
				CRedGuiListItem* item = m_rows[ m_lockedItems[ index ] ];

				Float y = m_croppClient->GetAbsoluteTop() + (Float)( index * GDefaultListItemHeight );
				Vector2 position( x, y );

				GetTheme()->DrawRawFilledRectangle( position, size, item->GetBackgroundColor() );
				GetTheme()->DrawRawRectangle( position, size, GBorder );

				if( m_highlightIndex >= 0 && index == (Uint32)m_highlightIndex )
				{
					GetTheme()->DrawRawFilledRectangle( position, size, GHighlight );
				}
				else
				{
					GetTheme()->DrawRawFilledRectangle( position, size, GLocked );
				}
				GetTheme()->DrawRawRectangle( position, size, GLocked );

				// draw text
				DrawItemText( position, item, item->GetTextColor() );
			}

			for( Uint32 i=m_firstVisibleItem; i<(Uint32)m_lastVisibleItem; ++i, ++index )
			{
				CRedGuiListItem* item = m_rows[ m_displayOrder[ i ] ];

				Float y = m_croppClient->GetAbsoluteTop() + (Float)( index * GDefaultListItemHeight );

				Vector2 position( x, y );

				GetTheme()->DrawRawFilledRectangle( position, size, item->GetBackgroundColor() );
				GetTheme()->DrawRawRectangle( position, size, GBorder );

				if( item->GetSelected() == true )
				{
					GetTheme()->DrawRawFilledRectangle( position, size, GHighlight );
					GetTheme()->DrawRawRectangle( position, size, GPushed );
				}

				if( m_highlightIndex >= 0 && index == (Uint32)m_highlightIndex )
				{
					GetTheme()->DrawRawFilledRectangle( position, size, GHighlight );
					GetTheme()->DrawRawRectangle( position, size, GHighlight );
				}

				if( m_pushedIndex != -1 && index == (Uint32)m_highlightIndex )
				{
					GetTheme()->DrawRawFilledRectangle( position, size, GPushed );
					GetTheme()->DrawRawRectangle( position, size, GPushed );
				}

				// draw text
				DrawItemText( position, item, item->GetTextColor() );
			}

			GetTheme()->ResetCroppedParent();
		}
	}

	Int32 CRedGuiList::CheckPoint( const Vector2& position )
	{
		const Vector2 relativePosition = position - m_croppClient->GetAbsolutePosition();
		if ( relativePosition.X > 0.0 && relativePosition.Y > 0.0 )
		{
			// first check if we're not over locked items
			Uint32 visibleItemIndex = (Uint32)( relativePosition.Y / (Float)GDefaultListItemHeight );
			if ( visibleItemIndex <= m_maxVerticalItemCount )
			{
				return visibleItemIndex;
			}
		}

		return -1;
	}

	Vector2 CRedGuiList::GetTextSize( const String& text )
	{
		CFont* font = GetFont();
		if(font == nullptr)
		{
			return Vector2( 0.0f, 0.0f );
		}

		Int32 unusedX, unusedY;
		Uint32 textWidth, textHeight;
		font->GetTextRectangle( text, unusedX, unusedY, textWidth, textHeight );

		return Vector2( (Float)textWidth, (Float)textHeight );
	}

	void CRedGuiList::RecalculateMaxWidth( const String& newText )
	{
		m_maxItemWidth = Max< Uint32 >( (Uint32)GetTextSize( newText ).X, m_maxItemWidth );
	}

	void CRedGuiList::NotifyHeaderClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		Uint32 columnIndex = 0;
		FromString< Uint32 >( sender->GetUserString( TXT("ColumnIndex") ), columnIndex );
		EventColumnClicked( this, columnIndex );
		if( m_sorting == true )
		{
			if( GRedGui::GetInstance().GetInputManager()->IsShiftPressed() )
			{
				m_sortedColumn = columnIndex;
			}
			else
			{
				m_sortedColumn = -1;
			}

			// change sort mode
			if( m_sortMode == ST_Ascending )
			{
				m_sortMode = ST_Descending;
			}
			else
			{
				m_sortMode = ST_Ascending;
			}

			SortItems( columnIndex );
		}
	}

	void CRedGuiList::NotifyScrollChangePosition( RedGui::CRedGuiEventPackage& eventPackage, Uint32 value )
	{
		RedGui::CRedGuiControl* sender = eventPackage.GetEventSender();

		if( sender == m_verticalBar )
		{
			Vector2 point = m_firstItemPosition;
			point.Y = -(Float)value;
			m_firstItemPosition = point;
		}
		else if( sender == m_horizontalBar )
		{
			Vector2 point = m_firstItemPosition;
			point.X = -(Float)value;
			m_firstItemPosition = point;
		}

		UpdateView();
	}

	void CRedGuiList::NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta )
	{
		RED_UNUSED( eventPackage );

		if( m_verticalRange != 0 )
		{
			Vector2 position = m_firstItemPosition;
			Int32 offset = -(Int32)position.Y;

			if( delta < 0 )
			{
				offset += GScrollWheelValue;
			}
			else
			{
				offset -= GScrollWheelValue;
			}

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_verticalRange )
			{
				offset = m_verticalRange;
			}

			if( offset != position.Y )
			{
				position.Y = -(Float)offset;
				if( m_verticalBar != nullptr )
				{
					m_verticalBar->SetScrollPosition( offset );
				}
				m_firstItemPosition = position;
			}
		}
		else if( m_horizontalRange != 0 )
		{
			Vector2 position = m_firstItemPosition;
			Int32 offset = -(Int32)position.X;

			if( delta < 0 )
			{
				offset += GScrollWheelValue;
			}
			else
			{
				offset -= GScrollWheelValue;
			}

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_horizontalRange )
			{
				offset = m_horizontalRange;
			}

			if( offset != position.X )
			{
				position.X = -(Float)offset;
				if( m_horizontalBar != nullptr )
				{
					m_horizontalBar->SetScrollPosition( offset );
				}
				m_firstItemPosition = position;
			}
		}

		UpdateView();
	}

	void CRedGuiList::UpdateView()
	{
		UpdateScrollSize();
		UpdateScrollPosition();
	}

	void CRedGuiList::UpdateScrollSize()
	{
		Vector2 viewSize = m_croppClient->GetSize();
		Vector2 contentSize = Vector2( (Float)m_maxItemWidth, (Float)( ( m_rows.Size() + m_lockedItems.Size() ) * GDefaultListItemHeight ) );

		// horizontal content doesn't fit
		if(contentSize.Y > viewSize.Y)
		{
			if( m_verticalBar != nullptr )
			{
				if( m_verticalBar->GetVisible() == false )
				{
					m_verticalBar->SetVisible( true );

					if(m_horizontalBar != nullptr)
					{
						// recalculate horizontal bar after add vertical bar
						if( ( contentSize.X > viewSize.X ) && ( m_horizontalBar->GetVisible() == false ) )
						{
							m_horizontalBar->SetVisible( true );
						}
					}
				}
			}
		}
		else
		{
			if( m_verticalBar != nullptr )
			{
				if( m_verticalBar->GetVisible() == true )
				{
					m_verticalBar->SetVisible( false );

					if( m_horizontalBar != nullptr )
					{
						// recalculate horizontal bar after remove vertical bar
						if( ( contentSize.X <= viewSize.X ) && ( m_horizontalBar->GetVisible() == true ) )
						{
							m_horizontalBar->SetVisible( false );
						}
					}
				}
			}
		}

		// vertical content doesn't fit
		if( contentSize.X > viewSize.X )
		{
			if( m_horizontalBar != nullptr )
			{
				if( m_horizontalBar->GetVisible() == false )
				{
					m_horizontalBar->SetVisible( true );

					if( m_verticalBar != nullptr )
					{
						// recalculate vertical bar after add horizontal bar
						if( ( contentSize.Y > viewSize.Y ) && ( m_verticalBar->GetVisible() == false ) )
						{
							m_verticalBar->SetVisible( true );
						}
					}
				}
			}
		}
		else
		{
			if( m_horizontalBar != nullptr )
			{
				if( m_horizontalBar->GetVisible() == true )
				{
					m_horizontalBar->SetVisible( false );

					if( m_verticalBar != nullptr )
					{
						// recalculate vertical bar after remove horizontal bar
						if( ( contentSize.Y <= viewSize.Y ) && ( m_verticalBar->GetVisible() == true ) )
						{
							m_verticalBar->SetVisible( false );
						}
					}
				}
			}
		}

		// calculate ranges
		m_verticalRange = ( viewSize.Y >= contentSize.Y ) ? 0 : (Uint32)( contentSize.Y - viewSize.Y );
		m_horizontalRange = ( viewSize.X >= contentSize.X ) ? 0 : (Uint32)( contentSize.X - viewSize.X );

		// set new values
		if( m_verticalBar != nullptr )
		{
			m_verticalBar->SetScrollPage( GScrollPageSize );
			m_verticalBar->SetScrollRange( m_verticalRange + 1 );
			if( contentSize.Y > 0 )
			{
				m_verticalBar->SetTrackSize( (Int32)( (Float)( m_verticalBar->GetLineSize() * viewSize.Y ) / (Float)( contentSize.Y ) ) );
			}
		}
		if( m_horizontalBar != nullptr )
		{
			m_horizontalBar->SetScrollPage( GScrollPageSize );
			m_horizontalBar->SetScrollRange( m_horizontalRange + 1 );
			if( contentSize.X > 0 )
			{
				m_horizontalBar->SetTrackSize( (Int32)( (Float)( m_horizontalBar->GetLineSize() * viewSize.X ) / (Float)( contentSize.X ) ) );
			}
		}
	}

	void CRedGuiList::UpdateScrollPosition()
	{
		if( m_verticalRange != 0 )
		{
			Vector2 position = m_firstItemPosition;
			Int32 offset = -(Int32)position.Y;

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_verticalRange )
			{
				offset = m_verticalRange;
			}

			if( offset != position.Y )
			{
				position.Y = -(Float)offset;
				if( m_verticalBar != nullptr )
				{
					m_verticalBar->SetScrollPosition( offset );
				}
				
				// calculate first and last render item
				m_firstItemPosition = position;
			}
		}
		else if( m_horizontalRange != 0 )
		{
			Vector2 position = m_firstItemPosition;
			Int32 offset = -(Int32)position.X;

			if( offset < 0 )
			{
				offset = 0;
			}
			else if( offset > (Int32)m_horizontalRange )
			{
				offset = m_horizontalRange;
			}

			if( offset != position.X )
			{
				position.X = -(Float)offset;
				if( m_horizontalBar != nullptr )
				{
					m_horizontalBar->SetScrollPosition( offset );
				}
				// calculate first and last render item
				m_firstItemPosition = position;
			}
		}

		// set current visible items
		Uint32 invisibleTop = (Uint32)( (-m_firstItemPosition.Y) / GDefaultListItemHeight );
		Uint32 invisibleBottom = invisibleTop + m_maxVerticalItemCount - m_lockedItems.Size() + 2;

		m_firstVisibleItem = invisibleTop;
		m_lastVisibleItem = ( invisibleBottom > m_rows.Size() ) ? m_rows.Size() : invisibleBottom;
	}

	void CRedGuiList::UpdateSelection( Uint32 selectedItemIndex )
	{
		switch( m_mode )
		{
		case SM_None:
			break;
		case SM_Single:
			DeselectAll();
			m_rows[selectedItemIndex]->SetSelected( true );
			break;
		case SM_Multiple:
			m_rows[selectedItemIndex]->SetSelected( !m_rows[selectedItemIndex]->GetSelected() );
			break;
		case SM_Extended:
			DeselectAll();
			if( GRedGui::GetInstance().GetInputManager()->IsShiftPressed() == true && m_firstSelectedIndex != -1 )
			{
				if( selectedItemIndex > (Uint32)m_firstSelectedIndex )
				{
					for( Uint32 i=m_firstSelectedIndex; i<selectedItemIndex+1; ++i )
					{
						m_rows[i]->SetSelected( true );
					}
				}
				else
				{
					for( Uint32 i=selectedItemIndex; i<(Uint32)m_firstSelectedIndex+1; ++i )
					{
						m_rows[i]->SetSelected( true );
					}
				}
			}
			else
			{
				m_firstSelectedIndex = selectedItemIndex;
				m_rows[selectedItemIndex]->SetSelected( true );
			}
			break;
		}
	}

	void CRedGuiList::NotifyMouseButtonDoubleClick( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		OnDoubleClickOnRow();
	}

	void CRedGuiList::NotifyMouseMove( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition )
	{
		RED_UNUSED( eventPackage );

		Int32 itemUnderMouse = CheckPoint( mousePosition );

		if( m_highlightIndex != itemUnderMouse )
		{
			m_highlightIndex = itemUnderMouse;
			GRedGui::GetInstance().GetToolTipManager()->HideToolTip( m_croppClient );
		}
	}

	void CRedGuiList::NotifyClientMouseLostFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* newFocusedCtrl )
	{
		RED_UNUSED( eventPackage );

		m_highlightIndex = -1;
	}

	void CRedGuiList::NotifyClientSizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldSize, const Vector2& newSize )
	{
		RED_UNUSED( eventPackage );

		m_maxItemWidth = (Uint32)( m_croppClient->GetSize().X );
		const Uint32 itemCount = m_rows.Size();
		for( Uint32 i=0; i<itemCount; ++i )
		{
			RecalculateMaxWidth( m_rows[i]->GetText() );
		}

		m_maxVerticalItemCount = (Uint32)( m_croppClient->GetHeight() / GDefaultListItemHeight );
		m_firstItemPosition.X = 0.0f;

		UpdateView();
	}

	void CRedGuiList::NotifyClientMouseButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		OnClickOnRow();

		if ( button == MB_Right )
		{
			OnRightClickOnRow();
		}

	}

	void CRedGuiList::NotifyItemTextChanged( RedGui::CRedGuiEventPackage&, Uint32 columnIndex )
	{
		if( m_sortedColumn == (Int32)columnIndex )
		{
			m_dirty = true;
		}
	}

	void CRedGuiList::NotifyClientMouseButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button )
	{
		RED_UNUSED( eventPackage );

		m_pushedIndex = -1;
	}

	void CRedGuiList::DrawItemText( const Vector2& position, CRedGuiListItem* item, const Color& color )
	{
		Uint32 startPosX = (Uint32)m_croppClient->GetPosition().X;

		for( Uint32 i=0; i<m_columnCount; ++i )
		{
			const Uint32 columnWidth = (Uint32)m_columnsLabels[i]->GetSize().X;
			String text = item->GetText( i );
			const Vector2 textSize = GetTextSize( text );
			const CRedGuiCroppedRect croppedRect( Box2( position.X + (Float)startPosX, position.Y, (Float)columnWidth, (Float)GDefaultListItemHeight ) );

			GetTheme()->SetCroppedParent( &croppedRect );
			{
				// calculate horizontal position
				Float horizontalDelta = (Float)startPosX;
				switch( m_textAlign )
				{
				case IA_TopLeft:
				case IA_MiddleLeft:
				case IA_BottomLeft:
					horizontalDelta += 10.0f;	// item with 10 pixels padding looks better
					break;

				case IA_TopCenter:
				case IA_MiddleCenter:
				case IA_BottomCenter:
					horizontalDelta += ( columnWidth - textSize.X ) / 2.0f;
					break;

				case IA_TopRight:
				case IA_MiddleRight:
				case IA_BottomRight:
					horizontalDelta -= ( ( textSize.X + 10.0f ) - columnWidth );	// item with 10 pixels padding looks better
					break;
				}

				// calculate vertical position
				const Float verticalDelta = ( GDefaultListItemHeight - textSize.Y - 1 ) / 2.0f;

				// compose final position
				const Vector2 finalPosition = position + Vector2( horizontalDelta, verticalDelta );

				GetTheme()->DrawRawText( finalPosition, text, color );
			}
			GetTheme()->ResetCroppedParent();

			startPosX += columnWidth;
		}
	}

	void CRedGuiList::SortItems( Uint32 columnIndex )
	{
		// data sorter
		struct LocalSorter : public Red::System::NonCopyable
		{
			Bool operator()( Uint32 a, Uint32 b ) const
			{
				Float result = 0;
				switch( m_sortAs )
				{
				case SA_String:
					{
						const String& strA = m_data[ a ]->GetText( m_columnIndex );
						const String& strB = m_data[ b ]->GetText( m_columnIndex );
						result = (Float)Red::System::StringCompareNoCase( strA.AsChar(), strB.AsChar() );
					}
					break;
				case SA_Integer:
					{
						Int32 intA; FromString< Int32 >( m_data[ a ]->GetText( m_columnIndex ), intA );
						Int32 intB; FromString< Int32 >( m_data[ b ]->GetText( m_columnIndex ), intB );
						result = (Float)( intA - intB);
					}
					break;
				case SA_Real:
					{
						Float floatA; FromString< Float >( m_data[ a ]->GetText( m_columnIndex ), floatA );
						Float floatB; FromString< Float >( m_data[ b ]->GetText( m_columnIndex ), floatB );
						result = floatA - floatB;
					}
					break;
				}

				if( m_sortMode == ST_Ascending )
				{
					return result < 0.0f;
				}
				else
				{
					return result > 0.0f;
				}
			}

			LocalSorter( const ListItemCollection& data, ESortingType sort, Uint32 columnIndex, ESortingAs sortAs )
			: m_data( data )
			, m_sortMode( sort )
			, m_columnIndex( columnIndex )
			, m_sortAs( sortAs )
			{
			}

		private:
			const ListItemCollection&	m_data;
			ESortingType				m_sortMode;
			ESortingAs					m_sortAs;
			Uint32						m_columnIndex;
		};

		LocalSorter sorter( m_rows, m_sortMode, columnIndex, GetColumnSortType( columnIndex ) );

		Sort( m_displayOrder.Begin(), m_displayOrder.End(), sorter );
		Sort( m_lockedItems.Begin(), m_lockedItems.End(), sorter );
	}

	void CRedGuiList::ScrollToBottom()
	{
		SetFirstItem( m_displayOrder.Size() - 1 );
	}

	void CRedGuiList::ScrollToTop()
	{
		SetFirstItem( 0 );
	}

	void CRedGuiList::SetToolTip( CRedGuiControl* tooltip )
	{
		m_croppClient->SetNeedToolTip( true );
		m_croppClient->SetToolTip( tooltip );
	}

	CRedGuiListItem* CRedGuiList::GetItem( Uint32 index )
	{
		if( index < m_rows.Size() )
		{
			return m_rows[index];
		}
		return nullptr;
	}

	Bool CRedGuiList::FindDisplayOrder( Uint32 itemIndex, Uint32& displayOrder ) const
	{
		for( Uint32 i = 0; i < m_displayOrder.Size(); ++i )
		{
			if( m_displayOrder[ i ] == itemIndex )
			{
				displayOrder = i;
				return true;
			}
		}

		return false;
	}

	void CRedGuiList::AppendColumn( const String& caption, Int32 width, enum ESortingAs value /*= RedGui::SA_String*/ )
	{
		CRedGuiButton* header = new CRedGuiButton( 0, 0, width, GDefaultListItemHeight );
		header->EventButtonClicked.Bind( this, &CRedGuiList::NotifyHeaderClicked );
		header->SetBackgroundColor( Color( 30, 30, 30, 255 ) );
		header->SetTextAlign( IA_MiddleCenter );
		header->SetText( caption );
		header->SetUserString( TXT("ColumnIndex"), ToString( m_columnCount ) );
		header->SetUserString( TXT("SortAs"), ToString( SA_String ) );
		m_colLabelsPanel->AddChild( header );		
		m_columnsLabels.PushBack( header );
		m_columnCount = m_columnsLabels.Size();

		//
		if( m_columnCount == 1 )
		{
			header->SetDock( DOCK_Fill );
		}
		else if( m_columnCount == 2 )
		{
			m_columnsLabels[0]->SetDock( DOCK_Left );
			header->SetDock( DOCK_Left );
		}
		else
		{
			header->SetDock( DOCK_Left );
		}

		SetColumnSortType( m_columnCount-1, value );
	}

	void CRedGuiList::DeleteColumn( Uint32 index )
	{
		if( index < m_columnsLabels.Size() )
		{
			CRedGuiButton* button = m_columnsLabels[index];
			m_colLabelsPanel->RemoveChild( button );
			button->Dispose();

			m_columnsLabels.Erase( m_columnsLabels.Begin() + index );
			m_columnCount = m_columnsLabels.Size();

			if( m_sortedColumn == (Int32)index )
			{
				m_sortedColumn = -1;
			}
		}
	}

	void CRedGuiList::SetColumnLabel(  const String& text, Uint32 index /*= 0 */ )
	{
		if( index < m_columnsLabels.Size() )
		{
			m_columnsLabels[index]->SetText( text );
		}
	}

	String CRedGuiList::GetColumnLabel( Uint32 index ) const
	{
		if( index < m_columnsLabels.Size() )
		{
			return m_columnsLabels[index]->GetText();
		}
		return String::EMPTY;
	}

	void CRedGuiList::SetColumnWidth( Uint32 index, Int32 width /*= -1 */ )
	{
		if( index < m_columnsLabels.Size() )
		{
			m_columnsLabels[index]->SetSize( width, GDefaultListItemHeight );
		}
	}

	Uint32 CRedGuiList::GetColumnCount() const
	{
		return m_columnCount;
	}

	Uint32 CRedGuiList::GetColumnWidth( Uint32 index )
	{
		if( index < m_columnsLabels.Size() )
		{
			return (Uint32)m_columnsLabels[index]->GetSize().X;
		}
		return 0;
	}

	void CRedGuiList::SetItemText( const String& text, Uint32 rowIndex, Uint32 columnIndex )
	{
		if( rowIndex < m_rows.Size() )
		{
			CRedGuiListItem* item = GetItem( rowIndex );
			item->SetText( text, columnIndex );
		}
	}

	void CRedGuiList::SetColumnSortType( Uint32 columnIndex, enum ESortingAs value )
	{
		if( columnIndex < m_columnsLabels.Size() )
		{
			m_columnsLabels[columnIndex]->SetUserString( TXT("SortAs"), ToString( (Uint32)value ) );
		}
	}

	enum ESortingAs CRedGuiList::GetColumnSortType( Uint32 columnIndex ) const
	{
		if( columnIndex < m_columnsLabels.Size() )
		{
			CRedGuiButton* clickedButton = m_columnsLabels[columnIndex];
			Uint32 value = 0;
			FromString< Uint32 >( clickedButton->GetUserString( TXT("SortAs") ), value );
			ESortingAs sortAs = static_cast< ESortingAs >( value );
			return sortAs;
		}

		return SA_String;
	}

	void CRedGuiList::SetColumnSortIndex(Int32 columnSortIndex)
	{
		m_sortedColumn = columnSortIndex;
	}

	Bool CRedGuiList::RemoveItem( Uint32 itemIndex )
	{
		if( itemIndex < m_rows.Size() )
		{			
			Uint32 itemCount = m_displayOrder.Size();
			for( Uint32 i=0; i<itemCount; ++i )
			{
				if( m_displayOrder[i] == itemIndex )
				{
					m_displayOrder.RemoveAt( i );
					break;
				}
			}

			m_rows.RemoveAt( itemIndex );

			itemCount = m_displayOrder.Size();
			for( Uint32 i=0; i<itemCount; ++i )
			{
				if( m_displayOrder[i] > itemIndex)
				{
					--m_displayOrder[i];
				}
			}

			if ( m_lockedItems.Exist( itemIndex ) )
			{
				m_lockedItems.Remove( itemIndex );
			}

			itemCount = m_lockedItems.Size();
			for( Uint32 i=0; i<itemCount; ++i )
			{
				if( m_lockedItems[i] > itemIndex )
				{
					--m_lockedItems[i];
				}
			}

			m_maxItemWidth = (Uint32)( m_croppClient->GetSize().X );
			UpdateView();
			return true;
		}

		return false;
	}

	void CRedGuiList::OnClickOnRow()
	{
		// keep in mind that this is called also on right click, so should contain only basic functionality

		// convert from visible list order to order in memory
		m_pushedIndex = -1;
		if( m_highlightIndex >= 0 )
		{
			if( m_highlightIndex <= (Int32)m_maxVerticalItemCount )
			{
				if ( m_highlightIndex < (Int32)m_lockedItems.Size() )
				{
					m_pushedIndex = m_lockedItems[ m_highlightIndex ];
				}
				else
				{
					Uint32 highlightInd = (Uint32)( -m_firstItemPosition.Y / GDefaultListItemHeight ) + m_highlightIndex - m_lockedItems.Size();
					if ( highlightInd < m_displayOrder.Size() )
					{
						m_pushedIndex = m_displayOrder[ highlightInd ];
					}
				}
			}
		}

		if( m_pushedIndex != -1 )
		{
			UpdateSelection( m_pushedIndex );
			EventSelectedItem( this, m_pushedIndex );
		}
	}

	void CRedGuiList::OnRightClickOnRow()
	{
		if( m_highlightIndex >= 0 && m_highlightIndex <= (Int32)m_maxVerticalItemCount )
		{
			Int32 index = -1;
			if ( m_highlightIndex < (Int32)m_lockedItems.Size() )
			{
				index = m_lockedItems[ m_highlightIndex ];
			}
			else
			{
				Uint32 displayInd = (Uint32)( -m_firstItemPosition.Y / GDefaultListItemHeight ) + m_highlightIndex - m_lockedItems.Size();
				if ( displayInd < m_displayOrder.Size() )
				{
					index = m_displayOrder[ displayInd ];
				}
			}

			if ( index != -1 )
			{
				LockItem( index, !m_lockedItems.Exist( index ) );
			}
		}
	}

	void CRedGuiList::OnDoubleClickOnRow()
	{
		if ( m_highlightIndex != -1 )
		{
			UpdateSelection( m_displayOrder[ m_highlightIndex ] );
			EventDoubleClickItem( this, m_displayOrder[ m_highlightIndex ] );
		}
	}

	void CRedGuiList::AdjustAreaToCursor()
	{
		// from top
		while( m_highlightIndex < 0 )
		{
			m_verticalBar->MovePageUp();
			m_highlightIndex += GScrollPageSize / GDefaultListItemHeight;
			if( m_verticalBar->GetScrollPosition() == 0 )
			{
				m_highlightIndex = 0;
				break;
			}
		}

		// from bottom
		while( m_highlightIndex > (Int32)m_maxVerticalItemCount )
		{
			m_verticalBar->MovePageDown();
			m_highlightIndex -= GScrollPageSize / GDefaultListItemHeight;
			if( m_verticalBar->GetScrollPosition() == m_verticalBar->GetScrollRange() )
			{
				m_highlightIndex = m_maxVerticalItemCount;
				break;
			}
		}
	}

	Bool CRedGuiList::OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data )
	{
		if( CRedGuiControl::OnInternalInputEvent( event, data ) == true )
		{
			return true;
		}

		if( event == RGIE_Down )
		{
			++m_highlightIndex;
			AdjustAreaToCursor();
			return true;
		}
		else if( event == RGIE_Up )
		{
			--m_highlightIndex;
			AdjustAreaToCursor();
			return true;
		}
		else if( event == RGIE_ActivateSpecialMode )
		{
			SetGamepadLevel( true );
			return true;
		}
		else if( event == RGIE_Back )
		{
			SetGamepadLevel( false );
		}
		else if( event == RGIE_Select )
		{
			OnClickOnRow();
			return true;
		}
		else if( event == RGIE_Execute )
		{
			OnDoubleClickOnRow();
			return true;
		}
		else if( event == RGIE_OptionsButton )
		{
			OnRightClickOnRow();
			return true;
		}
		else if( event == RGIE_Move )
		{
			m_horizontalBar->SetScrollPosition( (Uint32)( m_horizontalBar->GetScrollPosition() + data.X ) );
			m_verticalBar->SetScrollPosition( (Uint32)( m_verticalBar->GetScrollPosition() + data.Y ) );
			return true;
		}

		return false;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
