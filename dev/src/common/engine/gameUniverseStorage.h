#pragma once

#include "layerStorage.h"

class CExtractLayersTask;
class CUniverseStorage;

class CLayerVisibilityStorage
{
	struct Hash
	{
		Uint64 m_hash;
		RED_INLINE Hash() : m_hash( 0 ) {}
		RED_INLINE Hash( const Hash& other ) : m_hash( other.m_hash ) {}
		RED_INLINE Hash( const String& str ) : m_hash( Red::System::CalculateHash64( UNICODE_TO_ANSI( str.ToLower().AsChar() ) ) ) {}
		RED_INLINE Hash( Uint64 hash ) : m_hash( hash ) {}
		RED_INLINE Bool operator==( const Hash& other ) const { return m_hash == other.m_hash; }
	};

	TDynArray< Hash > m_hidden;
	TDynArray< Hash > m_shown;

public:
	enum EStatus
	{
		LAYER_NotFound,
		LAYER_Hidden,
		LAYER_Shown
	};

	EStatus GetStatus( const String& layer ) const;
	void UpdateStatus( const String& layer, Bool falg );

	void Load( IGameLoader* loader );
	void Save( IGameSaver* saver ) const;

private:
	void Clear();
};

//! Storage for world's state
class CWorldStorage
{
	friend class CUniverseStorage;
	friend class CExtractLayersTask;

	String						m_depotPath;			// Unique world depot path
	IGameDataStorage*			m_storage;				// Static layers in a single storage
	CLayerStorage				m_dynamicLayerStorage;	// Dynamic layer storage

	CLayerVisibilityStorage		m_layerVisibilityStorage;

	CLayerGroup**				m_layerGroups;			// Layer groups array for current capture task (NOT including root)
	Uint32						m_numLayerGroups;		// Number of layer groups for current capture task

	CLayerInfo**				m_layers;				// Layer table for current capture task	(including root group layers and dynamic layer)
	Uint32						m_numLayers;			// Number of elements of the above array

	CExtractLayersTask*			m_extractLayersTask;	// async task for extracting layers
	CParallelForTaskSingleArray< CLayerInfo*, CWorldStorage >::SParams*	m_captureTaskParams;	// control struct for current capture task

	CWorld*						m_worldBeingCaptured;	// world currently being catured
	Uint8						m_currentCaptureStage;	// current stage

public:
	CWorldStorage();
	~CWorldStorage();

	void Reset();

	void Load( IGameLoader* loader );

	Bool LoadPlayerOnly( CWorld* activeWorld, IGameLoader* loader, const IdTag& playerIdTag, const TDynArray< IdTag >& attachments );

	void Save( IGameSaver* saver );

	void CaptureStateStage1( CWorld* world );

	void CaptureStateStage2();

	void CaptureStateStage3();

	void CancelCaptureState();

	void UpdateLayerStorage( CLayerInfo*& output );

	void UpdateVisibilityInfo( const String& path, Bool flag );

	Bool GetLayerGroups( CLayerGroup**& outArray, Uint32& outNumber ); 

	void ApplyState( CWorld* world );

	void OnLayersVisibilityChanged( const TDynArray< String > &groupsToHide, const TDynArray< String > &groupsToShow );

	Bool ShouldLayerGroupBeVisible( const String& path, Bool isVisibleOnStart ) const;
};

class CExtractLayersTask : public CTask
{
	CWorld*						m_world;
	CWorldStorage*				m_storage;

public:
	CExtractLayersTask( CWorld* world );

#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override { return TXT("CExtractLayersTask"); }
	virtual Uint32 GetDebugColor() const override { return COLOR_UINT32( 13, 133, 250 ); }
#endif

	virtual void Run() override;
	void WaitForFinish();
};

//! Basic player storage used for spawning player entity
class CActivePlayerStorage
{
	friend class CUniverseStorage;
private:
	IdTag m_idTag;
	Vector m_position;
	EulerAngles m_rotation;
	String m_templatePath;
	TDynArray< IdTag > m_managedAttachmentsIdTags;
	Bool m_attachmentsEnabled;

	void Reset();
	void Load( IGameLoader* loader );
	void Save( IGameSaver* saver );
	void CaptureState( CPeristentEntity* player );

	void CaptureManagedAttachments( CPeristentEntity* player );
	void RestoreManagedAttachments( CUniverseStorage* universeStorage, CPeristentEntity* player );

public:
	const IdTag& GetIdTag() const { return m_idTag; }
	const Vector& GetPosition() const { return m_position; }
	const EulerAngles& GetRotation() const { return m_rotation; }
	const String& GetTemplatePlath() const { return m_templatePath; }
	void EnableAttachments() { m_attachmentsEnabled = true; }
	void DisableAttachments() { m_attachmentsEnabled = false; }
	Bool AttachmentsEnabled() const { return m_attachmentsEnabled; }
	
};

//! Storage for state of all worlds (aka universe)
class CUniverseStorage
{
private:
	TDynArray< CWorldStorage* > m_worldStorages;		// Storage for all worlds
	CActivePlayerStorage m_playerStorage;				// Active player storage

public:
	CUniverseStorage();
	~CUniverseStorage();
	
	// Resets
	void Reset();

	// Loads whole universe from loader into memory
	void Load( IGameLoader* loader );

	// Loads player data only
	void LoadPlayerOnly( CWorld* activeWorld, IGameLoader* loader );
	
	// Saves whole universe from data in memory to saver
	void Save( IGameSaver* saver );
	
	// Applies in memory world state to world
	void ApplyWorldState( CWorld* world );
	
	// Removes entity permanently from the savegame
	void RemoveEntity( CWorld* world, const IdTag& idTag );
	
	// Sets current/active player id
	void CapturePlayerState( CPeristentEntity* player );
	
	// Gets active player state
	const CActivePlayerStorage& GetActivePlayerStorage() const;

	void CapturePlayerManagedAttachments( CEntity* playerEntity );
	void RestorePlayerManagedAttachments( CEntity* playerEntity );

	void EnablePlayerAttachments();
	void DisablePlayerAttachments();

	// Transfers entity storage data from some layer storage (belonging to dynamic layer) into specific target storage (belonging to current world's dynamic layer); returns entity storage data if found (and transfered), nullptr otherwise
	CLayerStorage::EntityData* TransferEntityData( CLayerStorage* targetStorage, const IdTag& idTag );

	CWorldStorage* GetWorldStorage( const CWorld* world );

	CWorldStorage* GetWorldStorage( const String& world );

	const CWorldStorage* GetWorldStorage( const CWorld* world ) const;

	const CWorldStorage* GetWorldStorage( const String& world ) const;

	void OnLayersVisibilityChanged( const String& world, const TDynArray< String > &groupsToHide, const TDynArray< String > &groupsToShow );

	Bool ShouldLayerGroupBeVisible( const CLayerGroup* group ) const;
};