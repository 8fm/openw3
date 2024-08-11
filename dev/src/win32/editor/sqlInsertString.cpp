// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "sqlInsertString.h"

/*
Ctor.
*/
SqlInsertString::SqlInsertString()
: m_stmt( SQL_NULL_HSTMT )
{
	Reset();
}

/*
Dtor.
*/
SqlInsertString::~SqlInsertString()
{
	Reset();
}

Bool SqlInsertString::Reset()
{
	SQLRETURN sqlret = SQL_SUCCESS;

	if( m_stmt != SQL_NULL_HSTMT )
	{
		sqlret = SQLFreeHandle( SQL_HANDLE_STMT, m_stmt );
	}

	m_stmt = SQL_NULL_HSTMT;

	m_paramStringId = 0;
	m_paramLangId = 0;
	m_paramVersion = 0;

	m_paramTextLenInd = SQL_NULL_DATA;
	m_paramText[ 0 ] = 0;

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlInsertString::Prepare( SQLHDBC conn )
{
	ASSERT( conn != SQL_NULL_HDBC );
	ASSERT( m_stmt == SQL_NULL_HSTMT );

	SQLWCHAR* query = L"INSERT INTO STRINGS VALUES (?, ?, ?, ?, CURRENT_USER, CURRENT_TIMESTAMP)";
	
	SQLRETURN sqlret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &m_stmt );

	sqlret = SQLBindParameter( m_stmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &m_paramStringId, 0, nullptr );
	sqlret = SQLBindParameter( m_stmt, 2, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &m_paramLangId, 0, nullptr );
	sqlret = SQLBindParameter( m_stmt, 3, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &m_paramVersion, 0, nullptr );

	SQLLEN ignoredBufferLength = 0; // this is ignored as it's not needed for input buffer
	sqlret = SQLBindParameter( m_stmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, m_textColumnSize, 0, m_paramText, ignoredBufferLength, &m_paramTextLenInd );

	sqlret = SQLPrepareW( m_stmt, query, SQL_NTS );

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlInsertString::Execute(  const StringRow& stringRow )
{
	ASSERT( m_stmt != SQL_NULL_HSTMT );

	m_paramStringId = stringRow.m_stringId;
	m_paramLangId = stringRow.m_langId;
	m_paramVersion = stringRow.m_version;

	if( stringRow.m_text.Empty() )
	{
		// for empty strings we insert NULL value
		m_paramTextLenInd = SQL_NULL_DATA;
	}
	else
	{
		const Uint32 destSizeInChars = sizeof( m_paramText ) / sizeof( m_paramText[ 0 ] );
		m_paramTextLenInd = stringRow.m_text.GetLength() * sizeof( String::value_type );
		wcscpy_s( m_paramText, destSizeInChars, stringRow.m_text.AsChar() );
	}

	SQLRETURN sqlret = SQLExecute( m_stmt );

	if( sqlret == SQL_ERROR )
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
