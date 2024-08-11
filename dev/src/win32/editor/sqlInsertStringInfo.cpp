// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "sqlInsertStringInfo.h"

/*
Ctor.
*/
SqlInsertStringInfo::SqlInsertStringInfo()
: m_stmt( SQL_NULL_HSTMT )
{
	Reset();
}

/*
Dtor.
*/
SqlInsertStringInfo::~SqlInsertStringInfo()
{
	Reset();
}

Bool SqlInsertStringInfo::Reset()
{
	SQLRETURN sqlret = SQL_SUCCESS;

	if( m_stmt != SQL_NULL_HSTMT )
	{
		sqlret = SQLFreeHandle( SQL_HANDLE_STMT, m_stmt );
	}

	m_stmt = SQL_NULL_HSTMT;

	m_paramStringId = 0;

	m_paramResourceLenInd = SQL_NULL_DATA;
	m_paramResource[ 0 ] = 0;

	m_paramPropertyNameLenInd = SQL_NULL_DATA;
	m_paramPropertyName[ 0 ] = 0;

	m_paramVoiceoverNameLenInd = SQL_NULL_DATA;
	m_paramVoiceoverName[ 0 ] = 0;

	m_paramStringKeyLenInd = SQL_NULL_DATA;
	m_paramStringKey[ 0 ] = 0;

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlInsertStringInfo::Prepare( SQLHDBC conn )
{
	ASSERT( conn != SQL_NULL_HDBC );
	ASSERT( m_stmt == SQL_NULL_HSTMT );

	SQLWCHAR* query =
		L"INSERT INTO STRING_INFO "
		L"(STRING_ID, RESOURCE, PROPERTY_NAME, VOICEOVER_NAME, STRING_KEY) "
		L"VALUES (?, ?, ?, ?, ?)";

	SQLRETURN sqlret = SQLAllocHandle( SQL_HANDLE_STMT, conn, &m_stmt );

	SQLLEN ignoredBufferLength = 0; // used as a buffer length argument in cases where it's ignored

	sqlret = SQLBindParameter( m_stmt, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &m_paramStringId, 0, nullptr );
	sqlret = SQLBindParameter( m_stmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, m_resourceColumnSize, 0, m_paramResource, ignoredBufferLength, &m_paramResourceLenInd );
	sqlret = SQLBindParameter( m_stmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, m_propertyNameColumnSize, 0, m_paramPropertyName, ignoredBufferLength, &m_paramPropertyNameLenInd );
	sqlret = SQLBindParameter( m_stmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, m_voiceoverNameColumnSize, 0, m_paramVoiceoverName, ignoredBufferLength, &m_paramVoiceoverNameLenInd );
	sqlret = SQLBindParameter( m_stmt, 5, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WCHAR, m_stringKeyColumnSize, 0, m_paramStringKey, ignoredBufferLength, &m_paramStringKeyLenInd );

	sqlret = SQLPrepareW( m_stmt, query, SQL_NTS );

	return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
}

Bool SqlInsertStringInfo::Execute( const StringInfoRow& stringInfoRow )
{
	ASSERT( m_stmt != SQL_NULL_HSTMT );

	m_paramStringId = stringInfoRow.m_stringId;

	ASSERT( !stringInfoRow.m_resourceName.Empty() ); // RESOURCE column doesn't accept null values
	m_paramResourceLenInd = stringInfoRow.m_resourceName.GetLength() * sizeof( String::value_type );
	wcscpy_s( m_paramResource, sizeof( m_paramResource ) / sizeof( m_paramResource[ 0 ] ), stringInfoRow.m_resourceName.AsChar() );

	ASSERT( !stringInfoRow.m_propertyName.Empty() ); // PROPERTY_NAME column doesn't accept null values
	m_paramPropertyNameLenInd = stringInfoRow.m_propertyName.GetLength() * sizeof( String::value_type );
	wcscpy_s( m_paramPropertyName, sizeof( m_paramPropertyName ) / sizeof( m_paramPropertyName[ 0 ] ), stringInfoRow.m_propertyName.AsChar() );

	if( stringInfoRow.m_voiceoverName.Empty() )
	{
		m_paramVoiceoverNameLenInd = SQL_NULL_DATA; // indicate null value
	}
	else
	{
		m_paramVoiceoverNameLenInd = stringInfoRow.m_voiceoverName.GetLength() * sizeof( String::value_type );
		wcscpy_s( m_paramVoiceoverName, sizeof( m_paramVoiceoverName ) / sizeof( m_paramVoiceoverName[ 0 ] ), stringInfoRow.m_voiceoverName.AsChar() );
	}

	if( stringInfoRow.m_stringKey.Empty() )
	{
		m_paramStringKeyLenInd = SQL_NULL_DATA; // indicate null value
	}
	else
	{
		m_paramStringKeyLenInd = stringInfoRow.m_stringKey.GetLength() * sizeof( String::value_type );
		wcscpy_s( m_paramStringKey, sizeof( m_paramStringKey ) / sizeof( m_paramStringKey[ 0 ] ), stringInfoRow.m_stringKey.AsChar() );
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
