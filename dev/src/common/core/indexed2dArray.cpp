#include "build.h"
#include "indexed2dArray.h"

#include "depot.h"
#include "scriptStackFrame.h"
#include "scriptingSystem.h"

IMPLEMENT_ENGINE_CLASS( CIndexed2dArray );

CIndexed2dArray::CIndexed2dArray()
{
}

CIndexed2dArray::~CIndexed2dArray()
{
}

Int32 CIndexed2dArray::GetRowIndex(const CName key) const
{
	Uint32 index = 0;

	if(m_keyToRow.Find( key, index ))
	{
		return (Int32)index;
	}
	return -1;
}

void CIndexed2dArray::OnModified(Int32 col, Int32 row)
{
	if( col == 0 || row != 0 )
	{
		GenerateKeyToRowMapping();
	}
}	

void CIndexed2dArray::GenerateKeyToRowMapping()
{
	const Uint32 numberOfRows = GetNumberOfRows();
	m_keyToRow.Clear();
	m_keyToRow.Reserve( numberOfRows );
	for(Uint32 index = 0; index < numberOfRows; ++index)
	{
		const String& value = GetValue( 0, index );
		AddKeyToRowMap( value, index );
	}
}

void CIndexed2dArray::AddKeyToRowMap(const String& value, Uint32 index)
{
	m_keyToRow.Insert( CName( value ), index );
}

void CIndexed2dArray::RemoveKeyFromRowMap(const String& value )
{
	m_keyToRow.Erase( CName( value ) );
}

// -------------------------------------------------------
// ------------------- scripting support -----------------
// -------------------------------------------------------

void CIndexed2dArray::funcGetRowIndexByKey(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( CName, key, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_INT( GetRowIndex( key ) );
}
