/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_MARKER_SYSTEMS

#include "../../../internal/MarkersSystemDB/MarkersSystemDB.h"

class CSticker;
class CStickerDB;

class CStickerDatabaseConnector
{
public:
	CStickerDatabaseConnector(void) { /*intentionally empty*/ };
	~CStickerDatabaseConnector(void) { /*intentionally empty*/ };

	// manage connection with database
	Bool Connect();
	void Disconnect();
	Bool IsConnected() const;
	
	// main operations on database
	Bool AddNewSticker	(CSticker& newSticker);
	Bool ModifySticker	(CSticker& sticker);
	Bool DeleteSticker	(CSticker& sticker);
	void Synchronize	(TDynArray<CSticker>& stickers, String& mapName);

	// select
	void GetAllStickers(TDynArray<CSticker>& stickers, String& mapName);
	void GetAllCategories(TDynArray<String>& categories);

private:
	// translate database result to suitable format
	void ParseSticker(CSticker& sticker, SSelectedStickers* stickerFromDB, Uint32 currentSticker);

	CStickersDB* m_connection;
};
#endif // NO_MARKER_SYSTEMS
