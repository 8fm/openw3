// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "stringDbTypes.h"
#include <sql.h>
#include <sqlext.h>

/*
Queries String DB to get next string id.
*/
class SqlGetNextStringId
{
public:
	SqlGetNextStringId();
	~SqlGetNextStringId();

	Bool Reset();
	Bool Prepare( SQLHDBC conn );
	Bool Execute( Uint32& outNextStringId );

private:
	SqlGetNextStringId( const SqlGetNextStringId& );				// cctor - not defined
	SqlGetNextStringId& operator=( const SqlGetNextStringId& );		// op= - not defined

	SQLHSTMT m_stmt;												// Statement handle.
	SQLUINTEGER m_resultNextStringId;								// Bound to result set column containing next string id.
};
