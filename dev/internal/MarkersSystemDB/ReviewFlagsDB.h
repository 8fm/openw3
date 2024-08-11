/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifdef REVIEW_SYSTEM_DB_EXPORTS 
#define DLL __declspec(dllexport)
#else
#define DLL __declspec(dllimport)
#endif

class CReviewDBConnection;

struct SVec3
{
public:
	SVec3() : m_x(0.0f), m_y(0.0f), m_z(0.0f) { /*intentionally empty*/ };
	SVec3(const SVec3& v) : m_x(v.m_x), m_y(v.m_y), m_z(v.m_z) { /*intentionally empty*/ };
	SVec3(double x, double y, double z) : m_x(x), m_y(y), m_z(z) { /*intentionally empty*/ };

	double m_x;
	double m_y;
	double m_z;
};

struct SSelectedFlags
{
public:
	SSelectedFlags( unsigned int size ) : m_flagCount( size )
	{
		if ( m_flagCount > 0 )
		{
			m_ids				= new unsigned int [ m_flagCount ];
			m_testTrackIds		= new unsigned int [ m_flagCount ];
			m_types				= new unsigned int [ m_flagCount ];
			m_summaries			= new WCHAR* [ m_flagCount ];
			m_descriptions		= new WCHAR* [ m_flagCount ];
			m_mapNames			= new WCHAR* [ m_flagCount ];
			m_linksToVideos		= new WCHAR* [ m_flagCount ];
			m_lastUpdate		= new WCHAR* [ m_flagCount ];
		}
	}

	unsigned int* m_ids;
	unsigned int* m_testTrackIds;	
	unsigned int* m_types;	
	WCHAR** m_summaries;
	WCHAR** m_descriptions;
	WCHAR** m_mapNames;
	WCHAR** m_linksToVideos;
	WCHAR** m_lastUpdate;

	size_t	m_flagCount;
};

struct SSelectedComments
{
public:
	SSelectedComments( unsigned int size ) : m_commentCount( size )
	{
		if ( m_commentCount > 0 )
		{
			m_ids					= new unsigned int [ m_commentCount ];
			m_flagId				= new unsigned int [ m_commentCount ];
			m_author				= new WCHAR* [ m_commentCount ];
			m_creationDate			= new WCHAR* [ m_commentCount ];
			m_state					= new unsigned int [ m_commentCount ];
			m_priority				= new unsigned int [ m_commentCount ];
			m_cameraPositions		= new SVec3  [ m_commentCount ];
			m_cameraOrientations	= new SVec3  [ m_commentCount ];
			m_flagPositions			= new SVec3  [ m_commentCount ];
			m_screen				= new WCHAR* [ m_commentCount ];
			m_comment				= new WCHAR* [ m_commentCount ];
		}
	}

	unsigned int* m_ids;
	unsigned int* m_flagId;
	WCHAR** m_author;
	WCHAR** m_creationDate;
	unsigned int* m_state;
	unsigned int* m_priority;
	SVec3*  m_cameraPositions;
	SVec3*  m_cameraOrientations;
	SVec3*  m_flagPositions;
	WCHAR** m_screen;
	WCHAR** m_comment;

	size_t	m_commentCount;
};

struct SSelectedTypes
{
public:
	SSelectedTypes( unsigned int size ) : m_typeCount( size )
	{
		if ( m_typeCount > 0 )
		{
			m_ids		= new unsigned int [ m_typeCount ];
			m_type		= new WCHAR* [ m_typeCount ];
		}
	}

	unsigned int* m_ids;
	WCHAR** m_type;

	size_t	m_typeCount;
};

struct SSelectedStates
{
public:
	SSelectedStates( unsigned int size ) : m_stateCount( size )
	{
		if ( m_stateCount > 0 )
		{
			m_ids		= new unsigned int [ m_stateCount ];
			m_state		= new WCHAR* [ m_stateCount ];
		}
	}

	unsigned int* m_ids;
	WCHAR** m_state;

	size_t	m_stateCount;
};

struct SIndexFlagToUpdate
{
public:
	SIndexFlagToUpdate( unsigned int size ) : m_flagCount( size )
	{
		if ( m_flagCount > 0 )
		{
			m_ids = new unsigned int [ m_flagCount ];
		}
	}

	unsigned int* m_ids;

	size_t	m_flagCount;
};

struct SDBInitInfo
{
	WCHAR* m_info;

	CReviewDBConnection* m_connection;
};

// open and close connection
DLL SDBInitInfo ReviewDBInit( const WCHAR* databaseAddress, const WCHAR* sqlUser, const WCHAR* sqlPassword );
DLL void ReviewDBClose( CReviewDBConnection * );

// get new id
DLL int ReviewDBNextIDForFlag(CReviewDBConnection* connection, unsigned int *id );
DLL int ReviewDBNextIDForVector(CReviewDBConnection* connection, unsigned int *id );

// get TT project id 
// if reading operation succeed, but there is no tt project with given name in the database, id return value will be 0
DLL int ReviewDBGetTTProjectID(CReviewDBConnection* connection, const WCHAR* ttProjectName, unsigned int *id);

// insert
DLL int ReviewDBInsertFlag(CReviewDBConnection* connection, unsigned int flagId, unsigned int testTrackId, unsigned int flagType, const WCHAR* summary, const WCHAR* mapName, const WCHAR* linkToVideo, const WCHAR* ttProject );
DLL int ReviewDBInsertComment(CReviewDBConnection* connection, unsigned int flagId, const WCHAR* author, const WCHAR* description, unsigned int priority, SVec3 cameraPosition, SVec3 cameraOrientation, SVec3 flagPosition, const WCHAR* pathToScreen, unsigned int state);
DLL int ReviewDBInsertVector(CReviewDBConnection* connection, unsigned int vectorId, SVec3 vector);
DLL int ReviewDBInsertTTProject(CReviewDBConnection* connection, const WCHAR* ttProjectName);

// updates
DLL int ReviewDBUpdateFlag( CReviewDBConnection* connection, unsigned int flagId);

// select
DLL SSelectedFlags* ReviewDBSelectFlag( CReviewDBConnection* connection, unsigned int flagId);
DLL SSelectedFlags* ReviewDBSelectAllFlags( CReviewDBConnection* connection, const WCHAR* mapName, const WCHAR* ttProject );
DLL SSelectedFlags* ReviewDBSelectFlagsWithoutClosed( CReviewDBConnection* connection, const WCHAR* mapName, const WCHAR* ttProject );
SSelectedFlags* ReviewDBSelectFlags( CReviewDBConnection* connection, const WCHAR* command );

DLL SSelectedComments* ReviewDBSelectCommentForFlag( CReviewDBConnection* connection, unsigned int flagId );
DLL SSelectedComments* ReviewDBSelectNewestCommentForFlag( CReviewDBConnection* connection, unsigned int flagId );
DLL SSelectedComments* ReviewDBSelectNewCommentsForUpdatedFlag( CReviewDBConnection* connection, unsigned int flagId , const WCHAR* editorLastUpdate );
SSelectedComments* ReviewDBSelectComment( CReviewDBConnection* connection, const WCHAR* command, unsigned int flagId, const WCHAR* editorLastUpdate = NULL );

DLL SSelectedTypes* ReviewDBSelectFlagTypes( CReviewDBConnection* connection );
DLL SSelectedStates* ReviewDBSelectFlagStates( CReviewDBConnection* connection );
DLL SVec3* ReviewDBSelectVector( CReviewDBConnection* connection, unsigned int vectorId);
DLL SIndexFlagToUpdate* ReviewDBSelectIndexFlagToUpdate( CReviewDBConnection* connection, const WCHAR* editorLastUpdate );


// free allocated memories
DLL int ReviewDBFreeSeletedFlags( SSelectedFlags* flags );
DLL int ReviewDBFreeSelectedComments( SSelectedComments* comments);
DLL int ReviewDBFreeSelectFlagTypes( SSelectedTypes* types );
DLL int ReviewDBFreeSelectFlagStates( SSelectedStates* states );
DLL int ReviewDBFreeSelectFlagStates( SVec3* vector );
DLL int ReviewDBFreeSelectIndexFlagToUpdates( SIndexFlagToUpdate* indexes );
DLL int ReviewDBFreeInfoErrorMessage( SDBInitInfo& errInfo );