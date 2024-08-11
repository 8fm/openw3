// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#include "build.h"
#include "redUserPrivileges.h"
#include "odbcMgr.h"
#include "sqlGetRedUserPrivileges.h"

#include <lmcons.h> // for UNLEN (maximum length of user name in Windows)

/*
Convenience function for retrieving privileges of current Red User from Red DB.

This is a convenience function that does everything that's necessary to get Red User privileges from Red DB,
i.e. it connects to Red DB, prepares necessary queries, executes those queries, retrieves results and and then
it closes the connection. It's not meant to be heavily used. You should call RetrieveRedUserPrivileges() and
cache results somewhere. If you need to bombard Red DB with many privilege queries then please devise our own
solution (use COdbcMgr to connect to Red DB, prepare privilege query once, execute it as many times as necessary
and then close the connection).

\return Current Red User privileges. If connection to Red DB could not be established then function returns
CRedUserPrivileges object with all privileges revoked.
*/
CRedUserPrivileges RetrieveRedUserPrivileges()
{
	CRedUserPrivileges redUserPrivileges;
	redUserPrivileges.RevokeAll();

	// query database to get privileges
	COdbcMgr odbcMgr;
	if( odbcMgr.Initialize() )
	{
		// connect to Red DB
		SQLHDBC connRedDb = ConnectRedDb( odbcMgr );

		// prepare query
		SqlGetRedUserPrivileges queryGetPrivileges;
		queryGetPrivileges.Prepare( connRedDb );

		// get user name
		DWORD size = UNLEN + 1;
		wchar_t name[ UNLEN + 1 ] = { 0 };
		GetUserNameW( name, &size );
		String username( name );

		// get privileges
		queryGetPrivileges.Execute( username, redUserPrivileges );

		// release
		queryGetPrivileges.Reset();
		odbcMgr.Disconnect( connRedDb );
		odbcMgr.Uninitialize();
	}

	return redUserPrivileges;
}
