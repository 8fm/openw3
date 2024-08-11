// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "sqlSelectStrings.h"

/*
Ctor.
*/
SqlSelectStrings::SqlSelectStrings()
: m_stmt( SQL_NULL_HSTMT )
{
	Reset();
}

/*
Dtor.
*/
SqlSelectStrings::~SqlSelectStrings()
{
	Reset();
}

Bool SqlSelectStrings::Reset()
{
	SQLRETURN sqlret = SQL_SUCCESS;

	if( m_stmt != SQL_NULL_HSTMT )
	{
		sqlret = SQLFreeHandle( SQL_HANDLE_STMT, m_stmt );
	}

	m_stmt = SQL_NULL_HSTMT;

	m_paramStringId = 0;

	m_resultLangIdLenInd = SQL_NULL_DATA;
	m_resultLangId = 0;

	m_resultVersionLenInd = SQL_NULL_DATA;
	m_resultVersion = 0;

	m_resultTextLenInd = SQL_NULL_DATA;
	m_resultText[ 0 ] = 0;

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlSelectStrings::Prepare( SQLHDBC conn )
{
	ASSERT( conn != SQL_NULL_HDBC );
	ASSERT( m_stmt == SQL_NULL_HSTMT );

	SQLWCHAR* query = L"SELECT LANG, VERSION, TEXT FROM STRINGS WHERE STRING_ID = ?";
	SQLRETURN sqlret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &m_stmt );
	sqlret = SQLBindParameter( m_stmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &m_paramStringId, 0, nullptr );
	sqlret = SQLPrepareW( m_stmt, query, SQL_NTS );

	sqlret = SQLBindCol( m_stmt, 1, SQL_C_ULONG, &m_resultLangId, 0, &m_resultLangIdLenInd );
	sqlret = SQLBindCol( m_stmt, 2, SQL_C_ULONG, &m_resultVersion, 0, &m_resultVersionLenInd );
	sqlret = SQLBindCol( m_stmt, 3, SQL_C_WCHAR, m_resultText, sizeof( m_resultText ), &m_resultTextLenInd );

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

/*
Executes statement.

TEXT column of STRINGS table accepts NULL values but we don't distinguish them
from empty string values. In both cases StringRow::m_text is an empty string.
*/
Bool SqlSelectStrings::Execute( Uint32 stringId, TDynArray< StringRow >& outStringRows )
{
	ASSERT( m_stmt != SQL_NULL_HSTMT );

	m_paramStringId = stringId;
	SQLRETURN sqlret = SQLExecute( m_stmt );

	StringRow stringRow;

	Bool fetchNextRow = true;
	while( fetchNextRow )
	{
		sqlret = SQLFetch( m_stmt );

		if( sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO )
		{
			// string id
			stringRow.m_stringId = m_paramStringId;

			// language id
			ASSERT( m_resultLangIdLenInd != SQL_NULL_DATA ); // LANG is a NOT NULL column so we can't get NULL value
			stringRow.m_langId = m_resultLangId;

			// version number
			ASSERT( m_resultVersionLenInd != SQL_NULL_DATA ); // VERSION is a NOT NULL so we can't get NULL value
			stringRow.m_version = m_resultVersion;

			// text
			if( m_resultTextLenInd == SQL_NULL_DATA )
			{
				// we represent NULL values as empty string (we don't distinguish NULL values from empty string values)
				stringRow.m_text = String::EMPTY;
			}
			else
			{
				stringRow.m_text = m_resultText;
			}

			outStringRows.PushBack( stringRow );
		}
		else
		{
			// stop when all rows were fetched (SQL_NO_DATA) or an error was encountered
			fetchNextRow = false;
		}
	}

	sqlret = SQLCloseCursor( m_stmt );

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}
