// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "stringDbTypes.h"
#include <sql.h>
#include <sqlext.h>

/*
Deletes string from String DB.

Deletes STRINGS table rows whose STRING_ID matches specified string id.
*/
class SqlDeleteString
{
public:
	SqlDeleteString();
	~SqlDeleteString();

	Bool Reset();
	Bool Prepare( SQLHDBC conn );
	Bool Execute( Uint32 stringId );

private:
	SqlDeleteString( const SqlDeleteString& );				// cctor - not defined
	SqlDeleteString& operator=( const SqlDeleteString& );	// op= - not defined

	SQLHSTMT m_stmt;										// Statement handle.
	SQLUINTEGER m_paramStringId;							// Bound to statement parameter containing string id.
};
