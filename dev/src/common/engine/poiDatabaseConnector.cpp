/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "POIDatabaseConnector.h"
#include "poiSystem.h"
#include "entity.h"
#include "../core/configFileManager.h"
#include "../core/feedback.h"
#include "commonConfigs.h"

#ifdef _WIN64
#pragma comment ( lib, "../../../internal/MarkersSystemDB/MarkersSystemDB_x64.lib" )
#elif defined(W2_PLATFORM_WIN32)
#pragma comment ( lib, "../../../internal/MarkersSystemDB/MarkersSystemDB_x86.lib" )
#endif

#ifndef NO_MARKER_SYSTEMS
Bool CPOIDatabaseConnector::Connect()
{
	Disconnect();

	m_connection = PointsofInterestDBInit( Config::cvDatabaseAddress.Get().AsChar() );

	return (m_connection != NULL);
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CPOIDatabaseConnector::Disconnect()
{
	if( m_connection != NULL )
	{
		PointsofInterestDBClose( m_connection );
		m_connection = NULL;
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
Bool CPOIDatabaseConnector::IsConnected() const
{
	return ( m_connection != NULL );
}
#endif


#ifndef NO_MARKER_SYSTEMS
Bool CPOIDatabaseConnector::AddNewPoint(CPointofInterest& newPoi)
{
	Uint32 newId;

	if(PointsofInterestDBNextIDForPOI(m_connection, &newId) < 0)
	{
		GFeedback->ShowError( TXT( "Unable to insert point into database") );
		ERR_ENGINE( TXT( "Unable to insert point into database" ));
		return false;
	}

	if( PointsofInterestDBInsertPOI( m_connection, newId, newPoi.m_levelId, newPoi.m_name.AsChar(), newPoi.m_description.AsChar(),
							newPoi.m_category, newPoi.m_coordinateX, newPoi.m_coordinateY, newPoi.m_snappedToTerrain, newPoi.m_worldPosition.Z ) < 0 )
	{
		GFeedback->ShowError( TXT( "Unable to insert point into database") );
		ERR_ENGINE( TXT( "Unable to insert point into database" ));
		return false;
	}

	newPoi.m_databaseId = newId;

	return true;
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CPOIDatabaseConnector::GetAllPoints(TDynArray<CPointofInterest>& points, Uint32 levelId)
{
	SSelectedPoints* readResult = NULL;

	readResult = PointsofInterestDBSelectAllPoints(m_connection, levelId);

	if ( readResult != NULL )
	{
		for ( Uint32 i = 0; i < readResult->m_pointsCount; ++i )
		{
			CPointofInterest newPoi;
			ParsePoint(newPoi, readResult, i);
			points.PushBack(newPoi);
		}

		PointsofInterestDBFreeSelectedPoint( readResult );
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CPOIDatabaseConnector::GetAllLevels(TDynArray<SPointLevelInfo>& levels)
{
	levels.Clear();

	SSelectedLevelsInfo* readResult = NULL;

	readResult = PointsofInterestDBSelectAllLevelesInfo(m_connection);

	if ( readResult != NULL )
	{
		for ( Uint32 i = 0; i < readResult->m_levelCount; ++i )
		{
			SPointLevelInfo levelInfo;
			levelInfo.m_levelName = readResult->m_levelNames[i];
			levelInfo.m_tileCount = readResult->m_tileCount[i];
			levelInfo.m_tileSize = readResult->m_tileSize[i];
			levels.PushBack(levelInfo);
		}

		PointsofInterestDBFreeSelectedLevelesInfo( readResult );
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CPOIDatabaseConnector::GetAllCategories(TDynArray<String>& categories)
{
	categories.Clear();

	SSelectedCategories* readResult = NULL;

	readResult = PointsofInterestDBSelectAllCategories(m_connection);

	if ( readResult != NULL )
	{
		for ( Uint32 i = 0; i < readResult->m_categoryCount; ++i )
		{
			categories.PushBack(readResult->m_categoryNames[i]);
		}

		PointsofInterestDBFreeSelectedCategories( readResult );
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CPOIDatabaseConnector::Synchronize(TDynArray<CPointofInterest>& points, Uint32 levelId)
{
	points.Clear();
	GetAllPoints(points, levelId);
}
#endif

#ifndef NO_MARKER_SYSTEMS
Bool CPOIDatabaseConnector::ModifyPoint( CPointofInterest& poi )
{
	if( PointsofInterestDBUpdatePOI( m_connection, poi.m_coordinateX, poi.m_coordinateY, poi.m_snappedToTerrain, poi.m_worldPosition.Z,
		poi.m_name.AsChar(), poi.m_description.AsChar(), poi.m_category, poi.m_databaseId ) < 0 )
	{
		GFeedback->ShowError( TXT( "Unable to update point in database") );
		ERR_ENGINE( TXT( "Unable to update point in database" ));
		return false;
	}

	return true;
}
#endif

#ifndef NO_MARKER_SYSTEMS
Bool CPOIDatabaseConnector::DeletePoint( CPointofInterest& poi )
{
	if( PointsofInterestDBDeletePOI( m_connection, poi.m_databaseId) < 0 )
	{
		GFeedback->ShowError( TXT( "Unable to delete point in database") );
		ERR_ENGINE( TXT( "Unable to delete point in database" ));
		return false;
	}

	return true;
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CPOIDatabaseConnector::ParsePoint(CPointofInterest& poi, SSelectedPoints* poiFromDB, Uint32 currentPoi)
{
	poi.m_databaseId = poiFromDB->m_ids[currentPoi];
	poi.m_levelId = poiFromDB->m_locationIds[currentPoi];
	poi.m_name = poiFromDB->m_names[currentPoi];
	poi.m_description = poiFromDB->m_descriptions[currentPoi];
	poi.m_category = poiFromDB->m_categories[currentPoi];
	poi.m_coordinateX = poiFromDB->m_coordinatesX[currentPoi];
	poi.m_coordinateY = poiFromDB->m_coordinatesY[currentPoi];
	poi.m_snappedToTerrain = poiFromDB->m_snapToTerrain[currentPoi];
	if ( !poi.m_snappedToTerrain )
	{
		poi.m_worldPosition.Z = poiFromDB->m_height[currentPoi];
	}
}
#endif
