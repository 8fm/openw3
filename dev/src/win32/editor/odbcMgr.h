// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include <sql.h>
#include <sqlext.h>

/*
ODBC manager.
*/
class COdbcMgr
{
public:
	COdbcMgr();
	~COdbcMgr();

	Bool IsInitialized() const;

	Bool Initialize();
	Bool Uninitialize();

	SQLHDBC Connect( SQLWCHAR* connString );
	Bool Disconnect( SQLHDBC conn );

private:
	COdbcMgr( const COdbcMgr& );				// cctor - not defined
	COdbcMgr& operator=( const COdbcMgr& );		// op= - not defined

	SQLHENV m_henv;								// Environment handle.
	TDynArray< SQLHDBC > m_conns;				// List of connection handles.
};

// convenience functions for connecting with W2 String DB, W3 String DB and Red DB
SQLHDBC ConnectW2StringDb( COdbcMgr& odbcMgr );
SQLHDBC ConnectW3StringDb( COdbcMgr& odbcMgr );
SQLHDBC ConnectRedDb( COdbcMgr& odbcMgr );
