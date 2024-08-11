/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifndef NO_MARKER_SYSTEMS

#include "markersSystem.h"
#include "reviewDatabaseConnector.h"
#include "reviewTestTrackConnector.h"

/* Contains information about changes in review flag */
class CReviewFlagComment
{
public:
	CReviewFlagComment	() : m_screen( nullptr ) { /*intentionally empty*/ };
	~CReviewFlagComment	()  { /*intentionally empty*/ };

	// this function protects against null pointer
	CBitmapTexture* GetFlagScreen();

	Uint32	m_id;				// id in database
	Uint32	m_flagId;			// id parent flag
	String	m_author;			// change author
	tm		m_creationDate;		// date of adding change
	Uint32	m_priority;			// new priority for parent flag
	Uint32	m_state;			// new state for parent flag
	Vector	m_cameraPosition;	// new camera position for flag
	Vector	m_cameraOrientation;// new camera orientation (rotation) for flag
	Vector	m_flagPosition;		// new flag position
	String	m_pathToScreen;		// path to screen
	String	m_description;		// change description

	THandle< CBitmapTexture >	m_screen;
};

/* Contains information about review flag */
class CReviewFlag
{
public:
	CReviewFlag	() : m_flagEntity(nullptr) { /*intentionally empty*/ };
	~CReviewFlag()						{ /*intentionally empty*/ };

	Uint32							m_databaseId;
	Uint32							m_testTrackNumber;
	Uint32							m_type;
	String							m_summary;
	String							m_mapName;
	tm								m_lastUpdate;
	String							m_linkToVideo;
	TDynArray<CReviewFlagComment>	m_comments;

	THandle< CEntity >				m_flagEntity;
};

// Enums
enum EReviewFlagState
{
	RFS_Opened,
	RFS_Fixed,
	RFS_Closed,
	RFS_ReOpened,

	RFS_Count
};

enum EReviewSortCategory
{
	ReviewSummary,
	ReviewPriority,
	ReviewState,
	ReviewType,
	ReviewCreationDate,
};

enum EReviewSearchType
{
	ReviewSearchNone,
	ReviewSearchBySummary,
	ReviewSearchByTestTrackID,
};

// Main review system class
class CReviewSystem : public AbstractMarkerSystem
{
	friend class CReviewFlagComment;

public:
			CReviewSystem	();
	virtual ~CReviewSystem	();

	// implement IMarkerSystemInterface
	virtual void Initialize() override;
	virtual void Tick( Float timeDelta ) override;
	virtual void Shutdown() override;

	virtual void Connect() override;
	virtual void BackgroundUpdate() override;
	virtual void SetNewEntity(CEntity* newEntity) override;

	// manage the system
	Bool IsConnected			() const;
	CReviewFlag* CreateNewFlag	();
	Bool AddNewFlag				( String& screenPath );
	Bool ModifyFlag				( CReviewFlag& flag, CReviewFlagComment& comment, Bool makeScreen );

	// filters
	void SetFilters				( const TDynArray<Bool>& states, const TDynArray<Bool>& types );
	void SetSearchFilter		( EReviewSearchType searchType, const String& condition );
	void SetSortingSettings		( EReviewSortCategory category, EMarkerSystemSortOrder order );

	// accessors - set
	void SetDownloadClosedFlags		( Bool download );		// Parameters: true - we will download closed flag, false - we will skip closed flag
	void SetAutoSync				( Bool autosync );
	void SetShowOnMap				( Bool showOnMap );
	void SetSyncTime				( Uint32 time );

	// accessors - get
	Bool							GetDownloadClosedFlags	() const;		// Return: true - we will download closed flag, false - we will skip closed flag
	Bool							GetAutoSync				() const;
	Bool							GetShowOnMap			() const;
	Uint32							GetSyncTime				() const;
	CReviewFlag*					GetNewFlag				() const;
	void							GetFlags				( TDynArray< CReviewFlag* >& filteredFlags );
	CReviewFlag*					GetFlag					( Uint32 number );

	EReviewSearchType				GetSearchType			() const;
	String							GetSearchCondition		() const;

	THandle< CEntityTemplate >		GetFlagTemplate	( enum EReviewFlagState flagState ) const;

	void GetProjectList( TDynArray< String >& projectList );
	void GetMilestoneList( TDynArray< String >& milestoneList );
	void GetPriorityList( TDynArray< String >& priorityList );
	void GetStateList( TDynArray< String >& stateList );
	void GetBugTypeList( TDynArray< String >& bugTypeList );

	void GetStateValues( TDynArray< Bool >& stateValues );
	void GetTypesValues( TDynArray< Bool >& typeValues );

	String GetDefaultProjectName() const;
	String GetDefaultMilestoneName() const;
	void SetDefaultProjectName( const String& projectName );
	void SetDefaultMilestoneName( const String& milestoneName );

	Bool TryToLoadToTTP( const String& user, const String& password );
	const String& GetDBInitError()	{ return m_initializationErrorMessage; }

private:
	void							LoadSettings();
	void							SaveSettings();

	void							OnLoadData();
	void							OnReleaseData();

	void							SynchronizeFlags		();
	void							UpdateData				();

	void							FilterFlags				();
	void							SortFilteredFlags		();

	void							AddEntitiesOnMap		();
	void							RemoveEntietiesFromMap	();

	void							SaveUpdateTimeForMap	( String& mapName);	
	Bool							ImportScreenForComment	( CReviewFlagComment* comment );
	Bool							CreateScreenShot		( CReviewFlag* flag, CReviewFlagComment* comment );

	void							SetInternalFilters		( const TDynArray<Bool>& states, const TDynArray<Bool>& types, EReviewSearchType searchType, const String& condition );

	const String					GetTTProject		();

private:
	Bool							m_downloadClosedFlags;			// true - if we download closed flags from database
	Bool							m_showOnMap;					// on/off visibility for sticker entities
	Bool							m_autoSync;						// on/off auto synchronization for this system
	Bool							m_waitingForEntity;				// true - if we created new sticker and are putting entity for it on map
	Bool							m_filtersAreOn;					// we want to filter the stickers
	Uint32							m_autoSyncTime;					// time between data synchronizations
	String							m_filterCondition;				// condition on which we search flags
	EReviewSearchType				m_filterCategory;				// defines how we search flags
	CTimeCounter					m_lastSyncCounter;				// time counter for synchronization
	CReviewFlag*					m_newFlag;						// temporary variable for new flag
	String							m_ttProject;					// TTProject to log in
	String							m_initializationErrorMessage;	// Error caught during db init

	// other related subsystems
	CReviewDatabaseConnector		m_databaseConnector;	//
	CReviewTestTrackConnector		m_testtrackConnector;	//

	// containers for data
	TDynArray< String >				m_projectList;			// 
	TDynArray<String>				m_flagTypes;			// names of types for flags
	TDynArray<String>				m_flagStates;			// names of states for flags
	TDynArray<String>				m_flagMilestones;		// names of milestone for flags
	TDynArray<String>				m_flagPriorities;		// names of priority for flags
	TDynArray<CReviewFlag*>			m_filteredFlags;		// filtered and sorted flags from m_flags container
	TDynArray<CReviewFlag>			m_flags;				// container for all flags for active map
	TDynArray<Bool>					m_filterStatesValues;	// true is there, where is state of stickers, which we want to see
	TDynArray<Bool>					m_filterTypesValues;	// true is there, where is type of stickers, which we want to see
	EReviewSortCategory				m_category;
	EMarkerSystemSortOrder			m_sortingOrder;

	THandle< CEntityTemplate >		m_flagEntities[RFS_Count];

	// container 
	TDynArray< THandle< CEntity > >	m_entitiesToRemove;
	TDynArray< CReviewFlag* >		m_entitiesToAdd;
};

#endif	// NO_MARKER_SYSTEMS
