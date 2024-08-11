/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#define _HAS_EXCEPTIONS 0

#include "HelperFunctions.h"
#include "StickersDB.h"
#include "StickersSqlCommandsDefinitions.h"

class CStickersDB
{
public:
	CStickersDB()
	{
		m_connected = false;
		OleInitialize( 0 );
	}

	~CStickersDB()
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
CStickersDB GStickersConnectionDB;
#pragma endregion



// CONNECT AND DISCONNECT
DLL CStickersDB* StickersDBInit( const WCHAR* databaseAddress )
{
	if( GStickersConnectionDB.m_connected )
	{
		return NULL;	// only one connection at a time
	}

	HRESULT hr;

	CDBPropSet rgPropertySet[1] = {DBPROPSET_DBINIT};

	// default address: "CDPRS-MSSQL\\sqlexpress"
	rgPropertySet[0].AddProperty(DBPROP_INIT_DATASOURCE, databaseAddress );
	//rgPropertySet[0].AddProperty(DBPROP_INIT_DATASOURCE, L"192.168.100.11\\sqlexpress");
	rgPropertySet[0].AddProperty(DBPROP_INIT_CATALOG, L"EditorReview");
	rgPropertySet[0].AddProperty(DBPROP_AUTH_INTEGRATED, L"SSPI");

	CLSID clsid = {0xc7ff16cL,0x38e3,0x11d0,
	{0x97,0xab,0x0,0xc0,0x4f,0xc2,0xad,0x98}};

	hr = GStickersConnectionDB.m_dataSource.Open(clsid, rgPropertySet, 1);

	if( FAILED(hr) )
	{
		return NULL;
	}

	hr = GStickersConnectionDB.m_session.Open( GStickersConnectionDB.m_dataSource );

	if ( FAILED(hr) )
	{
		return NULL;
	}
	GStickersConnectionDB.m_connected = true;

	return &GStickersConnectionDB;
}

DLL void StickersDBClose( CStickersDB *conn )
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
DLL int StickersDBNextIDForSticker(CStickersDB* connection, unsigned int *id )
{
	int ret = 1;

	if( !connection )
	{
		return -1;
	}
	CCommand<CDynamicAccessor> rs;

	HRESULT hr = rs.Open( connection->m_session, GGetNextStickerIdQuery );

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
DLL int StickersDBInsertSticker(CStickersDB* connection, unsigned int stickerId, const WCHAR* mapName, const WCHAR* title, const WCHAR* description, unsigned int type, float x, float y, float z)
{
	int retVal = 1;

	if( !connection )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	// stickerId
	wstringstream wss0;
	wss0 << stickerId;

	params[L"PARAM_ID"] = wss0.str();
	paramQuotes[L"PARAM_ID"] = false;

	// mapName
	if ( wcslen( mapName ) != 0 )
	{
		params[L"PARAM_MAP_NAME"] = mapName;
		paramQuotes[L"PARAM_MAP_NAME"] = true;
	}
	else
	{
		return -1;
	}

	// title
	if ( wcslen( title ) != 0 )
	{
		params[L"PARAM_TITLE"] = title;
		paramQuotes[L"PARAM_TITLE"] = true;
	}
	else
	{
		return -1;
	}

	// description
	if ( wcslen( description ) != 0 )
	{
		params[L"PARAM_DESCRIPTION"] = description;
		paramQuotes[L"PARAM_DESCRIPTION"] = true;
	}
	else
	{
		params[L"PARAM_DESCRIPTION"] = L"NULL";
		paramQuotes[L"PARAM_DESCRIPTION"] = false;
	}

	// type
	wstringstream wss2;
	wss2 << type;

	params[L"PARAM_TYPE"] = wss2.str();
	paramQuotes[L"PARAM_TYPE"] = false;

	// x
	wstringstream wss3;
	wss3 << x;

	params[L"PARAM_X"] = wss3.str();
	paramQuotes[L"PARAM_X"] = false;

	// y
	wstringstream wss4;
	wss4 << y;

	params[L"PARAM_Y"] = wss4.str();
	paramQuotes[L"PARAM_Y"] = false;

	// z
	wstringstream wss5;
	wss5 << z;

	params[L"PARAM_Z"] = wss5.str();
	paramQuotes[L"PARAM_Z"] = false;

	CCommand<CNoAccessor, CNoRowset> cmd;

	wstring command = tokenReplacerWithoutSingleQuotes( GInsertStickerQuery, params, paramQuotes, true );

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
DLL int StickersDBUpdateSticker(CStickersDB* connection, unsigned int stickerId, const WCHAR* mapName, const WCHAR* title, const WCHAR* description, unsigned int type, float x, float y, float z, bool removed)
{
	int retVal = 1;

	if( !connection )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	// stickerId
	wstringstream wss0;
	wss0 << stickerId;

	params[L"PARAM_ID"] = wss0.str();
	paramQuotes[L"PARAM_ID"] = false;

	// mapName
	if ( wcslen( mapName ) != 0 )
	{
		params[L"PARAM_MAP_NAME"] = mapName;
		paramQuotes[L"PARAM_MAP_NAME"] = true;
	}
	else
	{
		return -1;
	}

	// mapName
	if ( wcslen( title ) != 0 )
	{
		params[L"PARAM_TITLE"] = title;
		paramQuotes[L"PARAM_TITLE"] = true;
	}
	else
	{
		return -1;
	}

	// description
	if ( wcslen( description ) != 0 )
	{
		params[L"PARAM_DESCRIPTION"] = description;
		paramQuotes[L"PARAM_DESCRIPTION"] = true;
	}
	else
	{
		params[L"PARAM_DESCRIPTION"] = L"NULL";
		paramQuotes[L"PARAM_DESCRIPTION"] = false;
	}

	// type
	wstringstream wss2;
	wss2 << type;

	params[L"PARAM_TYPE"] = wss2.str();
	paramQuotes[L"PARAM_TYPE"] = false;

	// x
	wstringstream wss3;
	wss3 << x;

	params[L"PARAM_X"] = wss3.str();
	paramQuotes[L"PARAM_X"] = false;

	// y
	wstringstream wss4;
	wss4 << y;

	params[L"PARAM_Y"] = wss4.str();
	paramQuotes[L"PARAM_Y"] = false;

	// z
	wstringstream wss5;
	wss5 << z;

	params[L"PARAM_Z"] = wss5.str();
	paramQuotes[L"PARAM_Z"] = false;

	// removed
	params[L"PARAM_REMOVED"] = (removed == true) ? L"1" : L"0";
	paramQuotes[L"PARAM_REMOVED"] = false;


	CCommand<CNoAccessor, CNoRowset> cmd;

	wstring command = tokenReplacerWithoutSingleQuotes( GUpdateStickerQuery, params, paramQuotes );

	HRESULT hr = cmd.Open( connection->m_session, command.c_str(), NULL, NULL, DBGUID_DBSQL, false );

	if( FAILED( hr ) )
	{
		retVal = -1;
	}

	cmd.Close();
	cmd.ReleaseCommand();

	return retVal;
}


// DELETE
DLL int StickersDBDeleteSticker( CStickersDB* connection, unsigned int stickerId )
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
	wss1 << stickerId;

	params[L"PARAM_ID"] = wss1.str();
	paramQuotes[L"PARAM_ID"] = false;

	CCommand<CNoAccessor, CNoRowset> cmd;

	wstring command = tokenReplacerWithoutSingleQuotes( GDeleteStickerQuery, params, paramQuotes );

	HRESULT hr = cmd.Open( connection->m_session, command.c_str(), NULL, NULL, DBGUID_DBSQL, false );

	if( FAILED( hr ) )
	{
		retVal = -1;
	}

	cmd.Close();
	cmd.ReleaseCommand();

	return retVal;
}


// SELECT
DLL SSelectedStickers* StickersDBSelectAllStickers( CStickersDB* connection, const WCHAR* mapName )
{
	SSelectedStickers* selectedStickers = NULL;

	if ( connection == NULL  )
	{
		return NULL;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	// mapName
	if ( wcslen( mapName ) != 0 )
	{
		params[L"PARAM_MAP_NAME"] = mapName;
		paramQuotes[L"PARAM_MAP_NAME"] = true;
	}
	else
	{
		return NULL;
	}

	wstring command = tokenReplacerWithoutSingleQuotes( GSelectAllStickersQuery, params, paramQuotes, true );

	CCommand< CDynamicAccessor > query;
	HRESULT queryResult = query.Open( connection->m_session, command.c_str() );
	if ( FAILED( queryResult ) == true )
	{
		return NULL;
	}

	// result parsing
	selectedStickers = new SSelectedStickers();

	unsigned int rowCount = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		rowCount += 1;
		queryResult = query.MoveNext();
	}

	// allocate memories
	selectedStickers->m_pointsCount		= rowCount;
	selectedStickers->m_ids				= new unsigned int [ selectedStickers->m_pointsCount ];
	selectedStickers->m_mapNames		= new WCHAR* [ selectedStickers->m_pointsCount ];
	selectedStickers->m_titles			= new WCHAR* [ selectedStickers->m_pointsCount ];
	selectedStickers->m_descriptions	= new WCHAR* [ selectedStickers->m_pointsCount ];
	selectedStickers->m_type			= new unsigned int [ selectedStickers->m_pointsCount ];
	selectedStickers->m_x				= new double [ selectedStickers->m_pointsCount ];
	selectedStickers->m_y				= new double [ selectedStickers->m_pointsCount ];
	selectedStickers->m_z				= new double [ selectedStickers->m_pointsCount ];
	selectedStickers->m_removed			= new bool [ selectedStickers->m_pointsCount ];

	unsigned int currentRow = 0;

	queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		// id
		selectedStickers->m_ids[ currentRow ]	= *(unsigned int*)query.GetValue(1);

		// m_mapNames
		WCHAR* mapNames			= (WCHAR*) query.GetValue(2);
		size_t mapNamesSize		= wcslen( mapNames );
		selectedStickers->m_mapNames[ currentRow ] = new WCHAR[ mapNamesSize + 1 ];
		wcscpy_s( selectedStickers->m_mapNames[ currentRow ], mapNamesSize + 1, mapNames );

		// type
		selectedStickers->m_type[ currentRow ]	= *(unsigned int*)query.GetValue(3);

		// title
		WCHAR* title			= (WCHAR*) query.GetValue(4);
		size_t titleSize		= wcslen( title );
		selectedStickers->m_titles[ currentRow ] = new WCHAR[ titleSize + 1 ];
		wcscpy_s( selectedStickers->m_titles[ currentRow ], titleSize + 1, title );

		// description
		WCHAR* description			= (WCHAR*) query.GetValue(5);
		size_t descriptionSize		= wcslen( description );
		selectedStickers->m_descriptions[ currentRow ] = new WCHAR[ descriptionSize + 1 ];
		wcscpy_s( selectedStickers->m_descriptions[ currentRow ], descriptionSize + 1, description );

		// x
		selectedStickers->m_x[ currentRow ]	= *(double*)query.GetValue(6);

		// y
		selectedStickers->m_y[ currentRow ]	= *(double*)query.GetValue(7);

		// z
		selectedStickers->m_z[ currentRow ]	= *(double*)query.GetValue(8);

		// removed
		selectedStickers->m_removed[ currentRow ]	= *(bool*)query.GetValue(9);

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	query.Close();
	query.ReleaseCommand();

	return selectedStickers;
}

DLL SSelectedStickerCategories* StickersDBSelectAllCategories( CStickersDB* connection )
{
	SSelectedStickerCategories* selectedCategories = NULL;

	if ( connection == NULL  )
	{
		return NULL;
	}

	CCommand< CDynamicAccessor > query;
	HRESULT queryResult = query.Open( connection->m_session, GSelectAllStickerCategoriesQuery );
	if ( FAILED( queryResult ) == true )
	{
		return NULL;
	}

	// result parsing
	selectedCategories = new SSelectedStickerCategories();

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

		// category name
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
DLL int StickersDBFreeSelectedPoint( SSelectedStickers* stickers )
{
	if( stickers != NULL )
	{
		delete [] stickers->m_ids;
		delete [] stickers->m_mapNames;
		delete [] stickers->m_type;
		delete [] stickers->m_titles;
		delete [] stickers->m_descriptions;
		delete [] stickers->m_x;
		delete [] stickers->m_y;
		delete [] stickers->m_z;
		delete [] stickers->m_removed;

		delete stickers;

		return 1;
	}

	return -1;
}

DLL int StickersDBFreeSelectedCategories( SSelectedStickerCategories* categories )
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

