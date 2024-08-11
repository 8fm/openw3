// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "stringDbTypes.h"
#include <sql.h>
#include <sqlext.h>

/*
Selects strings from String DB.

Selects STRINGS table rows whose STRING_ID matches specified string id.
*/
class SqlSelectStrings
{
public:
	SqlSelectStrings();
	~SqlSelectStrings();

	Bool Reset();
	Bool Prepare( SQLHDBC conn );
	Bool Execute( Uint32 stringId, TDynArray< StringRow >& outStringRows );

private:
	SqlSelectStrings( const SqlSelectStrings& );				// cctor - not defined
	SqlSelectStrings& operator=( const SqlSelectStrings& );		// op= - not defined

	SQLHSTMT m_stmt;											// Statement handle.

	SQLUINTEGER m_paramStringId;								// Bound to statement parameter containing string id.

	SQLLEN m_resultLangIdLenInd;								// Length/indicator buffer for m_resultLangId.
	SQLUINTEGER m_resultLangId;									// Bound to result set LANG column.

	SQLLEN m_resultVersionLenInd;								// Length/indicator buffer for m_resultVersion.
	SQLUINTEGER m_resultVersion;								// Bound to result set VERSION column.

	static const SQLULEN m_textColumnSize = 4000;				// Size (in chars) of TEXT column as defined in String DB.
	SQLLEN m_resultTextLenInd;									// Length/indicator buffer m_resultText.
	SQLWCHAR m_resultText[ m_textColumnSize + 1 ];				// Bound to result set TEXT column. + 1 for null.
};
