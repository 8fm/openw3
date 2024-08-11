// Copyright © 2015 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "sqlSelectStringsWithKeys.h"

/*
Ctor.
*/
SqlSelectStringsWithKeys::SqlSelectStringsWithKeys()
	: m_stmt( SQL_NULL_HSTMT )
{
	Reset();
}

/*
Dtor.
*/
SqlSelectStringsWithKeys::~SqlSelectStringsWithKeys()
{
	Reset();
}

Bool SqlSelectStringsWithKeys::Reset()
{
	SQLRETURN sqlret = SQL_SUCCESS;

	if( m_stmt != SQL_NULL_HSTMT )
	{
		sqlret = SQLFreeHandle( SQL_HANDLE_STMT, m_stmt );
	}

	m_stmt = SQL_NULL_HSTMT;

	m_resultStringIdLenInd = SQL_NULL_DATA;
	m_resultStringId = 0;

	m_resultPropertyNameLenInd = SQL_NULL_DATA;
	m_resultPropertyName[ 0 ] = 0;

	m_resultStringKeyLenInd = SQL_NULL_DATA;
	m_resultStringKey[ 0 ] = 0;

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlSelectStringsWithKeys::Prepare( SQLHDBC conn )
{
	ASSERT( conn != SQL_NULL_HDBC );
	ASSERT( m_stmt == SQL_NULL_HSTMT );

	SQLWCHAR* query =
		L"SELECT STRING_ID, PROPERTY_NAME, STRING_KEY FROM STRING_INFO "
		L"WHERE STRING_KEY IS NOT NULL "
		L"ORDER BY PROPERTY_NAME, STRING_KEY";

	SQLRETURN sqlret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &m_stmt );
	sqlret = SQLPrepareW( m_stmt, query, SQL_NTS );

	sqlret = SQLBindCol( m_stmt, 1, SQL_C_ULONG, &m_resultStringId, 0, &m_resultStringIdLenInd );
	sqlret = SQLBindCol( m_stmt, 2, SQL_C_WCHAR, m_resultPropertyName, sizeof( m_resultPropertyName ), &m_resultPropertyNameLenInd );
	sqlret = SQLBindCol( m_stmt, 3, SQL_C_WCHAR, m_resultStringKey, sizeof( m_resultStringKey ), &m_resultStringKeyLenInd );

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

/*
Executes statement.
*/
Bool SqlSelectStringsWithKeys::Execute( TDynArray< ResultsetRow >& outResult )
{
	ASSERT( m_stmt != SQL_NULL_HSTMT );

	SQLRETURN sqlret = SQLExecute( m_stmt );

	ResultsetRow row;

	Bool fetchNextRow = true;
	while( fetchNextRow )
	{
		sqlret = SQLFetch( m_stmt );

		if( sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO )
		{
			// STRING_ID column.
			ASSERT( m_resultStringIdLenInd != SQL_NULL_DATA ); // STRING_KEY is a NOT NULL column so we can't get NULL value
			row.m_stringId = m_resultStringId;

			// PROPERTY_NAME column.
			ASSERT( m_resultPropertyNameLenInd != SQL_NULL_DATA ); // PROPERTY_NAME is a NOT NULL column so we can't get NULL value
			row.m_stringPropertyName = m_resultPropertyName;

			// STRING_KEY column.
			// Note we represent NULL values as empty string (we don't distinguish NULL values from empty string values).
			row.m_stringKey = ( m_resultStringKeyLenInd != SQL_NULL_DATA )? m_resultStringKey : String::EMPTY;

			outResult.PushBack( row );
		}
		else
		{
			// Wtop when all rows were fetched (SQL_NO_DATA) or an error was encountered.
			fetchNextRow = false;
		}
	}

	sqlret = SQLCloseCursor( m_stmt );

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}
