// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "stringDbTypes.h"
#include <sql.h>
#include <sqlext.h>

/*
Deletes string info from String DB.

Deletes STRING_INFO table rows whose STRING_ID matches specified string id.
*/
class SqlDeleteStringInfo
{
public:
	SqlDeleteStringInfo();
	~SqlDeleteStringInfo();

	Bool Reset();
	Bool Prepare( SQLHDBC conn );
	Bool Execute( Uint32 stringId );

private:
	SqlDeleteStringInfo( const SqlDeleteStringInfo& );				// cctor - not defined
	SqlDeleteStringInfo& operator=( const SqlDeleteStringInfo& );	// op= - not defined

	SQLHSTMT m_stmt;												// Statement handle.
	SQLUINTEGER m_paramStringId;									// Bound to statement parameter containing string id.
};
