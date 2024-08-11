// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "stringDbTypes.h"
#include <sql.h>
#include <sqlext.h>

/*
Inserts string into String DB.

Inserts row into STRINGS table. Values for USER and MODIFICATION_DATE columns are generated automatically.
*/
class SqlInsertString
{
public:
	SqlInsertString();
	~SqlInsertString();

	Bool Reset();
	Bool Prepare( SQLHDBC conn );
	Bool Execute( const StringRow& stringRow );

private:
	SqlInsertString( const SqlInsertString& );				// cctor - not defined
	SqlInsertString& operator=( const SqlInsertString& );	// op= - not defined

	SQLHSTMT m_stmt;										// Statement handle.

	SQLUINTEGER m_paramStringId;							// Bound to statement parameter containing string id.
	SQLUINTEGER m_paramLangId;								// Bound to statement parameter containing language id.
	SQLUINTEGER m_paramVersion;								// Bound to statement parameter containing version number.

	static const SQLULEN m_textColumnSize = 4000;			// Size (in chars) of TEXT column as defined in String DB.
	SQLLEN m_paramTextLenInd;								// Length/indicator buffer for m_paramText.
	SQLWCHAR m_paramText[ m_textColumnSize + 1 ];			// Bound to statement parameter containing text. + 1 for null.
};
