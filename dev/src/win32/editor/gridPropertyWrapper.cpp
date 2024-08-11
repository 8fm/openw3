#include "build.h"
#include "gridTypeDesc.h"
#include "gridEditor.h"
#include "gridPropertyWrapper.h"


///////////////////////////////////////////////////////////////////////////////
// CGridPropertyWrapper::SComputationCache
///////////////////////////////////////////////////////////////////////////////

CGridPropertyWrapper::SComputationCache::SComputationCache()
	: m_isCaching( false )
{

}
CGridPropertyWrapper::SComputationCache::~SComputationCache()
{

}
void CGridPropertyWrapper::SComputationCache::StartCaching()
{
	m_isCaching = true;
}
void CGridPropertyWrapper::SComputationCache::StopCaching()
{
	m_isCaching = false;
	m_numRows.ClearFast();
	//m_values.ClearFast();
}

//Bool CGridPropertyWrapper::SComputationCache::GetValue( Int32 row, Int32 col, void *&data, const IRTTIType *&type ) const
//{
//	TypedPtr p;
//	if ( m_values.Find( Loc( Hash( row, col ), data ), p ) )
//	{
//		data = p.m_data;
//		type = p.m_type;
//		return true;
//	}
//	return false;
//}
//void CGridPropertyWrapper::SComputationCache::SetValue( Int32 row, Int32 col, void *basePtr, void *data, const IRTTIType *type )
//{
//	TypedPtr p;
//	p.m_data = data;
//	p.m_type = type;
//	m_values.Insert( Loc( Hash( row, col ), basePtr ), p );
//}

Bool CGridPropertyWrapper::SComputationCache::GetNumRowsForObject( const void *data, const IRTTIType *type, Int32& outRows ) const
{
	TypedPtr p;
	p.m_data = data;
	p.m_type = type;
	return m_numRows.Find( p, outRows );
}
void CGridPropertyWrapper::SComputationCache::SetNumRowsForObject( const void *data, const IRTTIType *type, Int32 rows )
{
	TypedPtr p;
	p.m_data = data;
	p.m_type = type;
	m_numRows.Insert( p, rows );
}

///////////////////////////////////////////////////////////////////////////////
// CGridPropertyWrapper::Cache
///////////////////////////////////////////////////////////////////////////////
CGridPropertyWrapper::Cache::Cache( CGridPropertyWrapper* owner )
	: m_owner( owner )
{
	m_owner->m_computationCache.StartCaching();
}
CGridPropertyWrapper::Cache::~Cache()
{
	m_owner->m_computationCache.StopCaching();
}


///////////////////////////////////////////////////////////////////////////////
// CGridPropertyWrapper
///////////////////////////////////////////////////////////////////////////////


CGridPropertyWrapper::CGridPropertyWrapper(void *data, const IRTTIType *type, const CGridEditor *grid)
: m_data(data)
, m_type(type)
, m_grid(grid)
{
	ASSERT(m_data);
	ASSERT(m_type);
	ASSERT(m_grid);

	InitExpandableBlocks(m_data, m_type);
}

Int32 CGridPropertyWrapper::GetMaxRows() const
{
	return GetNumRowsForObject(m_data, m_type);
}

Int32 CGridPropertyWrapper::GetNumRowsForObject(const void *data, const IRTTIType *type) const
{
	ERTTITypeType typeType = type->GetType();
	if ( typeType != RT_Class && typeType != RT_Array )
	{
		return 1;
	}
	Int32 maxRows = 1;
	if ( m_computationCache.IsCaching() )
	{
		if ( m_computationCache.GetNumRowsForObject( data, type, maxRows ) )
		{
			return maxRows;
		}
	}

	if ( typeType == RT_Class )
	{
		CClass *classPtr = (CClass *)type;
		TDynArray< CProperty* > properties;
		classPtr->GetProperties( properties );
		//const auto& properties = classPtr->GetCachedProperties();
		for (Uint32 i = 0, n = properties.Size(); i < n; ++i)
		{
			CProperty *propertyPtr = properties[i];
			if ( ! propertyPtr->IsEditable() )
				continue;

			const void *propertyData = propertyPtr->GetOffsetPtr(data);
			Int32 rows = GetNumRowsForObject(propertyData, propertyPtr->GetType());
			maxRows = Max( maxRows, rows );
		}
	}
	else // typeType == RT_Array
	{
		CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
		IRTTIType *innerType = arrayType->GetInnerType();
		const void *arrayData = data;
		const SGridExpandableBlock *blockPtr = m_expandableBlocks.FindPtr(const_cast<void *>(arrayData));
		if (blockPtr->m_expanded)
		{
			Int32 numRows = 0;
			for (Uint32 i = 0, size = arrayType->GetArraySize(arrayData); i < size; ++i)
			{
				const void *elementData = arrayType->GetArrayElement(arrayData, i);
				numRows += GetNumRowsForObject(elementData, innerType);
			}
			if ( numRows > 1 )
			{
				maxRows = numRows;
			}
		}
	}

	if ( m_computationCache.IsCaching() )
	{
		m_computationCache.SetNumRowsForObject( data, type, maxRows );
	} 
	return maxRows;
}

Uint32 CGridPropertyWrapper::GetNumColumns(const IRTTIType *type, const CGridEditor *grid)
{
	return GetNumColumnsForType(type, grid);
}

Uint32 CGridPropertyWrapper::GetNumColumnsForType(const IRTTIType *type, const CGridEditor *grid)
{
	if ( grid )
	{
		if ( IGridTypeDesc *typeDesc = grid->GetCustomTypeDesc( type->GetName() ) )
		{
			return 1;
		}
	}

	if (type->GetType() == RT_Class)
	{
		Uint32 columns = 0;
		CClass *classPtr = (CClass *)type;

		TDynArray< CProperty* > properties;
		classPtr->GetProperties( properties );
		//const auto& properties = classPtr->GetCachedProperties();
		for (Uint32 i = 0, n = properties.Size(); i < n; ++i)
		{
			if ( ! properties[i]->IsEditable() )
				continue;
			columns += GetNumColumnsForType( properties[i]->GetType(), grid );
		}

		return columns;
	}
	else if( grid->AllowEditPointers() && type->GetType() == RT_Pointer )
	{
		const IRTTIType *pointedType = ( ( CRTTIPointerType * )type )->GetPointedType();
		return GetNumColumnsForType(pointedType, grid);
	}
	else if (type->GetType() == RT_Array)
	{
		CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
		IRTTIType *innerType = arrayType->GetInnerType();
		return GetNumColumnsForType( innerType, grid ) + 1;
	}
	else
	{
		return 1;
	}
}

namespace
{
	String FormatName( const String & name )
	{
		if ( name.Empty() )
			return String::EMPTY;

		Char c = name[0];
		
		String res;
		res.Append( c ).MakeUpper();
		
		for ( Uint32 i = 1; i < name.GetLength(); ++i )
		{
			Char prev_c = c;
			c = name[i];

			if ( IsUpper( c ) && ! IsUpper( prev_c ) && ! IsWhiteSpace( prev_c ) )
				res.Append( TXT(' ') );
			else
			if ( IsNumber( c ) ^ IsNumber( prev_c ) )
				res.Append( TXT(' ') );

			if ( ! IsWhiteSpace( c ) || ! IsWhiteSpace( prev_c ) )
				res.Append( c );
		}

		return res;
	}
}

void CGridPropertyWrapper::GetColumnNames(const IRTTIType *type, TDynArray<wxString> &columnNames, const CGridEditor *grid, const CProperty *prop)
{

	if( type->GetName() == TXT( "ptr:CCommunityInitializers" ) )
	{
		RED_LOG( RED_LOG_CHANNEL( Feedback ), TXT( "Type: %s" ), type->GetName().AsChar() );
	}

	if ( grid )
	{
		if ( IGridTypeDesc *typeDesc = grid->GetCustomTypeDesc( type->GetName() ) )
		{
			const String &columnName = GetColumnNameByPropertyAndType( type, prop );
			columnNames.PushBack( FormatName( columnName ).AsChar() );
			return;
		}
	}

	if (type->GetType() == RT_Class)
	{
		CClass *classPtr = (CClass *)type;
		TDynArray< CProperty* > properties;
		classPtr->GetProperties( properties );
		//const auto& properties = classPtr->GetCachedProperties();
		// Iterate through properties
		for ( auto it = properties.Begin(), end = properties.End(); it < end; ++it)
		{
			if ( !(*it)->IsEditable() )
				continue;

			if ( (*it)->GetType()->GetType() == RT_Class || 
				 (*it)->GetType()->GetType() == RT_Array ||
				 ( (*it)->GetType()->GetType() == RT_Pointer && grid->AllowEditPointers() ) )
			{
				GetColumnNames((*it)->GetType(), columnNames, grid, *it);
			}
			else
			{
				columnNames.PushBack( FormatName( GetColumnNameByPropertyAndType(NULL, *it) ).AsChar() );
			}
		}
	}
	else if( grid->AllowEditPointers() && type->GetType() == RT_Pointer )
	{
		const IRTTIType *pointedType = ( ( CRTTIPointerType * )type )->GetPointedType();
		GetColumnNames(pointedType, columnNames, grid, prop);
	}
	// Replace array type into inner type
	else if (type->GetType() == RT_Array)
	{
		columnNames.PushBack( TXT("#") );

		CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
		IRTTIType *innerType = arrayType->GetInnerType();

		if ( innerType->GetType() == RT_Simple || 
			innerType->GetType() == RT_Enum || 
			innerType->GetType() == RT_Handle ||
			innerType->GetType() == RT_SoftHandle ||
			innerType->GetType() == RT_Fundamental ||
			( !grid->AllowEditPointers() && innerType->GetType() == RT_Pointer ) )
		{
			const String &columnName = GetColumnNameByPropertyAndType( type, prop );
			columnNames.PushBack( FormatName( columnName ).AsChar() );
		}
		else
		{
			GetColumnNames(innerType, columnNames, grid, prop);
		}
	}
}

const CProperty * CGridPropertyWrapper::GetProperty( const IRTTIType *type, const CGridEditor *grid, Int32 col, const CProperty *prop )
{
	if ( IGridTypeDesc *typeDesc = grid->GetCustomTypeDesc( type->GetName() ) )
	{
		return prop;
	}

	if (type->GetType() == RT_Class)
	{
		CClass *classPtr = (CClass *)type;
		
		TDynArray< CProperty* > properties;
		classPtr->GetProperties( properties );
		//const auto& properties = classPtr->GetCachedProperties();
		for (Uint32 i = 0, n = properties.Size(); i < n; ++i)
		{
			CProperty *propertyPtr = properties[i];

			if ( !propertyPtr->IsEditable() )
				continue;

			Int32 numColumnsForType = GetNumColumnsForType( propertyPtr->GetType(), grid );

			if (col >= numColumnsForType)
			{
				col -= numColumnsForType;
				continue;
			}

			return GetProperty( propertyPtr->GetType(), grid, col, propertyPtr );
		}
	}
	else if ( grid->AllowEditPointers() && type->GetType() == RT_Pointer )
	{
		const IRTTIType *pointedType = ( ( CRTTIPointerType * )type )->GetPointedType();
		return GetProperty(pointedType, grid, col, prop );
	}
	else if (type->GetType() == RT_Array)
	{
		if ( col-- == 0 )
		{
			return prop;
		}
		
		CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
		IRTTIType *innerType = arrayType->GetInnerType();
		return GetProperty( innerType, grid, col, prop );
	}
	
	return prop;
}

void CGridPropertyWrapper::GetValue( void*& data, const IRTTIType*& type )
{
	data = m_data;
	type = m_type;
}

wxString CGridPropertyWrapper::GetValue(Int32 row, Int32 col, const IRTTIType *&type) const
{
	void *data = m_data;
	type = m_type;

	if ( GetValueInternal( row, col, data, type ) )
	{
		if ( type->GetType() == RT_Pointer )
		{
			if ( (static_cast< const CRTTIPointerType * >(type))->GetPointed(data) == NULL )
			{
				return TXT("NULL");
			}
		}
		else if ( type->GetType() == RT_Handle )
		{
			if ( (static_cast< const CRTTIHandleType * >(type))->GetPointed(data) == NULL )
			{
				return TXT("NULL");
			}
		}
		else if ( type->GetType() == RT_SoftHandle )
		{
			/*if ( (static_cast< CRTTISoftHandleType * >(type))->GetPointed(data) == NULL )
			{
				return TXT("NULL");
			}*/
		}

		String valueString;
		if ( IGridTypeDesc *typeDesc = m_grid->GetCustomTypeDesc( type->GetName() ) )
		{
			valueString = typeDesc->ToString( data ).wc_str();
		}
		else
		{
			type->ToString(data, valueString);
		}

		return valueString.AsChar();
	}

	return TXT("");
}

Bool CGridPropertyWrapper::GetValue( Int32 row, Int32 col, void *&data, const IRTTIType *&type ) const
{
	void *dataInternal = m_data;
	type = m_type;
	if ( GetValueInternal( row, col, dataInternal, type ) )
	{
		data = dataInternal;
		return true;
	}
	else
	{
		return false;
	}
}

Bool CGridPropertyWrapper::SetValue(Int32 row, Int32 col, const wxString &value)
{
	void *data = m_data;
	const IRTTIType *type = m_type;
	if (GetValueInternal(row, col, data, type))
	{
		if ( IGridTypeDesc *typeDesc = m_grid->GetCustomTypeDesc( type->GetName() ) )
		{
			return typeDesc->FromString( data, value );
		}

		return type->FromString( data, value.wc_str() );
	}

	return false;
}

//Bool CGridPropertyWrapper::GetValueInternal(Int32 row, Int32 col, void *&data, const IRTTIType *&type) const
//{
//	if ( m_computationCache.IsCaching() )
//	{
//		if ( m_computationCache.GetValue( row, col, data, type ) )
//		{
//			return true;
//		}
//		void* basePtr = data;
//		if ( GetValueImplementation( row, col, data, type ) )
//		{
//			m_computationCache.SetValue( row, col, basePtr, data, type );
//			return true;
//		}
//		return false;
//	}
//	return GetValueImplementation( row, col, data, type );
//}
//Bool CGridPropertyWrapper::GetValueImplementation(  Int32 row, Int32 col, void *&data, const IRTTIType *&type ) const
Bool CGridPropertyWrapper::GetValueInternal(  Int32 row, Int32 col, void *&data, const IRTTIType *&type ) const
{
	if ( IGridTypeDesc *typeDesc = m_grid->GetCustomTypeDesc( type->GetName() ) )
	{
		if ( row == 0 && col == 0 )
		{
			return true;
		}	
		data = NULL;
		type = NULL;
		return false;
	}

	switch ( type->GetType() )
	{
	case RT_Class:
		{
			CClass *classPtr = (CClass *)type;
			void *classData = data;

			if( m_grid->AllowEditPointers() && classData == nullptr )			
			{
				type = NULL;
				data = NULL;
				return false;
			}

			TDynArray< CProperty* > properties;
			classPtr->GetProperties( properties );
			//const auto& properties = classPtr->GetCachedProperties();

			for (Uint32 i = 0, n = properties.Size(); i < n; ++i)
			{
				const CProperty *propertyPtr = properties[i];

				if ( !propertyPtr->IsEditable() )
					continue;

				Int32 numColumnsForType = GetNumColumnsForType( propertyPtr->GetType(), m_grid );

				if (col >= numColumnsForType)
				{
					col -= numColumnsForType;
					continue;
				}

				data = propertyPtr->GetOffsetPtr( classData );
				type = propertyPtr->GetType();

				return GetValueInternal(row, col, data, type);
			}
		}
		break;

	case RT_Pointer:
		{
			if( m_grid->AllowEditPointers() )
			{
				data = data ? const_cast< void* >( ( ( CRTTIPointerType * )type )->GetPointed( data ) ) : nullptr;
				type = ( ( CRTTIPointerType * )type )->GetPointedType();
				return GetValueInternal(row, col, data, type);
			}
			else
			{
				if (row == 0 && col == 0)
				{
					return true;
				}
				break;
			}
		}
		break;

	case RT_Array:
		{
			if (col == 0)
			{
				return (row == 0);
			}

			CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
			IRTTIType *innerType = arrayType->GetInnerType();
			void *arrayData = data;
			Uint32 size = arrayType->GetArraySize(arrayData);

			const SGridExpandableBlock *blockPtr = m_expandableBlocks.FindPtr(arrayData);
			ASSERT(blockPtr);
			col--;

			if (blockPtr->m_expanded || row == 0)
			{
				for (Uint32 i = 0; i < size; ++i)
				{
					void *elementData = arrayType->GetArrayElement(arrayData, i);
					Int32 numRowsForObject = GetNumRowsForObject(elementData, innerType);

					if (row >= numRowsForObject)
					{
						row -= numRowsForObject;
						continue;
					}

					type = innerType;
					data = elementData;
					return GetValueInternal(row, col, data, type);
				}
			}
		}
		break;

	default:
		if (row == 0 && col == 0)
		{
			return true;
		}
		break;
	}

	type = NULL;
	data = NULL;
	return false;
}

Bool CGridPropertyWrapper::GetPropertyInternal( Int32 row, Int32 col, void* data, const IRTTIType*type, const CProperty*& propertyPtr ) const
{
	if ( IGridTypeDesc *typeDesc = m_grid->GetCustomTypeDesc( type->GetName() ) )
	{
		return true;
	}

	if ( type->GetType() == RT_Class )
	{
		CClass *classPtr = (CClass *)type;
		void *classData = data;
		
		TDynArray< CProperty* > properties;
		classPtr->GetProperties( properties );
		//const auto& properties = classPtr->GetCachedProperties();

		for (Uint32 i = 0, n = properties.Size(); i < n; ++i)
		{
			const CProperty *prop = properties[i];

			if ( !prop->IsEditable() )
				continue;

			Int32 numColumnsForType = GetNumColumnsForType( prop->GetType(), m_grid );

			if (col >= numColumnsForType)
			{
				col -= numColumnsForType;
				continue;
			}

			data = prop->GetOffsetPtr( classData );
			type = prop->GetType();
			propertyPtr = prop;

			return GetPropertyInternal(row, col, data, type, propertyPtr);
		}
	}
	else if ( m_grid->AllowEditPointers() && type->GetType() == RT_Pointer )
	{
		const IRTTIType *pointedType = ( ( CRTTIPointerType * )type )->GetPointedType();
		void* pointedData = const_cast< void* >( ( ( CRTTIPointerType * )type )->GetPointed( data ) );

		if( pointedData )
		{
			return GetPropertyInternal(row, col, pointedData, pointedType, propertyPtr);
		}
		else
		{
			propertyPtr = NULL;
			return false;
		}
	}
	else if (type->GetType() == RT_Array)
	{
		if (col == 0)
		{
			return (row == 0);
		}

		CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
		IRTTIType *innerType = arrayType->GetInnerType();
		void *arrayData = data;
		Uint32 size = arrayType->GetArraySize(arrayData);

		const SGridExpandableBlock *blockPtr = m_expandableBlocks.FindPtr(arrayData);
		ASSERT(blockPtr);
		col--;

		if (blockPtr->m_expanded || row == 0)
		{
			for (Uint32 i = 0; i < size; ++i)
			{
				void *elementData = arrayType->GetArrayElement(arrayData, i);
				Int32 numRowsForObject = GetNumRowsForObject(elementData, innerType);

				if (row >= numRowsForObject)
				{
					row -= numRowsForObject;
					continue;
				}

				type = innerType;
				data = elementData;

				return GetPropertyInternal(row, col, data, type, propertyPtr);
			}
		}
	}
	else
	{
		if (row == 0 && col == 0)
		{
			return true;
		}
	}

	propertyPtr = NULL;
	return false;
}

Bool CGridPropertyWrapper::IsExpandableCell(Int32 row, Int32 col, SGridExpandableBlock *&block, Uint32 &size) const
{
	void *data = m_data;
	const IRTTIType *type = m_type;
	if (GetValueInternal(row, col, data, type))
	{
		if (const SGridExpandableBlock *blockPtr = m_expandableBlocks.FindPtr(data))
		{
			ASSERT(type->GetType() == RT_Array);
			CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
			size = arrayType->GetArraySize(data);				
			block = const_cast<SGridExpandableBlock *>(blockPtr);
			return true;
		}
	}

	return false;
}

Bool CGridPropertyWrapper::IsReadOnlyCell( Int32 row, Int32 col ) const
{
	const CProperty *propertyPtr = NULL;
	if (GetPropertyInternal(row, col, m_data, m_type, propertyPtr))
	{
		if ( ! propertyPtr )
			return false;

		return propertyPtr->IsReadOnly();
	}
	return false;
}

void CGridPropertyWrapper::RefreshExpandableBlocks()
{
	THashMap<void *, SGridExpandableBlock> expandableBlocks;
	THashMap<void *, SGridExpandableBlock>::iterator it;
	for (it = m_expandableBlocks.Begin(); it != m_expandableBlocks.End(); ++it)
	{
		expandableBlocks.Insert((*it).m_first, (*it).m_second);
	}
	
	m_expandableBlocks.Clear();
	InitExpandableBlocks(m_data, m_type);

	SGridExpandableBlock block;
	for (it = m_expandableBlocks.Begin(); it != m_expandableBlocks.End(); ++it)
	{
		if (expandableBlocks.Find((*it).m_first, block))
		{
			(*it).m_second = block;
		}
	}
}

void CGridPropertyWrapper::InitExpandableBlocks(const void *data, const IRTTIType *type)
{
	if (type->GetType() == RT_Class)
	{
		CClass *classPtr = (CClass *)type;
		TDynArray< CProperty* > properties;
		classPtr->GetProperties( properties );
		//const auto& properties = classPtr->GetCachedProperties();
		for (Uint32 i = 0, n = properties.Size(); i < n; ++i)
		{
			CProperty *propertyPtr = properties[i];
			if ( ! propertyPtr->IsEditable() )
				continue;
			const void *propertyData = propertyPtr->GetOffsetPtr(data);
			InitExpandableBlocks(propertyData, propertyPtr->GetType());
		}
	}
	else if (type->GetType() == RT_Array)
	{
		CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
		IRTTIType *innerType = arrayType->GetInnerType();
		Uint32 size = arrayType->GetArraySize(data);

		SGridExpandableBlock block;
		m_expandableBlocks.Insert(const_cast<void *>(data), block);
		for (Uint32 i = 0; i < size; ++i)
		{
			const void *elementData = arrayType->GetArrayElement(data, i);
			InitExpandableBlocks(elementData, innerType);
		}
	}
	else if( m_grid->AllowEditPointers() && type->GetType() == RT_Pointer )
	{
		const IRTTIType *pointedType = ( ( CRTTIPointerType * )type )->GetPointedType();
		void* pointedData = const_cast< void* >( ( ( CRTTIPointerType * )type )->GetPointed( data ) );

		if( pointedData )
		{
			InitExpandableBlocks( pointedData, pointedType );
		}
		else
		{
			RED_LOG( RED_LOG_CHANNEL( Feedback ), TXT( "Null pointer data..." ) );
		}
	}
}

void CGridPropertyWrapper::GetArraysBounds(TDynArray<SGridBlockRange> &bounds)
{
	Int32 level = 0;
	GetArraysBounds(m_data, m_type, bounds, 0, 0, level);
}

void CGridPropertyWrapper::GetArraysBounds(const void *data, const IRTTIType *type, TDynArray<SGridBlockRange> &bounds, Int32 row, Int32 col, Int32 level)
{
	if (type->GetType() == RT_Class)
	{
		Int32 columns = 0;
		CClass *classPtr = (CClass *)type;
		TDynArray< CProperty* > properties;
		classPtr->GetProperties( properties );
		//const auto& properties = classPtr->GetCachedProperties();
		for (Uint32 i = 0, n = properties.Size(); i < n; ++i)
		{
			if ( ! properties[i]->IsEditable() )
				continue;
			const void *propertyData = properties[i]->GetOffsetPtr( data );
			GetArraysBounds(propertyData, properties[i]->GetType(), bounds, row, col, level);
			const Int32 numCols = GetNumColumnsForType( properties[i]->GetType(), m_grid );
			col += numCols;
		}
	}
	else if (type->GetType() == RT_Array)
	{
		CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
		IRTTIType *innerType = arrayType->GetInnerType();
		const Uint32 size = arrayType->GetArraySize( data );
		const Int32 numCols = GetNumColumnsForType( innerType, m_grid );
		
		const SGridExpandableBlock *blockPtr = m_expandableBlocks.FindPtr( const_cast<void *>(data) );
		ASSERT(blockPtr);

		if (blockPtr->m_expanded && size > 0)
		{
			for (Uint32 i = 0; i < size; ++i)
			{
				const void *elementData = arrayType->GetArrayElement(data, i);
				const Int32 numRows = GetNumRowsForObject(elementData, innerType);
				GetArraysBounds(elementData, innerType, bounds, row, col + 1, level + 1);
				bounds.PushBack(SGridBlockRange(row, col + 1, numRows, numCols, level + 1));
				row += numRows;
			}
		}
		else
		{
			bounds.PushBack(SGridBlockRange(row, col + 1, 1, numCols, level + 1));
		}
	}
}

void CGridPropertyWrapper::GetCellDescriptor(Int32 row, Int32 col, SGridCellDescriptor &desc) const
{
	desc.cellData = NULL;
	//desc.cellProperty = NULL;
	desc.cellType = NULL;
	desc.arrayData = NULL;
	//desc.arrayProperty = NULL;
	desc.arrayType = NULL;
	desc.arraySize = -1;
	desc.level = 0;
	desc.isSeparator = false;
	desc.isValue = false;
	desc.arrayElement = -1;
	desc.elementRange = wxRect(0, 0, -1, -1);
	GetCellDescriptor(row, col, desc, m_data, m_type, 0, 0, 0);
}

void CGridPropertyWrapper::GetCellDescriptor(Int32 row, Int32 col, SGridCellDescriptor &desc, const void *data, const IRTTIType *type, Int32 level, Int32 x, Int32 y) const
{
	desc.level = level;

	if (type->GetType() == RT_Class)
	{
		CClass *classPtr = (CClass *)type;
		TDynArray< CProperty* > properties;
		classPtr->GetProperties( properties );
		//const auto& properties = classPtr->GetCachedProperties();
		for (Uint32 i = 0, n = properties.Size(); i < n; ++i)
		{
			if ( ! properties[i]->IsEditable() )
				continue;

			const void *propertyData = properties[i]->GetOffsetPtr( data );
			const IRTTIType *propertyType = properties[i]->GetType();
			const Int32 numColumnsForType = GetNumColumnsForType( propertyType, m_grid );

			if (col >= numColumnsForType)
			{
				col -= numColumnsForType;
				x += numColumnsForType;
				continue;
			}

			desc.cellData = const_cast<void *>(propertyData);
			//desc.cellProperty = properties[i];
			desc.cellType = const_cast<IRTTIType *>(propertyType);

			GetCellDescriptor(row, col, desc, propertyData, propertyType, level, x, y);
			return;
		}

	}
	else if ( m_grid->AllowEditPointers() && type->GetType() == RT_Pointer )
	{
		const IRTTIType *pointedType = ( ( CRTTIPointerType * )type )->GetPointedType();
		void* pointedData = const_cast< void* >( ( ( CRTTIPointerType * )type )->GetPointed( data ) );

		if( pointedData )
		{
			GetCellDescriptor(row, col, desc, pointedData, pointedType, level, x, y);
		}
	}
	else if (type->GetType() == RT_Array)
	{
		CRTTIArrayType *arrayType = (CRTTIArrayType *)type;
		IRTTIType *innerType = arrayType->GetInnerType();
		void *arrayData = const_cast<void *>( data );
		const Uint32 size = arrayType->GetArraySize( arrayData );
		const Int32 numCols = GetNumColumnsForType( innerType, m_grid );
		const Int32 numRows = GetNumRowsForObject( arrayData, arrayType );

		const SGridExpandableBlock *blockPtr = m_expandableBlocks.FindPtr(arrayData);
		ASSERT(blockPtr);

		if (blockPtr->m_expanded || row == 0)
		{
			desc.arrayData = arrayData;
			//desc.arrayProperty = desc.cellProperty;
			desc.arrayType = arrayType;
			desc.arrayElement = -1;
			desc.isSeparator = (col == 0);
			desc.arraySize = size;
			desc.elementRange.SetX( x );
			desc.elementRange.SetY( y );
			desc.elementRange.SetWidth( numCols + 1 );
		}

		// mind a separator
		col--;
		x++;

		if (blockPtr->m_expanded)
		{
			for (Uint32 i = 0; i < size; ++i)
			{
				void *elementData = arrayType->GetArrayElement( arrayData, i );
				const Int32 numRowsForObject = GetNumRowsForObject( elementData, innerType );

				if (row >= numRowsForObject)
				{
					row -= numRowsForObject;
					y += numRowsForObject;
					continue;
				}

				desc.arrayElement = i;
				desc.elementRange.SetY(y);
				desc.elementRange.SetHeight( numRowsForObject );
				GetCellDescriptor( row, col, desc, elementData, innerType, level + 1, x, y );
				return;
			}
		}
		else
		{
			desc.elementRange.SetHeight( numRows );
		}
	}
	else if (row == 0 && col == 0)
	{
		desc.isValue = true;
	}
}

String CGridPropertyWrapper::GetColumnNameByPropertyAndType( const IRTTIType *type, const CProperty *prop )
{
	//return prop ? prop->GetName().AsString() : type->GetName().AsString();

	if ( prop )
	{
		String propName = prop->GetName().AsString();
		const String &hint = prop->GetHint();
		if ( hint.GetLength() <= 10 || hint.GetLength() <= propName.GetLength() )
		{
			return hint;
		}
		else
		{
			return propName;
		}
	}
	else
	{
		return type->GetName().AsString();
	}
}
