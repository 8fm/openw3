// Copyright © 2015 CD Projekt Red. All Rights Reserved.

#pragma once

#include "stringDbTypes.h"
#include <sql.h>
#include <sqlext.h>

/*
Selects STRING_INFO table rows whose STRING_KEY is not null.
*/
class SqlSelectStringsWithKeys
{
public:

	class ResultsetRow
	{
	public:
		Uint32 m_stringId;
		String m_stringKey;
		String m_stringPropertyName;
	};

	SqlSelectStringsWithKeys();
	~SqlSelectStringsWithKeys();

	Bool Reset();
	Bool Prepare( SQLHDBC conn );
	Bool Execute( TDynArray< ResultsetRow >& outResult );

private:
	SqlSelectStringsWithKeys( const SqlSelectStringsWithKeys& );			// cctor - not defined
	SqlSelectStringsWithKeys& operator=( const SqlSelectStringsWithKeys& );	// op= - not defined

	SQLHSTMT m_stmt;														// Statement handle.

	SQLLEN m_resultStringIdLenInd;											// Length/indicator buffer for m_resultStringId.
	SQLUINTEGER m_resultStringId;											// Bound to result set STRING_ID column.

	static const SQLULEN m_propertyNameColumnSize = 4000;					// Size (in chars) of PROPERTY_NAME column as defined in String DB.
	SQLLEN m_resultPropertyNameLenInd;										// Length/indicator buffer for m_resultPropertyName.
	SQLWCHAR m_resultPropertyName[ m_propertyNameColumnSize + 1 ];			// Bound to result set PROPERTY_NAME column. + 1 for null.

	static const SQLULEN m_stringKeyColumnSize = 128;						// Size (in chars) of STRING_KEY column as defined in String DB.
	SQLLEN m_resultStringKeyLenInd;											// Length/indicator buffer for m_resultStringKey.
	SQLWCHAR m_resultStringKey[ m_stringKeyColumnSize + 1 ];				// Bound to result set STRING_KEY column. + 1 for null.
};
