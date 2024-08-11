// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "sqlGetRedUserPrivileges.h"
#include "redUserPrivileges.h"

/*
Ctor.
*/
SqlGetRedUserPrivileges::SqlGetRedUserPrivileges()
: m_stmt( SQL_NULL_HSTMT )
{
	Reset();
}

/*
Dtor.
*/
SqlGetRedUserPrivileges::~SqlGetRedUserPrivileges()
{
	Reset();
}

Bool SqlGetRedUserPrivileges::Reset()
{
	SQLRETURN sqlret = SQL_SUCCESS;

	if( m_stmt != SQL_NULL_HSTMT )
	{
		sqlret = SQLFreeHandle( SQL_HANDLE_STMT, m_stmt );
	}

	m_stmt = SQL_NULL_HSTMT;

	m_paramUserLenInd = SQL_NULL_DATA;
	m_paramUser[ 0 ] = 0;

	m_resultEditDialogsPrivilegeLenInd = SQL_NULL_DATA;
	m_resultEditDialogsPrivilege = 0;

	m_resultApproveVoPrivilegeLenInd = SQL_NULL_DATA;
	m_resultApproveVoPrivilege = 0;

	m_resultEditRedStringsPrivilegeLenInd = SQL_NULL_DATA;
	m_resultEditRedStringsPrivilege = 0;

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlGetRedUserPrivileges::Prepare( SQLHDBC conn )
{
	ASSERT( conn != SQL_NULL_HDBC );
	ASSERT( m_stmt == SQL_NULL_HSTMT );

	SQLWCHAR* query = L"SELECT EDIT_DIALOGS, APPROVE_VO, EDIT_REDSTRINGS FROM RED_USER_PRIVILEGES WHERE RED_USER = ?";

	SQLRETURN sqlret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &m_stmt );

	SQLLEN ignoredBufferLength = 0; // this is ignored as it's not needed for input buffer
	sqlret = SQLBindParameter( m_stmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, m_userColumnSize, 0, m_paramUser, ignoredBufferLength, &m_paramUserLenInd );

	sqlret = SQLPrepareW( m_stmt, query, SQL_NTS );

	sqlret = SQLBindCol( m_stmt, 1, SQL_C_BIT, &m_resultEditDialogsPrivilege, 0, &m_resultEditDialogsPrivilegeLenInd );
	sqlret = SQLBindCol( m_stmt, 2, SQL_C_BIT, &m_resultApproveVoPrivilege, 0, &m_resultApproveVoPrivilegeLenInd );
	sqlret = SQLBindCol( m_stmt, 3, SQL_C_BIT, &m_resultEditRedStringsPrivilege, 0, &m_resultEditRedStringsPrivilegeLenInd );
	// TODO: make sure that only 1 byte is written and not more (due to possible differences between SQL-92 bit and ODBC bit)

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

/*
Executes the query to get Red User privileges from String DB.

\param redUser Red User name. Must not be empty.
\param outRedUserPrivleges (out) Object that will receive Red User privileges.
\return True - Red User privileges were successfully retrieved from Red DB, false - an error occurred
(in this case outRedUserPrivileges will have all privileges revoked).
*/
Bool SqlGetRedUserPrivileges::Execute(  const String& redUser, CRedUserPrivileges& outRedUserPrivileges )
{
	ASSERT( m_stmt != SQL_NULL_HSTMT );
	ASSERT( !redUser.Empty() );

	// set value of user parameter
	const Uint32 destSizeInChars = sizeof( m_paramUser ) / sizeof( m_paramUser[ 0 ] );
	m_paramUserLenInd = redUser.GetLength() * sizeof( String::value_type );
	wcscpy_s( m_paramUser, destSizeInChars, redUser.AsChar() );

	SQLRETURN sqlret = SQLExecute( m_stmt );
	sqlret = SQLFetch( m_stmt );

	if( sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO )
	{
		outRedUserPrivileges.m_editDialogs = ( m_resultEditDialogsPrivilege > 0 );
		outRedUserPrivileges.m_approveVo = ( m_resultApproveVoPrivilege > 0 );
		outRedUserPrivileges.m_editRedStrings = ( m_resultEditRedStringsPrivilege > 0 );
	}
	else
	{
		// Revoke all privileges if there's no such user in Red DB (or in case of some error).
		outRedUserPrivileges.RevokeAll();
	}

	sqlret = SQLCloseCursor( m_stmt );

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}
