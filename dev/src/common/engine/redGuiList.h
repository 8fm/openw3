/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiControl.h"

namespace RedGui
{
	class CRedGuiList : public CRedGuiControl
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_RedGui, MC_RedGuiControls );
	public:
		CRedGuiList(Uint32 x, Uint32 y, Uint32 width, Uint32 height);
		virtual ~CRedGuiList();

		// Events
		Event2_PackageInt32 EventSelectedItem;
		Event2_PackageInt32 EventDoubleClickItem;
		Event2_PackageInt32 EventColumnClicked;

		// add items
		Uint32 AddItem( const String& item, const Color& textColor = Color::WHITE, RedGuiAny userData = nullptr );
		void AddItems( const TDynArray< String >& items );
		Uint32 AddItem( CRedGuiListItem* item );
		void RemoveAllItems();
		Bool RemoveItem( Uint32 itemIndex );
		Uint32 GetItemCount() const;

		// lock item
		void LockItem( Uint32 index, Bool value = true );

		// selection
		ESelectionMode GetSelectionMode() const;
		void SetSelectionMode( ESelectionMode value );

		void DeselectAll();
		void Deselect( Uint32 index );

		Int32 GetSelection() const;
		Uint32 GetSelections( TDynArray< Uint32 >& selections ) const;

		void SetSelection( Uint32 index, Bool value = true );
		void SetSelection( const String& item, Bool value = true );
		void SetSelection( const TDynArray< Uint32 >& selections, Bool value = true );

		Bool IsSelected( Uint32 index );

		CRedGuiListItem* GetItem( Uint32 index );
		
		const Color& GetItemColor( Uint32 index );
		void SetItemColor( Uint32 index, const Color& color );
		void SetAllItemColors( const Color& color );

		void SetItemText( const String& text, Uint32 rowIndex, Uint32 columnIndex );
		const String GetItemText( Uint32 rowIndex, Uint32 columnIndex = 0 );

		RedGuiAny GetItemUserData( Uint32 index );
		void SetItem( Uint32 index, const String& item, const Color& textColor = Color::WHITE, RedGuiAny userData = nullptr );
		Int32 Find( const String& item, Uint32 column = 0, Bool matchCase = true );

		// headers / columns
		void SetColLabelsVisible( Bool value );
		Bool GetColLabelsVisible() const;

		Uint32 GetColumnCount() const;

		Uint32 GetColumnWidth( Uint32 index );
		void SetColumnWidth( Uint32 index, Int32 width );

		void AppendColumn( const String& caption, Int32 width, enum ESortingAs value = RedGui::SA_String );
		void DeleteColumn( Uint32 index );

		void SetColumnSortType( Uint32 columnIndex, enum ESortingAs value );
		void SetColumnSortIndex( Int32 columnSortIndex );
		enum ESortingAs GetColumnSortType( Uint32 columnIndex ) const;

		void SetColumnLabel(  const String& text, Uint32 index = 0 );
		String GetColumnLabel( Uint32 index ) const;
		
		Bool IsSorting() const;
		void SetSorting( Bool value );

		void SetFirstItem( Uint32 index );
		void SetFirstItem( const String& item );

		void SetTextAlign( EInternalAlign align );
		EInternalAlign GetTextAlign() const;

		void ScrollToTop();
		void ScrollToBottom();

		void Draw();

		virtual void SetToolTip( CRedGuiControl* tooltip );
		void SortItems( Uint32 columnIndex );

	private:
		Int32 CheckPoint( const Vector2& position );
		Vector2 GetTextSize( const String& text );
		void RecalculateMaxWidth( const String& newText );
		void DrawItemText( const Vector2& position, CRedGuiListItem* item, const Color& color );
		Bool FindDisplayOrder( Uint32 itemIndex, Uint32& displayOrder ) const;

		void UpdateView();
		void UpdateScrollPosition();
		void UpdateScrollSize();

		void UpdateSelection( Uint32 selectedItemIndex );

		void NotifyHeaderClicked( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyScrollChangePosition( RedGui::CRedGuiEventPackage& eventPackage, Uint32 value );
		void NotifyMouseMove( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition );
		void NotifyMouseWheel( RedGui::CRedGuiEventPackage& eventPackage, Int32 delta );
		void NotifyMouseButtonDoubleClick( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button );

		void OnDoubleClickOnRow();

		void NotifyClientMouseLostFocus( RedGui::CRedGuiEventPackage& eventPackage, CRedGuiControl* newFocusedCtrl );
		void NotifyClientSizeChanged( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& oldSize, const Vector2& newSize );
		void NotifyClientMouseButtonPressed( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button );

		void NotifyItemTextChanged( RedGui::CRedGuiEventPackage& eventPackage, Uint32 columnIndex );

		void OnClickOnRow();
		void OnRightClickOnRow();

		void NotifyClientMouseButtonReleased( RedGui::CRedGuiEventPackage& eventPackage, const Vector2& mousePosition, enum EMouseButton button );

		void AdjustAreaToCursor();

		Bool OnInternalInputEvent( enum ERedGuiInputEvent event, const Vector2& data );

		virtual void OnPendingDestruction() override final;

	private:
		CRedGuiPanel*				m_colLabelsPanel;
		TDynArray< CRedGuiButton*, MC_RedGuiControls, MemoryPool_RedGui > m_columnsLabels;
		Uint32						m_columnCount;

		CRedGuiPanel*		m_croppClient;
		CRedGuiScrollBar*	m_horizontalBar;
		CRedGuiScrollBar*	m_verticalBar;

		ListItemCollection	m_rows;
		ListItemIndicies	m_displayOrder;
		ListItemIndicies	m_lockedItems;

		Int32				m_pushedIndex;
		Int32				m_highlightIndex;
		Int32				m_firstSelectedIndex;

		EInternalAlign		m_textAlign;

		ESelectionMode		m_mode;
		Bool				m_sorting;
		ESortingType		m_sortMode;
		Int32				m_sortedColumn;
		Bool				m_dirty;

		Int32				m_firstVisibleItem;
		Int32				m_lastVisibleItem;

		Uint32				m_horizontalRange;
		Uint32				m_verticalRange;

		Uint32				m_maxItemWidth;
		Vector2				m_firstItemPosition;

		Uint32				m_maxVerticalItemCount;
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
