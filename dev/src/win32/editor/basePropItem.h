/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CEdPropertiesPage;
class CBaseGroupItem;
class CPropertyItem;

// This wrapper allows the property editor to handle non-CObject-derived RTTI objects
struct STypedObject
{
	STypedObject()
		: m_object( nullptr )
		, m_class( nullptr )
	{ }

	STypedObject( void* object, IRTTIType* type )
		: m_object( object )
		, m_class( dynamic_cast< CClass* >( type ) )
	{ }

	STypedObject( void* object, CClass* aClass )
		: m_object( object ), m_class( aClass )
	{ }

	explicit STypedObject( CObject* object )
		: m_object( object ), m_class( object->GetClass() )
	{ }

	template < typename T >
	T* As() const
	{
		if ( m_object != nullptr && m_class != nullptr )
		{
			return m_class->CastTo< T >( m_object );
		}
		else
		{
			return nullptr;
		}
	}

	CObject* AsObject() const
	{
		return As< CObject >();
	}

	ISerializable* AsSerializable() const
	{
		return As< ISerializable >();
	}

	Bool operator == ( const STypedObject& r ) const
	{
		return m_object == r.m_object && m_class == r.m_class;
	}

	Bool operator != ( const STypedObject& r ) const
	{
		return ! operator == ( r );
	}

	void*	m_object;
	CClass*	m_class;
};

//! Property item style for property pages
struct SEdPropertiesPagePropertyStyle
{
	wxColor					m_backgroundColor;		//!< Background color for the property name
	TDynArray< wxBitmap >	m_icons;				//!< Icons to put at the right side of the property name

	//! Default constructor
	SEdPropertiesPagePropertyStyle()
		: m_backgroundColor( 0, 0, 0, 0 )
	{
	}

	//! Copy constructor
	SEdPropertiesPagePropertyStyle( const SEdPropertiesPagePropertyStyle& src )
		: m_backgroundColor( src.m_backgroundColor )
		, m_icons( src.m_icons )
	{}

	//! Constructor for background color
	SEdPropertiesPagePropertyStyle( const wxColor& backgroundColor )
		: m_backgroundColor( backgroundColor )
	{}

	//! Constructor for background color and a single icon
	SEdPropertiesPagePropertyStyle( const wxColor& backgroundColor, wxBitmap icon )
		: m_backgroundColor( backgroundColor )
	{
		m_icons.PushBack( icon );
	}

	//! Constructor for a single icon only
	SEdPropertiesPagePropertyStyle( wxBitmap icon )
		: m_backgroundColor( 0, 0, 0, 0 )
	{
		m_icons.PushBack( icon );
	}
};

/// Base property grid item
class CBasePropItem : public wxEvtHandler
{
public:
	CBasePropItem( CEdPropertiesPage* page, CBasePropItem* parent );
	~CBasePropItem();

	RED_INLINE CEdPropertiesPage* GetPage() const { return m_page; }
	RED_INLINE CBasePropItem* GetParent() const { return m_parent; }
	RED_INLINE const TDynArray< CBasePropItem* >& GetChildren() const { return m_children; }
	RED_INLINE const TDynArray< CPropertyButton* >& GetButtons() const { return m_buttons; }
	RED_INLINE const wxRect& GetRect() const { return m_rect; }

	// Recurses through entire tree
	Uint32 GetChildCount( Bool onlyExpanded ) const;

	CBasePropItem* GetItemAtPoint( wxPoint point );
	CPropertyButton* GetButtonAtPoint( wxPoint point );
	Bool IsSelected() const;
	Bool IsExpanded() const;
	void Linearize( TDynArray< CBasePropItem* >& items );
	CPropertyButton* AddButton( const wxBitmap& bitmap, wxObjectEventFunction eventHandler, wxEvtHandler *eventSink = NULL );
	virtual String GetCaption() const { return TXT("NoCaption"); };

	virtual Int32 GetIndent() const;
	virtual Int32 GetLocalIndent() const;
	virtual Int32 GetHeight() const;

	virtual void Expand();
	virtual void Collapse();
	virtual void ToggleExpand();

	virtual Bool IsClassGroupItem() const { return false; }
    virtual Bool CanUseSelectedResource() const { return false; }

	virtual void UpdateLayout( Int32& yOffset, Int32 x, Int32 width );
    virtual Bool SerializeXML( IXMLFile& file );

	virtual Int32 GetNumObjects() const;
	virtual STypedObject GetRootObject( Int32 objectIndex ) const;	
	virtual STypedObject GetParentObject( Int32 objectIndex ) const;

	template < typename T >
	T* FindPropertyParentOfType( Int32 objectIndex ) const
	{
		const CBasePropItem *pitem = this;
		T* propertyOwner = nullptr;
		while ( !propertyOwner && pitem )
		{
			propertyOwner = pitem->GetParentObject( objectIndex ).As< T >();
			pitem = pitem->GetParent();
		}
		return propertyOwner;
	}

	template < typename T >
	T* FindPropertyParentWithInterface( Int32 objectIndex ) const
	{
		const CBasePropItem *pitem = this;
		T* propertyOwner = nullptr;
		while ( !propertyOwner && pitem )
		{
			propertyOwner = dynamic_cast< T* >( pitem->GetParentObject( objectIndex ).AsObject() );
			pitem = pitem->GetParent();
		}
		return propertyOwner;
	}

	virtual Bool IsReadOnly() const;
	virtual Bool IsInlined() const;
	virtual String GetName() const;
	virtual String GetCustomEditorType() const;
	virtual const SEdPropertiesPagePropertyStyle* GetPropertyStyle() const;

	void OnExpandAll( wxCommandEvent& event ); // TODO: fix by moving to protected

protected:
	CEdPropertiesPage*					m_page;
	CBasePropItem*						m_parent;
	TDynArray< CBasePropItem* >			m_children;
	TDynArray< CPropertyButton* >		m_buttons;
	wxRect								m_rect;
	Bool								m_isExpandable;
	Int32								m_buttonsWidth;

protected:
	// WARNING: following methods are intended to be used either by descendants or CEdPropertisPage. Calling them directly
	// may leave controls in the undefined state, cause various stability issues due to attached object lifespan, etc. 
	// DO NOT expose those methods as public. Use the functionality provided by the property page or extend it there. 
	friend class CEdPropertiesPage;

	void DrawButtons( wxDC& dc );
	void DrawChildren( wxDC& dc );
	void DrawIcons( wxDC& dc );

	void DestroyButtons();
	void DestroyChildren();

	void UpdateButtonsLayout();
	void RecursiveCloseControls();

	CBaseGroupItem* CreateGroupItem( CEdPropertiesPage *page, CBasePropItem *parent, CClass *classItem );
	CPropertyItem*  CreatePropertyItem( CEdPropertiesPage *page, CBasePropItem *parent, CProperty* property );
	CPropertyItem*  CreatePropertyItem( CEdPropertiesPage *page, CBasePropItem *parent, IRTTIType* type, Int32 arrayIndex = -1 );

	virtual void DrawLayout( wxDC& dc );
	virtual void CreateControls();
	virtual void CloseControls();
	virtual void OnObjectDiscarded( CObject* object );
	virtual void UseSelectedResource() { }
	// Are mouse scroll events allowed when this item is active
	virtual Bool ShouldSuppressMouseScrollEvent( const wxMouseEvent& event ) const { return false; }

	// Browser events
	virtual void OnBrowserMouseEvent( wxMouseEvent& event );
	virtual void OnBrowserKeyDown( wxKeyEvent& event );

	virtual Bool ReadProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 );
	virtual Bool WriteProperty( CProperty *property, void* buffer, Int32 objectIndex = 0 );

	virtual Bool ReadImp( class CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );
	virtual Bool WriteImp( class CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 );

private:
	wxRect CalcTreeIconRect() const;
	CPropertyItem* InternalCreatePropertyItem( CEdPropertiesPage *page, CBasePropItem *parent, IRTTIType* type );
};

