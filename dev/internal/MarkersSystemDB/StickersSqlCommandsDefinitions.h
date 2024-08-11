/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//////////////////////////////////////////////////////////////////////////
// STICKERS
//////////////////////////////////////////////////////////////////////////
// Select

WCHAR* GSelectAllStickersQuery = 
	L"SELECT * FROM [STICKERS] "
	L"WHERE "
	L"[MAP_NAME] = %PARAM_MAP_NAME% "
	L" AND [REMOVED] = 0";

WCHAR* GSelectAllStickerCategoriesQuery =
	L"SELECT * FROM [STICKER_CATEGORIES]";


//////////////////////////////////////////////////////////////////////////
// Get new id for Sticker
WCHAR* GGetNextStickerIdQuery = 
	L"SELECT max([ID])+1 " 
	L"FROM [STICKERS]";


//////////////////////////////////////////////////////////////////////////
// Insert new Sticker
WCHAR* GInsertStickerQuery = L"INSERT INTO [STICKERS]"
	L"([ID] "
	L",[MAP_NAME] "
	L",[TYPE] "
	L",[TITLE] "
	L",[DESCRIPTION] "
	L",[X] "
	L",[Y] "
	L",[Z] "
	L",[REMOVED]) "
	L"VALUES "
	L"(%PARAM_ID% "
	L",%PARAM_MAP_NAME% "
	L",%PARAM_TYPE% "
	L",%PARAM_TITLE% "
	L",%PARAM_DESCRIPTION% "
	L",%PARAM_X% "
	L",%PARAM_Y% "
	L",%PARAM_Z% "
	L",0)";


//////////////////////////////////////////////////////////////////////////
// Update Sticker
WCHAR* GUpdateStickerQuery = 
	L"UPDATE [STICKERS] SET "
	L"[MAP_NAME] = %PARAM_MAP_NAME% "
	L",[TYPE] = %PARAM_TYPE% "
	L",[TITLE] = %PARAM_TITLE% "
	L",[DESCRIPTION] = %PARAM_DESCRIPTION% "
	L",[X] = %PARAM_X% "
	L",[Y] = %PARAM_Y% "
	L",[Z] = %PARAM_Z% "
	L",[REMOVED] = %PARAM_REMOVED% "
	L"WHERE "
	L"[ID] = %PARAM_ID%";


//////////////////////////////////////////////////////////////////////////
// Delete Sticker
WCHAR* GDeleteStickerQuery =
	L"UPDATE [STICKERS] SET "
	L"[REMOVED] = 1 "
	L"WHERE "
	L"[ID] = %PARAM_ID%";

