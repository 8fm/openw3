
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

// i'll remove it later -bs
#define NO_NEW_CLIPBOARD_STUFF

#include "propertiesPage.h"
#include "propertiesBrowserWithStatusbar.h"
#include "shortcutsEditor.h"
#include "undoProperty.h"
#include "../../common/engine/curveEntity.h"
#include "../../common/core/xmlWriter.h"
#include "../../common/core/xmlReader.h"
#include "../../common/core/memoryFileWriter.h"
#include "../../common/core/memoryFileReader.h"

#ifndef NO_NEW_CLIPBOARD_STUFF
#include "../../common/core/clipboardBase.h"
#endif

#define EPP_TIMER_TOOLTIP_ID 19843
#ifndef TooltipRefreshDefine
	static const int TooltipRefresh = 100; // milliseconds
	static const float TooltipDelay = 1.0f; // seconds
	#define TooltipRefreshDefine
#endif

wxDEFINE_EVENT( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEvent );
wxDEFINE_EVENT( wxEVT_COMMAND_REFRESH_PROPERTIES, wxCommandEvent );
wxDEFINE_EVENT( wxEVT_COMMAND_PROPERTY_SELECTED, wxCommandEvent );

Uint32 CPropertyTransaction::sm_lastUsedId = 0;

CPropertyTransaction::CPropertyTransaction( CEdPropertiesPage &page )
    : m_page( &page ), m_id( ++sm_lastUsedId )
{
    ++m_page->m_transactionsInProgress;
//	LOG_EDITOR ( TXT("++TRANSACTON page=%i id=%i, inProgress=%i"), &m_page, m_id, m_page->m_transactionsInProgress );

	if ( m_id == 0 ) 
	{
		m_id = ++sm_lastUsedId; // id 0 is reserved as "no transaction"
	}
}

CPropertyTransaction::~CPropertyTransaction()
{
	--m_page->m_transactionsInProgress;
//	LOG_EDITOR ( TXT("--TRANSACTON page=%i id=%i, inProgress=%i"), &m_page, m_id, m_page->m_transactionsInProgress );

	TDynArray<CEdPropertiesPage::SPropertyEventData> changes = Move( m_page->m_transactionChanges );

	for ( Int32 i = 0, e = changes.SizeInt()-1; i <= e; ++i )
	{
		Bool closeUndo = ( i == e );
		m_page->SendPropertyChangedEvent( changes[i], closeUndo );
	}
}

BEGIN_EVENT_TABLE( CEdPropertiesPage, wxScrolledWindow )
	EVT_PAINT( CEdPropertiesPage::OnPaint )
	EVT_ERASE_BACKGROUND( CEdPropertiesPage::OnEraseBackground )
	EVT_MOUSE_EVENTS( CEdPropertiesPage::OnMouseEvents )
	EVT_KEY_DOWN( CEdPropertiesPage::OnKeyDown )
	EVT_SIZE( CEdPropertiesPage::OnSize )
	EVT_SET_CURSOR( CEdPropertiesPage::OnSetCursor )
	EVT_TIMER( EPP_TIMER_TOOLTIP_ID, CEdPropertiesPage::OnTimerTooltip )

	EVT_MENU( XRCID( "editCopy" ), CEdPropertiesPage::OnEditCopy )
	EVT_MENU( XRCID( "editPaste" ), CEdPropertiesPage::OnEditPaste )
	EVT_MENU( XRCID( "editDelete" ), CEdPropertiesPage::OnEditDelete )

	EVT_SCROLLWIN(CEdPropertiesPage::OnScroll)
END_EVENT_TABLE()


PropertiesPageDayCycleSettings::PropertiesPageDayCycleSettings ()
{
	Reset();
}

void PropertiesPageDayCycleSettings::Reset()
{
	m_timeMin    = 0.f;
	m_timeMax    = 1.f - 0.001f;
	m_valueScale = 1.f;
}

bool PropertiesPageDayCycleSettings::IsValid() const
{
	return m_timeMin  < m_timeMax && m_valueScale > 0;
}


CEdPropertiesPage::CEdPropertiesPage( wxWindow* parent, const PropertiesPageSettings& settings, CEdUndoManager* undoManager )
	: wxScrolledWindow( parent, wxID_ANY, wxDefaultPosition, parent->GetSize(), wxEXPAND | wxVSCROLL | wxWANTS_CHARS )
	, CDropTarget( this )
	, m_undoManager( undoManager )
	, m_splitter( 200 )
	, m_root( NULL )
	, m_activeItem( NULL )
	, m_activeButton( NULL )
	, m_splitterDrag( false )
	, m_tooltipAvaiable( false )
	, m_tooltipText( String::EMPTY )
	, m_tooltipPos( 0, 0 )
	, m_tooltipTimer( this, EPP_TIMER_TOOLTIP_ID )
	, m_beforeLastMousePos( 0, 0 )
	, m_lastMousePos( 0, 0 )
	, m_tooltipEnabled ( true )
	, m_tooltipMouseNotMoved ( false )
	, m_tooltipCurrentProperty( NULL )
	, m_tooltipLastProperty( NULL )
	, m_statusBar( NULL )
	, m_mouseGrab( false )
	, m_transactionsInProgress( 0 )
	, m_isEntityEditorOwner( false )
	, m_settings( settings )
	, m_scrollSpeedSet( false )
	, m_defaultParentObject( nullptr )
	, m_duringMessageSending( false )
{
	// Setup tooltips
	m_tooltipTimer.Start(TooltipRefresh);

	SEvents::GetInstance().RegisterListener( RED_NAME( ScriptCompilationStart ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( ScriptCompilationEnd ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( SelectionChanged ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( SimpleCurveDisplayedTimeChanged ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( OnObjectDiscarded ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( RefreshPropertiesPage ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( EditorPreUndoStep ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( EditorPostUndoStep ), this );

	CEdShortcutsEditor::Load(*this, *GetAccelerators(), TXT("PropertyPage"));
}

CEdPropertiesPage::~CEdPropertiesPage()
{
	SEvents::GetInstance().UnregisterListener( this );

	if ( m_root )
	{
		m_transactionChanges.Clear(); // Drop pending events to not enable handlers to access a partially-deleted page
		DiscardItem( m_root );
	}
}

void CEdPropertiesPage::SetUndoManager( CEdUndoManager* undoManager ) 
{ 
	m_undoManager = undoManager;
}


void CEdPropertiesPage::DiscardItem( CBasePropItem *item )
{
	ASSERT( item );

	// Reset active item
	if ( GetActiveItem() == item )
	{
		m_activeItem = nullptr;
	}

	delete item;
}

void CEdPropertiesPage::SendPropertyChangedEvent( SPropertyEventData &eventData, Bool closeUndo )
{
	SEvents::GetInstance().DispatchEvent( RED_NAME( EditorPropertyChanging ), CreateEventData( eventData ) );

    if ( m_transactionsInProgress > 0 )
	{
        m_transactionChanges.PushBack( eventData );
	}
    else
	{
		SEvents::GetInstance().DispatchEvent( RED_NAME( EditorPropertyPostChange ), CreateEventData( eventData ) );

		if ( m_undoManager && closeUndo )
		{
			SEvents::GetInstance().DispatchEvent( RED_NAME( EditorPropertyPostTransaction ), CreateEventData( eventData ) );
			CUndoProperty::FinalizeStep( *m_undoManager );
		}
	}
}

CEdPropertiesDrawingStyle::CEdPropertiesDrawingStyle()
	: m_drawFont( wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT ) )
{
	// Setup bold font
	m_boldFont = m_drawFont;
	m_boldFont.SetWeight( wxFONTWEIGHT_BOLD );

	// Button icons
	m_iconAdd = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_ADD") );
	m_iconDelete = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DELETE") );
	m_iconUse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_USE") );
	m_iconFind = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_FIND") );
	m_iconBrowse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_BROWSE") );
	m_iconNew = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_NEW") );
	m_iconClear = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_CLEAR") );
	m_iconEmpty = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_EMPTY") );
	m_iconPick = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
	m_iconInsert = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_INSERT") );
	m_iconDots = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DOTS") );
	m_iconGrab = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_GRAB") );

	// Tree view icons
	m_iconTreeExpand = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_TREE_EXPAND") );
	m_iconTreeCollapse = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_TREE_COLLAPSE") );

	// Checkbox icons
	m_iconCheckOn = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHECK_ON") );
	m_iconCheckOff = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHECK_OFF") );
	m_iconCheckGray = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHECK_GRAY") );
}

void CEdPropertiesPage::RefreshValues()
{
	// Do nothing if we have no root
	if ( !m_root )
	{
		return;
	}

	// Save existing property objects
	TDynArray< STypedObject > rootObjects = GetObjects();

	// Just re-set the same objects (TODO: this is costly, maybe RecursiveGrabValue instead?)
	ForcedSetObjects( rootObjects );

	// Force a refresh
	Refresh( false );
}

TDynArray< STypedObject > CEdPropertiesPage::GetObjects() const
{
	TDynArray< STypedObject > objects;

	if ( m_root )
	{
		for ( Int32 i=0; i< m_root->GetNumObjects(); ++i )
		{
			objects.PushBack( m_root->GetParentObject( i ) );
		}
	}

	return objects;
}


Bool CEdPropertiesPage::AreObjectSetsTheSame( const TDynArray< STypedObject >& s1, const TDynArray< STypedObject >& s2 ) const
{
	if ( s1.Size() != s2.Size() )
	{	// clearly different
		return false;
	}
	else if ( s1.Size() == 0 )
	{	// both sets are empty
		return true;
	}
	else if ( s1.Size() == 1 )
	{	
		STypedObject to1 = s1[0];
		STypedObject to2 = s2[0];

		// Refresh class for CObjects since it can change (f.e. entity editor)
		to1.m_class = to1.m_class->IsA< CObject >() ? static_cast< CObject* >( to1.m_object )->GetClass() : to1.m_class;
		to2.m_class = to2.m_class->IsA< CObject >() ? static_cast< CObject* >( to2.m_object )->GetClass() : to2.m_class;

		return to1 == to2;
	}
	else
	{
		TDynArray< STypedObject > sorted1 = s1;
		TDynArray< STypedObject > sorted2 = s2;

		// Refresh class for CObjects since it can change (f.e. entity editor)
		for ( STypedObject& to : sorted1 )
		{
			to.m_class = to.m_class->IsA< CObject >() ? static_cast< CObject* >( to.m_object )->GetClass() : to.m_class;
		}
		for ( STypedObject& to : sorted2 )
		{
			to.m_class = to.m_class->IsA< CObject >() ? static_cast< CObject* >( to.m_object )->GetClass() : to.m_class;
		}

		// Sort to make sure we're order independent
		Sort( sorted1.Begin(), sorted1.End(), []( const STypedObject& a, const STypedObject& b ) { return a.m_object < b.m_object; } );
		Sort( sorted2.Begin(), sorted2.End(), []( const STypedObject& a, const STypedObject& b ) { return a.m_object < b.m_object; } );

		return sorted1 == sorted2;
	}
}

void CEdPropertiesPage::RememberState()
{
    // Remember current state of expanded nodes 
    if ( m_root )
    {
        if ( CClass *baseClass = m_root->GetRootObject( -1 ).m_class )
        {
            const CName &className = baseClass->GetName();
            if ( !m_expandedItemsByClass.KeyExist( className ) )
            {
                m_expandedItemsByClass.Insert( className, TDynArray< String >() );
            }

            if ( TDynArray< String > *expandedItems = m_expandedItemsByClass.FindPtr( className ) )
            {
                //expandedItems->Clear();
                SavePropItemsExpandState( m_root, *expandedItems );
            }
        }
    }
}

void CEdPropertiesPage::RestoreState()
{
    if ( m_root )
    {
        if ( CClass *baseClass = m_root->GetRootObject( -1 ).m_class )
        {
            const CName &className = baseClass->GetName();
            if ( TDynArray< String > *expandedItems = m_expandedItemsByClass.FindPtr( className ) )
            {
                RestorePropItemsExpandState( m_root, *expandedItems );
            }
        }
    }

	// Redraw
	Refresh( false );
}

void CEdPropertiesPage::ForcedSetRoot( CObjectsRootItem* rootItem )
{
	// Ensure that we are not in the middle of processing messages
	ASSERT( !m_duringMessageSending, TXT("You are going to rebuild the page from an OnPropertyChange handler. This is not supported and will propably crash. FIX IT!") );

	// Ensure that there are no pending events left
	ASSERT( m_transactionChanges.Empty(), TXT("There are some pending property events left, and we are about to rebuild the page. FIX IT!") );

	// === WARNING: ===
	// If you hit one of the above asserts, most probably it's because you are setting an object in reaction to
	// PropertyChanged event from _the same page_ - which is generally a bad thing to do. If you _really_ 
	// have to do this, try to delay a call to SetObject / RefreshValues with RunLater.

	// NOTE: SelectItem( nullptr ) won't help here, because if we are dealing with a situation described
	// above, ending a transaction and sending pending events will recurse us here again. On the other
	// hand, sending a queued events (as it was previously) may solve recursing problem, but is also 
	// a really bad option, since it causes a ton of problems with attached objects lifespan. And we cannot
	// rely on the GC as non-RTTI objects also need to be supported.

	m_transactionChanges.Clear(); // prevent from sending events accessing partially-rebuilt page

	Freeze();

	RememberState();

	// drop the current root
	if ( m_root )
	{
		DiscardItem( m_root );
		m_root = nullptr;
	}

	// create new root
	m_root = rootItem;

	RestoreState();

	Thaw();
}

void CEdPropertiesPage::ForcedSetObjects( const TDynArray< STypedObject >& typedObjects )
{
	CObjectsRootItem* newRoot = new CObjectsRootItem( this, NULL );
	newRoot->SetObjects( typedObjects );
	ForcedSetRoot( newRoot );
}

void CEdPropertiesPage::SetNoObject()
{
	TDynArray< CObject* > objects;
	SetObjects( objects );
}

void CEdPropertiesPage::SetTypedObject( STypedObject typedObject )
{
	TDynArray< STypedObject > oldObjects = GetObjects();

	TDynArray< STypedObject > newObjects;
	if ( typedObject.m_object && typedObject.m_class )
	{
		newObjects.PushBack( typedObject );
	}

	if ( !AreObjectSetsTheSame( oldObjects, newObjects ) )
	{
		ForcedSetObjects( newObjects );
	}
}

void CEdPropertiesPage::SetDayCycleEditSettings( const PropertiesPageDayCycleSettings &newSettings )
{
	if ( newSettings.IsValid() )
	{
		m_settings.m_dayCycleSettings = newSettings;
	}
}

void CEdPropertiesPage::PropertyPreChange( CProperty* property, STypedObject typedObject )
{
	if ( typedObject.m_class == nullptr || typedObject.m_object == nullptr )
	{
		return;
	}

	// Emit event
	SPropertyEventData eventData( this, typedObject, property->GetName() );
	SEvents::GetInstance().DispatchEvent( RED_NAME( EditorPropertyPreChange ), CreateEventData( eventData ) );

	if ( ISerializable* serializable = typedObject.AsSerializable() )
	{
		serializable->OnPropertyPreChange( property );

		if ( m_undoManager )
		{
			CUndoProperty::PrepareStep( *m_undoManager, typedObject, property->GetName() );
		}
	}
}



void CEdPropertiesPage::PropertyPostChange( CProperty* property, STypedObject typedObject )
{
	ASSERT( property );

	if ( typedObject.m_class == nullptr || typedObject.m_object == nullptr )
	{
		return;
	}

	{
		Red::System::ScopedFlag< Bool > sf( m_duringMessageSending = true, false );

		if ( ISerializable* serializable = typedObject.AsSerializable() )
		{
			serializable->OnPropertyPostChange( property );
		}

		// Emit event
		SPropertyEventData eventData( this, typedObject, property->GetName() );
		SendPropertyChangedEvent( eventData );

		// Emit wx event
		wxCommandEvent event( wxEVT_COMMAND_PROPERTY_CHANGED );
		event.SetClientData( &eventData );
		ProcessEvent( event ); // Have to be Process not Post, as we are passing a local stack variable by reference here
	}

	// Redraw browser
	Refresh( false );

	// Sometimes property requires some custom refreshment (e.g. changing one property has influence on 
	// other properties, which have to be refreshed too).
	RefreshCustomProperties( property );
}

CBasePropItem* CEdPropertiesPage::GetItemAtPoint( wxPoint point )
{
	return m_root ? m_root->GetItemAtPoint( point ) : NULL;
}

CBasePropItem* CEdPropertiesPage::GetSiblingItem( CBasePropItem* item, Int32 delta )
{
	// Linearize tree into a item list
	TDynArray< CBasePropItem* > items;
	if ( m_root )
	{
		m_root->Linearize( items );
	}

	TDynArray< CBasePropItem* >::const_iterator itemIter = Find( items.Begin(), items.End(), item );
	if ( itemIter != items.End() )
	{
		// Get sibling item
		const Int32 curIndex = itemIter - items.Begin();
		const Int32 newIndex = curIndex + delta;
		if ( newIndex >= 0 && newIndex < (Int32)items.Size() )
		{
			return items[ newIndex ];
		}
	}

	// Out of range or not found
	return NULL;
}

void CEdPropertiesPage::SelectItem( CBasePropItem* item )
{
	// Deselect selected item
	if ( m_activeItem )
	{
		m_activeItem->CloseControls();
		m_activeItem = nullptr;
	}

	// deselect if we try to select root item
	if ( item == m_root )
	{
		item = nullptr;
	}

	// Set new active item
	m_activeItem = item;

	// Select new item
	if ( m_activeItem )
	{
		m_activeItem->CreateControls();
	}

	Refresh( false );
}

void CEdPropertiesPage::NavigatePrevious()
{
	CBasePropItem* sibling = GetSiblingItem( m_activeItem, -1 );
	if ( sibling )
	{
		SelectItem( sibling );
	}
}

void CEdPropertiesPage::NavigateNext()
{
	CBasePropItem* sibling = GetSiblingItem( m_activeItem, 1 );
	if ( sibling )
	{
		SelectItem( sibling );
	}
}

void CEdPropertiesPage::UpdateLayout( Int32 width, Int32 height )
{
	// Get true window size if not specified
	if ( !width || !height )
	{
		GetClientSize( &width, &height );
	}

	// Update properties layout
	if ( m_root )
	{
		// Prepare layout
		Int32 yOffset = 0;
		m_root->UpdateLayout( yOffset, 0, width );

		// Update window virtual size
		SetVirtualSize( 0, yOffset );

		if( !m_scrollSpeedSet && yOffset > 0 )
		{
			// +1 to include the root item itself
			Uint32 numberOfItems = m_root->GetChildCount( true );

			Int32 scrollRate = yOffset / static_cast< Int32 >( numberOfItems + 1 );

			SetScrollRate( 0, scrollRate );

			m_scrollSpeedSet = true;
		}
	}
}

void CEdPropertiesPage::OnPaint( wxPaintEvent& event )
{
	wxBufferedPaintDC dc( this );
	DoPrepareDC( dc );

	// Clear background
	dc.SetFont( GetStyle().m_drawFont );
	if( IsEnabled() )
	{
		dc.SetBackground( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) ) );
	}
	else
	{
		dc.SetBackground( wxBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) ) );
	}
	dc.Clear();

	// Update display layout
	UpdateLayout();

	// Draw tree item
	if ( m_root )
	{
		m_root->DrawLayout( dc );
	}

	// Draw property grid tooltip
	DrawTooltip( dc );
}

void CEdPropertiesPage::OnSize( wxSizeEvent& event )
{
	if ( m_activeItem )
	{
		// Deactivate item
		m_activeItem->CloseControls();

		// Update display layout
		UpdateLayout();

		// Activate item
		m_activeItem->CreateControls();

		// Redraw everything
		Refresh( true, NULL );
	}
}

void CEdPropertiesPage::OnScroll(wxScrollWinEvent &event)
{
	wxScrolledWindow::HandleOnScroll(event);
}

void CEdPropertiesPage::OnEraseBackground( wxEraseEvent& event )
{

}

void CEdPropertiesPage::OnKeyDown( wxKeyEvent& event )
{
	// Navigation up
	if ( event.GetKeyCode() == WXK_UP )
	{
		NavigatePrevious();
		return;
	}

	// Navigation down
	if ( event.GetKeyCode() == WXK_DOWN )
	{
		NavigateNext();
		return;
	}

	// Pass event to property
	if ( m_activeItem )
	{
   		m_activeItem->OnBrowserKeyDown( event );
	}
}

void CEdPropertiesPage::OnMouseEvents( wxMouseEvent& event )
{
	// Translate mouse position
	wxPoint mousePos = CalcUnscrolledPosition( event.GetPosition() );

	if ( event.LeftDown() )
	{
		// Get active button
		CPropertyButton* but = NULL;
		if ( m_activeItem )
		{
			but = m_activeItem->GetButtonAtPoint( mousePos );
		}

		// New button highlighted
		if ( but != m_activeButton )
		{
			m_activeButton = but;
			Refresh( false );
		}

		// Button clicked
		if ( m_activeButton )
		{
			m_activeButton->Clicked();
			PostMessage( (HWND)GetHWND(), WM_LBUTTONUP, 0, 0 );
			return;
		}
	}

	if ( event.LeftDown() || event.RightDown() )
	{
		if( !m_mouseGrab )
		{
			m_mouseGrab = true;
			SetCapture( (HWND) GetHWND() );
		}
		// Splitter dragging
		if ( abs( (Int32)( mousePos.x - m_splitter) ) < 3 )
		{
			if ( m_activeItem )
			{
				m_activeItem->CloseControls();
				m_activeItem = nullptr;
			}
			m_splitterDrag = true;

			return;
		}

		// Get clicked item
		CBasePropItem* clicked = GetItemAtPoint( mousePos );
		if ( clicked && clicked != m_activeItem )
		{
			if ( m_activeItem )
			{
				m_activeItem->CloseControls();
				m_activeItem = nullptr;
			}

			// obtain clicked item once again in case ClosingControls for previously active item caused properties change
			clicked = GetItemAtPoint( mousePos );

			SetFocus();
			SelectItem( clicked );
			Refresh( false );
		}
	}

	if ( event.LeftUp() )
	{
		if( m_mouseGrab )
		{
			m_mouseGrab = false;
			ReleaseCapture();
		}

		if ( m_splitterDrag )
		{
			if ( m_activeItem )
			{
				m_activeItem->CreateControls();
			}
			m_splitterDrag = false;

			return;
		}
	}

	// Button selection
	if ( event.GetEventType() == wxEVT_MOTION )
	{
		// Drag splitter
		if ( m_splitterDrag )
		{
			m_splitter = Clamp( mousePos.x, 50, GetClientSize().x - 50 );
			UpdateLayout();
			Refresh( false );
			return;
		}

		// Get active button
		CPropertyButton* but = NULL;
		if ( m_activeItem )
		{
			but = m_activeItem->GetButtonAtPoint( mousePos );
		}

		// New button highlighted
		if ( but != m_activeButton )
		{
			m_activeButton = but;
			Refresh( false );
		}

		// Tooltips
		m_lastMousePos = mousePos;
		TooltipMouseMoved();

		// Status bar
		StatusBarMouseMoved();
	}

	// Eat doubleclick events if we have an active button
	if ( event.LeftDClick() && m_activeButton )
	{
		return;
	}

	// Just disable the tooltips, clear status bar and refresh the window
	if ( event.Leaving() )
	{
		ClearStatusBar();
		m_tooltipAvaiable = false;
		Refresh( true, NULL );
	}

	if ( event.Entering() )
	{
		m_tooltipMouseNotMoved = false;
	}

	// Mouse scroll handle
	if ( !(m_activeItem && m_activeItem->ShouldSuppressMouseScrollEvent( event )) )
	{
		if ( event.GetWheelRotation() > 0 )
		{
			wxScrollWinEvent scrollEvent( wxEVT_SCROLLWIN_LINEUP, wxVERTICAL );
			OnScroll( scrollEvent );
		}
		if ( event.GetWheelRotation() < 0 )
		{
			wxScrollWinEvent scrollEvent( wxEVT_SCROLLWIN_LINEDOWN, wxVERTICAL );
			OnScroll( scrollEvent );
		}
	}

	// Pass mouse event to active item
	if ( m_activeItem )
	{
		m_activeItem->OnBrowserMouseEvent( event );
	}
}

void CEdPropertiesPage::OnSetCursor( wxSetCursorEvent& event )
{
	if ( abs( (Int32)(event.GetX() - m_splitter) ) < 3 )
	{
		event.SetCursor( wxCURSOR_SIZEWE );
	}
}

void CEdPropertiesPage::TooltipMouseMoved()
{
	m_tooltipAvaiable = false;

	CPropertyItem *basePropItem = dynamic_cast< CPropertyItem * >( GetItemAtPoint( m_lastMousePos ) );
	m_tooltipCurrentProperty = NULL;
	if ( basePropItem )
	{
		m_tooltipCurrentProperty = basePropItem->GetProperty();
		if ( m_tooltipCurrentProperty )
		{
			m_tooltipText = m_tooltipCurrentProperty->GetHint();
			m_tooltipPos = m_lastMousePos + wxPoint(0, -12);
			m_tooltipAvaiable = true;
		}
	}

	if ( !m_tooltipAvaiable )
	{
		Refresh( true, NULL );
	}
	else
	{
		TooltipProcess();
		Refresh( true, NULL );
	}
}

void CEdPropertiesPage::ClearStatusBar()
{
	if (m_statusBar) m_statusBar->SetLabel(wxEmptyString);
}

void CEdPropertiesPage::StatusBarMouseMoved()
{
	if (m_statusBar)
	{
		if ( !m_tooltipAvaiable )
		{
			if ( String(m_statusBar->GetLabel()) != String::EMPTY )
				ClearStatusBar();
		}
		else
		{
			if ( String(m_statusBar->GetLabel()) != m_tooltipText )
			{
				m_statusBar->SetLabel( m_tooltipText.AsChar() );
			}
		}
	}
}

void CEdPropertiesPage::DrawTooltip( wxDC &dc )
{
	if ( m_tooltipEnabled && m_tooltipAvaiable && m_tooltipMouseNotMoved )
	{
		const Int32 propertyPageWidth = GetClientSize().GetWidth();
		DrawTooltip(m_tooltipPos, m_tooltipText, dc, propertyPageWidth);
	}
}

void CEdPropertiesPage::DrawTooltip( wxPoint point, String text, wxDC &dc, Int32 maxWidth /* = 0 */ )
{
	// If there is no tooltip, just exit
	if ( text == String() ) return;

	// If tooltip is too long, divide it into multiple lines
	const Uint32 TooltipDivideWidth = 30;
	if (text.GetLength() > TooltipDivideWidth)
	{
		String tmpText(text);
		int k = 0;
		text.Clear();

		const unsigned int TextLength = tmpText.GetLength();

		for ( unsigned int i=0; i < TextLength; i++ )
		{
			if (++k > 30 && tmpText[i] == L' ')
			{
				text += TXT("\n");
				k = 0;
			}
			else
			{
				text += String::Chr(tmpText[i]);
			}
		}
	}

	dc.SetBrush(wxBrush(wxColour(240, 240, 0, 0)));
	wxSize extent = dc.GetMultiLineTextExtent(text.AsChar());
	extent.IncBy(10,4);
	if ( maxWidth > 0 )
	{
		// clip coordinates, so the tooltip is always visible
		const Int32 TOOLTIP_HORIZONTAL_CLIP = extent.GetX();
		const Int32 TOOLTIP_VERTICAL_CLIP = extent.GetY();

		if (point.y < TOOLTIP_VERTICAL_CLIP )
		{
			point.y = TOOLTIP_VERTICAL_CLIP;
		}
		if (point.x > maxWidth - TOOLTIP_HORIZONTAL_CLIP )
		{
			point.x = maxWidth - TOOLTIP_HORIZONTAL_CLIP;
		}
	}
	dc.DrawRoundedRectangle( point - wxPoint(5, 2), extent, 1.0 );

	// Draw tooltip text
	dc.DrawText( text.AsChar(), point );
}

void CEdPropertiesPage::OnTimerTooltip( wxTimerEvent& event )
{
	// CHUJ CI W DUPE Z TYM TIMEREM KURWA !
	/*if ( TooltipProcess() )
	{
		Refresh( true, NULL );
	}*/
}

Bool CEdPropertiesPage::TooltipProcess()
{
	if ( m_tooltipMouseNotMoved && m_tooltipLastProperty == m_tooltipCurrentProperty )
	{
	}
	else
	{
		m_tooltipLastProperty = m_tooltipCurrentProperty;

		if ( m_beforeLastMousePos == m_lastMousePos ) // cursor not moved
		{
			if ( (Red::System::Clock::GetInstance().GetTimer().GetSeconds() - m_tooltipTime > TooltipDelay) )
			{
				m_tooltipMouseNotMoved = true;
			}
		}
		else
		{
			m_tooltipTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
			m_tooltipMouseNotMoved = false;
		}
	}

	m_beforeLastMousePos = m_lastMousePos;
	return m_tooltipMouseNotMoved;
}

void CEdPropertiesPage::SetStatusBar( wxStaticText *statusBar )
{
	m_statusBar = statusBar;
}

wxDragResult CEdPropertiesPage::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    const TDynArray<CResource*> &resources = GetDraggedResources();

    if ( m_root == NULL )
        return wxDragNone;
    
    Int32 cnt = m_root->GetNumObjects();
    if( cnt )
    {
        for( Int32 iObject = 0; iObject < cnt; ++iObject )
        {
			if ( CSkeletalAnimationSet* animSet = m_root->GetRootObject( iObject ).As< CSkeletalAnimationSet >(  ) )
            {
                for( Uint32 iRes = 0; iRes < resources.Size(); ++iRes )
                    if( resources[ iRes ]->IsA< CSkeletalAnimation >() )
                        return def;
            }
        }
    }

    CBasePropItem *hoveredItem = GetItemAtPoint(wxPoint(x,y));
    if (hoveredItem && hoveredItem->CanUseSelectedResource())
    {
        if (hoveredItem != GetActiveItem())
            SelectItem( hoveredItem );
        return def;
    }

    return wxDragNone;
}

Bool CEdPropertiesPage::OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources )
{
    CPropertyTransaction transaction( *this );

	Int32 cnt = m_root->GetNumObjects();
	if( cnt )
	{
		for( Int32 i=0; i<cnt; i++ )
		{
			if ( CSkeletalAnimationSet* animSet = m_root->GetRootObject( i ).As< CSkeletalAnimationSet >() )
			{
				for( Uint32 j=0; j<resources.Size(); j++ )
					if( resources[ j ]->IsA< CSkeletalAnimation >() )
					{
						CSkeletalAnimation *skeletalAnim = ( CSkeletalAnimation* )resources[ j ];
						animSet->AddAnimation( skeletalAnim );
						animSet->MarkModified();
					}
			}
		}
	}

    CBasePropItem *hoveredItem = GetItemAtPoint(wxPoint(x,y));
    if (hoveredItem)
    {
        hoveredItem->UseSelectedResource();
    }

	RefreshValues();
/*
	CBasePropItem *basePropItem = GetItemAtPoint( m_lastMousePos );
	if( basePropItem )
	{
		
	}
*/
 	return true;
}

TEdShortcutArray* CEdPropertiesPage::GetAccelerators()
{
	if (m_shortcuts.Empty())
	{
		m_shortcuts.PushBack(SEdShortcut(TXT("PropertyPage\\Copy"),wxAcceleratorEntry(wxACCEL_CTRL,'C', XRCID( "editCopy" ))) );
		m_shortcuts.PushBack(SEdShortcut(TXT("PropertyPage\\Paste"),wxAcceleratorEntry(wxACCEL_CTRL,'V', XRCID( "editPaste" ))) );
		m_shortcuts.PushBack(SEdShortcut(TXT("PropertyPage\\Delete"),wxAcceleratorEntry(wxACCEL_NORMAL,WXK_DELETE, XRCID( "editDelete" ))) );
	}

	return &m_shortcuts;
}

#ifndef NO_NEW_CLIPBOARD_STUFF
static void CopyPropertyItemToClipboard( CBasePropItem* propItem )
{
	if ( CPropertyItem *propertyItem = dynamic_cast< CPropertyItem* >( propItem ) )
	{
		TDynArray< Uint8 > buffer;
		CProperty* property = dynamic_cast< CProperty* >( propertyItem->GetProperty() );
		buffer.PushBack( 42 );
		buffer.PushBack( 32 ); // magic numbers
		buffer.PushBack( 22 );
		if ( property )
		{
			STypedObject obj = propertyItem->GetParentObject( 0 );
			IRTTIType* type = property->GetType();
			CMemoryFileWriter writer( buffer );
			type->Serialize( writer, property->GetOffsetPtr( obj.m_object ) );
		}
		else if ( propertyItem->GetArrayIndex() != -1 )
		{
			CPropertyItemArray* parentItemArray = static_cast< CPropertyItemArray* >( propertyItem->GetParent() );
			void* obj = parentItemArray->GetArrayElement( propertyItem->GetArrayIndex(), 0 );
			CRTTIArrayType* parentArrayType = static_cast< CRTTIArrayType* >( parentItemArray->GetPropertyType() );
			IRTTIType* type = parentArrayType->GetInnerType();
			CMemoryFileWriter writer( buffer );
			type->Serialize( writer, obj );
		}
		if ( buffer.Size() > 3 )
		{
			GClipboard->Copy( buffer );
		}
	}
	else if ( CBaseGroupItem *groupItem = dynamic_cast< CBaseGroupItem* >( propItem ) )
	{
		CopyPropertyItemToClipboard( propItem->GetParent() );
	}
}

static void PastePropertyItemFromClipboard( CBasePropItem* propItem )
{
	if ( CPropertyItem *propertyItem = dynamic_cast< CPropertyItem* >( propItem ) )
	{
		CProperty* property = dynamic_cast< CProperty* >( propertyItem->GetProperty() );

		// Get data from clipboard
		TDynArray< Uint8 > buffer;
		if ( !GClipboard->Paste( buffer ) )
		{
			return;
		}

		// Check for the magic numbers
		if ( buffer[0] != 42 || buffer[1] != 32 || buffer[2] != 22 )
		{
			return;
		}
		buffer.Erase( buffer.Begin(), buffer.Begin() + 3 );
		
		if ( property )
		{
			STypedObject obj = propertyItem->GetParentObject( 0 );
			IRTTIType* type = property->GetType();
			CMemoryFileReader reader( buffer, 0 );
			type->Serialize( reader, property->GetOffsetPtr( obj.m_object ) );
		}
		else if ( propertyItem->GetArrayIndex() != -1 )
		{
			CPropertyItemArray* parentItemArray = static_cast< CPropertyItemArray* >( propertyItem->GetParent() );
			void* obj = parentItemArray->GetArrayElement( propertyItem->GetArrayIndex(), 0 );
			CRTTIArrayType* parentArrayType = static_cast< CRTTIArrayType* >( parentItemArray->GetPropertyType() );
			IRTTIType* type = parentArrayType->GetInnerType();
			CMemoryFileReader reader( buffer, 0 );
			type->Serialize( reader, obj );
		}
	}
	else if ( CBaseGroupItem *groupItem = dynamic_cast< CBaseGroupItem* >( propItem ) )
	{
		PastePropertyItemFromClipboard( propItem->GetParent() );
	}
}
#endif

void CEdPropertiesPage::OnEditCopyViaRTTI( wxCommandEvent& event )
{
	if ( CBasePropItem *basePropItem = dynamic_cast< CBasePropItem * >( m_activeItem ) )
	{
		const Int32 numObjects = basePropItem->GetNumObjects();
		STypedObject object = basePropItem->GetParentObject( 0 );

		// Serialize property value to variant

		CVariant variant;
		variant.Init( object.m_class->GetName(), object.m_object );

		TDynArray< Uint8 > buffer;
		CMemoryFileWriter writer( buffer );
		variant.Serialize( writer );

		// Put serialized variant into clipboard

		if ( wxTheClipboard->Open() )
		{
			wxTheClipboard->SetData( new CClipboardData( TXT("CVariant"), buffer, true ) );
			wxTheClipboard->Close();
		}
	}
}

void CEdPropertiesPage::OnEditCopy( wxCommandEvent& event )
{
#ifndef NO_NEW_CLIPBOARD_STUFF
	CopyPropertyItemToClipboard( m_activeItem );
#else
	if ( CBasePropItem *basePropItem = dynamic_cast< CBasePropItem * >( m_activeItem ) )
	{
		STypedObject object = basePropItem->GetParentObject( 0 );

		// Create writer
        CXMLWriter writer;
        writer.BeginNode( TXT("CopyToClipboard") );
        String value = object.m_class->GetName().AsString();
        writer.Attribute( TXT("type"), value );

        if ( !basePropItem->SerializeXML( writer ) )
        {
            ERR_EDITOR( TXT("Unable to serialize data to xml. See the clipboard for more info.") );

            // Do not stop, let the corrupted xml content go to the clipboard
            //return;
        }

        writer.EndNode();
        writer.Flush();

        // Open clipboard
        if ( wxTheClipboard->Open() )
        {
            wxTheClipboard->SetData( new wxTextDataObject( writer.GetContent().AsChar() ) );
            wxTheClipboard->Close();
			wxTheClipboard->Flush();
        }
	}
#endif
}

void CEdPropertiesPage::OnEditPaste( wxCommandEvent& event )
{
#ifndef NO_NEW_CLIPBOARD_STUFF
	PastePropertyItemFromClipboard( m_activeItem );
	RefreshValues();
#else

	// Extract data from clipboard (either stored as variant or text)

	CVariant variant;
	String clipboardContent = String::EMPTY;

	if ( wxTheClipboard->Open())
	{
		if ( wxTheClipboard->IsSupported( wxDF_TEXT ) )
		{
			wxTextDataObject data;
			wxTheClipboard->GetData( data );
			clipboardContent = data.GetText().wc_str();
			if ( clipboardContent.Empty() )
			{
				return;
			}
		} 
		else
		{
			CClipboardData data( TXT("CVariant") );
			if ( wxTheClipboard->IsSupported( data.GetDataFormat() ) )
			{
				if ( wxTheClipboard->GetData( data ) )
				{
					const TDynArray< Uint8 >& buffer = data.GetData();

					CMemoryFileReader reader( buffer, 0 );
					variant.Serialize( reader );
					if ( !variant.IsValid() )
					{
						return;
					}
				}
			}
		}
		wxTheClipboard->Close();
	}

	// Copy data into an object
	if ( CBasePropItem *basePropItem = dynamic_cast< CBasePropItem * >( m_activeItem ) )
	{
		STypedObject object = basePropItem->GetParentObject( 0 );

		if ( variant.IsValid() )
		{
			if ( !SRTTI::GetInstance().CanCast( variant.GetRTTIType(), object.m_class ) )
			{
				return;
			}
			variant.GetRTTIType()->Copy( object.m_object, variant.GetData() );
			RefreshValues();
		}
		else
		{
			// Create reader
			CXMLReader reader( clipboardContent );
			if ( !reader.BeginNode( TXT("CopyToClipboard") ) )
			{
				ERR_EDITOR( TXT("Unable to serialize the object. The data in the clipboard is improper for paste operation.") );
				return;
			}
			String value;
			reader.Attribute( TXT("type"), value );
			if ( value != object.m_class->GetName().AsString() )
			{
				ERR_EDITOR( TXT("Unable to serialize the object. The data format in the clipboard does not match selected object.") );
				return;
			}

			if ( !basePropItem->SerializeXML( reader ) )
			{
				ERR_EDITOR( TXT("Unable to parse the xml data from the clipboard.") );
				ERR_EDITOR( TXT("It's possible that some data has been pasted correctly but not all! Make sure the object you selected has valid data or recreate the object.") );
			}
			else
			{
				reader.EndNode();
			}
		}
	}
#endif
}

void CEdPropertiesPage::OnEditDelete( wxCommandEvent& event )
{
	wxWindow* win = wxWindow::FindFocus();
	::SendMessage( (HWND) win->GetHandle(), WM_KEYDOWN, VK_DELETE, 0 );
}

void CEdPropertiesPage::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == RED_NAME( ScriptCompilationStart ) )
	{
		auto objects = GetObjects();
		for ( STypedObject& obj : objects )
		{
			if ( CObject* asObject = obj.AsObject() )
			{
				m_scriptCompilationLatch.PushBack( asObject );
			}
		}

		SetNoObject();
	}
	else if ( name == RED_NAME( ScriptCompilationEnd ) )
	{
		TDynArray< STypedObject > objects;
		for ( THandle< CObject > obj : m_scriptCompilationLatch )
		{
			if ( obj )
			{ // restore object only if it survived script recompilation
				objects.PushBack( STypedObject( obj ) );
			}
		}

		m_scriptCompilationLatch.Clear();
		ForcedSetObjects( objects );
	}

	else if ( name == RED_NAME( SelectionChanged ) )
	{
		if ( m_activeItem )
		{
			m_activeItem->DestroyButtons();
			SelectItem( m_activeItem );
		}
	}
	else if ( name == RED_NAME( SimpleCurveDisplayedTimeChanged ) )
	{
		Refresh( false );
	}
	else if ( name == RED_NAME( OnObjectDiscarded ) )
	{
		CObject *discardedObject = GetEventData< CObject* >( data );
		if ( m_root )
		{
			m_root->OnObjectDiscarded( discardedObject );
		}
	}
	else if ( name == RED_NAME( RefreshPropertiesPage ) )
	{
		static Bool whileRefreshing = false;
		if( !whileRefreshing ) 
		{
			whileRefreshing = true;
			RefreshValues();
			whileRefreshing = false;
		}
	}
	else if ( name == RED_NAME( EditorPreUndoStep ) )
	{
		if ( GetEventData< CEdUndoManager* >( data ) == m_undoManager )
		{
			SelectItem( nullptr );
		}
	}
	else if ( name == RED_NAME( EditorPostUndoStep ) )
	{
		if ( GetEventData< CEdUndoManager* >( data ) == m_undoManager )
		{
			RefreshValues();
		}
	}
}

// Remember, which properties are expanded.
void CEdPropertiesPage::SavePropItemsExpandState( CBasePropItem *root, TDynArray< String >& expandedItems )
{
    if ( root )
    {
        //CBaseGroupItem *groupItem = dynamic_cast< CBaseGroupItem * >( root );
        if ( root->IsExpanded() )
        {
            expandedItems.PushBackUnique( root->GetCaption() );
        }
        else
        {
            expandedItems.Remove( root->GetCaption() );
        }
    	
        const TDynArray< CBasePropItem*> children = root->GetChildren();
        for ( Uint32 i = 0; i < children.Size(); i++ )
        {
            SavePropItemsExpandState( children[i], expandedItems );
        }
    }
}

// Expand previously expanded properties.
void CEdPropertiesPage::RestorePropItemsExpandState( CBasePropItem *root, const TDynArray< String >& expandedItems )
{
    if ( root )
    {
        if ( !root->IsExpanded() )
        {
            if ( expandedItems.Exist( root->GetCaption() ) )
            {
                root->Expand();
            }
        }

        const TDynArray< CBasePropItem*> children = root->GetChildren();
        for ( Uint32 i = 0; i < children.Size(); i++ )
        {
            RestorePropItemsExpandState( children[i], expandedItems );
        }
    }
}

void CEdPropertiesPage::RefreshCustomProperties( CProperty* property )
{
	// Here we decide if the changed property requires refreshing of all other properties
	// For now, we refresh all properties if the modified property isn't an array.
	// In the future, we can (or we should) make a better decision, for example a property
	// could have a variable deciding about the necessity of refreshing.
	if ( property->GetType()->GetType() != RT_Array )
	{
		// Send the refresh command from properties browser (in fact properties page) to 
		// the all editors that are interested in receiving such event.
		wxCommandEvent event( wxEVT_COMMAND_REFRESH_PROPERTIES );
		wxPostEvent( this, event );
	}
}

void CEdPropertiesPage::SetReadOnly( bool readOnly )
{
	m_settings.m_readOnly = readOnly;
	Refresh( false );
}

Bool CEdPropertiesPage::IsReadOnly() const
{
	return m_settings.m_readOnly;
}

Bool CEdPropertiesPage::IsReadOnly( CProperty* prop ) const
{
	return IsReadOnly() || ( m_settings.m_noReadOnlyProperties.Size() == 0 ? false : !m_settings.m_noReadOnlyProperties.Exist( prop->GetName() ) );
}

void CEdPropertiesPage::SetAllowGrabbing( Bool allow )
{
	m_settings.m_allowGrabbing = allow;
}

void CEdPropertiesPage::ClearPropertyStyles()
{
	m_propertyStyles.Clear();
	RefreshLater( this );
}

void CEdPropertiesPage::SetPropertyStyle( const CName& propertyName, const SEdPropertiesPagePropertyStyle& style )
{
	m_propertyStyles[propertyName] = style;
	RefreshLater( this );
}

const SEdPropertiesPagePropertyStyle* CEdPropertiesPage::GetPropertyStyle( const CName& propertyName ) const
{
	return m_propertyStyles.FindPtr( propertyName );
}

void CEdPropertiesPage::RemovePropertyStyle( const CName& propertyName )
{
	m_propertyStyles.Erase( propertyName );
	RefreshLater( this );
}

STypedObject CEdPropertiesPage::ConstructTypedObject( CClass* cls, void* obj ) const
{
	if ( cls->IsA< CCurveEntity >() ) // Show curve owner properties when curve is selected (curve itself is just a temporary entity created for the purpose of editing the curve)
	{
		CCurveEntity* curveEntity = cls->CastTo< CCurveEntity >( obj );
		SMultiCurve* curve = curveEntity->GetCurve();
		if ( CNode* curveParent = curve->GetParent() )
		{
			if ( CComponent* curveParentAsComponent = Cast<CComponent>( curveParent ) )
			{
				curveParent = curveParentAsComponent->GetEntity();
			}

			return STypedObject( curveParent, curveParent->GetClass() );
		}
	}

	return STypedObject( obj, cls );
}
