	/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/game/commonGameResource.h"
#include "mapPinData.h"
#include "worldMap.h"

class CWitcherGameResource : public CCommonGameResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CWitcherGameResource, CCommonGameResource, "redgame", "R4 game definition" );

protected:
	SMapPinConfig								m_mapPinConfig;
	TSoftHandle< C2dArray >						m_huntingClueCategoryResource;
	TDynArray< CEnum* >							m_huntingClueCategories;
	String										m_journalRootDirectory;

public:
	RED_INLINE const SMapPinConfig&								GetMapPinConfig() const { return m_mapPinConfig; }
	RED_INLINE const String&										GetJournalPath() const { return m_journalRootDirectory; }

	RED_INLINE const TDynArray< CEnum* >&							GetHuntingClueCategories()
	{
		EnumerateHuntingQuestEnums();
		return m_huntingClueCategories;
	}

private:

	// Turn csv resource into array of CEnum* pointers (each enum being a hunting clue category)
	void EnumerateHuntingQuestEnums();
};

BEGIN_CLASS_RTTI( CWitcherGameResource );
	PARENT_CLASS( CCommonGameResource );
	PROPERTY_EDIT( m_mapPinConfig, TXT( "Configuration of map pins" ) );
	PROPERTY_EDIT( m_huntingClueCategoryResource, TXT( "Hunting Clue Categories" ) );
	PROPERTY_CUSTOM_EDIT( m_journalRootDirectory, TXT( "Journal Root Directory" ), TXT( "DirectorySelectionEditor" ) );
END_CLASS_RTTI();
