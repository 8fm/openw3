// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "stringDbTypes.h"

#include "odbcMgr.h"
#include "sqlSelectStrings.h"
#include "sqlGetNextStringId.h"
#include "sqlInsertString.h"
#include "sqlInsertStringInfo.h"
#include "sqlDeleteString.h"
#include "sqlDeleteStringInfo.h"

#include <sql.h>
#include <sqlext.h>

/*
W2 string importer - imports strings from W2 String DB into W3 String DB.
*/
class W2StringImporter
{
public:
	W2StringImporter();
	~W2StringImporter();

	Bool IsInitialized() const;

	Bool Initialize();
	Bool Uninitialize();

	Bool ProcessSection( const CStorySceneSection& section );

private:
	void ProcessLocalizedString( LocalizedString& lstr, const String& resourceName, const String& propertyName );

	Bool m_isInitialized;							// Indicates whether importer is initialized and ready to use.

	COdbcMgr m_odbcMgr;								// ODBC Manager.
	SQLHDBC m_connW2StringDb;						// W2 String DB connection handle.
	SQLHDBC m_connW3StringDb;						// W3 String DB connection handle.

	SqlSelectStrings m_querySelectStringsW2;		// Selects strings from STRINGS table in W2 String DB.
	SqlGetNextStringId m_queryGetNextStringIdW3;	// Queries W3 String DB to get next string id.
	SqlInsertStringInfo m_queryInsertStringInfoW3;	// Inserts row into STRING_INFO table in W3 String DB.
	SqlInsertString m_queryInsertStringW3;			// Inserts row into STRINGS table in W3 String DB.
	SqlDeleteString m_queryDeleteStringW3;			// Deletes row from STRINGS table in W3 String DB.
	SqlDeleteStringInfo m_queryDeleteStringInfoW3;	// Deletes row from STRING_INFO table in W3 String DB.
};
