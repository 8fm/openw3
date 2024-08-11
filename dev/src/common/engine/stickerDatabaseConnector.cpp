/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "stickerDatabaseConnector.h"
#include "stickersSystem.h"
#include "../core/configVar.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/feedback.h"
#include "entity.h"
#include "commonConfigs.h"

#ifdef _WIN64
#pragma comment ( lib, "../../../internal/MarkersSystemDB/MarkersSystemDB_x64.lib" )
#elif defined(W2_PLATFORM_WIN32)
#pragma comment ( lib, "../../../internal/MarkersSystemDB/MarkersSystemDB_x86.lib" )
#endif

#ifndef NO_MARKER_SYSTEMS
Bool CStickerDatabaseConnector::Connect()
{
	Disconnect();

	String databaseAddress = TXT("CDPRS-MSSQL\\sqlexpress");	// default address for Warsaw
	databaseAddress = Config::cvDatabaseAddress.Get();
	m_connection = StickersDBInit( databaseAddress.AsChar() );

	return (m_connection != NULL);
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CStickerDatabaseConnector::Disconnect()
{
	if( m_connection != NULL )
	{
		StickersDBClose( m_connection );
		m_connection = NULL;
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
Bool CStickerDatabaseConnector::IsConnected() const
{
	return ( m_connection != NULL );
}
#endif


#ifndef NO_MARKER_SYSTEMS
Bool CStickerDatabaseConnector::AddNewSticker(CSticker& newSticker)
{
	Uint32 newId;

	if(StickersDBNextIDForSticker(m_connection, &newId) < 0)
	{
		GFeedback->ShowError( TXT( "Unable to insert sticker into database") );
		ERR_ENGINE( TXT( "Unable to insert sticker into database" ));
		return false;
	}

	if( StickersDBInsertSticker( m_connection, newId, newSticker.m_mapName.AsChar(), newSticker.m_title.AsChar(),
								newSticker.m_description.AsChar(), newSticker.m_type, newSticker.m_position.X, newSticker.m_position.Y, newSticker.m_position.Z) < 0 )
	{
		GFeedback->ShowError( TXT( "Unable to insert sticker into database") );
		ERR_ENGINE( TXT( "Unable to insert sticker into database" ));
		return false;
	}

	newSticker.m_databaseId = newId;

	return true;
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CStickerDatabaseConnector::GetAllStickers(TDynArray<CSticker>& stickers, String& mapName)
{
	SSelectedStickers* readResult = NULL;

	readResult = StickersDBSelectAllStickers(m_connection, mapName.AsChar());

	if ( readResult != NULL )
	{
		for ( Uint32 i = 0; i < readResult->m_pointsCount; ++i )
		{
			CSticker newSticker;
			ParseSticker(newSticker, readResult, i);
			stickers.PushBack(newSticker);
		}

		StickersDBFreeSelectedPoint( readResult );
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CStickerDatabaseConnector::GetAllCategories(TDynArray<String>& categories)
{
	categories.Clear();

	SSelectedStickerCategories* readResult = NULL;

	readResult = StickersDBSelectAllCategories(m_connection);

	if ( readResult != NULL )
	{
		for ( Uint32 i = 0; i < readResult->m_categoryCount; ++i )
		{
			categories.PushBack(readResult->m_categoryNames[i]);
		}

		StickersDBFreeSelectedCategories( readResult );
	}
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CStickerDatabaseConnector::Synchronize(TDynArray<CSticker>& stickers, String& mapName)
{
	stickers.Clear();
	GetAllStickers(stickers, mapName);
}
#endif

#ifndef NO_MARKER_SYSTEMS
Bool CStickerDatabaseConnector::ModifySticker(CSticker& sticker)
{
	if( StickersDBUpdateSticker( m_connection, sticker.m_databaseId, sticker.m_mapName.AsChar(), sticker.m_title.AsChar(),
		 sticker.m_description.AsChar(), sticker.m_type, sticker.m_position.X, sticker.m_position.Y, sticker.m_position.Z, sticker.m_removed) < 0 )
	{
		GFeedback->ShowError( TXT( "Unable to update sticker in database") );
		ERR_ENGINE( TXT( "Unable to update sticker in database" ));
		return false;
	}

	return true;
}
#endif

#ifndef NO_MARKER_SYSTEMS
Bool CStickerDatabaseConnector::DeleteSticker(CSticker& sticker)
{
	if( StickersDBDeleteSticker( m_connection, sticker.m_databaseId) < 0 )
	{
		GFeedback->ShowError( TXT( "Unable to delete sticker in database") );
		ERR_ENGINE( TXT( "Unable to delete sticker in database" ));
		return false;
	}

	return true;
}
#endif

#ifndef NO_MARKER_SYSTEMS
void CStickerDatabaseConnector::ParseSticker(CSticker& sticker, SSelectedStickers* stickerFromDB, Uint32 currentSticker)
{
	sticker.m_databaseId	= stickerFromDB->m_ids[currentSticker];
	sticker.m_mapName		= stickerFromDB->m_mapNames[currentSticker];
	sticker.m_title			= stickerFromDB->m_titles[currentSticker];
	sticker.m_description	= stickerFromDB->m_descriptions[currentSticker];
	sticker.m_type			= stickerFromDB->m_type[currentSticker];
	sticker.m_position.X	= (float)stickerFromDB->m_x[currentSticker];
	sticker.m_position.Y	= (float)stickerFromDB->m_y[currentSticker];
	sticker.m_position.Z	= (float)stickerFromDB->m_z[currentSticker];
	sticker.m_removed		= stickerFromDB->m_removed[currentSticker];
}
#endif
