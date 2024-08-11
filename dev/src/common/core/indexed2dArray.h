#pragma once

#include "2darray.h"
#include "sortedmap.h"

// An extension of a 2dArray, storing a hash map of first column values as keys, and row indexes as mapped values.
// This implies, that first column values should be unique per row.
class CIndexed2dArray : public C2dArray
{
	DECLARE_ENGINE_RESOURCE_CLASS( CIndexed2dArray, C2dArray, "redicsv", "Indexed 2D Array" );

	friend class CDependencyLoader2dArray;
	friend class CCookerDependencyLoader2dArray;

private:
	THashMap< CName , Uint32 > m_keyToRow;

public:
	CIndexed2dArray();
	~CIndexed2dArray();

	// Returns the index of the first row with given key (first column is considered a key)
	Int32 GetRowIndex(const CName key) const;

	virtual void OnModified(Int32 col = 0, Int32 row = 0);

private:
	void GenerateKeyToRowMapping();
	void AddKeyToRowMap(const String& value, Uint32 index);
	void RemoveKeyFromRowMap(const String& value);

	// scripting support
private:
	void funcGetRowIndexByKey( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( CIndexed2dArray );
	PARENT_CLASS( C2dArray );

	NATIVE_FUNCTION( "GetRowIndexByKey", funcGetRowIndexByKey );
END_CLASS_RTTI();

