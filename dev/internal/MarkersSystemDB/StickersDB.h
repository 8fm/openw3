/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifdef STICKERS_DB_EXPORTS 
#define DLL __declspec(dllexport)
#else
#define DLL __declspec(dllimport)
#endif

class CStickersDB;

struct SSelectedStickers
{
	unsigned int*	m_ids;
	WCHAR**			m_mapNames;
	WCHAR**			m_titles;
	WCHAR**			m_descriptions;
	unsigned int*	m_type;
	double*			m_x;
	double*			m_y;
	double*			m_z;
	bool*			m_removed;

	size_t	m_pointsCount;
};

struct SSelectedStickerCategories
{
	unsigned int* m_ids;
	WCHAR** m_categoryNames;

	size_t	m_categoryCount;
};


// CONNECT AND DISCONNECT
DLL CStickersDB* StickersDBInit( const WCHAR* databaseAddress );
DLL void StickersDBClose( CStickersDB *conn );


// GET NEW ID
DLL int StickersDBNextIDForSticker(CStickersDB* connection, unsigned int *id );


// INSERT
DLL int StickersDBInsertSticker(CStickersDB* connection, unsigned int stickerId, const WCHAR* mapName, const WCHAR* title, const WCHAR* description, unsigned int type, float x, float y, float z);


// UPDATE
DLL int StickersDBUpdateSticker(CStickersDB* connection, unsigned int stickerId, const WCHAR* mapName, const WCHAR* title, const WCHAR* description, unsigned int type, float x, float y, float z, bool removed);


// DELETE
DLL int StickersDBDeleteSticker( CStickersDB* connection, unsigned int stickerId );


// SELECT
DLL SSelectedStickers* StickersDBSelectAllStickers( CStickersDB* connection, const WCHAR* mapName );
DLL SSelectedStickerCategories* StickersDBSelectAllCategories( CStickersDB* connection );


// FREE ALLOCATED MEMORIES
DLL int StickersDBFreeSelectedPoint( SSelectedStickers* stickers );
DLL int StickersDBFreeSelectedCategories( SSelectedStickerCategories* categories );

