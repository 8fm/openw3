/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#define _HAS_EXCEPTIONS 0

#include "HelperFunctions.h"
#include "ReviewFlagsDB.h"
#include "ReviewFlagsSqlCommandsDefinitions.h"


#pragma region GLOBAL CONNECTION OBJECT
// This shouldn't be made global, but is a hack #1 to ensure proper dll unloading order
CReviewDBConnection GReviewFlagConnectionDB;
#pragma endregion

// CONNECT AND DISCONNECT
DLL SDBInitInfo ReviewDBInit( const WCHAR* databaseAddress, const WCHAR* sqlUser, const WCHAR* sqlPassword )
{
	SDBInitInfo info;
	info.m_connection = NULL;
	info.m_info = NULL;

	if( GReviewFlagConnectionDB.m_connected )
	{
		info.m_info = new WCHAR[30];
		size_t size = 0;
		mbstowcs_s( &size, info.m_info, 30, "Only one connection allowed!", _TRUNCATE );
		return info;	// only one connection at a time
	}

	HRESULT hr;

	CDBPropSet rgPropertySet[1] = {DBPROPSET_DBINIT};

	// default address: "CDPRS-MSSQL\\sqlexpress"
	rgPropertySet[0].AddProperty(DBPROP_INIT_DATASOURCE, databaseAddress );
	rgPropertySet[0].AddProperty(DBPROP_INIT_CATALOG, L"EditorReview");
	if( wcslen( sqlUser ) == 0 )
	{
		rgPropertySet[0].AddProperty(DBPROP_AUTH_INTEGRATED, L"SSPI");
	}
	else
	{
		rgPropertySet[0].AddProperty(DBPROP_AUTH_USERID, sqlUser);
		rgPropertySet[0].AddProperty(DBPROP_AUTH_PASSWORD, sqlPassword);
		rgPropertySet[0].AddProperty(DBPROP_AUTH_ENCRYPT_PASSWORD, VARIANT_TRUE);
	}

	CLSID clsid = {0xc7ff16cL,0x38e3,0x11d0,
	{0x97,0xab,0x0,0xc0,0x4f,0xc2,0xad,0x98}};

	hr = GReviewFlagConnectionDB.m_dataSource.Open(clsid, rgPropertySet, 1);

	if( SUCCEEDED (hr) )
	{
		hr = GReviewFlagConnectionDB.m_session.Open( GReviewFlagConnectionDB.m_dataSource );
		if ( SUCCEEDED (hr) )
		{
			GReviewFlagConnectionDB.m_connected = true;
			info.m_connection = &GReviewFlagConnectionDB;
			return info;
		}
	}

	//failed - get error message
	IErrorInfo* pErrInfo;
	hr = ::GetErrorInfo(0, &pErrInfo);
	if(SUCCEEDED(hr))
	{
		BSTR bstrErrDescription;
		pErrInfo->GetDescription(&bstrErrDescription);
		UINT length = SysStringLen( bstrErrDescription );

		info.m_info = new WCHAR[length + 1];
		wcscpy_s( info.m_info, length + 1, bstrErrDescription );

		pErrInfo->Release();
		::SysFreeString(bstrErrDescription);
	}
	return info;
}

DLL void ReviewDBClose( CReviewDBConnection *conn )
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
DLL int ReviewDBNextIDForFlag( CReviewDBConnection* connection, unsigned int *id )
{
	int retVal = PerformIdSelectCommand( GGetNextFlagIdQuery, connection, id );
	if( *id == 0 )
	{
		// select didn't return any rows - we're going to insert first element
		(*id) = 1;
	}
	return retVal;
}

DLL int ReviewDBNextIDForVector( CReviewDBConnection* connection, unsigned int *id )
{
	int retVal = PerformIdSelectCommand( GGetNextVectorIdQuery, connection, id );
	if( retVal == 0 )
	{
		// select didn't return any rows - we're going to insert first element
		(*id) = 1;
	}
	return retVal;
}

// GET TT PROJECT ID
DLL int ReviewDBGetTTProjectID( CReviewDBConnection* connection, const WCHAR* ttProjectName, unsigned int* id )
{
	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	// project name
	if ( AppendParameter( params, paramQuotes, L"PARAM_NAME", ttProjectName ) < 0 )
	{
		return -1;
	}

	wstring command = tokenReplacer( GGetTTProjectIdQuery, params, paramQuotes, true );
	return PerformIdSelectCommand( command.c_str(), connection, id );
}

// INSERT
DLL int ReviewDBInsertFlag( CReviewDBConnection* connection, unsigned int flagId, unsigned int testTrackId, unsigned int flagType, const WCHAR* summary, const WCHAR* mapName, const WCHAR* linkToVideo, const WCHAR* ttProject )
{
	if( !connection )
	{
		return -1;
	}

	// TTProject
	unsigned int ttProjectId;
	// try reading project id to check if it exists in the database
	if ( ReviewDBGetTTProjectID( connection, ttProject, &ttProjectId ) < 0 )
	{
		return -1;
	}

	// if there is no project with name ttProject in our database, insert new project
	if( ttProjectId == 0 )
	{
		if ( ReviewDBInsertTTProject( connection, ttProject ) < 0 )
		{
			return -1;
		}
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	AppendParameter( params, paramQuotes, L"PARAM_FLAG_ID", flagId );				// flagId
	AppendParameter( params, paramQuotes, L"PARAM_TESTTRACK_ID", testTrackId );		// testTrackId
	AppendParameter( params, paramQuotes, L"PARAM_TYPE", flagType );				// flagType

	if ( AppendParameter( params, paramQuotes, L"PARAM_SUMMARY", summary ) < 0 
		|| AppendParameter( params, paramQuotes, L"PARAM_MAP_NAME", mapName ) < 0
		|| AppendParameter( params, paramQuotes, L"PARAM_LINK_TO_VIDEO", linkToVideo ) < 0
		|| AppendParameter( params, paramQuotes, L"PARAM_TTPROJECT", ttProject ) < 0 )
	{
		return -1;
	}

	wstring command = tokenReplacer( GInsertFlagQuery, params, paramQuotes, true );
	return PerformUpdateCommand( command.c_str(), connection );
}

DLL int ReviewDBInsertComment( CReviewDBConnection *connection, unsigned int flagId, const WCHAR* author, const WCHAR* description, unsigned int priority, SVec3 cameraPosition, SVec3 cameraOrientation, SVec3 flagPosition, const WCHAR* pathToScreen, unsigned int state )
{
	if( !connection )
	{
		return -1;
	}

	// add vectors to table VECTORS
	unsigned int camPosId, camOrnId, flagPosId;

	// add flag position
	if(ReviewDBNextIDForVector(connection, &flagPosId) < 0)
	{
		return -1;
	}
	if(ReviewDBInsertVector(connection, flagPosId, flagPosition) < 0)
	{
		return -1;
	}

	// add camera position
	if(ReviewDBNextIDForVector(connection, &camPosId) < 0)
	{
		return -1;
	}
	if(ReviewDBInsertVector(connection, camPosId, cameraPosition) < 0)
	{
		return -1;
	}

	// add camera orientation
	if(ReviewDBNextIDForVector(connection, &camOrnId) < 0)
	{
		return -1;
	}
	if(ReviewDBInsertVector(connection, camOrnId, cameraOrientation) < 0)
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	if( AppendParameter( params, paramQuotes, L"PARAM_AUTHOR", author ) < 0
		|| AppendParameter( params, paramQuotes, L"PARAM_COMMENT", description ) < 0 
		|| AppendParameter( params, paramQuotes, L"PARAM_SCREEN", pathToScreen ) < 0 )
	{
		return -1;
	}
	
	AppendParameter( params, paramQuotes, L"PARAM_FLAG_ID", flagId );				// flagId
	AppendParameter( params, paramQuotes, L"PARAM_STATE", state );					// state
	AppendParameter( params, paramQuotes, L"PARAM_PRIORITY", priority );			// priority
	AppendParameter( params, paramQuotes, L"PARAM_CAMERA_POSITION", camPosId );		// cameraPosition
	AppendParameter( params, paramQuotes, L"PARAM_CAMERA_ORIENTATION", camOrnId );	// cameraOrientation
	AppendParameter( params, paramQuotes, L"PARAM_FLAG_POSITION", flagPosId );		// flagPosition

	wstring command = tokenReplacer( GInsertCommentQuery, params, paramQuotes, true );
	return PerformUpdateCommand( command.c_str(), connection );
}

DLL int ReviewDBInsertVector( CReviewDBConnection* connection, unsigned int vectorId, SVec3 vector )
{
	if( !connection )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	AppendParameter( params, paramQuotes, L"PARAM_VECTOR_ID", vectorId );	// vectorId
	AppendParameter( params, paramQuotes, L"PARAM_VECTOR_X", vector.m_x );	// vectorX
	AppendParameter( params, paramQuotes, L"PARAM_VECTOR_Y", vector.m_y );	// vectorY
	AppendParameter( params, paramQuotes, L"PARAM_VECTOR_Z", vector.m_z );	// vectorZ

	wstring command = tokenReplacer( GInsertVectorQuery, params, paramQuotes, true );
	return PerformUpdateCommand( command.c_str(), connection );
}

DLL int ReviewDBInsertTTProject(CReviewDBConnection* connection, const WCHAR* ttProjectName)
{
	if( !connection )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	if ( AppendParameter( params, paramQuotes, L"PARAM_NAME", ttProjectName ) < 0 )
	{
		return -1;
	}

	wstring command = tokenReplacer( GInsertTTProjectQuery, params, paramQuotes, true );
	return PerformUpdateCommand( command.c_str(), connection );
}


// UPDATE
DLL int ReviewDBUpdateFlag( CReviewDBConnection* connection, unsigned int flagId )
{
	if( !connection )
	{
		return -1;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;
	AppendParameter( params, paramQuotes, L"PARAM_FLAG_ID", flagId );

	wstring command = tokenReplacer( GUpdateFlagQuery, params, paramQuotes );
	return PerformUpdateCommand( command.c_str(), connection );
}


// SELECT
DLL SSelectedFlags* ReviewDBSelectAllFlags( CReviewDBConnection* connection, const WCHAR* mapName, const WCHAR* ttProject )
{
	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	if ( AppendParameter( params, paramQuotes, L"PARAM_MAP_NAME", mapName ) < 0 
		|| AppendParameter( params, paramQuotes, L"PARAM_TTPROJECT", ttProject ) < 0 )
	{
		return NULL;
	}

	wstring command = tokenReplacer( GSelectAllFlags, params, paramQuotes, true );
	return ReviewDBSelectFlags( connection, command.c_str() );
}

DLL SSelectedFlags* ReviewDBSelectFlagsWithoutClosed( CReviewDBConnection* connection, const WCHAR* mapName, const WCHAR* ttProject )
{
	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;

	if ( AppendParameter( params, paramQuotes, L"PARAM_MAP_NAME", mapName ) < 0 
		|| AppendParameter( params, paramQuotes, L"PARAM_TTPROJECT", ttProject ) < 0 )
	{
		return NULL;
	}

	wstring command = tokenReplacer( GSelectFlagsWithoutClosed, params, paramQuotes, true );
	return ReviewDBSelectFlags( connection, command.c_str() );
}

SSelectedFlags* ReviewDBSelectFlags( CReviewDBConnection* connection, const WCHAR* command )
{
	if ( connection == NULL  )
	{
		return NULL;
	}

	CCommand< CDynamicAccessor > query;
	unsigned int rowCount = 0;
	if ( PerformSelectCommand( command, connection, query, rowCount ) < 0 )
	{
		return NULL;
	}

	// result parsing
	SSelectedFlags* selectedFlags = new SSelectedFlags( rowCount );
	unsigned int currentRow = 0;

	HRESULT queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		selectedFlags->m_ids[ currentRow ]			= *(unsigned int*)query.GetValue(1);				// id
		selectedFlags->m_testTrackIds[ currentRow ] = *(unsigned int*)query.GetValue(2);				// test track id
		selectedFlags->m_types[ currentRow ]		= *(unsigned int*)query.GetValue(3);				// flag type
		
			// flag summary
		WCHAR* summary			= (WCHAR*) query.GetValue(4);
		size_t summarySize		= wcslen( summary );
		selectedFlags->m_summaries[ currentRow ] = new WCHAR[ summarySize + 1 ];
		wcscpy_s( selectedFlags->m_summaries[ currentRow ], summarySize + 1, summary );

		// map name
		WCHAR* mapName			= (WCHAR*) query.GetValue(5);
		size_t mapNameSize		= wcslen( mapName );
		selectedFlags->m_mapNames[ currentRow ] = new WCHAR[ mapNameSize + 1 ];
		wcscpy_s( selectedFlags->m_mapNames[ currentRow ], mapNameSize + 1, mapName );

		// link to video
		WCHAR* linkToVideo			= (WCHAR*) query.GetValue(6);
		size_t linkToVideoSize		= wcslen( linkToVideo );
		selectedFlags->m_linksToVideos[ currentRow ] = new WCHAR[ linkToVideoSize + 1 ];
		wcscpy_s( selectedFlags->m_linksToVideos[ currentRow ], linkToVideoSize + 1, linkToVideo );

		// last update
		// this is a hack, because sometime we can have two zero as minutes or seconds
		// functions wcslen and wcscpy_s interpret zero as end of text and don't copy rest 
		WCHAR* lastUpdate			= (WCHAR*) query.GetValue(7);
		size_t lastUpdateSize		= sizeof( WCHAR ) * 9;
		selectedFlags->m_lastUpdate[ currentRow ] = (WCHAR*)malloc(lastUpdateSize);
		memcpy(selectedFlags->m_lastUpdate[ currentRow ], lastUpdate, lastUpdateSize);

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	query.Close();
	query.ReleaseCommand();

	return selectedFlags;
}

DLL SSelectedComments* ReviewDBSelectCommentForFlag( CReviewDBConnection* connection, unsigned int flagId )
{
	return ReviewDBSelectComment( connection, GSelectAllCommentsForFlag, flagId );
}

DLL SSelectedComments* ReviewDBSelectNewestCommentForFlag( CReviewDBConnection* connection, unsigned int flagId )
{
	return ReviewDBSelectComment( connection, GSelectNewestCommentsForFlag, flagId );
}

SSelectedComments* ReviewDBSelectComment( CReviewDBConnection* connection, const WCHAR* command, unsigned int flagId, const WCHAR* editorLastUpdate )
{
	if ( connection == NULL  )
	{
		return NULL;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;
	AppendParameter( params, paramQuotes, L"PARAM_FLAG_ID", flagId );
	if ( editorLastUpdate != NULL && AppendParameter( params, paramQuotes, L"PARAM_LAST_UPDATE", editorLastUpdate ) < 0 )
	{
		return NULL;
	}

	wstring queryText = tokenReplacer( command, params, paramQuotes, true );

	CCommand< CDynamicAccessor > query;
	unsigned int rowCount = 0;
	if ( PerformSelectCommand( queryText.c_str(), connection, query, rowCount ) < 0 )
	{
		return NULL;
	}

	// result parsing
	SSelectedComments* selectedComments = new SSelectedComments( rowCount );

	// ids for vectors
	unsigned int* camPos = new unsigned int[rowCount];
	unsigned int* camOrn = new unsigned int[rowCount];
	unsigned int* flagPos = new unsigned int[rowCount];

	unsigned int currentRow = 0;


	HRESULT queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		selectedComments->m_ids[ currentRow ]		= *(unsigned int*)query.GetValue(1);			// ids
		selectedComments->m_flagId[ currentRow ]	= *(unsigned int*)query.GetValue(2);			// flag id

		// comment author
		WCHAR* author			= (WCHAR*) query.GetValue(3);
		size_t authorSize		= wcslen( author );
		selectedComments->m_author[ currentRow ] = new WCHAR[ authorSize + 1 ];
		wcscpy_s( selectedComments->m_author[ currentRow ], authorSize + 1, author );

		// comment creation date
		// this is a hack, because sometime we can have two zero as minutes or seconds
		// functions wcslen and wcscpy_s interpret zero as end of text and don't copy rest 
		WCHAR* creationDate			= (WCHAR*) query.GetValue(4);
		size_t creationDateSize		= sizeof( WCHAR ) * 9;
		selectedComments->m_creationDate[ currentRow ] = (WCHAR*)malloc(creationDateSize);
		memcpy(selectedComments->m_creationDate[ currentRow ], creationDate, creationDateSize);

		selectedComments->m_state[ currentRow ]		= *(unsigned int*)query.GetValue(5);			// state
		selectedComments->m_priority[ currentRow ]	= *(unsigned int*)query.GetValue(6);			// priority
		camPos[currentRow]	= *(unsigned int*)query.GetValue(7);									// camera position
		camOrn[currentRow]	= *(unsigned int*)query.GetValue(8);									// camera orientation
		flagPos[currentRow]	= *(unsigned int*)query.GetValue(9);									// flag position
		
		// screen
		WCHAR* screen			= (WCHAR*) query.GetValue(10);
		size_t screenSize		= wcslen( screen );
		selectedComments->m_screen[ currentRow ] = new WCHAR[ screenSize + 1 ];
		wcscpy_s( selectedComments->m_screen[ currentRow ], screenSize + 1, screen );

		// comment
		WCHAR* comment			= (WCHAR*) query.GetValue(11);
		size_t commentSize		= wcslen( comment );
		selectedComments->m_comment[ currentRow ] = new WCHAR[ commentSize + 1 ];
		wcscpy_s( selectedComments->m_comment[ currentRow ], commentSize + 1, comment );

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	// get vectors
	for(unsigned int i=0; i<rowCount; ++i)
	{
		SVec3* camPosVec = ReviewDBSelectVector(connection, camPos[i]);
		selectedComments->m_cameraPositions[ i ] = *camPosVec;
		ReviewDBFreeSelectFlagStates(camPosVec);

		SVec3* camOrnVec = ReviewDBSelectVector(connection, camOrn[i]);
		selectedComments->m_cameraOrientations[ i ] = *camOrnVec;
		ReviewDBFreeSelectFlagStates(camOrnVec);

		SVec3* flagPosVec = ReviewDBSelectVector(connection, flagPos[i]);
		selectedComments->m_flagPositions[ i ] = *flagPosVec;
		ReviewDBFreeSelectFlagStates(flagPosVec);
	}

	delete[] camPos;
	delete[] camOrn;
	delete[] flagPos;

	query.Close();
	query.ReleaseCommand();

	return selectedComments;
}

DLL SSelectedTypes* ReviewDBSelectFlagTypes( CReviewDBConnection* connection )
{
	if ( connection == NULL  )
	{
		return NULL;
	}

	CCommand< CDynamicAccessor > query;
	unsigned int rowCount = 0;
	if ( PerformSelectCommand( GSelectFlagTypes, connection, query, rowCount ) < 0 )
	{
		return NULL;
	}

	// result parsing
	SSelectedTypes* selectedTypes = new SSelectedTypes( rowCount );

	unsigned int currentRow = 0;
	HRESULT queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		selectedTypes->m_ids[ currentRow ] = *(unsigned int*)query.GetValue(1);					// ids
		
		// type name
		WCHAR* typeName			= (WCHAR*) query.GetValue(2);
		size_t typeNameSize		= wcslen( typeName );
		selectedTypes->m_type[ currentRow ] = new WCHAR[ typeNameSize + 1 ];
		wcscpy_s( selectedTypes->m_type[ currentRow ], typeNameSize + 1, typeName );

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	query.Close();
	query.ReleaseCommand();

	return selectedTypes;
}

DLL SSelectedStates* ReviewDBSelectFlagStates( CReviewDBConnection* connection )
{
	if ( connection == NULL  )
	{
		return NULL;
	}

	CCommand< CDynamicAccessor > query;
	unsigned int rowCount = 0;
	if ( PerformSelectCommand( GSelectFlagStates, connection, query, rowCount ) < 0 )
	{
		return NULL;
	}

	// result parsing
	SSelectedStates* selectedStates = new SSelectedStates( rowCount );

	unsigned int currentRow = 0;
	HRESULT queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		selectedStates->m_ids[ currentRow ]	= *(unsigned int*)query.GetValue(1);				// ids

		// state name
		WCHAR* stateName			= (WCHAR*) query.GetValue(2);
		size_t stateNameSize		= wcslen( stateName );
		selectedStates->m_state[ currentRow ] = new WCHAR[ stateNameSize + 1 ];
		wcscpy_s( selectedStates->m_state[ currentRow ], stateNameSize + 1, stateName );

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	query.Close();
	query.ReleaseCommand();

	return selectedStates;
}

DLL SVec3* ReviewDBSelectVector( CReviewDBConnection* connection, unsigned int vectorId )
{
	if ( connection == NULL  )
	{
		return NULL;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;
	AppendParameter( params, paramQuotes, L"PARAM_VECTOR_ID", vectorId );

	wstring command = tokenReplacer( GSelectVector, params, paramQuotes, true );

	CCommand< CDynamicAccessor > query;
	unsigned int rowCount = 0;
	if ( PerformSelectCommand( command.c_str(), connection, query, rowCount ) < 0 )
	{
		return NULL;
	}

	// result parsing
	SVec3* selectedVector = new SVec3();

	HRESULT queryResult = query.MoveFirst();
	if( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		query.GetValue(L"VECTOR_X", &selectedVector->m_x);
		query.GetValue(L"VECTOR_Y", &selectedVector->m_y);
		query.GetValue(L"VECTOR_Z", &selectedVector->m_z);
	}

	query.Close();
	query.ReleaseCommand();

	return selectedVector;
}

DLL SIndexFlagToUpdate* ReviewDBSelectIndexFlagToUpdate( CReviewDBConnection* connection, const WCHAR* editorLastUpdate )
{
	if ( connection == NULL  )
	{
		return NULL;
	}

	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;
	if ( AppendParameter( params, paramQuotes, L"PARAM_LAST_UPDATE", editorLastUpdate ) < 0 )
	{
		return NULL;
	}

	wstring command = tokenReplacer( GSelectIndexFlagToUpdate, params, paramQuotes, true );

	CCommand< CDynamicAccessor > query;
	unsigned int rowCount = 0;
	if ( PerformSelectCommand( command.c_str(), connection, query, rowCount ) < 0 )
	{
		return NULL;
	}

	// result parsing
	SIndexFlagToUpdate* indexFlagToUpdate = new SIndexFlagToUpdate( rowCount );

	unsigned int currentRow = 0;
	HRESULT queryResult = query.MoveFirst();
	while ( SUCCEEDED( queryResult ) && queryResult != DB_S_ENDOFROWSET )
	{
		indexFlagToUpdate->m_ids[ currentRow ]	= *(unsigned int*)query.GetValue(1);	// ids

		currentRow += 1;
		queryResult = query.MoveNext();
	}

	return indexFlagToUpdate;
}

DLL SSelectedComments* ReviewDBSelectNewCommentsForUpdatedFlag( CReviewDBConnection* connection, unsigned int flagId , const WCHAR* editorLastUpdate )
{
	return ReviewDBSelectComment( connection, GSelectNewCommentsForUpdatedFlag, flagId, editorLastUpdate );
}

DLL SSelectedFlags* ReviewDBSelectFlag( CReviewDBConnection* connection, unsigned int flagId )
{
	std::map< wstring, wstring > params;
	std::map< wstring, bool > paramQuotes;
	AppendParameter( params, paramQuotes, L"PARAM_FLAG_ID", flagId );
	
	wstring command = tokenReplacer( GSelectFlag, params, paramQuotes, true );
	return ReviewDBSelectFlags( connection, command.c_str() );
}



// FREE ALLOCATED MEMORIES
DLL int ReviewDBFreeSeletedFlags( SSelectedFlags* flags )
{
	if( flags != NULL )
	{
		if ( flags->m_flagCount > 0 )
		{
			delete flags->m_ids;
			delete flags->m_testTrackIds;
			delete flags->m_types;

			delete [] flags->m_summaries;
			delete [] flags->m_descriptions;
			delete [] flags->m_mapNames;
			delete [] flags->m_linksToVideos;
			delete [] flags->m_lastUpdate;
		}

		delete flags;

		return 1;
	}

	return -1;
}

DLL int ReviewDBFreeSelectedComments( SSelectedComments* comments )
{
	if( comments != NULL )
	{
		if ( comments->m_commentCount > 0 )
		{
			delete comments->m_ids;
			delete comments->m_flagId;
			delete comments->m_state;
			delete comments->m_priority;

			delete [] comments->m_author;
			delete [] comments->m_creationDate;
			delete [] comments->m_cameraPositions;
			delete [] comments->m_cameraOrientations;
			delete [] comments->m_comment;
			delete [] comments->m_flagPositions;
			delete [] comments->m_screen;
		}

		delete comments;

		return 1;
	}

	return -1;
}

DLL int ReviewDBFreeSelectFlagTypes( SSelectedTypes* types )
{
	if( types != NULL)
	{
		if ( types->m_typeCount > 0 )
		{
			delete types->m_ids;
			delete[] types->m_type;
		}
		delete types;
		
		return 1;
	}

	return -1;
}

DLL int ReviewDBFreeSelectFlagStates( SSelectedStates* states )
{
	if( states != NULL)
	{
		if ( states->m_stateCount > 0 )
		{
			delete states->m_ids;
			delete[] states->m_state;
		}
		delete states;
		
		return 1;
	}

	return -1;
}

DLL int ReviewDBFreeSelectFlagStates( SVec3* vector )
{
	if( vector != NULL)
	{
		delete vector;
		
		return 1;
	}

	return -1;
}

DLL int ReviewDBFreeSelectIndexFlagToUpdates( SIndexFlagToUpdate* indexes )
{
	if( indexes != NULL)
	{
		if ( indexes->m_flagCount > 0 )
		{
			delete[] indexes->m_ids;
		}
		delete indexes;
		
		return 1;
	}

	return -1;
}

DLL int ReviewDBFreeInfoErrorMessage( SDBInitInfo& errInfo )
{
	if ( errInfo.m_info != NULL )
	{
		delete[] errInfo.m_info;

		return 1;
	}

	return -1;
}
