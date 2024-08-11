#include "build.h"
#include "gridTypeDesc.h"
#include "gridColumnDesc.h"
#include "gridPropertyWrapper.h"
#include "gridCellEditors.h"
#include "gridCustomCellEditors.h"
#include "undoGridEditor.h"
#include "gridEditor.h"
#include "gridCellAttrCustomizer.h"

#define ID_MENU_COLUMN_HIDE				9001
#define ID_MENU_COLUMN_SHOWALL			9002
#define ID_MENU_COLUMN_SETCOLOR			9003
#define ID_MENU_ELEMENT_ADD				9004
#define ID_MENU_ELEMENT_INSERT_BEFORE	9005
#define ID_MENU_ELEMENT_INSERT_AFTER	9006
#define ID_MENU_ELEMENT_REMOVE			9007
#define ID_MENU_ELEMENT_MOVE_UP         9008
#define ID_MENU_ELEMENT_MOVE_DOWN       9009
#define ID_MENU_ELEMENT_CUT				9010
#define ID_MENU_ELEMENT_COPY			9011
#define ID_MENU_ELEMENT_COPY_CUR_ELEM	9012
#define ID_MENU_ELEMENT_PASTE_BEFORE	9013
#define ID_MENU_ELEMENT_PASTE_AFTER		9014
#define ID_MENU_ELEMENT_PASTE			9015
#define ID_MENU_OBJECT_CREATE			9016
#define ID_MENU_OBJECT_DELETE			9017
#define ID_MENU_COLUMN_TOGGLE			9050

wxDEFINE_EVENT( wxEVT_GRID_VALUE_CHANGED, CGridEvent );
wxDEFINE_EVENT( wxEVT_GRID_ELEMENTS_CHANGED, wxCommandEvent );
wxDEFINE_EVENT( wXEVT_GRID_EDITING, CGridEvent );

BEGIN_EVENT_TABLE( CGridEditor, wxGrid )
	EVT_GRID_CELL_CHANGED( CGridEditor::OnCellChanged )
	EVT_GRID_EDITOR_SHOWN( CGridEditor::OnCellEditing )
	EVT_GRID_CELL_LEFT_CLICK( CGridEditor::OnCellLeftClick )
	EVT_GRID_CELL_RIGHT_CLICK( CGridEditor::OnCellRightClick )
	EVT_MOTION( CGridEditor::OnMouseMotion )
	EVT_SET_FOCUS( CGridEditor::OnFocus )
END_EVENT_TABLE()

IMPLEMENT_ENGINE_CLASS( CCopiedData );

CCopiedData * CGridEditor::m_copiedData;

class CElementWrapper : public wxObject
{
public:

	CElementWrapper( IRTTIType *type, void *data, Int32 index )
		: m_type( type )
		, m_data( data )
		, m_index( index )
	{
	}

	void *m_data;
	IRTTIType *m_type;
	const Int32 m_index;
};

CGridEditor::CGridEditor( wxWindow *parent, Bool allowEditPointers )
: COutlineGrid( parent )
, m_selectedType( NULL )
, m_selectedProperty( NULL )
, m_ctrlColorPicker( NULL )
, m_lockUpdateColumnsSize( false )
, m_undoManager( NULL )
, m_separateObjects( true )
, m_attrCustomizer( nullptr )
, m_allowEditPointers( allowEditPointers )
{
	CreateGrid( 0, 0 );
	GetGridColLabelWindow()->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( CGridEditor::OnColLabelRightClick ), NULL, this );
	GetGridColLabelWindow()->Connect( wxEVT_MOTION, wxMouseEventHandler( CGridEditor::OnColLabelMouseMotion ), NULL, this );
	GetGridWindow()->Connect( wxEVT_MOTION, wxMouseEventHandler( CGridEditor::OnMouseMotion ), NULL, this );
	GetGridWindow()->Connect( wxEVT_CHOICE_CHANGED, wxNotifyEventHandler( CGridEditor::OnChoiceChanged ), NULL, this );
	

	if ( m_copiedData == NULL )
	{
		m_copiedData = new CCopiedData();
		m_copiedData->AddToRootSet();
	}

	m_objectEditor = new CGridObjectEditor( this );

	SEvents::GetInstance().RegisterListener( CNAME( EditorPreUndoStep ), this );
}

CGridEditor::~CGridEditor()
{
	THashMap< CName, IGridTypeDesc * >::iterator typeDescIter;
	for ( typeDescIter = m_gridTypesDescMap.Begin(); typeDescIter != m_gridTypesDescMap.End(); ++typeDescIter )
	{
		delete ( *typeDescIter ).m_second;
	}
	m_gridTypesDescMap.Clear();

	THashMap< String, IGridColumnDesc* >::iterator columnDescIter;
	for ( columnDescIter = m_gridColumnDescMap.Begin(); columnDescIter != m_gridColumnDescMap.End(); ++columnDescIter )
	{
		delete ( *columnDescIter ).m_second;
	}
	m_gridColumnDescMap.Clear();

	for ( Uint32 i = 0; i < m_propertyWrappers.Size(); ++i )
	{
		delete m_propertyWrappers[i];
	}
	m_propertyWrappers.Clear();

	if ( m_attrCustomizer )
	{
		delete m_attrCustomizer;
	}

	SEvents::GetInstance().UnregisterListener( this );
}

void CGridEditor::SetObject( void *data, IRTTIType *type )
{
	m_selectedProperty = NULL;
	BeginBatch();

	// Remove existing wrappers
	for ( Uint32 i = 0; i < m_propertyWrappers.Size(); ++i )
		delete m_propertyWrappers[i];
	m_propertyWrappers.Clear();

	// Create new wrappers
	m_selectedType = type;
	if ( data && m_selectedType )
	{
		m_propertyWrappers.PushBack( new CGridPropertyWrapper( data, m_selectedType, this ) );

		CreateHeader();
		UpdateBlocks();
		ForceRefresh();
	}

	EndBatch();
}

void CGridEditor::SetObject( void *data, const CProperty *property )
{
	m_selectedProperty = property;
	BeginBatch();

	// Remove existing wrappers
	for ( Uint32 i = 0; i < m_propertyWrappers.Size(); ++i )
	{
		delete m_propertyWrappers[i];
	}
	m_propertyWrappers.ClearFast();

	// Create new wrappers
	m_selectedType = property->GetType();
	if ( data && m_selectedType )
	{
		m_propertyWrappers.PushBack( new CGridPropertyWrapper( data, m_selectedType, this ) );

		CreateHeader();
		UpdateBlocks();
		ForceRefresh();
	}

	EndBatch();
}

void CGridEditor::SetObjects( TDynArray< void* >& objects, IRTTIType & type, Bool separateWithBlankRow )
{
	m_separateObjects = separateWithBlankRow;
	m_selectedProperty = NULL;
	BeginBatch();

	// Remove existing wrappers
	for ( Uint32 i = 0; i < m_propertyWrappers.Size(); ++i )
		delete m_propertyWrappers[i];
	m_propertyWrappers.Clear();

	// Create new wrappers
	m_selectedType = & type;
	for ( Uint32 i = 0; i < objects.Size(); ++i )
	{
		void* data = objects[ i ];
		if ( data )
			m_propertyWrappers.PushBack( new CGridPropertyWrapper( data, m_selectedType, this ) );
	}

	CreateHeader();
	UpdateBlocks();
	ForceRefresh();

	EndBatch();
}

void CGridEditor::SetObjects( TDynArray< CObject* >& objects, Bool separateWithBlankRow )
{
	m_separateObjects = separateWithBlankRow;
	m_selectedProperty = NULL;
	BeginBatch();

	// Remove existing wrappers
	for ( Uint32 i = 0; i < m_propertyWrappers.Size(); ++i )
		delete m_propertyWrappers[i];
	m_propertyWrappers.Clear();

	// Create new wrappers
	m_selectedType = NULL;
	for ( Uint32 i = 0; i < objects.Size(); ++i )
	{
		void* data = objects[ i ];
		if ( m_selectedType == NULL )
		{
			m_selectedType = objects[ i ]->GetClass();
		}
		else if ( objects[ i ]->GetClass() != m_selectedType )
		{
			continue;
		}
		
		if ( data && m_selectedType )
		{
			m_propertyWrappers.PushBack( new CGridPropertyWrapper( data, m_selectedType, this ) );
		}
	}

	CreateHeader();
	UpdateBlocks();
	ForceRefresh();	

	EndBatch();
}

void CGridEditor::SaveLayout( const String &identifier )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	for ( THashMap<Int32, wxColour>::const_iterator it = m_colorsByColumnIndex.Begin(); it != m_colorsByColumnIndex.End(); ++it )
	{
		config.Write( identifier + String::Printf( TXT( "col%d" ), ( *it ).m_first ), ( *it ).m_second.GetAsString().wc_str() );
	}
}

void CGridEditor::LoadLayout( const String &identifier )
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	m_colorsByColumnIndex.Clear();
	for ( Int32 i = 0; i < GetNumberCols(); ++i )
	{
		String value;
		if ( config.Read( identifier + String::Printf( TXT( "col%d" ), i ), &value ) )
		{
			wxColour color( wxString( value.AsChar() ) );
			SetColumnColor( i, color );
		}
	}
}

Bool CGridEditor::IsColSeparator( Int32 col ) const
{
	TDynArray<wxString> columnNames;
	CGridPropertyWrapper::GetColumnNames( m_selectedType, columnNames, this, m_selectedProperty );
	if ( col >= 0 && col < ( Int32 )columnNames.Size() )
	{
		return ( columnNames[col].Cmp( TXT( "#" ) ) == 0 );
	}

	return false;
}

Bool CGridEditor::RegisterCustomType( IGridTypeDesc *typeDesc )
{
	ASSERT( typeDesc );
	if ( m_gridTypesDescMap.KeyExist( typeDesc->GetName() ) )
	{
		return false;
	}

	m_gridTypesDescMap.Insert( typeDesc->GetName(), typeDesc );
	return true;
}

IGridTypeDesc *CGridEditor::GetCustomTypeDesc( const CName &typeName ) const
{
	IGridTypeDesc *typeDesc;
	if ( m_gridTypesDescMap.Find( typeName, typeDesc ) )
	{
		return typeDesc;
	}

	return NULL;
}

Bool CGridEditor::RegisterCustomColumnDesc( const String& columnName, IGridColumnDesc* columnDesc )
{
	ASSERT( columnName.Empty() == false && columnDesc != NULL );


	if ( m_gridColumnDescMap.KeyExist( columnName ) == true )
	{
		return false;
	}
	m_gridColumnDescMap.Insert( columnName, columnDesc );
	return true;
}

Bool CGridEditor::UnregisterCustomColumnDesc( const String& columnName )
{
	return m_gridColumnDescMap.Erase( columnName );
}

IGridColumnDesc* CGridEditor::GetCustomColumnDesc( const wxString& columnName ) const
{
	IGridColumnDesc* columnDesc;
	if ( m_gridColumnDescMap.Find( columnName.wc_str(), columnDesc ) == true )
	{
		return columnDesc;
	}
	return NULL;
}

void CGridEditor::CreateHeader()
{
	if ( GetNumberCols() > 0 )
		DeleteCols( 0, GetNumberCols() );

	if ( !m_selectedType )
		return;

	SetRowLabelSize( 50 );
	SetColLabelSize( 30 );
	SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	SetLabelFont( wxFont( 8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD ) );
	SetLabelBackgroundColour( wxColour( 232, 238, 247 ) );
	SetLabelTextColour( wxColour( 23, 17, 8 ) );
	SetBackgroundColour( wxColour( 255, 255, 255 ) );
	SetSelectionBackground( wxColour( 215, 225, 245 ) );
	SetSelectionForeground( wxColour( 0, 0, 0 ) );
	SetCellHighlightColour( wxColour( 51, 102, 204 ) );
	SetHighlightColor( wxColour( 255, 0, 0 ) );
	SetDefaultCellBackgroundColour( GetBackgroundColour() );
	SetColMinimalAcceptableWidth( 0 );
	SetDefaultColSize( 10 );
	SetDefaultCellAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
	EnableGridLines( false );
	DisableCellEditControl();
	DisableDragGridSize();
	DisableDragColSize();
	DisableDragRowSize();

	wxGridCellAttr *separatorAttr = new wxGridCellAttr();
	separatorAttr->SetTextColour( wxColour( 255, 255, 255 ) );
	separatorAttr->SetBackgroundColour( wxColour( 100, 100, 100 ) );
	separatorAttr->SetRenderer( new CGridCellSeparatorRenderer );
	separatorAttr->SetReadOnly();

	TDynArray<wxString> columnNames;
	CGridPropertyWrapper::GetColumnNames( m_selectedType, columnNames, this, m_selectedProperty );
	AppendCols( ( Int32 )columnNames.Size() );
	int cols = GetNumberCols();
	ASSERT( ( Int32 )columnNames.Size() == GetNumberCols() );

	for ( Int32 col = 0; col < GetNumberCols(); ++col )
	{
		Bool isSeparator = ( columnNames[col].Cmp( TXT( "#" ) ) == 0 );
		if ( isSeparator )
		{
			SetColLabelValue( col, TXT( "" ) );
			separatorAttr->IncRef();
			SetColAttr( col, separatorAttr );
		}
		else
		{
			SetColLabelValue( col, columnNames[col] );
		}
	}

	UpdateColumnsSize();
}

void CGridEditor::UpdateBlocks()
{
	BeginBatch();

	Int32 scrollX, scrollY;
	GetViewStart( &scrollX, &scrollY );
	Int32 cursorRow = GetGridCursorRow();
	Int32 cursorColumn = GetGridCursorCol();

	ClearOutlines();
	if ( GetNumberRows() > 0 )
		DeleteRows( 0, GetNumberRows() );

	if ( !m_selectedType )
	{
		EndBatch();
		return;
	}

	wxGridCellAttr *emptyRowAttr;
	
	if ( m_separateObjects )
	{
		emptyRowAttr = new wxGridCellAttr;
		emptyRowAttr->SetBackgroundColour( GetBackgroundColour() );
		emptyRowAttr->SetReadOnly();
		emptyRowAttr->SetRenderer( new CGridCellEmptyRenderer );
	}

	const Int32 columnsCount = CGridPropertyWrapper::GetNumColumns( m_selectedType, this );
	TDynArray<wxString> columnNames;
	CGridPropertyWrapper::GetColumnNames( m_selectedType, columnNames, this, m_selectedProperty );

	ASSERT( columnsCount == columnNames.Size() );

	wxColour outlineColors[4] = 
	{
		wxColour( 247, 150, 70 ),	// level 0
		wxColour( 79, 129, 189 ),	// level 1
		wxColour( 0, 0, 0 ),		// level 2
	};

	Int32 rowOffset = 0;
	for ( Uint32 i = 0, n = m_propertyWrappers.Size(); i < n; ++i )
	{
		CGridPropertyWrapper* propWrapper = m_propertyWrappers[ i ];
		CGridPropertyWrapper::Cache cacheGuard( propWrapper );

		propWrapper->RefreshExpandableBlocks();
		const Int32 rowsCount = propWrapper->GetMaxRows();
		AppendRows( rowsCount );

		if ( m_separateObjects )
		{
			AppendRows(1);
		}

		for ( Int32 row = 0; row < rowsCount; ++row )
		{
			for ( Int32 col = 0; col < columnsCount; ++col )
			{
				// Check if current column is a separator
				Bool isSeparator = ( columnNames[col].Cmp( TXT( "#" ) ) == 0 );
				if ( isSeparator )
				{
					SGridExpandableBlock *block;
					Uint32 size;
					if ( propWrapper->IsExpandableCell( row, col, block, size ) )
					{
						wxString value;
						if ( size > 0 )
							value = block->m_expanded ? TXT( "-" ) : TXT( "+" );
						else
							value = TXT( "0" );
						SetCellValue( row + rowOffset, col, value );

						if ( propWrapper->IsReadOnlyCell( row, col ) )
						{
							wxGridCellAttr *cellAttr = new wxGridCellAttr;
							cellAttr->SetReadOnly( true );
							SetAttr( row + rowOffset, col, cellAttr );
						}
					}
				}
				else
				{
					const IRTTIType *type;
					wxString value = propWrapper->GetValue( row, col, type );

					IGridColumnDesc* columnDesc = GetCustomColumnDesc( columnNames[ col ] );

					wxGridCellRenderer* cellRenderer = NULL;
					wxGridCellEditor* cellEditor = NULL;
					if ( columnDesc != NULL && type != NULL )
					{

						// Deal with editors that requires additional info from other cell
						if ( columnDesc->DoesRequireExtraInfo() )
						{
							Int32 colNum = columnDesc->GetInterestColumnNumber();	

							const IRTTIType *extraInfoType = 0;
							void *extraInfoData = 0;
							CGridPropertyWrapper *wrapper = GetRelativeCoords( row, colNum );
							if ( wrapper )
							{
								// Find the destination cell
								Int32 rowNum = row;
								do
								{
									wrapper->GetValue( rowNum--, colNum, extraInfoData, extraInfoType );
								} while ( rowNum >= 0 && extraInfoType == NULL );

								columnDesc->SetExtraInfo( extraInfoType, extraInfoData );
							}
						}

						cellEditor = columnDesc->GetCellEditor();
						cellRenderer = columnDesc->GetCellRenderer();
					}

					if ( cellEditor != NULL )
					{
						SetCellEditor( row + rowOffset, col, cellEditor );
					}
					else
					{
						void *data = NULL;
						const IRTTIType* rttiType;
						propWrapper->GetValue( row, col, data, rttiType );
						SetCellEditorByType( row + rowOffset, col, type, data );
					}

					if ( cellRenderer != NULL )
					{
						SetCellRenderer( row + rowOffset, col, cellRenderer );
					}
					
					SetCellValue( row + rowOffset, col, value );

					if ( m_attrCustomizer )
					{
						wxGridCellAttr* cellAttr = GetOrCreateCellAttr( row + rowOffset, col );

						if ( cellAttr )
						{
							void* data = nullptr;
							const IRTTIType* type = nullptr;

							propWrapper->GetValue( data, type );

							void* cellData = nullptr;
							const IRTTIType* cellType = nullptr;

							if ( !propWrapper->GetValue( row, col, cellData, cellType ) )
							{
								cellData = nullptr;
								cellType = nullptr;
							}

							const CProperty * prop = CGridPropertyWrapper::GetProperty( m_selectedType, this, col, m_selectedProperty );
					
							m_attrCustomizer->CustomizeCellAttr( cellAttr, type, data, cellType, cellData, prop );
						}
					}
					
					if ( propWrapper->IsReadOnlyCell( row, col ) )
					{
						wxGridCellAttr *cellAttr = new wxGridCellAttr;
						cellAttr->SetReadOnly( true );
						SetAttr( row + rowOffset, col, cellAttr );
					}
				}
			}
		}

		// Set outlines for arrays elements
		TDynArray<SGridBlockRange> bounds;
		propWrapper->GetArraysBounds( bounds );
		for ( Uint32 j = 0; j < bounds.Size(); ++j )
		{
			SGridBlockRange &bound = bounds[j];
			Int32 colorIndex = max( 0, min( bound.m_level, sizeof( outlineColors ) / sizeof( wxColour ) - 1 ) );
			Int32 width = max( 1, min( 3 - bound.m_level, 2 ) );
			//SetOutline( bound.m_row + rowOffset, bound.m_column, bound.m_numRows, bound.m_numColumns, Outline_Right | Outline_Bottom, outlineColors[colorIndex], 1, false );
			SetOutline( bound.m_row + rowOffset, bound.m_column, bound.m_numRows, bound.m_numColumns, Outline_Vertical | Outline_Bottom, outlineColors[colorIndex], width, true );
		}

		// Set outline for the object wrapper
		SetOutline( rowOffset, 0, rowsCount, columnsCount, Outline_All, outlineColors[0], 2, true );

		rowOffset += rowsCount;

		if ( m_separateObjects )
		{
			rowOffset += 1;

			for ( Int32 col = 0; col < columnsCount; ++col )
			{
				emptyRowAttr->IncRef();
				SetAttr( rowOffset - 1, col, emptyRowAttr );
			}
		}
		//SetRowAttr( rowOffset - 1, rowAttr );
	}

	UpdateColumnsSize();

	Scroll( scrollX, scrollY );
	if ( cursorRow != -1 && cursorColumn != -1 )
		SetGridCursor( cursorRow, cursorColumn );

	EndBatch();
}

void CGridEditor::UpdateColumnsSize()
{
	if ( m_lockUpdateColumnsSize )
	{
		return;
	}
	Red::System::ScopedFlag< Bool > lock( m_lockUpdateColumnsSize = true, false );

	TDynArray<wxString> columnNames;
	CGridPropertyWrapper::GetColumnNames( m_selectedType, columnNames, this, m_selectedProperty );

	for ( Int32 col = 0; col < GetNumberCols(); ++col )
	{
		Bool isHidden = ( GetColSize( col ) == 0 );
		Bool isSeparator = ( columnNames[col].Cmp( TXT( "#" ) ) == 0 );
		if ( isSeparator )
		{
			//const Int32 level = columnNames[col].m_second;
			Int32 numVisibleColumns = 0;
			for ( Int32 i = col + 1; i < GetNumberCols(); ++i )
			{
				//if ( level > columnNames[i].m_second )
				//	break;
					
				if ( GetColSize( i ) > 0 && !GetColLabelValue( i ).IsEmpty() )
					++numVisibleColumns;
			}

			if ( isHidden && numVisibleColumns > 0 )
				SetColSize( col, 15 );

			if ( !isHidden && numVisibleColumns == 0 )
				SetColSize( col, 0 );
		}
	}

	for ( Int32 col = 0; col < GetNumberCols(); ++col )
	{
		// Ignore hidden columns
		Bool isHidden = ( GetColSize( col ) == 0 );
		if ( isHidden )
			continue;

		Bool isSeparator = ( columnNames[col].Cmp( TXT( "#" ) ) == 0 );
		if ( isSeparator )
		{
			//SetColMinimalWidth( col, 15 );
			SetColSize( col, 15 );
		}
		else
		{
			IGridColumnDesc* columnDesc = GetCustomColumnDesc( columnNames[ col ] );
			
			// Do not perform auto size for custom columns
			if ( columnDesc == NULL || columnDesc->AllowAutoSize() == true )
			{
				SetColSize( col, 20 );
				AutoSizeColumn( col );
			}
		}
	}
}

Bool CGridEditor::SetCellEditorByType( Int32 row, Int32 col, const IRTTIType *type, void* data )
{
	if ( type == NULL )
	{
		// Disable inactive cells
		SetReadOnly( row, col, true );
		SetCellBackgroundColour( row, col, wxColour( 216, 216, 216 ) );
		SetCellRenderer( row, col, new CGridCellEmptyRenderer );
		return false;
	}

	if ( IGridTypeDesc *typeDesc = GetCustomTypeDesc( type->GetName() ) )
	{
		SetCellAlignment( row, col, typeDesc->GetHorizontalAlignment(), typeDesc->GetVerticalAlignment() );

		if ( wxGridCellEditor *cellEditor = typeDesc->GetCellEditor( row, col ) )
		{
			SetCellEditor( row, col, cellEditor );
		}

		if ( wxGridCellRenderer *cellRenderer = typeDesc->GetCellRenderer() )
		{
			SetCellRenderer( row, col, cellRenderer );
		}

		return true;
	}

	Bool typeFound = true;

	// Simple types
	if ( type->GetType() == RT_Simple || type->GetType() == RT_Fundamental )
	{
		if ( type->GetName() == CNAME( Bool ) )
		{
			SetReadOnly( row, col );
			SetCellRenderer( row, col, new CGridCellBoolRenderer );
		}
		else if ( type->GetName() == CNAME( Float ) )
		{
			SetCellEditor( row, col, new wxGridCellFloatEditor );
			SetCellRenderer( row, col, new CGridCellFloatRenderer( 4, 4 ) );
		}
		else if ( ( type->GetName() == CNAME( Int32 ) ) || ( type->GetName() == CNAME( Uint32 ) ) )
		{
			SetCellEditor( row, col, new wxGridCellNumberEditor );
			SetCellRenderer( row, col, new CGridCellNumberRenderer );
		}
		else if ( type->GetName() == CNAME( String ) || type->GetName() == CNAME( CName ) )
		{
			// Leave default editor
			SetCellAlignment( row, col, wxALIGN_LEFT, wxALIGN_CENTER );
		}
		else
		{
			typeFound = false;
		}
	}
	// Enum types
	else if ( type->GetType() == RT_Enum )
	{
		wxArrayString choices;
		CEnum *enumType = ( CEnum * )type;
		const TDynArray<CName> &options = enumType->GetOptions();
		for ( Uint32 i = 0; i < options.Size(); ++i )
			choices.Add( options[i].AsString().AsChar() );
		SetCellEditor( row, col, new CGridCellChoiceEditor( choices, false ) );
	}
	// Class types
	else if ( type->GetType() == RT_Class )
	{
		CClass *classPtr = ( CClass * )type;
		if ( classPtr->IsA< CResource >() )
		{
			CResource* res = classPtr->GetDefaultObject< CResource >();
			SetCellAlignment( row, col, wxALIGN_LEFT, wxALIGN_CENTER );
			SetCellEditor( row, col, new CGridCellResourceEditor( res->GetExtension() ) );
			SetCellRenderer( row, col, new CGridCellResourceRenderer );
		}
		else if( classPtr->IsObject() )
		{
			SetCellAlignment( row, col, wxALIGN_LEFT, wxALIGN_CENTER );
			SetCellEditor( row, col, new CGridCellObjectEditor() );
			SetCellRenderer( row, col, new CGridCellObjectRenderer() );
		}
		else
		{
			typeFound = false;
		}
	}
	else if ( type->GetType() == RT_Pointer )
	{
		const IRTTIType *pointedType = ( ( CRTTIPointerType * )type )->GetPointedType();
		void* pointedData = AllowEditPointers() ? ( data ? ( ( CRTTIPointerType * )type )->GetPointed( data ) : nullptr ) :
												  ( ( CRTTIPointerType * )type )->GetPointed( data );
		return SetCellEditorByType( row, col, pointedType, pointedData );
	}
	else if ( type->GetType() == RT_Handle )
	{
		const IRTTIType *pointedType = ( ( CRTTIHandleType * )type )->GetPointedType();
		void* pointedData = ( ( CRTTIHandleType * )type )->GetPointed( data );
		return SetCellEditorByType( row, col, pointedType, pointedData );
	}
	else if ( type->GetType() == RT_SoftHandle )
	{
		void* pointedData = ( ( CRTTIHandleType * )type )->GetPointed( data );
		const IRTTIType *pointedType = ( ( CRTTISoftHandleType * )type )->GetPointedType();
		return SetCellEditorByType( row, col, pointedType, pointedData );
	}
	else
	{
		typeFound = false;
	}

	if ( !typeFound )
	{
		WARN_EDITOR( TXT( "Cannot assign any editor to the given type: %s" ), type->GetName().AsString().AsChar() );
	}

	return typeFound;
}

CGridPropertyWrapper *CGridEditor::GetRelativeCoords( Int32 &row, Int32 &col ) const
{
	for ( Uint32 i = 0; i < m_propertyWrappers.Size(); ++i )
	{
		Int32 numRows = m_propertyWrappers[i]->GetMaxRows();
		if ( m_separateObjects )
		{
			numRows += 1;
		}

		if ( row < numRows )
		{
			return m_propertyWrappers[i];
		}

		row -= numRows;
	}

	return NULL;
}

wxColour CGridEditor::GetColumnColor( Int32 col )
{
	wxColour color;
	if ( !m_colorsByColumnIndex.Find( col, color ) )
	{
		// Get default color
		color = GetBackgroundColour();
	}

	return color;
}

void CGridEditor::SetColumnColor( Int32 col, const wxColour &color )
{
	if ( col >= 0 && col < GetNumberCols() )
	{
		m_colorsByColumnIndex.Erase( col );

		if ( IsColSeparator( col ) )
			return;

		wxColour backgroundColor = GetBackgroundColour();
		if ( color != backgroundColor )
		{
			m_colorsByColumnIndex.Insert( col, color );
		}

		wxGridCellAttr *attr = new wxGridCellAttr();
		attr->SetBackgroundColour( color );
		SetColAttr( col, attr );
	}
}

void CGridEditor::ReparentInlinedObjects( IRTTIType* rttiType, void* data )
{
	if( rttiType->GetType() == RT_Class )
	{
		CClass* c = static_cast< CClass* >( rttiType );
		TDynArray< CProperty* > prop;
		c->GetProperties( prop );
		for( Uint32 i=0; i<prop.Size(); i++ )
		{
			ReparentInlinedObjects( prop[i]->GetType(), prop[i]->GetOffsetPtr( data ) );			
		}
	}
	else if( rttiType->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = static_cast< CRTTIArrayType* >( rttiType );	
		Uint32 s = arrayType->GetArraySize( data );
		for( Uint32 i=0; i<s; i++ )
		{
			void * elem = arrayType->GetArrayElement( data, i );
			ReparentInlinedObjects( arrayType->GetInnerType(), elem ); 
		}
	}
	else if( rttiType->GetType() == RT_Pointer )
	{
		CRTTIPointerType* pointerType = static_cast< CRTTIPointerType* >( rttiType );
		if( pointerType->GetPointedType()->GetType() == RT_Class )
		{
			CClass* internalClass = static_cast< CClass* >( pointerType->GetPointedType() );
			if( internalClass->IsObject() )
			{
				// CObject found
				CObject* obj = (CObject*)pointerType->GetPointed( data );
				if( obj )
				{
					obj->SetParent( m_defaultObjectParent );
				}
			}
		}
	}
}

void CGridEditor::UpdateGUIDs( IRTTIType* rttiType, void* data )
{
	if( rttiType->GetType() == RT_Class )
	{
		CClass* c = static_cast< CClass* >( rttiType );
		TDynArray< CProperty* > prop;
		c->GetProperties( prop );
		for( Uint32 i=0; i<prop.Size(); i++ )
		{
			UpdateGUIDs( prop[i]->GetType(), prop[i]->GetOffsetPtr( data ) );			
		}
	}
	else if( rttiType->GetType() == RT_Array )
	{
		CRTTIArrayType* arrayType = static_cast< CRTTIArrayType* >( rttiType );	
		Uint32 s = arrayType->GetArraySize( data );
		for( Uint32 i=0; i<s; i++ )
		{
			void * elem = arrayType->GetArrayElement( data, i );
			UpdateGUIDs( arrayType->GetInnerType(), elem ); 
		}
	}
	else if( rttiType->GetType() == RT_Simple )
	{
		if( rttiType->GetName() == TTypeName< CGUID >::GetTypeName() )
		{
			CGUID* guid = ( CGUID* )data;
			*guid = CGUID::Create();
		}
	}
}

void CGridEditor::ElementPaste( CElementWrapper *wrapper, Bool insert )
{
	if ( wrapper == NULL ) return;
	if ( wrapper->m_data == NULL ) return;

	if ( wrapper->m_type->GetType() == RT_Array )
	{
		const CRTTIArrayType *arrayType = static_cast<const CRTTIArrayType *>( wrapper->m_type );
		ASSERT( arrayType->GetInnerType() == m_copiedData->m_data.GetRTTIType() );

		if ( insert && !arrayType->InsertArrayElementAt( wrapper->m_data, wrapper->m_index ) )
		{
			const Uint32 size = arrayType->GetArraySize( wrapper->m_data );
			ERR_EDITOR( TXT( "Cannot insert new element to the array %s[%d] at index %d." ), wrapper->m_type->GetName().AsString().AsChar(), size, wrapper->m_index );	
		}
		else
		{
			if ( m_undoManager )
			{
				if ( insert )
				{
					CUndoGridElementExistance::CreateCreationStep( *m_undoManager, this, arrayType, wrapper->m_data, wrapper->m_index );
				}
				else
				{
					CUndoGridElementSnapshot::PrepareStep( *m_undoManager, this, arrayType, wrapper->m_data, wrapper->m_index );
					CUndoGridElementSnapshot::FinalizeStep( *m_undoManager );
				}
			}

			void * dstElement = arrayType->GetArrayElement( wrapper->m_data, wrapper->m_index );
			arrayType->GetInnerType()->Copy( dstElement, m_copiedData->m_data.GetData() );

			ReparentInlinedObjects( arrayType->GetInnerType(), dstElement );
			UpdateGUIDs( arrayType->GetInnerType(), dstElement );
			UpdateBlocks();

			// Prepare an event
			wxCommandEvent* e = new wxCommandEvent( wxEVT_GRID_ELEMENTS_CHANGED, GetId() );
			e->SetEventObject( this );
			e->SetInt( 1 );	// Store information about 1 element added;

			// Send it
			GetEventHandler()->QueueEvent( e );
		}
	}
	else if ( wrapper->m_type->GetType() == RT_Simple || wrapper->m_type->GetType() == RT_SoftHandle || wrapper->m_type->GetType() == RT_Fundamental  )
	{
		if ( m_undoManager )
		{
			CUndoGridElementSnapshot::PrepareStep( *m_undoManager, this, wrapper->m_type, wrapper->m_data );
			CUndoGridElementSnapshot::FinalizeStep( *m_undoManager );
		}

		if ( wrapper->m_type->GetName() == m_copiedData->m_data.GetType() )
		{
			wrapper->m_type->Copy( wrapper->m_data, m_copiedData->m_data.GetData() );
			UpdateGUIDs( wrapper->m_type, wrapper->m_data );
			UpdateBlocks();
		}
	}
	else
	{
		ERR_EDITOR( TXT( "Wrong format of the property. Array expected." ) );
	}
}

void CGridEditor::SetupEvent( CGridEvent& gridEvent, Int32 row, Int32 col )
{
	wxString value = GetCellValue( row, col );

	if ( CGridPropertyWrapper* wrapper = GetRelativeCoords( row, col ) )
	{
		void* data = nullptr;
		const IRTTIType* type = nullptr;

		wrapper->GetValue( data, type );
		gridEvent.SetGridData( type, data );

		void* cellData = nullptr;
		const IRTTIType* cellType = nullptr;

		if ( wrapper->GetValue( row, col, cellData, cellType ) )
		{
			gridEvent.SetCellData( cellType, cellData );
		}

		const CProperty * prop = CGridPropertyWrapper::GetProperty( m_selectedType, this, col, m_selectedProperty );
		gridEvent.SetCellProperty( prop );
	}

	gridEvent.SetEventObject( this );
}

void CGridEditor::OnCellEditing( wxGridEvent& event )
{
	// Prepare an event
	Int32 row = event.GetRow();
	Int32 col = event.GetCol();

	CGridEvent gridEvent( wXEVT_GRID_EDITING, GetId() );
	SetupEvent( gridEvent, row, col );

	// Send it
	GetEventHandler()->ProcessEvent( gridEvent );
	if ( !gridEvent.IsAllowed() )
	{
		event.Veto();
	}
}

void CGridEditor::OnCellChanged( wxGridEvent &event )
{
	Int32 row = event.GetRow();
	Int32 col = event.GetCol();
	wxString value = GetCellValue( row, col );

	if ( CGridPropertyWrapper *wrapper = GetRelativeCoords( row, col ) )
	{
		if ( m_undoManager )
		{
			const IRTTIType* type;
			wxString val = wrapper->GetValue( row, col, type );
			CUndoGridValueChange::CreateStep( *m_undoManager, this, wrapper, row, col, val );
		}

		if ( wrapper->SetValue( row, col, value ) )
		{
			// Prepare an event
			CGridEvent* gridEvent = new CGridEvent( wxEVT_GRID_VALUE_CHANGED, GetId() );
			SetupEvent( *gridEvent, event.GetRow(), event.GetCol() );

			gridEvent->SetString( value );
			gridEvent->SetEventObject( this );
			gridEvent->SetString( String::Printf( TXT( "Cell[%d, %d] value changed." ), row, col ).AsChar() );

			// Send it
			GetEventHandler()->QueueEvent( gridEvent );
		}
		else
		{
			WARN_EDITOR( TXT( "Unable to set value '%s' to cell [%d, %d]." ), value, row, col );

			const IRTTIType *type;
			wxString oldValue = wrapper->GetValue( row, col, type );
			SetCellValue( row, col, oldValue );
		}
	}
	UpdateColumnsSize();
	//ForceRefresh();

	event.Skip();
}

void CGridEditor::OnCellLeftClick( wxGridEvent &event )
{
	Int32 row = event.GetRow();
	Int32 col = event.GetCol();

	if ( CGridPropertyWrapper *wrapper = GetRelativeCoords( row, col ) )
	{
		SGridExpandableBlock *block;
		Uint32 size;
		if ( wrapper->IsExpandableCell( row, col, block, size ) )
		{
			if ( m_undoManager )
			{
				CUndoGridExpandCell::CreateStep( *m_undoManager, this, block );
			}

			block->m_expanded = !block->m_expanded;
			UpdateBlocks();
		}
		else if ( wrapper->IsReadOnlyCell( row, col ) == false )
		{
			const IRTTIType *type = NULL;
			wxString value = wrapper->GetValue( row, col, type );
			if ( type && type->GetName() == CNAME( Bool ) )
			{
				Bool isChecked = ( value == TXT( "true" ) );
				value = ( !isChecked ? TXT( "true" ) : TXT( "false" ) );

				if ( m_undoManager )
				{
					wxString prevVal = isChecked ? TXT( "true" ) : TXT( "false" );
					CUndoGridValueChange::CreateStep( *m_undoManager, this, wrapper, row, col, prevVal );
				}

				wrapper->SetValue( row, col, value );
				SetCellValue( event.GetRow(), event.GetCol(), value );
				OnCellChanged( event );
				return;
			}
		}
	}

	event.Skip();
}

void CGridEditor::OnCellRightClick( wxGridEvent &event )
{
	Int32 row = event.GetRow();
	Int32 col = event.GetCol();

	if ( CGridPropertyWrapper *wrapper = GetRelativeCoords( row, col ) )
	{
		SGridCellDescriptor desc;
		wrapper->GetCellDescriptor( row, col, desc );
		
		if ( desc.arrayData && desc.arrayType && ( desc.arrayElement != -1 || desc.arraySize == 0 ) )
		{
			Bool readOnly = wrapper->IsReadOnlyCell( desc.elementRange.y, desc.elementRange.x );
			if ( ! readOnly )
			{
				wxMenuItem *menuItem = NULL;
				//wxMenu menu( String::Printf( TXT( "Edit %s[%d] : %d" ), desc.arrayType->GetName().AsChar(), desc.arrayElement, desc.arraySize ).AsChar() );
				wxMenu menu;

				if ( desc.arraySize > 0 )
				{
					menu.Append( ID_MENU_ELEMENT_CUT, TXT( "Cut" ) );
					menu.Connect( ID_MENU_ELEMENT_CUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementCut ), new CElementWrapper( desc.arrayType, desc.arrayData, desc.arrayElement ), this );
					menu.Append( ID_MENU_ELEMENT_COPY, TXT( "Copy" ) );
					menu.Connect( ID_MENU_ELEMENT_COPY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementCopy ), new CElementWrapper( desc.arrayType, desc.arrayData, desc.arrayElement ), this );
					
					if( desc.cellData )
					{
						menu.Append( ID_MENU_ELEMENT_COPY_CUR_ELEM, TXT( "Copy current element" ) );
						menu.Connect( ID_MENU_ELEMENT_COPY_CUR_ELEM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementCopy ), new CElementWrapper( desc.cellType, desc.cellData, desc.arrayElement ), this );
					}
				}

				const CRTTIArrayType *arrayType = static_cast<const CRTTIArrayType *>( desc.arrayType );
				if ( arrayType->GetInnerType() == m_copiedData->m_data.GetRTTIType() )
				{
					if ( desc.arraySize > 0 )
					{
						menu.Append( ID_MENU_ELEMENT_PASTE_BEFORE, TXT( "Paste before" ) );
						menu.Connect( ID_MENU_ELEMENT_PASTE_BEFORE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementPaste ), new CElementWrapper( desc.arrayType, desc.arrayData, desc.arrayElement ), this );
						menu.Append( ID_MENU_ELEMENT_PASTE, TXT( "Paste" ) );
						menu.Connect( ID_MENU_ELEMENT_PASTE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementPasteIntoExisting ), new CElementWrapper( desc.arrayType, desc.arrayData, desc.arrayElement ), this );
						menu.Append( ID_MENU_ELEMENT_PASTE_AFTER, TXT( "Paste after" ) );
						menu.Connect( ID_MENU_ELEMENT_PASTE_AFTER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementPaste ), new CElementWrapper( desc.arrayType, desc.arrayData, desc.arrayElement+1 ), this );
					}
					else
					{
						menu.Append( ID_MENU_ELEMENT_PASTE_AFTER, TXT( "Paste" ) );
						menu.Connect( ID_MENU_ELEMENT_PASTE_AFTER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementPaste ), new CElementWrapper( desc.arrayType, desc.arrayData, 0 ), this );
					}
				}
				else if ( desc.cellType == m_copiedData->m_data.GetRTTIType() )
				{
					menu.Append( ID_MENU_ELEMENT_PASTE, TXT( "Paste" ) );
					menu.Connect( ID_MENU_ELEMENT_PASTE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementPasteIntoExisting ), new CElementWrapper( desc.cellType, desc.cellData, desc.arrayElement ), this );
				}

				menu.AppendSeparator();

				menu.Append( ID_MENU_ELEMENT_ADD, TXT( "Add element" ) );
				menu.Connect( ID_MENU_ELEMENT_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementInsert ), new CElementWrapper( desc.arrayType, desc.arrayData, desc.arraySize ), this );
				if ( desc.arrayElement != -1 )
				{
					menu.Append( ID_MENU_ELEMENT_INSERT_BEFORE, TXT( "Insert before" ) );
					menu.Connect( ID_MENU_ELEMENT_INSERT_BEFORE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementInsert ), new CElementWrapper( desc.arrayType, desc.arrayData, desc.arrayElement ), this );
					menu.Append( ID_MENU_ELEMENT_INSERT_AFTER, TXT( "Insert after" ) );
					menu.Connect( ID_MENU_ELEMENT_INSERT_AFTER, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementInsert ), new CElementWrapper( desc.arrayType, desc.arrayData, desc.arrayElement + 1 ), this );
					menu.Append( ID_MENU_ELEMENT_REMOVE, TXT( "Remove element" ) );
					menu.Connect( ID_MENU_ELEMENT_REMOVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementRemove ), new CElementWrapper( desc.arrayType, desc.arrayData, desc.arrayElement ), this );
				}

				if ( desc.arraySize > 0 )
				{
					menu.AppendSeparator();
					if ( desc.arrayElement > 0 )
					{
						menu.Append( ID_MENU_ELEMENT_MOVE_UP, TXT( "Move up" ) );
						menu.Connect( ID_MENU_ELEMENT_MOVE_UP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementMoveUp ), new CElementWrapper( desc.arrayType, desc.arrayData, desc.arrayElement ), this );
					}

					if ( desc.arrayElement < desc.arraySize - 1 )
					{
						menu.Append( ID_MENU_ELEMENT_MOVE_DOWN, TXT( "Move down" ) );
						menu.Connect( ID_MENU_ELEMENT_MOVE_DOWN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnElementMoveDown ), new CElementWrapper( desc.arrayType, desc.arrayData, desc.arrayElement ), this );
					}
				}

				if( desc.cellType && desc.cellType->GetType() == RT_Pointer )
				{
					CRTTIPointerType* pointerType = static_cast< CRTTIPointerType* >( desc.cellType );
					if( pointerType->GetPointedType()->GetType() == RT_Class )
					{
						menu.AppendSeparator();					
						if( pointerType->GetPointed( desc.cellData ) == NULL )
						{
							menu.Append( ID_MENU_OBJECT_CREATE, TXT( "Create object" ) );
							menu.Connect( ID_MENU_OBJECT_CREATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnObjectCreate ), new CElementWrapper( desc.cellType, desc.cellData, desc.arrayElement ), this ); 
						}
						else
						{
							menu.Append( ID_MENU_OBJECT_DELETE, TXT( "Delete object" ) );
							menu.Connect( ID_MENU_OBJECT_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnObjectDelete ), new CElementWrapper( desc.cellType, desc.cellData, desc.arrayElement ), this ); 
						}
					}
				}

				PopupMenu( &menu );
			}
		}
	}

	event.Skip();
}

class CColumnWrapper : public wxObject
{
public:
	wxArrayInt selectedColumnsId;

	CColumnWrapper( const wxArrayInt &arrayInt )
	{
		for ( wxArrayInt::const_iterator it = arrayInt.begin(); it != arrayInt.end(); ++it )
			selectedColumnsId.push_back( *it );
	}
};

void CGridEditor::OnColLabelRightClick( wxMouseEvent& event )
{
	wxPoint p = CalcUnscrolledPosition( wxPoint( event.GetX(), event.GetY() ) );
	const Int32 col = XToCol( p.x );

	if ( col != wxNOT_FOUND )
	{
		// Ignore an interaction with separating columns
		if ( GetColLabelValue( col ).IsEmpty() )
			return;
	}

	// Count visible columns and separators
	Int32 numVisibleColumns = 0;
	Int32 numSeparators = 0;
	for ( Int32 i = 0; i < GetNumberCols(); ++i )
	{
		// Ignore separators
		if ( GetColLabelValue( i ).IsEmpty() )
			++numSeparators;

		if ( GetColSize( i ) > 0 )
			++numVisibleColumns;
	}

	wxArrayInt arrayInt = GetSelectedCols();
	wxArrayInt selectedColumnsArray;
	for ( wxArrayInt::const_iterator it = arrayInt.begin(); it != arrayInt.end(); ++it )
	{
		if ( !GetColLabelValue( *it ).IsEmpty() && GetColSize( *it ) > 0 )
			selectedColumnsArray.push_back( *it );
	}

	if ( col != wxNOT_FOUND && selectedColumnsArray.empty() )
		selectedColumnsArray.push_back( col );

	wxMenuItem *menuItem = NULL;
	wxMenu menu( String::Printf( TXT( "Columns %d ( %d )" ), numVisibleColumns - numSeparators, GetNumberCols() - numSeparators ).AsChar() );

	if ( col != wxNOT_FOUND )
	{
		menu.Append( ID_MENU_COLUMN_HIDE, TXT( "Hide" ) );
		if ( numVisibleColumns == 0 )
			menu.Enable( ID_MENU_COLUMN_HIDE, false );
		else
			menu.Connect( ID_MENU_COLUMN_HIDE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnColumnHide ), new CColumnWrapper( selectedColumnsArray ), this );
	}
	
	menu.Append( ID_MENU_COLUMN_SHOWALL, TXT( "Show All" ) );
	if ( numVisibleColumns == GetNumberCols() )
		menu.Enable( ID_MENU_COLUMN_SHOWALL, false );
	else
		menu.Connect( ID_MENU_COLUMN_SHOWALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnColumnShowAll ), NULL, this );

	if ( col != wxNOT_FOUND )
	{
		menu.AppendSeparator();
		menu.Append( ID_MENU_COLUMN_SETCOLOR, TXT( "Column Color" ) );
		menu.Connect( ID_MENU_COLUMN_SETCOLOR, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnColumnSetColor ), new CColumnWrapper( selectedColumnsArray ), this );
	}

	// Add menu separator if first column in not a separating column
	if ( GetNumberCols() > 0 && !GetColLabelValue( 0 ).IsEmpty() )
		menu.AppendSeparator();

	for ( Int32 i = 0; i < GetNumberCols(); ++i )
	{
		wxString label = GetColLabelValue( i );
		if ( label.IsEmpty() )
		{
			menu.AppendSeparator();
		}
		else
		{
			menuItem = menu.AppendCheckItem( ID_MENU_COLUMN_TOGGLE + i, label );
			menuItem->Check( GetColSize( i ) > 0 );
			menu.Connect( ID_MENU_COLUMN_TOGGLE + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CGridEditor::OnColumnToggle ), NULL, this );
		}
	}

	PopupMenu( &menu );
}

void CGridEditor::OnColLabelMouseMotion( wxMouseEvent& event )
{
	wxPoint p = CalcUnscrolledPosition( wxPoint( event.GetX(), event.GetY() ) );
	const Int32 col = XToCol( p.x );

	if ( col != wxNOT_FOUND )
	{
		// Ignore an interaction with separating columns
		const CProperty * prop = CGridPropertyWrapper::GetProperty( m_selectedType, this, col, m_selectedProperty );
		if ( prop )
		{
			GetGridColLabelWindow()->SetToolTip( prop->GetHint().AsChar() );
			return;
		}
	}

	GetGridColLabelWindow()->SetToolTip( wxString() );
}

void CGridEditor::OnMouseMotion( wxMouseEvent& event )
{
	Bool resetHighlight = true;
	wxPoint p = CalcUnscrolledPosition( wxPoint( event.GetX(), event.GetY() ) );
	const Int32 col = XToCol( p.x );
	const Int32 row = YToRow( p.y );

	if ( row != wxNOT_FOUND && col != wxNOT_FOUND )
	{
		Int32 relativeRow = row;
		Int32 relativeCol = col;
		if ( CGridPropertyWrapper *wrapper = GetRelativeCoords( relativeRow, relativeCol ) )
		{
			const Int32 deltaRow = row - relativeRow;
			const Int32 deltaCol = col - relativeCol;

			SGridCellDescriptor desc;
			wrapper->GetCellDescriptor( relativeRow, relativeCol, desc );
			if ( desc.isSeparator || desc.arrayElement != -1 )
			{
				wxRect rc = desc.elementRange;
				rc.x += deltaCol;
				rc.y += deltaRow;
				SetHighlightRange( rc );
				resetHighlight = false;
			}
		}
	}

	// Reset highlight range
	if ( resetHighlight )
	{
		SetHighlightRange( 0, 0, -1, -1 );
	}
}

void CGridEditor::OnElementCut( wxCommandEvent& event )
{
	HideObjectEditor();

	if ( CElementWrapper *wrapper = ( CElementWrapper * )( event.m_callbackUserData ) )
	{
		if ( wrapper->m_type->GetType() == RT_Array )
		{
			const CRTTIArrayType *arrayType = static_cast<const CRTTIArrayType *>( wrapper->m_type );
			m_copiedData->m_data.~CVariant();
			new ( &m_copiedData->m_data ) CVariant( arrayType->GetInnerType()->GetName(), arrayType->GetArrayElement( wrapper->m_data, wrapper->m_index ) );
			OnElementRemove( event );
		}
		else
		{
			ERR_EDITOR( TXT( "Wrong format of the property. Array expected." ) );
		}
	}
}

void CGridEditor::OnElementCopy( wxCommandEvent& event )
{
	if ( CElementWrapper *wrapper = ( CElementWrapper * )( event.m_callbackUserData ) )
	{
		if( wrapper->m_type == nullptr )
		{
			ERR_EDITOR( TXT( "nullptr? Why?" ) );
		}
		else if ( wrapper->m_type->GetType() == RT_Array )
		{
			const CRTTIArrayType *arrayType = static_cast<const CRTTIArrayType *>( wrapper->m_type );

			m_copiedData->m_data.~CVariant();
			new ( &m_copiedData->m_data ) CVariant( arrayType->GetInnerType()->GetName(), arrayType->GetArrayElement( wrapper->m_data, wrapper->m_index ) );
		}
		else if ( wrapper->m_type->GetType() == RT_Simple || wrapper->m_type->GetType() == RT_SoftHandle || wrapper->m_type->GetType() == RT_Fundamental )
		{
			m_copiedData->m_data.~CVariant();
			new( &m_copiedData->m_data ) CVariant( wrapper->m_type->GetName(), wrapper->m_data );
		}
		else
		{
			ERR_EDITOR( TXT( "Wrong format of the property. Array expected." ) );
		}
	}
}

void CGridEditor::OnElementPaste( wxCommandEvent& event )
{
	if ( CElementWrapper *wrapper = ( CElementWrapper * )( event.m_callbackUserData ) )
	{
		ElementPaste( wrapper, true );
	}
}

void CGridEditor::OnElementPasteIntoExisting( wxCommandEvent& event )
{
	if ( CElementWrapper *wrapper = ( CElementWrapper * )( event.m_callbackUserData ) )
	{
		ElementPaste( wrapper, false );
	}
}

void CGridEditor::OnElementInsert( wxCommandEvent& event )
{
	if ( CElementWrapper *wrapper = ( CElementWrapper * )( event.m_callbackUserData ) )
	{
		if ( wrapper->m_type->GetType() == RT_Array )
		{
			const CRTTIArrayType *arrayType = static_cast<const CRTTIArrayType *>( wrapper->m_type );
			if ( arrayType->InsertArrayElementAt( wrapper->m_data, wrapper->m_index ) )
			{
				// is the inner type a pointer?
				if( AllowEditPointers() && arrayType->GetInnerType()->GetType() == RT_Pointer )
				{
					const IRTTIType *pointedType = ( ( CRTTIPointerType * )arrayType->GetInnerType() )->GetPointedType();
					const CClass* itemClass = static_cast< const CClass* >( pointedType );
					if( itemClass && itemClass->IsA< CObject >() )
					{
						CObject* newObject = CreateObject<CObject>( const_cast<CClass*>( itemClass ), m_defaultObjectParent, OF_Inlined );
						newObject->OnCreatedInEditor();
						void* createdObject = ( const_cast< void* >( arrayType->GetArrayElement( wrapper->m_data, wrapper->m_index ) ) );
						arrayType->GetInnerType()->Copy( createdObject, &newObject );
					}
				}

				if ( m_undoManager )
				{
					CUndoGridElementExistance::CreateCreationStep( *m_undoManager, this, arrayType, wrapper->m_data, wrapper->m_index );
				}

				UpdateBlocks();

				// Prepare an event
				wxCommandEvent* e = new wxCommandEvent( wxEVT_GRID_ELEMENTS_CHANGED, GetId() );
				e->SetEventObject( this );
				e->SetInt( 1 );	// Store information about 1 element added;

				// Send it
				GetEventHandler()->QueueEvent( e );
			}
			else
			{
				const Uint32 size = arrayType->GetArraySize( wrapper->m_data );
				ERR_EDITOR( TXT( "Cannot insert new element to the array %s[%d] at index %d." ), wrapper->m_type->GetName().AsString().AsChar(), size, wrapper->m_index );
			}
		}
		else
		{
			ERR_EDITOR( TXT( "Wrong format of the property. Array expected." ) );
		}
	}
}

void CGridEditor::OnElementRemove( wxCommandEvent& event )
{
	HideObjectEditor();

	if ( CElementWrapper *wrapper = ( CElementWrapper * )( event.m_callbackUserData ) )
	{
		if ( wrapper->m_type->GetType() == RT_Array )
		{
			const CRTTIArrayType *arrayType = static_cast<const CRTTIArrayType *>( wrapper->m_type );

			if ( m_undoManager )
			{
				CUndoGridElementExistance::CreateDeletionStep( *m_undoManager, this, arrayType, wrapper->m_data, wrapper->m_index );
			}				

			if ( arrayType->DeleteArrayElement( wrapper->m_data, wrapper->m_index ) )
			{
				UpdateBlocks();

				// Prepare an event
				wxCommandEvent* e = new wxCommandEvent( wxEVT_GRID_ELEMENTS_CHANGED, GetId() );
				e->SetEventObject( this );
				e->SetInt( -1 );	// Store information about 1 element removed;

				// Send it
				GetEventHandler()->QueueEvent( e );
			}
			else
			{
				const Uint32 size = arrayType->GetArraySize( wrapper->m_data );
				ERR_EDITOR( TXT( "Cannot remove the element at %d from the array %s[%d]." ), wrapper->m_index, wrapper->m_type->GetName().AsString().AsChar(), size );
			}
		}
		else
		{
			ERR_EDITOR( TXT( "Wrong format of the property. Array expected." ) );
		}
	}
}

void CGridEditor::OnElementMoveUp( wxCommandEvent& event )
{
	if ( CElementWrapper *wrapper = ( CElementWrapper * )( event.m_callbackUserData ) )
	{
		ASSERT( wrapper->m_index > 0 );
		if ( wrapper->m_type->GetType() == RT_Array )
		{
			const CRTTIArrayType *arrayType = static_cast<const CRTTIArrayType *>( wrapper->m_type );
			const Int32 index = wrapper->m_index;
			if ( arrayType->InsertArrayElementAt( wrapper->m_data, index - 1 ) )
			{
				if ( m_undoManager )
				{
					CUndoGridElementMove::CreateUpStep( *m_undoManager, this, arrayType, wrapper->m_data, index );
				}

				void *arrayElementDest = arrayType->GetArrayElement( wrapper->m_data, index - 1 );
				void *arrayElementSrc = arrayType->GetArrayElement( wrapper->m_data, index + 1 );
				arrayType->GetInnerType()->Copy( arrayElementDest, arrayElementSrc );
				if ( arrayType->DeleteArrayElement( wrapper->m_data, index + 1 ) )
				{
					UpdateBlocks();
				}
			}
		}
	}
}

void CGridEditor::OnElementMoveDown( wxCommandEvent& event )
{
	if ( CElementWrapper *wrapper = ( CElementWrapper * )( event.m_callbackUserData ) )
	{
		if ( wrapper->m_type->GetType() == RT_Array )
		{
			const CRTTIArrayType *arrayType = static_cast<const CRTTIArrayType *>( wrapper->m_type );
			const Int32 index = wrapper->m_index;
			if ( arrayType->InsertArrayElementAt( wrapper->m_data, index + 2 ) )
			{
				if ( m_undoManager )
				{
					CUndoGridElementMove::CreateDownStep( *m_undoManager, this, arrayType, wrapper->m_data, index );
				}

				void *arrayElementDest = arrayType->GetArrayElement( wrapper->m_data, index + 2 );
				void *arrayElementSrc = arrayType->GetArrayElement( wrapper->m_data, index );
				arrayType->GetInnerType()->Copy( arrayElementDest, arrayElementSrc );
				if ( arrayType->DeleteArrayElement( wrapper->m_data, index ) )
				{
					UpdateBlocks();
				}
			}
		}
	}
}

void CGridEditor::OnObjectCreate( wxCommandEvent& event )
{
	if ( CElementWrapper *wrapper = ( CElementWrapper * )( event.m_callbackUserData ) )
	{
		ASSERT( wrapper->m_type->GetType() == RT_Pointer );
		CRTTIPointerType* pointerType = static_cast< CRTTIPointerType* >( wrapper->m_type );
		ASSERT( pointerType->GetPointedType()->GetType() == RT_Class );
		CClass* c = static_cast< CClass* >( pointerType->GetPointedType() );
		CObject* obj = c->GetDefaultObject< CObject >()->Clone( m_defaultObjectParent );
		*((void**)wrapper->m_data) = (void*)obj;
		UpdateBlocks();
	}
}

void CGridEditor::OnObjectDelete( wxCommandEvent& event )
{
	if ( CElementWrapper *wrapper = ( CElementWrapper * )( event.m_callbackUserData ) )
	{
		ASSERT( wrapper->m_type->GetType() == RT_Pointer );
		CRTTIPointerType* pointerType = static_cast< CRTTIPointerType* >( wrapper->m_type );
		ASSERT( pointerType->GetPointedType()->GetType() == RT_Class );
		*((void**)wrapper->m_data) = NULL;
		UpdateBlocks();
		HideObjectEditor();
	}
}

void CGridEditor::OnColumnHide( wxCommandEvent& event )
{
	CColumnWrapper *wrapper = ( CColumnWrapper * )( event.m_callbackUserData );
	for ( wxArrayInt::const_iterator it = wrapper->selectedColumnsId.begin(); it != wrapper->selectedColumnsId.end(); ++it )
	{
		SetColMinimalWidth( *it, 0 );
		SetColSize( *it, 0 );
	}
	UpdateColumnsSize();
	ForceRefresh();
}

void CGridEditor::OnColumnShowAll( wxCommandEvent& event )
{
	for ( Int32 col = 0; col < GetNumberCols(); ++col )
	{
		SetColSize( col, 1 );
	}

	AutoSizeColumns();
	UpdateColumnsSize();
	ForceRefresh();
}

void CGridEditor::OnColumnSetColor( wxCommandEvent& event )
{
	CColumnWrapper *wrapper = ( CColumnWrapper * )( event.m_callbackUserData );
	if ( !wrapper->selectedColumnsId.empty() )
	{
		wxColour columnColor = GetColumnColor( *wrapper->selectedColumnsId.begin() );

		m_ctrlColorPicker = new CEdColorPicker( this );
		m_ctrlColorPicker->Bind( wxEVT_COMMAND_SCROLLBAR_UPDATED, &CGridEditor::OnColorPicked, this, wxID_ANY, wxID_ANY, new CColumnWrapper( wrapper->selectedColumnsId ) );
		m_ctrlColorPicker->Bind( wxEVT_SHOW, &CGridEditor::OnColorPickerShow, this );
		m_ctrlColorPicker->Show( Color( columnColor.Red(), columnColor.Green(), columnColor.Blue() ) );
	}
}

void CGridEditor::OnColumnToggle( wxCommandEvent& event )
{
	Int32 col = event.GetId() - ID_MENU_COLUMN_TOGGLE;
	Int32 columnWidth = GetColSize( col );

	if ( columnWidth == 0 )
	{
		AutoSizeColumn( col );
	}
	else
	{
		SetColMinimalWidth( col, 0 );
		SetColSize( col, 0 );
	}
	UpdateColumnsSize();
	ForceRefresh();
}

void CGridEditor::OnColorPicked( wxCommandEvent& event )
{
	ASSERT( m_ctrlColorPicker );

	ClearSelection();

	Color pickedColor = m_ctrlColorPicker->GetColor();
	wxColour newColor( pickedColor.R, pickedColor.G, pickedColor.B, pickedColor.A );	

	CColumnWrapper *wrapper = ( CColumnWrapper * )( event.m_callbackUserData );
	for ( wxArrayInt::const_iterator it = wrapper->selectedColumnsId.begin(); it != wrapper->selectedColumnsId.end(); ++it )
	{
		SetColumnColor( *it, newColor );
	}

	ForceRefresh();
}

void CGridEditor::OnColorPickerShow( wxShowEvent& event )
{
	if ( !event.IsShown() && m_ctrlColorPicker )
	{
		wxPendingDelete.Append( m_ctrlColorPicker );
		m_ctrlColorPicker = nullptr;
	}
}

void CGridEditor::ShowObjectEditor( CObject* object )
{
	m_objectEditor->ShowEditor( object );
}

void CGridEditor::HideObjectEditor()
{
	m_objectEditor->HideEditor();
	Refresh();
}

void CGridEditor::SetUndoManager( CEdUndoManager* undoManager )	
{ 
	m_undoManager = undoManager; 
	m_objectEditor->SetUndoManager( undoManager );
}

void CGridEditor::OnFocus( wxFocusEvent& event )
{
	UpdateBlocks();
	event.Skip();
}

void CGridEditor::OnChoiceChanged( wxNotifyEvent& event )
{
	UpdateBlocks();
}


void CGridEditor::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( EditorPreUndoStep ) )
	{
		 if ( m_undoManager == GetEventData< CEdUndoManager* >( data ) )
		 {
			 if ( IsCellEditControlEnabled() )
			 {
				 DisableCellEditControl();
			 }
		 }
	}
}

BEGIN_EVENT_TABLE( CGridObjectEditor, wxFrame )
	EVT_CLOSE( CGridObjectEditor::OnClose )
END_EVENT_TABLE()

CGridObjectEditor::CGridObjectEditor( wxWindow* parent )
: wxFrame( parent, wxID_ANY, wxT("Object editor") )
{
	PropertiesPageSettings settings;
	settings.m_showEntityComponents = false;	
	m_properties = new CEdSelectionProperties( this, settings, nullptr );
}

void CGridObjectEditor::ShowEditor( CObject* object )
{
	ASSERT( object );
	SetSize( 500, 500 );
	m_properties->Get().SetObject( object );

	SetTitle( object->GetClass()->GetName().AsString().AsChar() );

	m_properties->Show();
	Show();
	SetFocus();
}

void CGridObjectEditor::HideEditor()
{
	m_properties->Get().SetNoObject();
	Hide();
}

void CGridObjectEditor::OnClose(wxCloseEvent& event)
{
	HideEditor();
}

void CGridObjectEditor::SetUndoManager( CEdUndoManager* undoManager )
{
	m_properties->Get().SetUndoManager( undoManager );
}

