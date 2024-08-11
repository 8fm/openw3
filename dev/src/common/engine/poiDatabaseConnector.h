/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_MARKER_SYSTEMS

#include "../../../internal/MarkersSystemDB/MarkersSystemDB.h"

class CPointofInterest;
class CPointsofInterestDB;
struct SPointLevelInfo;

class CPOIDatabaseConnector
{
public:
	CPOIDatabaseConnector(void) { /*intentionally empty*/ };
	~CPOIDatabaseConnector(void) { /*intentionally empty*/ };

	// manage connection with database
	Bool Connect();
	void Disconnect();
	Bool IsConnected() const;
	
	// main operations on database
	Bool AddNewPoint	(CPointofInterest& newPoint);
	Bool ModifyPoint	(CPointofInterest& point);
	Bool DeletePoint	(CPointofInterest& point);
	void Synchronize	(TDynArray<CPointofInterest>& points, Uint32 levelId);

	// select
	void GetAllPoints(TDynArray<CPointofInterest>& points, Uint32 levelId);
	void GetAllLevels(TDynArray<SPointLevelInfo>& levels);
	void GetAllCategories(TDynArray<String>& categories);

private:
	// translate database result to suitable format
	void ParsePoint(CPointofInterest& poi, SSelectedPoints* poiFromDB, Uint32 currentPoi);

	CPointsofInterestDB* m_connection;
};
#endif // NO_MARKER_SYSTEMS
