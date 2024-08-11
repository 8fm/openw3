/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_MARKER_SYSTEMS

#include "markersSystem.h"
#include "poiDatabaseConnector.h"

/* Contains information about poi */
class CPointofInterest
{
public:
	CPointofInterest	() : m_poiEntity(nullptr), m_snappedToTerrain( true )	{ /*intentionally empty*/ };
	~CPointofInterest	()														{ /*intentionally empty*/ };

	Uint32				m_databaseId;
	Uint32				m_levelId;
	String				m_name;
	String				m_description;
	Uint32				m_category;
	Uint32				m_coordinateX;
	Uint32				m_coordinateY;
	Vector				m_worldPosition;
	Bool				m_snappedToTerrain;

	THandle< CEntity>	m_poiEntity;
};

struct SPointLevelInfo
{
	String	m_levelName;
	Uint32	m_tileCount;
	Uint32	m_tileSize;
};

// Enums
enum EPOICategory
{
	POIC_Quest,
	POIC_SideQuest,
	POIC_Landmark,
	POIC_Interrior,
	POIC_Dungeon,
	POIC_Gameplay,
	POIC_Mood,
	POIC_Community,
	POIC_RoadSign,
	POIC_Harbor,
	POIC_Settlement,
	POIC_Cutscene,

	POIC_Count,
};

enum EPOISortCategory
{
	POISC_Title,
	POISC_Category,

	POISC_Count
};


// Main points of interest system class
class CPoiSystem : public AbstractMarkerSystem
{
public:
			CPoiSystem	();
	virtual ~CPoiSystem	();

	// implement IMarkerSystemInterface
	virtual void Initialize();
	virtual void Tick( Float timeDelta ) override;
	virtual void Shutdown();

	virtual void Connect();
	virtual void BackgroundUpdate();
	virtual void SetNewEntity(CEntity* newEntity);

	// manage the system
	Bool IsConnected		() const;
	void CreateNewPoint		();
	Bool AddNewPoint		();
	Bool ModifyPoint		(CPointofInterest& point);
	Bool DeletePoint		(CPointofInterest& point);

	// filtering
	void SetFilters				( const TDynArray<Bool>& categories );
	void SetSearchFilter		( const String& condition );
	void SetSortingSettings		( enum EPOISortCategory category, enum EMarkerSystemSortOrder order );

	// accessors - set
	void SetAutoSync				( Bool autosync );
	void SetShowOnMap				( Bool showOnMap );
	void SetSyncTime				( Uint32 time );
	void SetBlackBoxPath			( const String& blackBoxPath );

	// accessors - get
	Bool								GetAutoSync				() const;
	Bool								GetShowOnMap			() const;
	Int32								GetActiveLevelId		() const;
	Uint32								GetSyncTime				() const;
	String								GetBlackBoxPath			() const;
	CPointofInterest*					GetNewPoint				() const;
	const TDynArray<String>&			GetPointCategories		() const;
	const TDynArray<CPointofInterest*>&	GetPoints				() const;
	CPointofInterest*					GetPoint				( Uint32 number ) const;

	THandle< CEntityTemplate >			GetPointTemplate( enum EPOICategory category ) const;

private:
	void LoadSettings();
	void SaveSettings();

	void OnLoadData();
	void OnReleaseData();

	// others private functions
	void SynchronizePoints();
	void UpdateData();

	void FilterPoints();
	void SortFilteredPoints();

	void AddEntitiesOnMap();
	void RemoveEntietiesFromMap();

	void CalculatePointsPositionToWorld		();
	void CalculatePointPositionToBlackBox	(CPointofInterest& point);

	void SetInternalFilters( const TDynArray<Bool>& categories, const String& filterCondition );

private:
	Bool				m_autoSync;				// on/off auto synchronization for this system
	Bool				m_showOnMap;			// on/off visibility for point entities
	Uint32				m_autoSyncTime;			// time between data synchronizations
	String				m_blackBoxPath;			// path to BlackBox executable file
	String				m_filterCondition;		// text which we looking for in title when m_filterByTitle is true
	CPointofInterest*	m_newPoint;				// temporary variable for new point
	CTimeCounter		m_lastSyncCounter;		// time counter for synchronization

	// subsystems
	CPOIDatabaseConnector			m_databaseConnector;
	
	// containers for data
	TDynArray<SPointLevelInfo>		m_pointLevels;			// informations about maps supported by BlackBox
	TDynArray<String>				m_pointCategories;		// names of categories for points
	TDynArray<CPointofInterest*>	m_filteredPoints;		// filtered and sorted points from m_points container
	TDynArray<CPointofInterest>		m_points;				// container for all points for active map
	TDynArray<Bool>					m_filterCategoryValues;	// true is there, where is category of points, which we want to see
	EPOISortCategory				m_sortCategory;
	EMarkerSystemSortOrder			m_sortOrder;

	THandle< CEntityTemplate >		m_pointsEntities[POIC_Count];

	// container 
	TDynArray< THandle< CEntity > >	m_entitiesToRemove;
	TDynArray< CPointofInterest* >	m_entitiesToAdd;
};

#endif	// NO_MARKER_SYSTEMS
