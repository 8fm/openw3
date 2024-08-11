// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "odbcMgr.h"

/*
Ctor.
*/
COdbcMgr::COdbcMgr()
: m_henv( SQL_NULL_HENV )
{}

/*
Dtor.
*/
COdbcMgr::~COdbcMgr()
{
	// assert that ODBC manager was properly uninitialized
	ASSERT( !IsInitialized() );
}

/*
Returns whether ODBC manager is initialized or not.
*/
Bool COdbcMgr::IsInitialized() const
{
	return m_henv != SQL_NULL_HENV;
}

/*
Initializes ODBC manager.

\return True - ODBC manager initialized successfully, false - otherwise.

It's obligatory to initialized ODBC manager before use.
*/
Bool COdbcMgr::Initialize()
{
	ASSERT( m_henv == SQL_NULL_HENV );

	SQLRETURN sqlret = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv );
	if( sqlret != SQL_SUCCESS && sqlret != SQL_SUCCESS_WITH_INFO )
	{
		return false;
	}

	sqlret = SQLSetEnvAttr( m_henv, SQL_ATTR_ODBC_VERSION, reinterpret_cast< void* >( SQL_OV_ODBC3 ), -1 );

	return true;
}

/*
Uninitializes ODBC manager.

\return True - ODBC manager uninitialized successfully, false - otherwise.

It's obligatory to uninitialize ODBC manager after use.
*/
Bool COdbcMgr::Uninitialize()
{
	ASSERT( m_henv != SQL_NULL_HENV );

	SQLFreeHandle( SQL_HANDLE_ENV, m_henv );
	m_henv = SQL_NULL_HENV;

	return true;
}

/*
Establishes connection using specified connection string.

\param connString Connection string. Must not be nullptr.
\return Connection handle. If connection couldn't be established then SQL_NULL_HDBC is returned.
*/
SQLHDBC COdbcMgr::Connect( SQLWCHAR* connString )
{
	ASSERT( connString );

	SQLHDBC conn = SQL_NULL_HDBC;
	SQLRETURN sqlret = SQLAllocHandle( SQL_HANDLE_DBC, m_henv, &conn );

	sqlret = SQLDriverConnectW( conn, NULL, connString, SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT );

	if( sqlret != SQL_SUCCESS )
	{
		if( sqlret == SQL_SUCCESS_WITH_INFO )
		{
			SQLINTEGER numStatusRecords = 0;
			SQLSMALLINT numBytes = 0;
			SQLGetDiagFieldW( SQL_HANDLE_DBC, conn, 0, SQL_DIAG_NUMBER, &numStatusRecords, 0, &numBytes );

			SQLWCHAR messageText[1024];
			SQLSMALLINT messageLen = 0;
			SQLWCHAR sqlState[6];
			SQLINTEGER nativeError = 0;
			for( SQLINTEGER recNumber = 1; recNumber <= numStatusRecords; ++recNumber )
			{
				SQLGetDiagRecW( SQL_HANDLE_DBC, conn, recNumber, sqlState, &nativeError, messageText, 1024, &messageLen);
			}
		}
	}

	if( sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO )
	{
		// connection was established
		m_conns.PushBack( conn );
		return conn;
	}
	else
	{
		return SQL_NULL_HDBC;
	}
}

/*
Disconnects connection.

\param conn Connection to disconnect. Must belong to this COdbcMgr. Must not be SQL_NULL_HDBC.
\return True - connection disconnected, false - error when disconnecting connection or connection doesn't belong to this COdbcMgr.
*/
Bool COdbcMgr::Disconnect( SQLHDBC conn )
{
	ASSERT( conn != SQL_NULL_HDBC );

	SQLRETURN sqlret = SQL_SUCCESS;

	Bool foundAndRemoved = m_conns.Remove( conn );
	if( foundAndRemoved )
	{
		sqlret = SQLDisconnect( conn );
		sqlret = SQLFreeHandle( SQL_HANDLE_DBC, conn );
		return sqlret == SQL_SUCCESS || sqlret == SQL_SUCCESS_WITH_INFO;
	}
	else
	{
		// Connection doesn't belong to this COdbcMgr.
		return false;
	}
}

/*
Convenience function for connecting with W2 String DB.

\return Connection handle. If connection couldn't be established then SQL_NULL_HDBC is returned.
*/
SQLHDBC ConnectW2StringDb( COdbcMgr& odbcMgr )
{
	ASSERT( odbcMgr.IsInitialized() );

	SQLWCHAR* connString = L"Driver={SQL Server};Server=CDPRS-MSSQL\\sqlexpress;Database=EditorStringDatabase;Trusted_Connection=Yes";
	return odbcMgr.Connect( connString );
}

/*
Convenience function for connecting with W3 String DB.

\return Connection handle. If connection couldn't be established then SQL_NULL_HDBC is returned.
*/
SQLHDBC ConnectW3StringDb( COdbcMgr& odbcMgr )
{
	ASSERT( odbcMgr.IsInitialized() );

	SQLWCHAR* connString = L"Driver={SQL Server};Server=CDPRS-MSSQL\\sqlexpress;Database=EditorStringDatabaseW3;Trusted_Connection=Yes";
	return odbcMgr.Connect( connString );
}

/*
Convenience function for connecting with Red DB.

\return Connection handle. If connection couldn't be established then SQL_NULL_HDBC is returned.

Note that Red DB is currently the same database as W3 String DB - this, however, may change in future.
*/
SQLHDBC ConnectRedDb( COdbcMgr& odbcMgr )
{
	ASSERT( odbcMgr.IsInitialized() );

	SQLWCHAR* connString = L"Driver={SQL Server};Server=CDPRS-MSSQL\\sqlexpress;Database=EditorStringDatabaseW3;Trusted_Connection=Yes";
	return odbcMgr.Connect( connString );
}
