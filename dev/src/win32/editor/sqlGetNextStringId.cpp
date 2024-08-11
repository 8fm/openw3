// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "sqlGetNextStringId.h"

/*
Ctor.
*/
SqlGetNextStringId::SqlGetNextStringId()
: m_stmt( SQL_NULL_HSTMT )
{
	Reset();
}

/*
Dtor.
*/
SqlGetNextStringId::~SqlGetNextStringId()
{
	Reset();
}

Bool SqlGetNextStringId::Reset()
{
	SQLRETURN sqlret = SQL_SUCCESS;

	if( m_stmt != SQL_NULL_HSTMT )
	{
		sqlret = SQLFreeHandle( SQL_HANDLE_STMT, m_stmt );
	}

	m_stmt = SQL_NULL_HSTMT;
	m_resultNextStringId = 0;

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlGetNextStringId::Prepare( SQLHDBC conn )
{
	ASSERT( conn != SQL_NULL_HDBC );
	ASSERT( m_stmt == SQL_NULL_HSTMT );

	SQLWCHAR* query = L"SELECT MAX(STRING_ID) + 1 FROM STRING_INFO";
	SQLRETURN sqlret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &m_stmt );
	sqlret = SQLPrepareW( m_stmt, query, SQL_NTS );
	sqlret = SQLBindCol( m_stmt, 1, SQL_C_ULONG, &m_resultNextStringId, 0, nullptr );

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlGetNextStringId::Execute( Uint32& outNextStringId )
{
	ASSERT( m_stmt != SQL_NULL_HSTMT );

	SQLRETURN sqlret = SQLExecute( m_stmt );
	sqlret = SQLFetch( m_stmt );
	sqlret = SQLCloseCursor( m_stmt );
	outNextStringId = m_resultNextStringId;

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}
