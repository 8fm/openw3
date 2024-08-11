/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#define _HAS_EXCEPTIONS 0

#include "HelperFunctions.h"
#include "PointsofInterestDB.h"
#include "POISqlCommandsDefinitions.h"

class CPointsofInterestDB
{
public:
	CPointsofInterestDB()
	{
		m_connected = false;
		OleInitialize( 0 );
	}

	~CPointsofInterestDB()
	{
		if( m_connected )
		{
			m_session.Close();
			m_dataSource.Close();
		}
	}

	CDataSource m_dataSource;
	CSession m_session;
	bool m_connected;
};



#pragma region GLOBAL CONNECTION OBJECT
// This shouldn't be made global, but is a hack #1 to ensure proper dll unloading order
CPointsofInterestDB GPOIConnectionDB;
#pragma endregion



// CONNECT AND DISCONNECT
DLL CPointsofInterestDB* PointsofInterestDBInit( const WCHAR* databaseAddress )
{
	if( GPOIConnectionDB.m_connected )
	{
		return NULL;	// only one connection at a time
	}

	HRESULT hr;

	CDBPropSet rgPropertySet[1] = {DBPROPSET_DBINIT};

	// deafult address: "CDPRS-MSSQL\\sqlexpress"
	rgPropertySet[0].AddProperty(DBPROP_INIT_DATASOURCE, databaseAddress );
	rgPropertySet[0].AddProperty(DBPROP_INIT_CATALOG, L"Assets");
	rgPropertySet[0].AddProperty(DBPROP_AUTH_INTEGRATED, L"SSPI");

	CLSID clsid = {0xc7ff16cL,0x38e3,0x11d0,
	{0x97,0xab,0x0,0xc0,0x4f,0xc2,0xad,0x98}};

	hr = GPOIConnectionDB.m_dataSource.Open(clsid, rgPropertySet, 1);

	if( FAILED(hr) )
	{
		return NULL;
	}

	hr = GPOIConnectionDB.m_session.Open( GPOIConnectionDB.m_dataSource );

	if ( FAILED(hr) )
	{
		return NULL;
	}
	GPOIConnectionDB.m_connected = true;

	return &GPOIConnectionDB;
}

DLL void PointsofInterestDBClose( CPointsofInterestDB *conn )
{
	if( !conn )
		return;

	if ( conn->m_connected )
	{
		conn->m_session.Close();
		conn->m_dataSource.Close();
		conn->m_connected = false;
	}
}


// GET NEW ID
DLL int PointsofInterestDBNextIDForPOI(CPointsofInterestDB* connection, unsigned int *id )
{
	int ret = 1;

	if( !connection )
	{
		return -1;
	}
	CCommand<CDynamicAccessor> rs;

	HRESULT hr = rs.Open( connection->m_session, GGetNextPOIIdQuery );

	if( FAILED(hr) )
	{
		return -1;
	}

	hr = rs.MoveFirst( );
	if( SUCCEEDED( hr ) && hr != DB_S_ENDOFROWSET )
	{
		long res = *(long*)rs.GetValue( 1 );
		(*id) = static_cast<unsigned int>(res);
	}
	else
	{
		ret = -1;
	}

	rs.Close();   
	rs.ReleaseCommand();

	return ret;
}


// INSERT
DLL int PointsofInterestDBInsertPOI(CPointsofInterestDB* connection, unsigned int POIId, unsigned int levelId, const WCHAR* title, const WCHAR* description, unsigned int category, unsigned int coordinateX, unsigned int coordinateY, bool snapToTerrain, float height)
{
	int retVal = 1;

	if( !connection )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	//// POIId
	//wstringstream wss0;
	//wss0 << POIId;

	//params[L"PARAM_AP_ID"] = wss0.str();
	//paramQuotes[L"PARAM_AP_ID"] = false;

	// levelId
	wstringstream wss1;
	wss1 << levelId;

	params[L"PARAM_LOCATION_ID"] = wss1.str();
	paramQuotes[L"PARAM_LOCATION_ID"] = false;

	// title
	if ( wcslen( title ) != 0 )
	{
		params[L"PARAM_AP_NAME"] = title;
		paramQuotes[L"PARAM_AP_NAME"] = true;
	}
	else
	{
		return -1;
	}

	// description
	if ( wcslen( description ) != 0 )
	{
		params[L"PARAM_AP_DESCRIPTION"] = description;
		paramQuotes[L"PARAM_AP_DESCRIPTION"] = true;
	}
	else
	{
		return -1;
	}

	// category
	wstringstream wss2;
	wss2 << category;

	params[L"PARAM_AP_CATEGORY"] = wss2.str();
	paramQuotes[L"PARAM_AP_CATEGORY"] = false;

	// coordinateX
	wstringstream wss3;
	wss3 << coordinateX;

	params[L"PARAM_AP_X"] = wss3.str();
	paramQuotes[L"PARAM_AP_X"] = false;

	// coordinateY
	wstringstream wss4;
	wss4 << coordinateY;

	params[L"PARAM_AP_Y"] = wss4.str();
	paramQuotes[L"PARAM_AP_Y"] = false;

	// height
	wstringstream wss5;
	if ( snapToTerrain )
	{
		wss5 << "NULL";
	}
	else
	{
		wss5 << height;
	}

	params[L"PARAM_HEIGHT"] = wss5.str();
	paramQuotes[L"PARAM_HEIGHT"] = false;


	CCommand<CNoAccessor, CNoRowset> cmd;

	wstring command = tokenReplacerWithoutSingleQuotes( GInsertPOIQuery, params, paramQuotes, true );

	HRESULT hr = cmd.Open( connection->m_session, command.c_str(), NULL, NULL, DBGUID_DEFAULT, false );

	if( FAILED( hr ) )
	{
		retVal = -1;
	}

	if(retVal != -1)
	{
		PointsofInterestDBInsertPOILog(connection, POIId, L"Create");
	}

	cmd.Close();
	cmd.ReleaseCommand();

	return retVal;
}

DLL int PointsofInterestDBInsertPOILog(CPointsofInterestDB* connection, unsigned int POIId, const WCHAR* modificationType)
{
	int retVal = 1;

	if( !connection )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	// POIId
	wstringstream wss0;
	wss0 << POIId;

	params[L"PARAM_APOINT_ID"] = wss0.str();
	paramQuotes[L"PARAM_APOINT_ID"] = false;

	// user
	wchar_t lpszUsername[255];
	DWORD dUsername = sizeof(lpszUsername);
	if(GetUserName(lpszUsername, &dUsername) == false)
	{
		wcscpy_s(lpszUsername, dUsername, L"Unknown user");
	}

	if ( wcslen( lpszUsername ) != 0 )
	{
		params[L"PARAM_USER"] = lpszUsername;
		paramQuotes[L"PARAM_USER"] = true;
	}
	else
	{
		return -1;
	}

	// title
	if ( wcslen( modificationType ) != 0 )
	{
		params[L"PARAM_MODIFICATION_TYPE"] = modificationType;
		paramQuotes[L"PARAM_MODIFICATION_TYPE"] = true;
	}
	else
	{
		return -1;
	}

	CCommand<CNoAccessor, CNoRowset> cmd;

	wstring command = tokenReplacerWithoutSingleQuotes( GInsertPOILogQuery, params, paramQuotes, true );

	HRESULT hr = cmd.Open( connection->m_session, command.c_str(), NULL, NULL, DBGUID_DEFAULT, false );

	if( FAILED( hr ) )
	{
		retVal = -1;
	}

	cmd.Close();
	cmd.ReleaseCommand();

	return retVal;
}



// UPDATE
DLL int PointsofInterestDBUpdatePOI( CPointsofInterestDB* connection, unsigned int coordinateX, unsigned int coordinateY, bool snapToTerrain, float height, const WCHAR* name, const WCHAR* description, unsigned int category, unsigned int POIId )
{
	int retVal = 1;

	if( !connection )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	// coordinateX
	wstringstream wss1;
	wss1 << coordinateX;

	params[L"PARAM_AP_X"] = wss1.str();
	paramQuotes[L"PARAM_AP_X"] = false;

	// coordinateY
	wstringstream wss2;
	wss2 << coordinateY;

	params[L"PARAM_AP_Y"] = wss2.str();
	paramQuotes[L"PARAM_AP_Y"] = false;

	// height
	wstringstream wssh;
	if ( snapToTerrain )
	{
		wssh << "NULL";
	}
	else
	{
		wssh << height;
	}

	params[L"PARAM_HEIGHT"] = wssh.str();
	paramQuotes[L"PARAM_HEIGHT"] = false;

	// POIId
	wstringstream wss3;
	wss3 << POIId;

	params[L"PARAM_AP_ID"] = wss3.str();
	paramQuotes[L"PARAM_AP_ID"] = false;

	// name
	if ( wcslen( name ) != 0 )
	{
		params[L"PARAM_AP_NAME"] = name;
		paramQuotes[L"PARAM_AP_NAME"] = true;
	}
	else
	{
		return -1;
	}

	// description
	if ( wcslen( description ) != 0 )
	{
		params[L"PARAM_AP_DESCRIPTION"] = description;
		paramQuotes[L"PARAM_AP_DESCRIPTION"] = true;
	}
	else
	{
		params[L"PARAM_AP_DESCRIPTION"] = L"NULL";
		paramQuotes[L"PARAM_AP_DESCRIPTION"] = false;
	}

	// category
	wstringstream wss4;
	wss4 << category;

	params[L"PARAM_AP_CATEGORY"] = wss4.str();
	paramQuotes[L"PARAM_AP_CATEGORY"] = false;


	CCommand<CNoAccessor, CNoRowset> cmd;

	wstring command = tokenReplacerWithoutSingleQuotes( GUpdatePOIQuery, params, paramQuotes );

	HRESULT hr = cmd.Open( connection->m_session, command.c_str(), NULL, NULL, DBGUID_DBSQL, false );

	if( FAILED( hr ) )
	{
		retVal = -1;
	}

	if(retVal != -1)
	{
		PointsofInterestDBInsertPOILog(connection, POIId, L"Update");
	}

	cmd.Close();
	cmd.ReleaseCommand();

	return retVal;
}


// DELETE
DLL int PointsofInterestDBDeletePOI( CPointsofInterestDB* connection, unsigned int POIId )
{
	int retVal = 1;

	if( !connection )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	// POIId
	wstringstream wss1;
	wss1 << POIId;

	params[L"PARAM_AP_ID"] = wss1.str();
	paramQuotes[L"PARAM_AP_ID"] = false;

	CCommand<CNoAccessor, CNoRowset> cmd;

	wstring command = tokenReplacerWithoutSingleQuotes( GDeletePOIQuery, params, paramQuotes );

	HRESULT hr = cmd.Open( connection->m_session, command.c_str(), NULL, NULL, DBGUID_DBSQL, false );

	if( FAILED( hr ) )
	{
		retVal = -1;
	}

	if(retVal != -1)
	{
		PointsofInterestDBInsertPOILog(connection, POIId, L"Delete");
	}

	cmd.Close();
	cmd.ReleaseCommand();

	return retVal;
}


// SELECT
DLL SSelectedPoints* PointsofInterestDBSelectAllPoints( CPointsofInterestDB* connection, unsigned int levelId )
{
	SSelectedPoints* selectedPoints = NULL;

	if ( connection == NULL  )
	{
		return NULL;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	// coordinateY
	wstringstream wss1;
	wss1 << levelId;

	params[L"PARAM_LOCATION_ID"] = wss1.str();
	paramQuotes[L"PARAM_LOCATION_ID"] = false;

	wstring command = tokenReplacerWithoutSingleQuotes( GSelectAllPointsQuery, params, paramQuotes, true );

	CCommand< CDynamicAccessor > query;
	HRESULT queryResult = query.Open( connection->m_session, command.c_str() );
	if ( FAILED( queryResult ) == true )
	{
		return NULL;
	}

	// result parsing
	selectedPoints = new SSelectedPoints();

	unsigned int rowCount = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		rowCount += 1;
		queryResult = query.MoveNext();
	}

	// allocate memories
	selectedPoints->m_pointsCount		= rowCount;
	selectedPoints->m_ids				= new unsigned int [ selectedPoints->m_pointsCount ];
	selectedPoints->m_locationIds		= new unsigned int [ selectedPoints->m_pointsCount ];
	selectedPoints->m_names				= new WCHAR* [ selectedPoints->m_pointsCount ];
	selectedPoints->m_descriptions		= new WCHAR* [ selectedPoints->m_pointsCount ];
	selectedPoints->m_categories		= new unsigned int [ selectedPoints->m_pointsCount ];
	selectedPoints->m_coordinatesX		= new unsigned int [ selectedPoints->m_pointsCount ];
	selectedPoints->m_coordinatesY		= new unsigned int [ selectedPoints->m_pointsCount ];
	selectedPoints->m_snapToTerrain		= new bool [ selectedPoints->m_pointsCount ];
	selectedPoints->m_height			= new float [ selectedPoints->m_pointsCount ];

	unsigned int currentRow = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		selectedPoints->m_ids[ currentRow ]			= *(unsigned int*)query.GetValue(1);	// id
		selectedPoints->m_locationIds[ currentRow ] = *(unsigned int*)query.GetValue(2);	// location id

		// name
		WCHAR* name			= (WCHAR*) query.GetValue(3);
		size_t nameSize		= wcslen( name );
		selectedPoints->m_names[ currentRow ] = new WCHAR[ nameSize + 1 ];
		wcscpy_s( selectedPoints->m_names[ currentRow ], nameSize + 1, name );

		// description
		WCHAR* description			= (WCHAR*) query.GetValue(4);
		size_t descriptionSize		= wcslen( description );
		selectedPoints->m_descriptions[ currentRow ] = new WCHAR[ descriptionSize + 1 ];
		wcscpy_s( selectedPoints->m_descriptions[ currentRow ], descriptionSize + 1, description );

		selectedPoints->m_categories[ currentRow ] = *(unsigned int*)query.GetValue(5);	// category
		selectedPoints->m_coordinatesX[ currentRow ] = *(unsigned int*)query.GetValue(6);	// coordinate X
		selectedPoints->m_coordinatesY[ currentRow ] = *(unsigned int*)query.GetValue(7);	// coordinate Y

		DBSTATUS status;
		if ( !query.GetStatus( 8, &status ) || status == DBSTATUS_S_ISNULL )
		{
			selectedPoints->m_snapToTerrain[ currentRow ] = true;
		}
		else
		{
			selectedPoints->m_snapToTerrain[ currentRow ] = false;
			double height = *(double*)query.GetValue(8);
			selectedPoints->m_height[ currentRow ] = (float)height;
		}

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	query.Close();
	query.ReleaseCommand();

	return selectedPoints;
}

DLL SSelectedLevelsInfo* PointsofInterestDBSelectAllLevelesInfo( CPointsofInterestDB* connection )
{
	SSelectedLevelsInfo* selectedLevelsInfo = NULL;

	if ( connection == NULL  )
	{
		return NULL;
	}

	CCommand< CDynamicAccessor > query;
	HRESULT queryResult = query.Open( connection->m_session, GSelectAllLevelsInfoQuery );
	if ( FAILED( queryResult ) == true )
	{
		return NULL;
	}

	// result parsing
	selectedLevelsInfo = new SSelectedLevelsInfo();

	unsigned int rowCount = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		rowCount += 1;
		queryResult = query.MoveNext();
	}

	// allocate memories
	selectedLevelsInfo->m_levelCount	= rowCount;
	selectedLevelsInfo->m_ids			= new unsigned int [ selectedLevelsInfo->m_levelCount ];
	selectedLevelsInfo->m_tileCount		= new unsigned int [ selectedLevelsInfo->m_levelCount ];
	selectedLevelsInfo->m_tileSize		= new unsigned int [ selectedLevelsInfo->m_levelCount ];
	selectedLevelsInfo->m_levelNames	= new WCHAR* [ selectedLevelsInfo->m_levelCount ];

	unsigned int currentRow = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		selectedLevelsInfo->m_ids[ currentRow ]	= *(unsigned int*)query.GetValue(1);	// id

		// level name
		WCHAR* name			= (WCHAR*) query.GetValue(2);
		size_t nameSize		= wcslen( name );
		selectedLevelsInfo->m_levelNames[ currentRow ] = new WCHAR[ nameSize + 1 ];
		wcscpy_s( selectedLevelsInfo->m_levelNames[ currentRow ], nameSize + 1, name );

		selectedLevelsInfo->m_tileCount[ currentRow ]	= *(unsigned int*)query.GetValue(5);	// tileCount
		selectedLevelsInfo->m_tileSize[ currentRow ]	= *(unsigned int*)query.GetValue(6);	// tileSize

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	query.Close();
	query.ReleaseCommand();

	return selectedLevelsInfo;
}

DLL SSelectedCategories* PointsofInterestDBSelectAllCategories( CPointsofInterestDB* connection )
{
	SSelectedCategories* selectedCategories = NULL;

	if ( connection == NULL  )
	{
		return NULL;
	}

	CCommand< CDynamicAccessor > query;
	HRESULT queryResult = query.Open( connection->m_session, GSelectAllCategoriesQuery );
	if ( FAILED( queryResult ) == true )
	{
		return NULL;
	}

	// result parsing
	selectedCategories = new SSelectedCategories();

	unsigned int rowCount = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		rowCount += 1;
		queryResult = query.MoveNext();
	}

	// allocate memories
	selectedCategories->m_categoryCount	= rowCount;
	selectedCategories->m_ids			= new unsigned int [ selectedCategories->m_categoryCount ];
	selectedCategories->m_categoryNames	= new WCHAR* [ selectedCategories->m_categoryCount ];

	unsigned int currentRow = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		selectedCategories->m_ids[ currentRow ]	= *(unsigned int*)query.GetValue(1);	// id

		// level name
		WCHAR* name			= (WCHAR*) query.GetValue(2);
		size_t nameSize		= wcslen( name );
		selectedCategories->m_categoryNames[ currentRow ] = new WCHAR[ nameSize + 1 ];
		wcscpy_s( selectedCategories->m_categoryNames[ currentRow ], nameSize + 1, name );

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	query.Close();
	query.ReleaseCommand();

	return selectedCategories;
}


// FREE ALLOCATED MEMORIES
DLL int PointsofInterestDBFreeSelectedPoint( SSelectedPoints* points )
{
	if( points != NULL )
	{
		delete [] points->m_ids;
		delete [] points->m_locationIds;
		delete [] points->m_names;
		delete [] points->m_descriptions;
		delete [] points->m_categories;
		delete [] points->m_coordinatesX;
		delete [] points->m_coordinatesY;
		delete [] points->m_snapToTerrain;
		delete [] points->m_height;

		delete points;

		return 1;
	}

	return -1;
}

DLL int PointsofInterestDBFreeSelectedLevelesInfo( SSelectedLevelsInfo* levelsInfo )
{
	if( levelsInfo != NULL )
	{
		delete [] levelsInfo->m_ids;
		delete [] levelsInfo->m_tileCount;
		delete [] levelsInfo->m_tileSize;
		delete [] levelsInfo->m_levelNames;

		delete levelsInfo;

		return 1;
	}

	return -1;
}

DLL int PointsofInterestDBFreeSelectedCategories( SSelectedCategories* categories )
{
	if( categories != NULL )
	{
		delete [] categories->m_ids;
		delete [] categories->m_categoryNames;

		delete categories;

		return 1;
	}

	return -1;
}

