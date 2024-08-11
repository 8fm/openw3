/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifdef POINTS_OF_INTEREST_DB_EXPORTS 
#define DLL __declspec(dllexport)
#else
#define DLL __declspec(dllimport)
#endif

class CPointsofInterestDB;

struct SSelectedPoints
{
	unsigned int* m_ids;
	unsigned int* m_locationIds;	
	WCHAR** m_names;
	WCHAR** m_descriptions;
	unsigned int* m_categories;	
	unsigned int* m_coordinatesX;	
	unsigned int* m_coordinatesY;
	bool* m_snapToTerrain;
	float* m_height;

	size_t	m_pointsCount;
};

struct SSelectedLevelsInfo
{
	unsigned int* m_ids;
	WCHAR** m_levelNames;
	unsigned int* m_tileCount;
	unsigned int* m_tileSize;

	size_t	m_levelCount;
};

struct SSelectedCategories
{
	unsigned int* m_ids;
	WCHAR** m_categoryNames;

	size_t	m_categoryCount;
};


// CONNECT AND DISCONNECT
DLL CPointsofInterestDB* PointsofInterestDBInit( const WCHAR* databaseAddress );
DLL void PointsofInterestDBClose( CPointsofInterestDB *conn );


// GET NEW ID
DLL int PointsofInterestDBNextIDForPOI(CPointsofInterestDB* connection, unsigned int *id );


// INSERT
DLL int PointsofInterestDBInsertPOI(CPointsofInterestDB* connection, unsigned int POIId, unsigned int levelId, const WCHAR* title, const WCHAR* description, unsigned int category, unsigned int coordinateX, unsigned int coordinateY, bool snapToTerrain, float height);
DLL int PointsofInterestDBInsertPOILog(CPointsofInterestDB* connection, unsigned int POIId, const WCHAR* modificationType);


// UPDATE
DLL int PointsofInterestDBUpdatePOI( CPointsofInterestDB* connection, unsigned int coordinateX, unsigned int coordinateY, bool snapToTerrain, float height, const WCHAR* name, const WCHAR* description, unsigned int category, unsigned int POIId );


// DELETE
DLL int PointsofInterestDBDeletePOI( CPointsofInterestDB* connection, unsigned int POIId );


// SELECT
DLL SSelectedPoints* PointsofInterestDBSelectAllPoints( CPointsofInterestDB* connection, unsigned int levelId );
DLL SSelectedLevelsInfo* PointsofInterestDBSelectAllLevelesInfo( CPointsofInterestDB* connection );
DLL SSelectedCategories* PointsofInterestDBSelectAllCategories( CPointsofInterestDB* connection );


// FREE ALLOCATED MEMORIES
DLL int PointsofInterestDBFreeSelectedPoint( SSelectedPoints* points );
DLL int PointsofInterestDBFreeSelectedLevelesInfo( SSelectedLevelsInfo* levelsInfo );
DLL int PointsofInterestDBFreeSelectedCategories( SSelectedCategories* categories );

