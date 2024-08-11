/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "propertyButton.h"
#include "basePropItem.h"
#include "baseGroupItem.h"
#include "classGroupItem.h"
#include "propertyItem.h"
#include "propertyItemClass.h"
#include "propertyItemArray.h"
#include "propertyItemPointer.h"
#include "propertyItemSimpleCurve.h"
#include "objectsRootItem.h"
#include "componentGroupItem.h"
#include "materialPropItem.h"
#include "materialGroupItem.h"
#include "materialInstanceGroupItem.h"
#include "matEngineValueGroupItem.h"
#include "shortcut.h"

/// Event
wxDECLARE_EVENT( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEvent );
wxDECLARE_EVENT( wxEVT_COMMAND_REFRESH_PROPERTIES, wxCommandEvent );
wxDECLARE_EVENT( wxEVT_COMMAND_PROPERTY_SELECTED, wxCommandEvent );

class CEdPropertiesDrawingStyle
{
public:
	wxFont				m_drawFont;
	wxFont				m_boldFont;
	wxBitmap			m_iconAdd;
	wxBitmap			m_iconDelete;
	wxBitmap			m_iconUse;
	wxBitmap			m_iconFind;
	wxBitmap			m_iconBrowse;
	wxBitmap			m_iconNew;
	wxBitmap			m_iconClear;
	wxBitmap			m_iconEmpty;
	wxBitmap			m_iconPick;
	wxBitmap			m_iconInsert;
	wxBitmap			m_iconDots;
	wxBitmap			m_iconGrab;
	wxBitmap			m_iconTreeExpand;
	wxBitmap			m_iconTreeCollapse;
	wxBitmap			m_iconCheckOn;
	wxBitmap			m_iconCheckOff;
	wxBitmap			m_iconCheckGray;

	CEdPropertiesDrawingStyle();
};

struct PropertiesPageDayCycleSettings
{
	Float m_timeMin;
	Float m_timeMax;
	Float m_valueScale;
	
public:
	PropertiesPageDayCycleSettings ();

public:
	bool IsValid() const;
	void Reset();
};

struct PropertiesPageSettings
{
	Bool		m_showEntityComponents;
	Bool		m_showDynamicProperties;
	Bool		m_autoExpandGroups;
	PropertiesPageDayCycleSettings	m_dayCycleSettings;
	TDynArray< CName >				m_noReadOnlyProperties;
	Bool		m_readOnly;
	Bool		m_allowGrabbing;

	PropertiesPageSettings()
		: m_showEntityComponents( true )
		, m_showDynamicProperties( true )
		, m_autoExpandGroups( false )
		, m_readOnly( false )
		, m_allowGrabbing( true )
	{};
};

class CEdPropertiesPage : public wxScrolledWindow, public CDropTarget, public IEdEventListener
{
    friend class CPropertyTransaction;

	DECLARE_EVENT_TABLE();

public:
    struct SPropertyEventData
    {
        CEdPropertiesPage   *m_page;
        STypedObject         m_typedObject;
        CName                m_propertyName;

        SPropertyEventData()
            : m_page( nullptr )
        {}

        SPropertyEventData( CEdPropertiesPage *page, STypedObject object, CName name )
            : m_page( page ), m_typedObject( object ), m_propertyName( name )
        {}
    };

public:
	CEdPropertiesPage( wxWindow* parent, const PropertiesPageSettings& settings, CEdUndoManager* undoManager );

	~CEdPropertiesPage();

	void SetUndoManager( CEdUndoManager* undoManager );

	const CEdPropertiesDrawingStyle& GetStyle() const { return m_style; }

	CBasePropItem* GetRootItem() const { return m_root; }

	CBasePropItem* GetActiveItem() const { return m_activeItem; }

	CPropertyButton* GetActiveButton() const { return m_activeButton; }

	Uint32 GetSplitterPos() const { return m_splitter; }

	const PropertiesPageSettings& GetSettings() const { return m_settings; }

	void RefreshValues();

	void SetNoObject();

	void SetTypedObject( STypedObject typedObj );

	//! You can pass any RTTI type object here (not only CObjects)
	template < class T > void SetObjects( const TDynArray< T* > &objects );

	//! You can pass any RTTI type object here (not only CObjects)
	template < class T > void SetObject( T* object );

	//! Pass nullptr to remove selection
	void SelectItem( CBasePropItem* item );

	void DiscardItem( CBasePropItem *item );


	// Set a CObject that will be used as a parent object when instantiating a CObject property and no CObject parent item can be found
	// (e.g. if the root object is not CObject).
	void SetDefaultParentObject( CObject* obj ) { m_defaultParentObject = obj; }
	CObject* GetDefaultParentObject() const { return m_defaultParentObject; }


	virtual void PropertyPreChange( CProperty* property, STypedObject object );
	virtual void PropertyPostChange( CProperty* property, STypedObject object );

	const PropertiesPageDayCycleSettings& GetDayCycleEditSettings() const { return m_settings.m_dayCycleSettings; }
	void SetDayCycleEditSettings( const PropertiesPageDayCycleSettings &newSettings );

	TEdShortcutArray* GetAccelerators();

	virtual class CEdEffectEditorProperties* QueryEffectEditorProperties() { return nullptr; }
	virtual class CEdBehaviorEditorProperties* QueryBehaviorEditorProperties() { return nullptr; }
	virtual class CEdEntitySlotProperties* QueryEntitySlotProperties() { return nullptr; }
	virtual class CEdComponentProperties* QueryComponentProperties() { return nullptr; }

	void UpdateLayout( Int32 width=0, Int32 height=0 );

	void SetReadOnly( bool readOnly );
	Bool IsReadOnly() const;
	Bool IsReadOnly( CProperty* prop ) const;

	void SetAllowGrabbing( Bool allow );
	Bool AllowGrabbing() const { return m_settings.m_allowGrabbing; }

	void ClearPropertyStyles();
	void SetPropertyStyle( const CName& propertyName, const SEdPropertiesPagePropertyStyle& style );
	const SEdPropertiesPagePropertyStyle* GetPropertyStyle( const CName& propertyName ) const;
	Bool HasPropertyStyle( const CName& propertyName ) const { return GetPropertyStyle( propertyName ) != NULL; }
	void RemovePropertyStyle( const CName& propertyName );

	void SetStatusBar( wxStaticText *statusBar );
	Bool IsEntityEditorOwner() { return m_isEntityEditorOwner; }
	void SetEntityEditorAsOwner() { m_isEntityEditorOwner = true; }

private:
	friend class CBasePropItem;
	void OnEditCopy( wxCommandEvent& event );
	void OnEditCopyViaRTTI( wxCommandEvent& event );
	void OnEditPaste( wxCommandEvent& event );
	void OnEditDelete( wxCommandEvent& event );
    void ClearActiveButton() { m_activeButton = nullptr; }

	CBasePropItem* GetItemAtPoint( wxPoint point );
	CBasePropItem* GetSiblingItem( CBasePropItem* item, Int32 delta );
	void NavigatePrevious();
	void NavigateNext();
	void RefreshCustomProperties( CProperty* property );
	void SavePropItemsExpandState( CBasePropItem *root, TDynArray< String >& expandedItems );
	void RestorePropItemsExpandState( CBasePropItem *root, const TDynArray< String >& expandedItems );

	void OnPaint( wxPaintEvent& event );
	void OnSize( wxSizeEvent& event );
	void OnEraseBackground( wxEraseEvent& event );
	void OnKeyDown( wxKeyEvent& event );
	void OnMouseEvents( wxMouseEvent& event );
	void OnSetCursor( wxSetCursorEvent& event );
	void OnScroll( wxScrollWinEvent &event );

    void SendPropertyChangedEvent( SPropertyEventData &eventData, Bool closeUndo = true );

	// Tooltips
	void DrawTooltip( wxPoint point, String text, wxDC &dc, Int32 maxWidth = 0 );
	void DrawTooltip( wxDC &dc );
	void OnTimerTooltip( wxTimerEvent& event ); // Method called by wxTimer (m_tooltipTimer)
	void TooltipMouseMoved();
	Bool TooltipProcess();

    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
	Bool OnDropResources( wxCoord x, wxCoord y, TDynArray< CResource* > &resources );

	void DispatchEditorEvent( const CName& name, IEdEventData* data );

	// Status bar
	void ClearStatusBar();
	void StatusBarMouseMoved();

	Bool AreObjectSetsTheSame( const TDynArray< STypedObject >& s1, const TDynArray< STypedObject >& s2 ) const;
	TDynArray< STypedObject > GetObjects() const;
	STypedObject ConstructTypedObject( CClass* cls, void* obj ) const;

	void RememberState();
	void RestoreState();
	// (re)sets the objects regardless if they differ from the previous ones or not
	void ForcedSetObjects( const TDynArray< STypedObject >& typedObjects );

protected:
	void ForcedSetRoot( CObjectsRootItem* rootItem );

private:
    Int32									m_transactionsInProgress;
    TDynArray<SPropertyEventData>			m_transactionChanges;
	THashMap< CName, TDynArray< String > >	m_expandedItemsByClass;
	Bool									m_isEntityEditorOwner;
	Bool									m_duringMessageSending;				// Debug variable - use only in asserts!
	TDynArray< THandle< CObject > >			m_scriptCompilationLatch;

	CEdPropertiesDrawingStyle				m_style;
	CObjectsRootItem*						m_root;
	CBasePropItem*							m_activeItem;
	CPropertyButton*						m_activeButton;
	Uint32									m_splitter;
	Bool									m_splitterDrag;
	Bool									m_mouseGrab;
	Bool									m_scrollSpeedSet;
	TEdShortcutArray						m_shortcuts;
	PropertiesPageSettings					m_settings;
	THashMap< CName, SEdPropertiesPagePropertyStyle >	m_propertyStyles;

	CObject*								m_defaultParentObject;				// When creating a new CObject for a property value, use this if no other valid parent is found.

	wxStaticText*		m_statusBar;

	Bool				m_tooltipAvaiable;
    Bool				m_tooltipMouseNotMoved;
	Bool				m_tooltipEnabled;
	String				m_tooltipText;				// Tooltip text
	wxPoint				m_tooltipPos;				// The position of tooltip
	CEdTimer			m_tooltipTimer;				// Timer for refreshing tooltip
	wxPoint				m_lastMousePos;
	wxPoint				m_beforeLastMousePos;
	Double				m_tooltipTime;
	const CProperty	*	m_tooltipCurrentProperty;
	const CProperty *	m_tooltipLastProperty;

	CEdUndoManager*		m_undoManager;
};

template < typename T >
void CEdPropertiesPage::SetObjects( const TDynArray< T* > &objects )
{
	TDynArray< STypedObject > oldObjects = GetObjects();

	TDynArray< STypedObject > newObjects;
	for ( T* obj : objects )
	{
		if ( obj )
		{
			CClass* aClass = obj->GetClass();
			void*   object = aClass->CastFrom( ClassID< T >(), obj );
			newObjects.PushBack( ConstructTypedObject( aClass, object ) );
		}
	}

	if ( !AreObjectSetsTheSame( oldObjects, newObjects ) ) 
	{	// Only proceed when the objects to set are actually different. Saves a lot of refreshing.
		ForcedSetObjects( newObjects );
	}
}

template < typename T >
void CEdPropertiesPage::SetObject( T* object )
{
	TDynArray< T* > objects;
	if ( object )
	{
		objects.PushBack( object );
	}
	SetObjects( objects );
}


class CPropertyTransaction
{
public:
	CPropertyTransaction( CEdPropertiesPage &page );

	~CPropertyTransaction();

private:
	CEdPropertiesPage*	m_page;
	Uint32				m_id;
	static Uint32		sm_lastUsedId;
	Bool				m_dropped;
};
