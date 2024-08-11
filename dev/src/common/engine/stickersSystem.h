/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_MARKER_SYSTEMS

#include "markersSystem.h"
#include "stickerComponent.h"
#include "stickerDatabaseConnector.h"

/* Contains information about sticker */
class CSticker
{
public:
	CSticker	() : m_stickerEntity( nullptr ) { /*intentionally empty*/ };
	~CSticker	()						{ /*intentionally empty*/ };

	Uint32				m_databaseId;
	String				m_mapName;
	Uint32				m_type;
	String				m_title;
	String				m_description;
	Bool				m_removed;
	Vector				m_position;
	THandle< CEntity >	m_stickerEntity;
};

// Main stickers system class
class CStickersSystem : public AbstractMarkerSystem
{
public:
			CStickersSystem	();
	virtual ~CStickersSystem();

	// implement IMarkerSystemInterface
	virtual void Initialize();
	virtual void Tick( Float timeDelta ) override;
	virtual void Shutdown();

	virtual void Connect();
	virtual void BackgroundUpdate();
	virtual void SetNewEntity(CEntity* newEntity);

	// manage the system
	Bool IsConnected		() const;
	void CreateNewSticker	();
	Bool AddNewSticker		();
	Bool ModifySticker		(CSticker& sticker);
	Bool DeleteSticker		(CSticker& sticker);

	// filtering
	// filtering is implement as a state machine, so when we call FilterPoints function,
	// all data will be filtering until we call ResetFilters function, and is the same in the other side
	void SetFilters			( const TDynArray<Bool>& categories );
	void SetSearchFilter	( const String& condition );

	// accessors - set
	void SetAutoSync				( Bool autosync );
	void SetShowOnMap				( Bool showOnMap );
	void SetSyncTime				( Uint32 time );
	void SetRenderDistance			( Uint32 distance );

	// accessors - get
	Bool								GetAutoSync				() const;
	Bool								GetShowOnMap			() const;
	Uint32								GetSyncTime				() const;
	Uint32								GetRenderDistance		() const;
	CSticker*							GetNewSticker			() const;
	const TDynArray<String>&			GetStickerCategories	() const;
	const TDynArray<CSticker*>&			GetStickers				() const;
	CSticker*							GetSticker				( Uint32 number ) const;

	THandle< CEntityTemplate >			GetStickerTemplate		() const;

private:
	// others private functions
	void SynchronizeStickers		();
	void FilterPoints				();
	void SortFilteredStickers		();

	void OnLoadData();
	void OnReleaseData();
	void UpdateData();

	void AddEntitiesOnMap();
	void RemoveEntietiesFromMap();

	void LoadSettings();
	void SaveSettings();

	void SetInternalFilter( const TDynArray<Bool>& categories, const String& m_filterCondition );

private:
	Bool				m_autoSync;				// on/off auto synchronization for this system
	Bool				m_showOnMap;			// on/off visibility for sticker entities
	Uint32				m_autoSyncTime;			// time between data synchronizations
	Uint32				m_renderDistance;		// distance from camera to sticker, determine visibility for sticker title
	String				m_filterCondition;		// text which we looking for in title when m_filterByTitle is true
	CSticker*			m_newSticker;			// temporary variable for new sticker
	CTimeCounter		m_lastSyncCounter;		// time counter for synchronization

	// other related subsystems
	CStickerDatabaseConnector	m_databaseConnector;
	
	// containers for data
	TDynArray<String>			m_stickerCategories;	// names of categories for stickers
	TDynArray<CSticker*>		m_filteredStickers;		// filtered and sorted stickers from m_stickers container
	TDynArray<CSticker>			m_stickers;				// container for all stickers for active map
	TDynArray<Bool>				m_filterCategoryValues;	// true is there, where is category of stickers, which we want to see

	// container 
	TDynArray< THandle< CEntity > >	m_entitiesToRemove;
	TDynArray< CSticker* >			m_entitiesToAdd;
};

#endif	// NO_MARKER_SYSTEMS
