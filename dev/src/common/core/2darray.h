/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "resource.h"

class CCookerDependencyLoader2dArray;

/// CSV file, a 2D array with strings
class C2dArray : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( C2dArray, CResource, "csv", "2D Array" );

	friend class CDependencySaver2dArray;
	friend class CDependencyLoader2dArray;
	friend class CCookerDependencyLoader2dArray;

protected:
	typedef TDynArray< String, MC_Arrays2D >		TStringArray;
	typedef TDynArray< TStringArray, MC_Arrays2D >	TStringArray2D;
	
	TStringArray		m_headers;	//!< columns name
	TStringArray2D		m_data;		//!< array data[col][row]

public:
	C2dArray();

	//! Serialize object ( binary form )
	void OnSerialize( IFile &file );

	//! Create from string
	static C2dArray* CreateFromString( const String& absPath, const String separator=TXT(";") );

	static void Copy( C2dArray& dest, const C2dArray& src );
	//! if headers are different return false and do not concatenate 2darrays
	static Bool Concatenate( C2dArray& dest, const C2dArray& src );
public:
//	void LoadFromString( String absFilePath );
//	void SaveToString( String absFilePath );

	TStringArray2D& GetData() { return m_data; }
	const TStringArray2D& GetData() const { return m_data; }
	void SetData( const TStringArray2D& data ) { m_data = data; }

	String GetValue(Uint32 col, Uint32 row) const;
	const String& GetValueRef(Uint32 col, Uint32 row) const;
	String GetValue(const String &header, Uint32 row) const;
	String GetHeader(Uint32 col) const;
	
	/*
		+--------+-----+-----+---------+	GetValue(Name, Name2, Attack) -> return att2
		|Name(1) | AAA | BBB |Attack(3)|			  (1)   (2)     (3)				 (4)
		+--------+-----+-----+---------+	
		|Name1   | a1  | b1  | att1    |
		|Name2(2)| a2  | b2  | att2(4) |
		|Name3   | a3  | b3  | att3    |
		+--------+-----+-----+---------+
	*/
	String GetValue(const String &keyFirstColName, const String &keyFirstColValue, const String &keySecondColName) const;

    Bool FindHeader(const String &header, Uint32 &col) const;

	Bool SetValue(String value, Uint32 col, Uint32 row);
	Bool SetValue(String value, const String& header, Uint32 row);
	Bool SetHeader(String value, Uint32 col);
	
	// See GetValue
	Bool SetValue(String value, const String& keyFirstColName, const String& keyFirstColValue, const String& keySecondColName);

	void AddColumn(const String& rowDefaultValue = String::EMPTY);
	void AddColumn(String colName, const String& rowDefaultValue);
	void AddRow(const String& rowDefaultValue = String::EMPTY);
	void AddRow(const TDynArray<String>& rowData, const String& rowDefaultValue = String::EMPTY);

	void InsertColumn(Uint32 colNum, const String& rowDefaultValue = String::EMPTY);
	void InsertColumn(Uint32 colNum, String colName, const String& rowDefaultValue);
	void InsertColumn(String colPrevName, String colName, const String& rowDefaultValue = String::EMPTY);
	void InsertRow(Uint32 rowNum, const String& rowDefaultValue = String::EMPTY );
	void InsertRow(Uint32 rowNum, const TDynArray<String>& rowData, const String& rowDefaultValue = String::EMPTY);

	void DeleteColumn(String colName);
	void DeleteColumn(Uint32 colNum);
	void DeleteRow(Uint32 rowNum);

	void GetSize(Uint32 &colSize, Uint32 &rowSize) const;
	Uint32 GetNumberOfRows() const { return m_data.Size(); }
	Uint32 GetNumberOfColumns() const { return m_headers.Size(); }
	Bool Empty() const ;
	void Clear();

	void Refresh();

	Bool HasUniqueRow(Uint32 rowNum) const;
	Bool HasUniqueColumnNames() const;
	Bool HasUniqueColumn(Uint32 colNum) const;
	Bool HasUniqueColumn(const String& colName) const;

	// Returns the index of the first column with given header, or -1 if no such header found
	Int32 GetColumnIndex( const String &header ) const;
	Int32 GetRowIndex( Int32 col, const String &value ) const;
	Int32 GetRowIndex( const String &header, const String &value) const;

	void ParseData( String strData, const String separator=TXT(";"));
protected:
	void PrepareData( String& strData, const String& separator=TXT(";"));
	void PrepareData( Char* strData, const Char separator = ';' );

	Uint32 CalculateDataLength();

	void WriteRow( String& data, const String& separator, const TStringArray& rowData);
	Bool Split( String& source, const String& separator, String *leftPart, String *rightPart);
	void RemoveChars( String& source, const TDynArray<String> &chars);

	void ColumnPositions(TDynArray< Uint32 >& colPos );
	String GenerateUniqueHeaderName();
	Bool FindSpecialChar(String &str);
	
	void ChangeInternalInvertedCommas(String &str, Bool loading);

	virtual void OnModified(Int32 col = 0, Int32 row = 0) {};
public:
	template< class T >
	static Bool ConvertValue( const String &src, T &dst )
	{
		return FromString( src, dst );
	}

	// a variation of the GetValue function described above. Returns the value of the given type. 
	// Will return 0 if value is not found or can not be converted into the given type.
	template< class T >
	T GetValue(const String &keyFirstColName, const String &keyFirstColValue, const String &keySecondColName) const
	{
		String stringVal = GetValue( keyFirstColName, keyFirstColValue, keySecondColName );
		T value;
		if ( ConvertValue( stringVal, value ) )
		{
			return value;
		}
		else 
		{
			return 0;
		}
	}

	template< class T >
	TDynArray< T > GetValues(const String &keyFirstColName, const String &keyFirstColValue, const String &keySecondColName) const
	{
		TDynArray< T > values;
		Uint32 destColNum = 0;
		while ( destColNum < m_headers.Size() && m_headers[ destColNum ] != keySecondColName)
		{
			destColNum++;
		}

		for( Uint32 colNum = 0; colNum < m_headers.Size(); colNum++ )
		{
			if ( m_headers[ colNum ] == keyFirstColName )
			{
				for ( Uint32 rowNum = 0; rowNum < m_data.Size(); rowNum++ )
				{
					if ( m_data[ rowNum ][ colNum ] == keyFirstColValue )
					{
						T val;
						if ( !ConvertValue( m_data[ rowNum ][ destColNum ], val ) )
						{
							val = 0;
						}
						values.PushBack( val );
					}
				}
				break;
			}
		}
		return values;
	}

	template< class T >
	TDynArray< T > GetColumn(const String &keyFirstColName) const
	{
		TDynArray< T > values;
		Uint32 colnum;
		if ( FindHeader( keyFirstColName, colnum ) )
		{
			for ( Uint32 rowNum = 0; rowNum < m_data.Size(); rowNum++ )
			{
				T val;
				if ( !ConvertValue( m_data[ rowNum ][ colnum ], val ) )
				{
					val = T();
				}
				values.PushBack( val );
			}
		}
		return values;
	}

// Load/Save helper methods;
private:
	RED_INLINE void WriteCell( String cellString, Char* dataBuffer, Uint32& position, const Char separator, Bool appendBrackets, Bool isLastCellInRow );
	RED_INLINE void WriteCharacter( const Char& character, Char* dataBuffer, Uint32& position );

	// scripting support
private:
	void funcLoadFromFile( CScriptStackFrame& stack, void* result );
	void funcGetValueAtAsName( CScriptStackFrame& stack, void* result );
	void funcGetValueAsName( CScriptStackFrame& stack, void* result );
	void funcGetValueAt( CScriptStackFrame& stack, void* result );
	void funcGetValue( CScriptStackFrame& stack, void* result );
	void funcGetNumRows( CScriptStackFrame& stack, void* result );
	void funcGetNumColumns( CScriptStackFrame& stack, void* result );
	void funcGetRowIndexAt( CScriptStackFrame& stack, void* result );
	void funcGetRowIndex( CScriptStackFrame& stack, void* result );
};

template<>
RED_INLINE Bool C2dArray::ConvertValue<String>( const String &src, String &dst )
{
	dst = src;
	return true;
}

BEGIN_CLASS_RTTI( C2dArray );
	PARENT_CLASS( CResource );
	PROPERTY(m_headers);
	PROPERTY(m_data);
	NATIVE_FUNCTION( "GetValueAt", funcGetValueAt );
	NATIVE_FUNCTION( "GetValue", funcGetValue );
	NATIVE_FUNCTION( "GetValueAtAsName", funcGetValueAtAsName );
	NATIVE_FUNCTION( "GetValueAsName", funcGetValueAsName );
	NATIVE_FUNCTION( "GetNumRows", funcGetNumRows );
	NATIVE_FUNCTION( "GetNumColumns", funcGetNumColumns );
	NATIVE_FUNCTION( "GetRowIndexAt", funcGetRowIndexAt );
	NATIVE_FUNCTION( "GetRowIndex", funcGetRowIndex );
END_CLASS_RTTI();

// Structure for passing additional parameters to 2da value selection custom editor

struct S2daValueFilter
{
	String              m_columnName; // CSV file column name
	TDynArray< String > m_keywords;   // filters (logic OR - filter is active if one keyword matches)
};

struct S2daValueProperties
{
	C2dArray                    *m_array;
	String                       m_descrColumnName;
	String                       m_valueColumnName;
	TDynArray< S2daValueFilter > m_filters; // filters (logic AND - all S2daValueFilter have to match)

	S2daValueProperties()
		: m_array( NULL )
	{}
};

struct SConst2daValueProperties
{
	const C2dArray               *m_array;
	String                       m_descrColumnName;
	String                       m_valueColumnName;
	TDynArray< S2daValueFilter > m_filters; // filters (logic AND - all S2daValueFilter have to match)

	SConst2daValueProperties()
		: m_array( NULL )
	{}
};

// interface for communication between 2da value selection custom editor and component, which is the property owner
class I2dArrayPropertyOwner
{
public:
	virtual void Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties ) = 0;
	virtual Bool SortChoices() const;
};

class C2dArraysResourcesManager
{
public:
	C2dArraysResourcesManager( CGatheredResource& defaultGatheredResource );

	Bool Load2dArray( const String& filePath );
	Bool Unload2dArray( const String& filePath );

	RED_INLINE const C2dArray& Get2dArray() const { return m_2dArray; }
	const C2dArray& Reload2dArray();

#ifndef NO_EDITOR 
	void GetFileNameAndOffset( Uint32 globalIndex, String& fileName, Uint32& localIndex );
#endif
protected:
	TDynArray<String>		m_2dArrayFilePaths;
	TDynArray< THandle<C2dArray> >	m_2dArrays;

	C2dArray				m_2dArray;

	CGatheredResource& m_defaultGatheredResource;
};
