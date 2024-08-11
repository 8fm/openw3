// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "sqlDeleteString.h"

/*
Ctor.
*/
SqlDeleteString::SqlDeleteString()
: m_stmt( SQL_NULL_HSTMT )
{
	Reset();
}

/*
Dtor.
*/
SqlDeleteString::~SqlDeleteString()
{
	Reset();
}

Bool SqlDeleteString::Reset()
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

Bool SqlDeleteString::Prepare( SQLHDBC conn )
{
	ASSERT( conn != SQL_NULL_HDBC );
	ASSERT( m_stmt == SQL_NULL_HSTMT );

	SQLWCHAR* query = L"DELETE FROM STRINGS WHERE STRING_ID = ?";
	SQLRETURN sqlret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &m_stmt );
	sqlret = SQLBindParameter( m_stmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &m_paramStringId, 0, nullptr );
	sqlret = SQLPrepareW( m_stmt, query, SQL_NTS );

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlDeleteString::Execute(  Uint32 stringId )
{
	ASSERT( m_stmt != SQL_NULL_HSTMT );

	m_paramStringId = stringId;
	SQLRETURN sqlret = SQLExecute( m_stmt );

	if( sqlret != SQL_SUCCESS )
	{
		SQLINTEGER numStatusRecords = 0;
		SQLSMALLINT numBytes = 0;
		SQLGetDiagFieldW( SQL_HANDLE_STMT, m_stmt, 0, SQL_DIAG_NUMBER, &numStatusRecords, 0, &numBytes );

		SQLWCHAR messageText[1024];
		SQLSMALLINT messageLen = 0;
		SQLWCHAR sqlState[6];
		SQLINTEGER nativeError = 0;
		for( SQLINTEGER recNumber = 1; recNumber <= numStatusRecords; ++recNumber )
		{
			SQLGetDiagRecW( SQL_HANDLE_STMT, m_stmt, recNumber, sqlState, &nativeError, messageText, 1024, &messageLen);
		}
	}

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}
