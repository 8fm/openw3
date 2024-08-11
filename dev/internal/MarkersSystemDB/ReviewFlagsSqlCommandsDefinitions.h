/**
* Copyright � 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

WCHAR* GGetNextFlagIdQuery = 
	L"SELECT max([FLAG_ID])+1 " 
	L"FROM [REVIEW_FLAGS] AS FLAG_ID";

WCHAR* GGetNextVectorIdQuery = 
	L"SELECT max([VECTOR_ID])+1 " 
	L"FROM [REVIEW_VECTORS] AS VECTOR_ID";

WCHAR* GGetTTProjectIdQuery = 
	L"SELECT TOP 1 [PROJECT_ID] "
	L"FROM [REVIEW_TTPROJECTS] AS PROJECT_ID "
	L"WHERE "
	L"[NAME] = %PARAM_NAME% ";

WCHAR* GInsertTTProjectQuery = 
	L"INSERT INTO [REVIEW_TTPROJECTS] "
	L"([NAME]) "
	L"VALUES(%PARAM_NAME%) ";

WCHAR* GInsertFlagQuery = L"INSERT INTO [REVIEW_FLAGS]"
	L"([FLAG_ID] "
	L",[TESTTRACK_ID] "
	L",[TYPE]"
	L",[SUMMARY] "
	L",[MAP_NAME] "
	L",[LINK_TO_VIDEO] "
	L",[LAST_UPDATE] "
	L",[TTPROJECT_ID]) "
	L"VALUES "
	L"(%PARAM_FLAG_ID% "
	L",%PARAM_TESTTRACK_ID% "
	L",%PARAM_TYPE%"
	L",%PARAM_SUMMARY% "
	L",%PARAM_MAP_NAME% "
	L",%PARAM_LINK_TO_VIDEO% "
	L",GETDATE() "
	L",(SELECT TOP 1 [PROJECT_ID] FROM [REVIEW_TTPROJECTS] WHERE [NAME] = %PARAM_TTPROJECT%))";

WCHAR* GInsertCommentQuery = L"INSERT INTO [REVIEW_COMMENTS]"
	L"([FLAG_ID] "
	L",[AUTHOR] "
	L",[CREATION_DATE] "
	L",[STATE] "
	L",[PRIORITY] "
	L",[CAMERA_POSITION] "
	L",[CAMERA_ORIENTATION] "
	L",[FLAG_POSITION] "
	L",[SCREEN] "
	L",[COMMENT]) "
	L"VALUES "
	L"(%PARAM_FLAG_ID% "
	L",%PARAM_AUTHOR% "
	L",GETDATE() "
	L",%PARAM_STATE% "
	L",%PARAM_PRIORITY% "
	L",%PARAM_CAMERA_POSITION% "
	L",%PARAM_CAMERA_ORIENTATION% "
	L",%PARAM_FLAG_POSITION% "
	L",%PARAM_SCREEN% "
	L",%PARAM_COMMENT%)";

WCHAR* GInsertVectorQuery = L"INSERT INTO [REVIEW_VECTORS]"
	L"([VECTOR_ID] "
	L",[VECTOR_X] "
	L",[VECTOR_Y] "
	L",[VECTOR_Z]) "
	L"VALUES "
	L"(%PARAM_VECTOR_ID% "
	L",%PARAM_VECTOR_X% "
	L",%PARAM_VECTOR_Y% "
	L",%PARAM_VECTOR_Z%)";

WCHAR* GUpdateFlagQuery = 
	L"UPDATE [REVIEW_FLAGS] SET "
	L"[LAST_UPDATE] = GETDATE() "
	L"WHERE "
	L"[FLAG_ID] = %PARAM_FLAG_ID%";

WCHAR* GSelectAllFlags = 
	L"SELECT * FROM [REVIEW_FLAGS] "
	L"WHERE "
	L"MAP_NAME = %PARAM_MAP_NAME% "
	L"AND TTPROJECT_ID = (SELECT TOP 1 [PROJECT_ID] FROM [REVIEW_TTPROJECTS] WHERE [NAME] = %PARAM_TTPROJECT%)";

WCHAR* GSelectFlagsWithoutClosed =
	L"SELECT * FROM [REVIEW_FLAGS] as FLAG_ALIAS "
	L"WHERE "
	L"MAP_NAME = %PARAM_MAP_NAME% AND "
	L"TTPROJECT_ID = (SELECT TOP 1 [PROJECT_ID] FROM [REVIEW_TTPROJECTS] WHERE [NAME] = %PARAM_TTPROJECT%) AND "
	L"(SELECT TOP 1  [STATE] FROM [REVIEW_COMMENTS] "
	L"WHERE "
	L"[FLAG_ID] = FLAG_ALIAS.FLAG_ID "
	L"ORDER BY CREATION_DATE DESC) <> (SELECT [STATE_ID] FROM [REVIEW_STATES] WHERE [NAME] = 'Closed')";

WCHAR* GSelectAllCommentsForFlag = 
	L"SELECT * FROM [REVIEW_COMMENTS] "
	L"WHERE "
	L"FLAG_ID = %PARAM_FLAG_ID%";

WCHAR* GSelectNewestCommentsForFlag = 
	L"SELECT TOP 1  * FROM [REVIEW_COMMENTS] "
	L"WHERE "
	L"[FLAG_ID] = %PARAM_FLAG_ID% "
	L"ORDER BY [CREATION_DATE] DESC";

WCHAR* GSelectFlagStates = L"SELECT * FROM [REVIEW_STATES]";

WCHAR* GSelectFlagTypes = L"SELECT * FROM [REVIEW_TYPES]";

WCHAR* GSelectVector = 
	L"SELECT * FROM [REVIEW_VECTORS] "
	L"WHERE "
	L"[VECTOR_ID] = %PARAM_VECTOR_ID%";

WCHAR* GSelectIndexFlagToUpdate = 
	L"SELECT [FLAG_ID] FROM [REVIEW_FLAGS] as FLAG_ALIAS "
	L"WHERE "
	L"[LAST_UPDATE] > %PARAM_LAST_UPDATE%";

WCHAR* GSelectNewCommentsForUpdatedFlag = 
	L"SELECT * FROM [REVIEW_COMMENTS] "
	L"WHERE "
	L"[FLAG_ID] = %PARAM_FLAG_ID% "
	L"AND "
	L"[CREATION_DATE] > %PARAM_LAST_UPDATE%";

WCHAR* GSelectFlag = 
	L"SELECT * FROM [REVIEW_FLAGS] "
	L"WHERE "
	L"[FLAG_ID] = %PARAM_FLAG_ID% ";
