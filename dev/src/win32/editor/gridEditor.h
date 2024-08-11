#pragma once

#include "gridOutline.h"

class CGridEvent;

wxDECLARE_EVENT( wxEVT_GRID_VALUE_CHANGED, CGridEvent );
wxDECLARE_EVENT( wxEVT_GRID_ELEMENTS_CHANGED, wxCommandEvent );
wxDECLARE_EVENT( wXEVT_GRID_EDITING, CGridEvent );

class CGridPropertyWrapper;
class IGridTypeDesc;
class IGridColumnDesc;
class CElementWrapper;
class IGridCellAttrCustomizer;

class CCopiedData : public CObject
{
private:
	DECLARE_ENGINE_CLASS( CCopiedData, CObject, 0 )

public:
	CVariant m_data;
};

BEGIN_CLASS_RTTI( CCopiedData )
	PARENT_CLASS( CObject );
	PROPERTY( m_data );
END_CLASS_RTTI();

class CGridObjectEditor;

class CGridEvent : public wxNotifyEvent
{
private: 
	const IRTTIType* m_cellDataType;
	void* m_cellData;
	const IRTTIType* m_gridDataType;
	void* m_gridData;
	const CProperty* m_cellProperty;

public:
	CGridEvent( wxEventType commandType = wxEVT_NULL, int winid = 0 )
		: wxNotifyEvent( commandType, winid )
	{}
	CGridEvent(const CGridEvent& event)
		: wxNotifyEvent( event )
		, m_cellDataType ( event.m_cellDataType )
		, m_cellData( event.m_cellData )
		, m_gridDataType ( event.m_gridDataType ) 
		, m_gridData ( event.m_gridData )
		, m_cellProperty ( event.m_cellProperty )
	{ }

	virtual wxEvent *Clone() const { return new CGridEvent( *this ); }

	RED_INLINE void SetCellData( const IRTTIType* cellDataType, void* cellData ) { m_cellDataType = cellDataType; m_cellData = cellData; }
	RED_INLINE void SetGridData( const IRTTIType* gridDataType, void* gridData ) { m_gridDataType = gridDataType; m_gridData = gridData; }
	RED_INLINE void SetCellProperty( const CProperty* property ) { m_cellProperty = property; }

	RED_INLINE void GetCellData( const IRTTIType*& cellDataType, void*& cellData ) { cellDataType = m_cellDataType; cellData = m_cellData; }
	RED_INLINE void GetGridData( const IRTTIType*& gridDataType, void*& gridData ) { gridDataType = m_gridDataType; gridData = m_gridData; }
	RED_INLINE const CProperty*	GetCellProperty() { return m_cellProperty; }
};

class CGridEditor : public COutlineGrid, public IEdEventListener
{
	friend class CGridPropertyWrapper;
	friend class CGridCellObjectEditor;
	friend class CGridEditorUndoStep;

public:

	CGridEditor( wxWindow *parent, Bool allowEditPointers = false );
	~CGridEditor();

	void SetObject( void *data, IRTTIType *type );
	void SetObject( void *data, const CProperty *property );

	void SetObjects( TDynArray< void* >& objects, IRTTIType & type, Bool separateWithBlankRow = true );
	void SetObjects( TDynArray< CObject* >& objects, Bool separateWithBlankRow = true );

	// Default parent for created inline objects
	void SetDefaultObjectParent( CObject* parent ) { m_defaultObjectParent = parent; }

	void SaveLayout( const String &identifier );
	void LoadLayout( const String &identifier );

	Bool IsColSeparator( Int32 col ) const;

	Bool RegisterCustomType( IGridTypeDesc *typeDesc );
	IGridTypeDesc *GetCustomTypeDesc( const CName &typeName ) const;
	Bool AllowEditPointers() const { return m_allowEditPointers; }

	Bool RegisterCustomColumnDesc( const String& columnName, IGridColumnDesc* columnDesc );
	Bool UnregisterCustomColumnDesc( const String& columnName );
	IGridColumnDesc* GetCustomColumnDesc( const wxString& columnName ) const;

	RED_INLINE void RegisterCustomCellAttrCustomizer( IGridCellAttrCustomizer* customizer ) { m_attrCustomizer = customizer; }

	void ShowObjectEditor( CObject* object );
	void HideObjectEditor();

	void SetUndoManager( CEdUndoManager* undoManager );

private:
	const IRTTIType *					m_selectedType;
	const CProperty *					m_selectedProperty;
	TDynArray< CGridPropertyWrapper * > m_propertyWrappers;
	THashMap< CName, IGridTypeDesc * >		m_gridTypesDescMap;
	THashMap< String, IGridColumnDesc* >	m_gridColumnDescMap;
	CEdColorPicker *					m_ctrlColorPicker;
	THashMap<Int32, wxColour>				m_colorsByColumnIndex;
	Bool								m_lockUpdateColumnsSize;
	CObject*							m_defaultObjectParent;
	CGridObjectEditor*					m_objectEditor;
	CEdUndoManager*						m_undoManager;
	Bool								m_separateObjects;
	IGridCellAttrCustomizer*			m_attrCustomizer;
	Bool								m_allowEditPointers;

	static CCopiedData * m_copiedData;

	// Internal methods
	void CreateHeader();
	void UpdateBlocks();
	void UpdateColumnsSize();
	Bool SetCellEditorByType( Int32 row, Int32 col, const IRTTIType *type, void * data );
	CGridPropertyWrapper *GetRelativeCoords( Int32 &row, Int32 &col ) const;
	wxColour GetColumnColor( Int32 col );
	void SetColumnColor( Int32 col, const wxColour &color );
	void ElementPaste( CElementWrapper *wrapper, Bool insert );
	void ReparentInlinedObjects( IRTTIType* rttiType, void* data );
	void UpdateGUIDs( IRTTIType* rttiType, void* data );

	// Event handlers
	void OnCellEditing( wxGridEvent &event );
	void OnCellChanged( wxGridEvent &event );
	void OnCellLeftClick( wxGridEvent &event );
	void OnCellRightClick( wxGridEvent &event );
	void OnColLabelRightClick( wxMouseEvent& event );
	void OnColLabelMouseMotion( wxMouseEvent& event );
	void OnMouseMotion( wxMouseEvent& event );

	void OnElementCut( wxCommandEvent& event );
	void OnElementCopy( wxCommandEvent& event );
	void OnElementPaste( wxCommandEvent& event );
	void OnElementPasteIntoExisting( wxCommandEvent& event );
	void OnElementInsert( wxCommandEvent& event );
	void OnElementRemove( wxCommandEvent& event );
    void OnElementMoveUp( wxCommandEvent& event );
    void OnElementMoveDown( wxCommandEvent& event );
	void OnObjectCreate( wxCommandEvent& event );
	void OnObjectDelete( wxCommandEvent& event );
	void OnColumnHide( wxCommandEvent& event );
	void OnColumnShowAll( wxCommandEvent& event );
	void OnColumnSetColor( wxCommandEvent& event );
	void OnColumnToggle( wxCommandEvent& event );
	void OnColorPicked( wxCommandEvent& event );
	void OnColorPickerShow( wxShowEvent& event );

	void OnFocus( wxFocusEvent& event );
	void OnChoiceChanged( wxNotifyEvent& event );

	void SetupEvent( CGridEvent& gridEvent, Int32 row, Int32 col );

	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data ) override;

	DECLARE_EVENT_TABLE()
};

class CGridObjectEditor : public wxFrame
{
	DECLARE_EVENT_TABLE()

private:
	CEdSelectionProperties* m_properties;

	void OnClose(wxCloseEvent& event);


public:
	CGridObjectEditor( wxWindow* parent );
	void ShowEditor( CObject* object );
	void HideEditor();
	void SetUndoManager( CEdUndoManager* undoManager );
};
