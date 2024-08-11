// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "stringDbTypes.h"
#include <sql.h>
#include <sqlext.h>

class CRedUserPrivileges;

/*
Gets Red User privileges from Red DB.
*/
class SqlGetRedUserPrivileges
{
public:
	SqlGetRedUserPrivileges();
	~SqlGetRedUserPrivileges();

	Bool Reset();
	Bool Prepare( SQLHDBC conn );
	Bool Execute( const String& redUser, CRedUserPrivileges& outRedUserPrivileges );

private:
	SqlGetRedUserPrivileges( const SqlGetRedUserPrivileges& );				// cctor - not defined
	SqlGetRedUserPrivileges& operator=( const SqlGetRedUserPrivileges& );	// op= - not defined

	SQLHSTMT m_stmt;														// Statement handle.

	static const SQLULEN m_userColumnSize = 256;							// Size (in chars) of RED_USER column as defined in Red DB. It's equal
																			// to UNLEN which defines maximum user name length in Windows.
	SQLLEN m_paramUserLenInd;												// Length/indicator buffer for m_paramUser.
	SQLWCHAR m_paramUser[ m_userColumnSize + 1 ];							// Bound to statement parameter containing user name. + 1 for null.

	SQLLEN m_resultEditDialogsPrivilegeLenInd;								// Length/indicator buffer for m_resultEditDialogsPrivilege.
	SQLCHAR m_resultEditDialogsPrivilege;									// Bound to result set EDIT_DIALOGS column.

	SQLLEN m_resultApproveVoPrivilegeLenInd;								// Length/indicator buffer for m_resultApproveVoPrivilege.
	SQLCHAR m_resultApproveVoPrivilege;										// Bound to result set APPROVE_VO column.

	SQLLEN m_resultEditRedStringsPrivilegeLenInd;							// Length/indicator buffer for m_resultEditRedStrings.
	SQLCHAR m_resultEditRedStringsPrivilege;								// Bound to result set EDIT_REDSTRINGS column.
};
