#pragma once

class CGridEditor;

struct SGridCellDescriptor
{
	wxRect elementRange;
	void *cellData;
	//CProperty *cellProperty;
	IRTTIType *cellType;
	void *arrayData;
	//CProperty *arrayProperty;	
	IRTTIType *arrayType;
	Int32 arrayElement;
	Int32 arraySize;
	Int32 level;
	Bool isSeparator;
	Bool isValue;
};

struct SGridExpandableBlock
{
	Bool m_expanded;
	SGridExpandableBlock( Bool expanded = true ) : m_expanded( expanded ) {}
};

struct SGridBlockRange
{
	Int32 m_row;
	Int32 m_column;
	Int32 m_numRows;
	Int32 m_numColumns;
	Int32 m_level;

	SGridBlockRange( Int32 row, Int32 col, Int32 numRows, Int32 numCols, Int32 level ) 
		: m_row( row ), m_column( col ), m_numRows( numRows ) , m_numColumns( numCols ), m_level( level ) {}
};

// Grid property wrapper
class CGridPropertyWrapper
{
public:
	struct TypedPtr
	{
		const void*			m_data;
		const IRTTIType*	m_type;

		Bool operator==( const TypedPtr& p ) const										{ return m_data == p.m_data && m_type == p.m_type; }
		RED_FORCE_INLINE Uint32 CalcHash() const { return GetPtrHash( m_data ) ^ GetPtrHash( m_type ); }
	};

	CGridPropertyWrapper( void *data, const IRTTIType *type, const CGridEditor *grid );

	static Uint32 GetNumColumns( const IRTTIType *type, const CGridEditor *grid );
	static void GetColumnNames( const IRTTIType *type, TDynArray<wxString> &columnNames, const CGridEditor *grid, const CProperty *prop = NULL );
	static const CProperty * GetProperty( const IRTTIType *type, const CGridEditor *grid, Int32 col, const CProperty * prop = NULL );
	Int32 GetMaxRows() const;
	void GetValue( void *&data, const IRTTIType *&type );
	wxString GetValue( Int32 row, Int32 col, const IRTTIType *&type ) const;
	Bool GetValue( Int32 row, Int32 col, void *&data, const IRTTIType *&type ) const;
	Bool SetValue( Int32 row, Int32 col, const wxString &value );
	Bool IsExpandableCell( Int32 row, Int32 col, SGridExpandableBlock *&block, Uint32 &size ) const;
	Bool IsReadOnlyCell( Int32 row, Int32 col ) const;
	void RefreshExpandableBlocks();
	void GetArraysBounds( TDynArray<SGridBlockRange> &bounds );
	void GetCellDescriptor( Int32 row, Int32 col, SGridCellDescriptor &desc ) const;

	struct Cache
	{
		Cache( CGridPropertyWrapper* owner );
		~Cache();
	private:
		CGridPropertyWrapper*			m_owner;
	};

private:
	// Computation cache stuff
	struct SComputationCache
	{
	private:
		typedef TPair< Uint64, const void* > Loc;
		Bool							m_isCaching;
		THashMap< TypedPtr, Int32 >		m_numRows;
		//THashMap< Loc, TypedPtr >		m_values;

		Uint64 Hash( Int32 row, Int32 col ) const											{ return Uint64( row ) << 32 | Uint64( col ); }
	public:
		SComputationCache();
		~SComputationCache();

		Bool IsCaching() const																{ return m_isCaching; }
		void StartCaching();
		void StopCaching();

		//Bool GetValue( Int32 row, Int32 col, void *&data, const IRTTIType *&type ) const;
		//void SetValue( Int32 row, Int32 col, void *basePtr, void *data, const IRTTIType *type );

		Bool GetNumRowsForObject( const void *data, const IRTTIType *typ, Int32& outRows ) const;
		void SetNumRowsForObject( const void *data, const IRTTIType *typ, Int32 rows );
	};


	void*												m_data;
	const IRTTIType*									m_type;
	const CGridEditor*									m_grid;
	THashMap<void *, SGridExpandableBlock>				m_expandableBlocks;
	mutable SComputationCache							m_computationCache;

	static Uint32 GetNumColumnsForType(  const IRTTIType *type, const CGridEditor *grid );
	Int32 GetNumRowsForObject( const void *data, const IRTTIType *type ) const;
	Bool GetValueInternal(  Int32 row, Int32 col, void *&data, const IRTTIType *&type ) const;
	//Bool GetValueImplementation(  Int32 row, Int32 col, void *&data, const IRTTIType *&type ) const;
	Bool GetPropertyInternal( Int32 row, Int32 col, void* data, const IRTTIType* type, const CProperty*& propertyPtr ) const;
	void InitExpandableBlocks( const void *data, const IRTTIType *type );
	void GetArraysBounds( const void *data, const IRTTIType *type, TDynArray<SGridBlockRange> &bounds, Int32 row, Int32 col, Int32 level );
	void GetCellDescriptor( Int32 row, Int32 col, SGridCellDescriptor &desc, const void *data, const IRTTIType *type, Int32 level, Int32 x, Int32 y ) const;
	static String GetColumnNameByPropertyAndType( const IRTTIType *type, const CProperty *prop );



};
