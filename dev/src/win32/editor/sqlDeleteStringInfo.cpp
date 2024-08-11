// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "sqlDeleteStringInfo.h"

/*
Ctor.
*/
SqlDeleteStringInfo::SqlDeleteStringInfo()
: m_stmt( SQL_NULL_HSTMT )
{
	Reset();
}

/*
Dtor.
*/
SqlDeleteStringInfo::~SqlDeleteStringInfo()
{
	Reset();
}

Bool SqlDeleteStringInfo::Reset()
{
	SQLRETURN sqlret = SQL_SUCCESS;

	if( m_stmt != SQL_NULL_HSTMT )
	{
		sqlret = SQLFreeHandle( SQL_HANDLE_STMT, m_stmt );
	}

	m_stmt = SQL_NULL_HSTMT;
	m_paramStringId = 0;

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlDeleteStringInfo::Prepare( SQLHDBC conn )
{
	ASSERT( conn != SQL_NULL_HDBC );
	ASSERT( m_stmt == SQL_NULL_HSTMT );

	SQLWCHAR* query = L"DELETE FROM STRING_INFO WHERE STRING_ID = ?";
	SQLRETURN sqlret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &m_stmt );
	sqlret = SQLBindParameter( m_stmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &m_paramStringId, 0, nullptr );
	sqlret = SQLPrepareW( m_stmt, query, SQL_NTS );

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlDeleteStringInfo::Execute(  Uint32 stringId )
{
	ASSERT( m_stmt != SQL_NULL_HSTMT );

	m_paramStringId = stringId;
	SQLRETURN sqlret = SQLExecute( m_stmt );
	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}
