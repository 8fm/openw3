/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/layerInfo.h"

/// wxWrapper for CClass
class PopupClassWrapper : public wxObject
{
public:
	CClass*		m_objectClass;

public:
	PopupClassWrapper( CClass* objectClass )
		: m_objectClass( objectClass )
	{};

	~PopupClassWrapper()
	{
	}
};

/// wxWrapper for CObject
class ClientDataObjectWrapper : public wxClientData
{
public:
	CObject*	m_objectClass;

public:
	ClientDataObjectWrapper( CObject* objectClass )
		: m_objectClass( objectClass )
	{};

	~ClientDataObjectWrapper()
	{
	}
};

/// wxWrapper for simple types
template <class T>
class TClientDataWrapper : public wxClientData
{
public:
	TClientDataWrapper( const T& data )
		: m_data( data ) {}

	RED_INLINE const T& GetData() const
	{ return m_data; }

	RED_INLINE T& GetData()
	{ return m_data; }

private:
	T m_data;
};

// wxWrapper for CName
class CNameItemWrapper : public wxObject
{
public:
	CName	m_name;

public:
	CNameItemWrapper( CName object )
		: m_name( object )
	{};
};

// wxWrapper for any ISerializable
class SerializableItemWrapper : public wxTreeItemData
{
public:
	ISerializable*	m_object;

public:
	SerializableItemWrapper( ISerializable* object )
		: m_object( object )
	{};
};

// wxWrapper for CObject
class ObjectItemWrapper : public wxTreeItemData
{
public:
	CObject*	m_object;

public:
	ObjectItemWrapper( CObject* object )
		: m_object( object )
	{};
};

// wxWrapper for CNode
class NodeItemWrapper : public wxTreeItemData
{
public:
	CNode*		m_node;

public:
	NodeItemWrapper( CNode* node )
		: m_node( node )
	{};
};

// wxWrapper for disk file
class DiskFileWrapper : public wxTreeItemData
{
public:
	CDiskFile*		m_file;

public:
	DiskFileWrapper( CDiskFile* file )
		: m_file( file )
	{};
};

// wxWrapper for handle
class HandleItemWrapper : public wxTreeItemData
{
public:
	THandle< CObject > m_handle;

	HandleItemWrapper( THandle< CObject > handle )
		: m_handle( handle )
	{}
};


// wxWrapper for directory
class DirectoryItemWrapper : public wxTreeItemData
{
public:
	CDirectory*	m_directory;

public:
	DirectoryItemWrapper( CDirectory* directory )
		: m_directory( directory )
	{};
};

/// wxWrapper for simple types
template <class T>
class TTreeItemDataWrapper : public wxTreeItemData
{
public:
	TTreeItemDataWrapper( const T& data )
		: m_data( data ) {}

	RED_INLINE const T& GetData() const
	{ return m_data; }

private:
	T m_data;
};

/// control size constraints

class CEdSizeConstaints
{
public:
	static CEdSizeConstaints EMPTY;

	CEdSizeConstaints()
	{ 
		*this = EMPTY; 
	}

	CEdSizeConstaints( const wxSize& aMin, const wxSize& aMax )
		: m_min( aMin ), m_max( aMax )
	{
	}

	CEdSizeConstaints( int minX, int minY, int maxX, int maxY )
		: m_min( minX, minY ), m_max( maxX, maxY )
	{
	}

	Bool IsEmpty() const
	{
		return *this == EMPTY;
	}

	Bool operator == ( const CEdSizeConstaints& r ) const
	{
		return m_min == r.m_min && m_max == r.m_max;
	}

	Bool operator != ( const CEdSizeConstaints& r ) const
	{
		return !operator==( r );
	}

	wxSize m_min;
	wxSize m_max;
};

/// Formatted dialog class

class CEdFormattedDialog
{
public:
	const static int ModalResultBase = 100000;

private:
	enum SubPanelType
	{
		SPT_Horizontal,
		SPT_Vertical,
		SPT_Table
	};

	enum FieldType
	{
		FT_String,
		FT_Integer,
		FT_Float
	};

	// Base element class
	struct Element
	{
		Element*	m_parent;
		Element*	m_firstChild;
		Element*	m_lastChild;
		Element*	m_previous;
		Element*	m_next;
		wxWindow*	m_window;
		Int32		m_index;

		inline Element( Element* parent )
			: m_parent( parent )
			, m_firstChild( NULL )
			, m_lastChild( NULL )
			, m_previous( NULL )
			, m_next( NULL )
			, m_window( NULL )
			, m_index( wxNOT_FOUND )
		{
			if ( parent )
			{
				m_previous = parent->m_lastChild;
				if ( m_previous )
				{
					m_previous->m_next = this;
				}
				if ( !m_parent->m_firstChild )
				{
					m_parent->m_firstChild = this;
				}
				m_parent->m_lastChild = this;
			}
		}

		inline ~Element()
		{
			for ( Element* child=m_firstChild; child; )
			{
				Element* next = child->m_next;
				delete child;
				child = next;
			}
		}

		virtual wxWindow* GetParentWindow() const
		{
			return m_parent ? m_parent->m_window : NULL;
		}

		virtual void StoreData( void* )
		{
		}

		virtual void LoadData( void* )
		{
		}

		virtual void Parse( CEdFormattedDialog* fmt )=0;
	};

	// SubPanel element
	struct SubPanelElement : public Element
	{
		wxString m_title;
		SubPanelType m_subPanelType;
		int m_columns;
		SubPanelElement( Element* parent ) : Element( parent ), m_subPanelType( SPT_Vertical ) {}
		virtual void Parse( CEdFormattedDialog* fmt );
	};

	// Root element
	struct RootElement : public SubPanelElement
	{
		wxWindow* m_parentWindow;
		RootElement( wxWindow* parentWindow ) : SubPanelElement( NULL ), m_parentWindow( parentWindow ) {}
		virtual wxWindow* GetParentWindow() const
		{
			return m_parentWindow;
		}
	};

	// Label element
	struct LabelElement : public Element
	{
		LabelElement( Element* parent ) : Element( parent ) {}
		virtual void Parse( CEdFormattedDialog* fmt );
	};

	// Button element
	struct ButtonElement : public Element, public wxEvtHandler
	{
		Int32 m_buttonIndex;
		ButtonElement( Element* parent, Int32 buttonIndex ) : Element( parent ), m_buttonIndex( buttonIndex ) {}
		virtual void Parse( CEdFormattedDialog* fmt );
		void OnClick( wxCommandEvent& event );
	};

	// Check Box element
	struct CheckBoxElement : public Element
	{
		CheckBoxElement( Element* parent ) : Element( parent ) {}
		virtual void StoreData( void* data );
		virtual void LoadData( void* data );
		virtual void Parse( CEdFormattedDialog* fmt );
	};

	// Field element
	struct FieldElement : public Element
	{
		FieldType m_type;
		FieldElement( Element* parent, FieldType type ) : Element( parent ), m_type( type ) {}
		virtual void StoreData( void* data );
		virtual void LoadData( void* data );
		virtual void Parse( CEdFormattedDialog* fmt );
	};

	// Choice element
	struct ChoiceElement : public Element
	{
		ChoiceElement( Element* parent ) : Element( parent ) {}
		virtual void StoreData( void* data );
		virtual void LoadData( void* data );
		virtual void Parse( CEdFormattedDialog* fmt );
	};

	// Radio element
	struct RadioElement : public Element
	{
		wxVector<wxRadioButton*> m_buttons;
		RadioElement( Element* parent ) : Element( parent ) {}
		virtual void StoreData( void* data );
		virtual void LoadData( void* data );
		virtual void Parse( CEdFormattedDialog* fmt );
	};

	// List element
	struct ListElement : public Element
	{
		ListElement( Element* parent ) : Element( parent ) {}
		virtual void StoreData( void* data );
		virtual void LoadData( void* data );
		virtual void Parse( CEdFormattedDialog* fmt );
	};

	// MultiCheckListElement element
	struct MultiCheckListElement : public Element
	{
		MultiCheckListElement( Element* parent ) : Element( parent ) {}
		virtual void StoreData( void* data );
		virtual void LoadData( void* data );
		virtual void Parse( CEdFormattedDialog* fmt );
	};

	wxString	m_code;
	Uint32		m_head;
	Element*	m_root;
	wxFrame*	m_hiddenFrame;
	Uint32		m_nextIndex;
	Uint32		m_buttons;
	wxWindow*	m_defaultWindow;
	wxWindow*	m_focusedWindow;

	void SkipSpaces();
	wxString ScanString();
	wxVector<wxString> ScanList();
	Int32 ScanInteger();

	Element* FindElementChildByIndex( Element* element, Int32 index ) const;
	Element* FindElementByIndex( Int32 index ) const;

	void ResetGUI();
	void DoParse();

public:
	CEdFormattedDialog();
	~CEdFormattedDialog();

	Bool Parse( const wxString& code );

	wxWindow* GetRootWindow() const;
	wxDialog* CreateDialog( wxWindow* parent, const wxString& caption );

	void StoreElementDataTo( Int32 index, void* data ) const;
	void LoadElementDataFrom( Int32 index, void* data );
	template <typename T>
	inline T GetElementData( Int32 index ) const
	{
		static T defvalue;
		T value = defvalue;
		StoreElementDataTo( index, &value );
		return value;
	}
	template <typename T>
	inline void SetElementData( Int32 index, const T& value ) const
	{
		LoadElementDataFrom( index, &value );
	}
	inline Int32 GetIndexCount() const { return m_nextIndex; }

	inline wxWindow* GetDefaultWindow() const { return m_defaultWindow; }
	inline wxWindow* GetFocusedWindow() const { return m_focusedWindow; }

	static Int32 ShowFormattedDialogVA( wxWindow* parent, const wxString& caption, const wxString& code, va_list va );
	static Int32 ShowFormattedDialog( wxWindow* parent, const wxString& caption, wxString code, ... );

	// Returns the given array as a list of the form ('item1' 'item2' ...) to pass in the code for formatted box
	// This uses a function, functor or lambda to convert each item in the array to a String
	template<typename T, typename F> static String ArrayToList( const TDynArray<T>& arr, F& stringConvertFunc )
	{
		String list = TXT("(");
		for ( auto it=arr.Begin(); it != arr.End(); ++it )
		{
			String entry = stringConvertFunc( *it );
			entry.ReplaceAll( TXT("\\"), TXT("\\\\") );
			entry.ReplaceAll( TXT("'"), TXT("\\'") );
			list += TXT('\'');
			list += entry;
			list += TXT('\'');
		}
		return list + TXT(")");
	}

	// Returns the given array as a list of the form ('item1' 'item2' ...) to pass in the code for formatted box
	// This uses the global ::ToString conversion functions
	template<typename T> static String ArrayToList( const TDynArray<T>& arr )
	{
		return ArrayToList( arr, []( const T& item ) { return ::ToString( item ); } );
	}
};

// Formatted dialog box (aka scanf with GUI) - check the top of utils.cpp for a description of the code
Int32 FormattedDialogBox( wxWindow* parent, const wxString& caption, wxString code, ... );
// Formatted dialog box (aka scanf with GUI) - check the top of utils.cpp for a description of the code
Int32 FormattedDialogBox( const wxString& caption, wxString code, ... );


class SimpleWebViewHandler 
{
public:
	virtual void HandlePage( const String& uri ) = 0;
};
// HTML box that can be used to show html content.
class wxWebViewHandler;
void HtmlBox( wxWindow* parent, const String& title, const String& html, Bool navbar = false, SimpleWebViewHandler* handler = nullptr );

// Converts screen coordinates to workspace coordinates (as used in Get/SetWindoPlacement)
void ScreenToWorkspace( Int32& x, Int32& y );

// Converts workspace coordinates (as used in Get/SetWindoPlacement) to screen coordinates 
void WorkspaceToScreen( Int32& x, Int32& y );

template < typename FunctT >
class RunnableFunctorWrapper : public CEdRunnable
{
public:
	RunnableFunctorWrapper( FunctT funct ) 
		: m_functor( funct )
		{ }

	void Run() override
	{
		m_functor();
	}

private:
	FunctT m_functor;
};

// Note: all the Run... functions EXCEPT RunInMainThreadEx take the ownership of the passed runnable object
void RunLaterEx( class CEdRunnable* runnable );
void RunLaterOnceEx( class CEdRunnable* runnable );
void RunParallelEx( class CEdRunnable* taskRunnable, class CEdRunnable* afterFinishRunnable );
void RunInMainThreadEx( class CEdRunnable* taskRunnable );

void DestroyLater( wxWindow* window );
void RefreshLater( wxWindow* window );

template < typename FunctT >
void RunLater( FunctT funct )
{
	RunLaterEx( new RunnableFunctorWrapper< FunctT >( funct ) );
}

template < typename FunctT >
void RunLaterOnce( FunctT funct )
{
	RunLaterOnceEx( new RunnableFunctorWrapper< FunctT >( funct ) );
}

template < typename FunctA, typename FunctB >
void RunParallel( FunctA functA, FunctB functB )
{
	RunParallelEx( new RunnableFunctorWrapper< FunctA >( funct ), new RunnableFunctorWrapper< FunctB >( functB ) );
}

template < typename FunctT >
void RunInMainThread( FunctT funct )
{
	Red::TScopedPtr< CEdRunnable > wrapper( new RunnableFunctorWrapper< FunctT >( funct ) );
	RunInMainThreadEx( wrapper.Get() );
}

// Helper for asking for string input
extern Bool InputBox( wxWindow* parent, const String& title, const String& message, String &value, Bool multiline = false );
extern String InputBox( wxWindow* parent, const String& title, const String& message, const String &value, Bool multiline = false );
extern Bool InputDoubleBox( wxWindow* parent, const String& title, const String& message, String &valueA, String &valueB, Bool multiline = false, Bool useConfig = false );
extern Bool InputMultiBox( wxWindow* parent, const String& title, const String& message, TDynArray< String >&values );
extern Bool InputBoxFileName( wxWindow* parent, const String& title, const String& message, String &value, const String& fileExtension );
extern Bool YesNo( const Char* msg, ... );
extern Int32 YesNoCancel( const Char* msg, ... ); // returns IDYES, IDNO, IDCANCEL
extern String InputComboBox( wxWindow* parent, const String& title, const String& message, const String &defaultChoice, const TDynArray<String>& choices);

//Helper for getting multiple Yes No answers 
//to pass default values put them in answers Array otherwise is false
extern Bool MultiBoolDialog(wxWindow* parent, const String& title, const TDynArray<String>& questions, TDynArray<Bool>& answers);

// Active resources
extern void SetActiveResources( const TDynArray<String>& resourcePaths );
extern Bool GetActiveResource( String& resourcePath );
extern Bool GetActiveResource( String& resourcePath, CClass* filterClass );
extern const TDynArray<String>& GetActiveResources();

extern void SetActiveDirectory( const String& path );
extern const String& GetActiveDirectory();

// Pick bone
extern Bool PickBone( wxWindow* parent, ISkeletonDataProvider* skeleton, CName& boneName );

// Selection
extern CEntity* GetSelectedEntity();
extern CEntity* GetSelectedEntityInTheSameLayerAs( CObject* object );

// Menu
extern wxMenu* GetSubMenu( wxMenu* baseMenu, const String& menuName, Bool createIfNotFound );
extern wxMenu* GetSubMenuPath( wxMenu* baseMenu, const String& menuPath, Bool createIfNotFound );

namespace wxMenuUtils
{
    void CloneMenu(wxMenu *srcMenu, wxMenu *&dstMenu);
    void CloneMenuBar(wxMenuBar * srcMenu, wxMenuBar *&dstMenu,
                      Bool onlyWithDolars  = false );
    void MergeCloneOfMenuBar(wxMenuBar *srcMenu, wxMenuBar *dstMenu);
	String ChangeItemLabelPreservingAccel( wxMenuItem* item, const String& newLabel );
	wxMenuItem* AppendMenuItemWithBitmap( wxMenu* parent, int id, const String& name, const String& bmpName );	

	// removes redundant and duplicate separators
	void CleanUpSeparators( wxMenu* menu );
}

// Recursively relayout everything optionally starting from the top window of the given window
extern void LayoutRecursively( wxWindow* win, bool fromTopWindow = true );

// Like LayoutRecursively, but also calls Fit() before laying out the children so that each
// window is resizet to fit its children
extern void FitRecursively( wxWindow* win, bool fromTopWindow = true );

DWORD FindAssociatedProcessWithName( String name );

// Launches the appropriate program for the given file (or the program, if a program is given)
Bool OpenExternalFile( String path, Bool silent = false );

// Returns true if the object inspector is available (always true for debug/noopts builds, depends on user ini for release)
Bool IsObjectInspectorAvailable();

// Show a new object inspector for the given object. The returned frame can be
// casted to a CEdObjectInspector. An optional "tag" is added to the title.
// This will return nullptr always if the object inspector is not available
// and you should use CEdObjectInspector::CreateInspector directly to override
wxFrame* InspectObject( ISerializable* object, const String& tag = String::EMPTY );

// Helper functions for modifying properties in objects without C++ getters/setters
// Setters can also emit editor events that (among others) add undo steps
void* GetPropertyDataPtr( CObject* obj, const CName& propertyName );
void* GetPropertyDataPtr( CObject* obj, const String& propertyName );
Bool GetPropertyValueIndirect( CObject* obj, const CName& propertyName, void* buffer );
Bool GetPropertyValueIndirect( CObject* obj, const String& propertyName, void* buffer );
Bool SetPropertyValueIndirect( CObject* obj, const CName& propertyName, const void* buffer, Bool emitEditorEvents = false );
Bool SetPropertyValueIndirect( CObject* obj, const String& propertyName, const void* buffer, Bool emitEditorEvents = false );

template <typename T>
Bool GetPropertyValue( CObject* obj, const CName& propertyName, T& value )
{
	return GetPropertyValueIndirect( obj, propertyName, &value );
}

template <typename T>
Bool SetPropertyValue( CObject* obj, const CName& propertyName, const T& value, Bool emitEditorEvents = false )
{
	return SetPropertyValueIndirect( obj, propertyName, &value, emitEditorEvents );
}

template <typename T>
Bool GetPropertyValue( CObject* obj, const String& propertyName, T& value )
{
	return GetPropertyValueIndirect( obj, propertyName, &value );
}

template <typename T>
Bool SetPropertyValue( CObject* obj, const String& propertyName, const T& value, Bool emitEditorEvents = false )
{
	return SetPropertyValueIndirect( obj, propertyName, &value, emitEditorEvents );
}

// Macro for auto connecting slider and edit

#define SLIDER_AND_EDIT_CONNECT( _funcUniqueName, _sliderName, _editName, _min, _max )									\
{																														\
	wxSlider* slider = XRCCTRL( *this, #_sliderName, wxSlider );														\
	slider->Connect( wxEVT_SCROLL_CHANGED, wxCommandEventHandler( _funcUniqueName##SliderUpdate ), NULL, this );		\
	slider->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( _funcUniqueName##SliderUpdating ), NULL, this );	\
	slider->SetMin( _min );																								\
	slider->SetMax( _max );																								\
	wxTextCtrl* editCtrl = XRCCTRL( *this, #_editName, wxTextCtrl );													\
	editCtrl->SetValidator( wxTextValidator( wxFILTER_NUMERIC ) );														\
	editCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( _funcUniqueName##Edit ), NULL, this );			\
};

#define SLIDER_AND_EDIT_DEFINE( _funcUniqueName, _sliderName, _editName, _min, _max )	\
void _funcUniqueName##SliderUpdate( wxCommandEvent& event )					\
{																			\
	wxSlider* slider = XRCCTRL( *this, #_sliderName, wxSlider );			\
	wxTextCtrl* edit = XRCCTRL( *this, #_editName, wxTextCtrl );			\
	Float var = ((Float)(slider->GetValue()) / 100.0f);						\
	wxString varStr = wxString::Format(wxT("%d"), slider->GetValue());		\
	edit->SetLabel(varStr);													\
}																			\
void _funcUniqueName##SliderUpdating( wxCommandEvent& event )				\
{																			\
	wxSlider* slider = XRCCTRL( *this, #_sliderName, wxSlider );			\
	wxTextCtrl* edit = XRCCTRL( *this, #_editName, wxTextCtrl );			\
	Int32 var = slider->GetValue();											\
	wxString varStr = wxString::Format(wxT("%d"), var);						\
	edit->SetLabel(varStr);													\
}																			\
void _funcUniqueName##Edit( wxCommandEvent& event )							\
{																			\
	wxSlider* slider = XRCCTRL( *this, #_sliderName, wxSlider );			\
	wxTextCtrl* edit = XRCCTRL( *this, #_editName, wxTextCtrl );			\
	String strVal = edit->GetValue().wc_str();								\
	if (strVal.Empty()) strVal = TXT("0");									\
	Int32 newVal;																\
	FromString(strVal, newVal);												\
	newVal = Clamp( newVal, _min, _max );									\
	slider->SetValue( newVal );												\
}

#define SLIDER_AND_EDIT_WITH_RANGE_CONNECT( _funcUniqueName, _sliderName, _editName, _editMin, _editMax )				\
{																														\
	SLIDER_AND_EDIT_CONNECT( _funcUniqueName, _sliderName, _editName, 0.f, 100.f );										\
	wxTextCtrl* editCtrlMin = XRCCTRL( *this, #_editMin, wxTextCtrl );													\
	editCtrlMin->SetValidator( wxTextValidator( wxFILTER_NUMERIC ) );													\
	editCtrlMin->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( _funcUniqueName##EditRange ), NULL, this );	\
	wxTextCtrl* editCtrlMax = XRCCTRL( *this, #_editMax, wxTextCtrl );													\
	editCtrlMax->SetValidator( wxTextValidator( wxFILTER_NUMERIC ) );													\
	editCtrlMax->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( _funcUniqueName##EditRange ), NULL, this );	\
	wxSlider* slider = XRCCTRL( *this, #_sliderName, wxSlider );														\
	Int32 val = 0; String temp;																							\
	temp = editCtrlMin->GetValue().c_str();																				\
	FromString( temp, val );																							\
	slider->SetMin( val );																								\
	temp = editCtrlMax->GetValue().c_str();																				\
	FromString( temp, val );																							\
	slider->SetMax( val );																								\
}

#define SLIDER_AND_EDIT_WITH_RANGE_DEFINE( _funcUniqueName, _sliderName, _editName, _editMin, _editMax )	\
SLIDER_AND_EDIT_DEFINE( _funcUniqueName, _sliderName, _editName, 0, 100 )									\
void _funcUniqueName##EditRange( wxCommandEvent& event )													\
{																											\
	wxSlider* slider = XRCCTRL( *this, #_sliderName, wxSlider );											\
	wxTextCtrl* editCtrlMin = XRCCTRL( *this, #_editMin, wxTextCtrl );										\
	wxTextCtrl* editCtrlMax = XRCCTRL( *this, #_editMax, wxTextCtrl );										\
	Int32 val = 0; String temp;																				\
	temp = editCtrlMin->GetValue().c_str();																	\
	FromString( temp, val );																				\
	slider->SetMin( val );																					\
	temp = editCtrlMax->GetValue().c_str();																	\
	FromString( temp, val );																				\
	slider->SetMax( val );																					\
}

// Collect components of given type from entity
template < class T >
void CollectEntityComponents( CEntity* entity, TDynArray< T* >& components )
{
	ASSERT( entity );
	if ( !entity )
		return;

	// Reserve memory block to allow to put components (as some of components may be null or of different class?
	// and we only want to add existing ones, and as dyn array helds pointers, 
	// reserving a little bit more than we actually need, won't hurt, this seems to be most elegant method)
	components.Reserve( components.Size() + entity->GetComponents().Size() );
	
	for ( ComponentIterator< T > it( entity ); it; ++it )
	{
		components.PushBack( *it );
	}
}

// Collect components of given type from entity and all its items
template < class T >
void CollectEntityComponentsWithItems( CEntity* entity, TDynArray< T* >& components )
{
	ASSERT( entity );
	if ( !entity )
	{
		return;
	}

	CollectEntityComponents( entity, components );

	if ( CActor* a = Cast< CActor >( entity ) )
	{
		if ( CInventoryComponent* inv = a->GetInventoryComponent() )
		{
			const auto& items = inv->GetItems();
			const Uint32 num = items.Size();
			for ( Uint32 i=0; i<num; ++i )
			{
				const SInventoryItem& item = items[ i ];

				CEntity* itemEntity = item.GetItemEntity();
				if ( itemEntity )
				{
					CollectEntityComponents( itemEntity, components );
				}
			}
		}
	}
}

// Collect *ALL* entities of type, placed on the world
template < class T >
void CollectAllEntities( CWorld* world, TDynArray< T* >& entities )
{
	if ( world )
	{
		// Get world layers
		TDynArray< CLayerInfo* > layers;
		world->GetWorldLayers()->GetLayers( layers, true );

		// Iterate through every layer
		for( TDynArray< CLayerInfo* >::iterator layerIter = layers.Begin();
			layerIter != layers.End(); ++layerIter )
		{
			CLayerInfo* layerInfo = *layerIter;
			ASSERT( layerInfo != NULL );
			CLayer* layer = layerInfo->GetLayer();
			ASSERT( layer != NULL );

			// Iterate through every entity
			const LayerEntitiesArray& ents = layer->GetEntities();

			for( auto entityIter = ents.Begin(), end = ents.End(); entityIter != end; ++entityIter )
			{
				T* entity = Cast< T >( *entityIter );

				if( entity )
				{
					entities.PushBack( Cast< T >( entity ) );
				}
			}
		}
	}
}

// Collect *ALL* components of type, placed on the world
template < class T >
void CollectAllComponents( CWorld* world, TDynArray< T* >& components )
{
	if ( world )
	{
		// Get world layers
		TDynArray< CLayerInfo* > layers;
		world->GetWorldLayers()->GetLayers( layers, true );

		// Iterate through every layer
		for( TDynArray< CLayerInfo* >::iterator layerIter = layers.Begin();
			layerIter != layers.End(); ++layerIter )
		{
			CLayerInfo* layerInfo = *layerIter;
			ASSERT( layerInfo != NULL );
			CLayer* layer = layerInfo->GetLayer();
			ASSERT( layer != NULL );

			// Iterate through every entity
			TDynArray< CEntity* > entities;
			layer->GetEntities( entities );

			for( TDynArray< CEntity* >::iterator entityIter = entities.Begin();
				entityIter != entities.End(); ++entityIter )
			{
				CEntity* entity = *entityIter;
				ASSERT( entity != NULL );

				// Get entity components
				CollectEntityComponents( entity, components );
			}
		}
	}
}

// Returns a random color
Color RandomPastelColor();

// Loads the layers in the given world that contain entities placed around the given position and radius
void LoadLayersWithEntitiesAroundPosition( CWorld* world, const Vector& position, Float radius, Bool showFeedback=true );

// Sorts an array of objects representable as chars (f.e. strings, cnames, etc)
template <typename T> void SortAsCharArray( TDynArray<T>& arr )
{
	::Sort( arr.Begin(), arr.End(), []( const T& a, const T& b ) { return Red::System::StringCompare( a.AsChar(), b.AsChar() ) < 0; } );
}

// Clears the scene entity list of the given name
void ClearEntityList( const String& name );

// Adds the given entity to the scene entity list with the given name
void AddEntityToEntityList( const String& name, CEntity* entity );

// Adds the given entities to the scene entity list with the given name
void AddEntitiesToEntityList( const String& name, const TDynArray< CEntity* >& entities );

// Removes the given entity from the scene entity list with the given name
void RemoveEntityFromEntityList( const String& name, CEntity* entity );

// Shows the scene entity list with the given name
void ShowEntityList( const String& name );

// Loads a scene entity list from the given file as the given name and shows it
class CEntityList* LoadEntityListFromFile( const String& name, const String& absolutePath );

// Loads a scene entity list from the given file using the filename as the list name
class CEntityList* LoadEntityListFromFile( const String& absolutePath );

// Performs a natural string comparison with special number handling (so that, f.e, "a3" < "a10")
int StringNaturalCompare( const Char* a, const Char* b );

// Returns path with only last two segment, so that a\b\c.d becomes b\c.d
String GetPathWithLastDirOnly( const String& path );

// Convers a hash set to a dynamic array
template <typename T> TDynArray<T> HashSetToDynArray( const THashSet<T>& hs )
{
	TDynArray<T> da;
	da.Reserve( hs.Size() );
	for ( T item : hs )
	{
		da.PushBack( item );
	}
	return da;
}

// Convers a dynamic array to a hash set
template <typename T> THashSet<T> DynArrayToHashSet( const TDynArray<T>& da )
{
	THashSet<T> hs;
	for ( T item : da )
	{
		hs.Insert( item );
	}
	return hs;
}

class CStringSanitizer
{
public:
	static void SanitizeString( String& stringVal );
};

// Quickhull 2D implementation as described in Wikipedia (except using a loop instad of recursion)
// Points are random points on plane (Z is ignored) and hullPoints is indices to the points in the
// first array that form the convex hull
Bool ComputeQuickHull2D( const TDynArray< Vector >& points, TDynArray< Uint32 >& hullPoints );
