/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//////////////////////////////////////////////////////////////////////////
// POINT OF INTEREST
//////////////////////////////////////////////////////////////////////////
// Select

WCHAR* GSelectAllPointsQuery = 
	L"SELECT * FROM [APOINTS_2] "
	L"WHERE "
	L"[LOCATION_ID] = %PARAM_LOCATION_ID%";

WCHAR* GSelectAllLevelsInfoQuery = 
	L"SELECT * FROM [LOCATION_2]";

WCHAR* GSelectAllCategoriesQuery =
	L"SELECT * FROM [CATEGORY_2]";


//////////////////////////////////////////////////////////////////////////
// Get new id for POI
WCHAR* GGetNextPOIIdQuery = 
	L"SELECT max([AP_ID])+1 " 
	L"FROM [APOINTS_2]";


//////////////////////////////////////////////////////////////////////////
// Insert new POI
WCHAR* GInsertPOIQuery = L"INSERT INTO [APOINTS_2]"
	L"([LOCATION_ID] "
	L",[AP_NAME] "
	L",[AP_DESCRIPTION] "
	L",[AP_CATEGORY] "
	L",[AP_X] "
	L",[AP_Y] "
	L",[HEIGHT]) "
	L"VALUES "
	L"(%PARAM_LOCATION_ID% "
	L",%PARAM_AP_NAME% "
	L",%PARAM_AP_DESCRIPTION% "
	L",%PARAM_AP_CATEGORY% "
	L",%PARAM_AP_X% "
	L",%PARAM_AP_Y% "
	L",%PARAM_HEIGHT%)";

WCHAR* GInsertPOILogQuery = L"INSERT INTO [APOINTS_2_CHANGELOG]"
	L"([APOINT_ID] "
	L",[USER] "
	L",[MODIFICATION_TYPE] "
	L",[MODIFICATION_DATE]) "
	L"VALUES "
	L"(%PARAM_APOINT_ID% "
	L",%PARAM_USER% "
	L",%PARAM_MODIFICATION_TYPE% "
	L",GETDATE())";


//////////////////////////////////////////////////////////////////////////
// Update POI
WCHAR* GUpdatePOIQuery = 
	L"UPDATE [APOINTS_2] SET "
	L"[AP_X] = %PARAM_AP_X% "
	L",[AP_Y] = %PARAM_AP_Y% "
	L",[HEIGHT] = %PARAM_HEIGHT% "
	L",[AP_NAME] = %PARAM_AP_NAME% "
	L",[AP_DESCRIPTION] = %PARAM_AP_DESCRIPTION% "
	L",[AP_CATEGORY] = %PARAM_AP_CATEGORY% "
	L"WHERE "
	L"[AP_ID] = %PARAM_AP_ID%";


//////////////////////////////////////////////////////////////////////////
// Delete POI
WCHAR* GDeletePOIQuery =
	L"DELETE FROM [APOINTS_2] "
	L"WHERE "
	L"[AP_ID] = %PARAM_AP_ID%";

