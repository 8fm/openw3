#pragma once

#include "../../common/game/aiPositioning.h"
#include "../../common/game/createEntityHelper.h"
#include "../../common/game/createEntityManager.h"


class CR4CreateEntityHelper : public CCreateEntityHelper
{
	DECLARE_RTTI_SIMPLE_CLASS( CR4CreateEntityHelper );
public:
	// Spawn parameters
	struct SpawnInfo
	{
		SpawnInfo();
		~SpawnInfo();

		Vector										m_spawnPos;
		EulerAngles									m_spawnRotation;
		IdTag										m_idTag;
		//CName										m_tag;
		ISpawnEventHandler*							m_eventHandler;
		THandle< CEntityTemplate >					m_template;
		Bool										m_detachTemplate;
		Bool										m_important;
		Uint8										m_entityFlags;
        TDynArray<CName>                            m_tagList;
        Bool                                        m_forceNonStreamed;
	};
protected:
	typedef TSoftHandle< CEntityTemplate >		AsyncEntityHandle;

	SpawnInfo									m_spawnInfo;
	CPositioningFilterRequest::Ptr				m_positioningQuery;
	AsyncEntityHandle							m_entityTemplate;
	Bool										m_isQueryPending;
	Bool										m_isLoadingResource;
	Bool										m_isJobRunning;
	Uint16										m_flags;

public:
	enum EFlags
	{
		FLAG_SPAWN_EVEN_IF_QUERY_FAILS,

		FLAGS_DEFAULT = 0,
	};

	CR4CreateEntityHelper();
	~CR4CreateEntityHelper();

	Bool				AsyncLoadTemplate( const String& fileName );
	void				RunAsyncPositioningQuery( CWorld* world, const CPositioningFilterRequest::Ptr& ptr );

	virtual Bool		Update( CCreateEntityManager* manager ) override;
	virtual void		Discard( CCreateEntityManager* manager ) override;

	void				SetFlags( Uint16 flags )										{ m_flags = flags; }

	SpawnInfo&			GetSpawnInfo()													{ return m_spawnInfo; }
};
BEGIN_CLASS_RTTI( CR4CreateEntityHelper );
	PARENT_CLASS( CCreateEntityHelper );
END_CLASS_RTTI();

class CR4CreateEntityManager : public CCreateEntityManager
{
	class CPlayerVehicleSpawnEventHandler : public ISpawnEventHandler
	{		
	protected:
		void			OnPostAttach( CEntity* entity ) override;
	public :
		static void		LinkVehicleToPlayer( CEntity* entity );
	};
	class CPoolMeBackCallback : public PathLib::IWalkableSpotQueryCallback
	{
	protected:
		void			Callback( PathLib::CWalkableSpotQueryRequest* request ) override;
	};
	class CRunOnPostSpawnCallback : public PathLib::IWalkableSpotQueryCallback
	{
	protected:
		THandle< CEntity >									m_entity;
		CCreateEntityHelper::CScriptSpawnEventHandler*		m_spawnEventHandler;

		void			Callback( PathLib::CWalkableSpotQueryRequest* request ) override;
	public:
		CRunOnPostSpawnCallback( CEntity* entity, CCreateEntityHelper::CScriptSpawnEventHandler* eventHandler );
		~CRunOnPostSpawnCallback();
	};
protected:
	typedef TDynArray< CPositioningFilterRequest::Ptr > RequestsPool;

	RequestsPool				m_requestsPool;
	CPoolMeBackCallback::Ptr	m_poolMeBackCallback;

	void								DefaultPositioningFilter( SPositioningFilter& outFilter );
	CPositioningFilterRequest::Ptr		SetupPositioningQuery( const Vector& pos, const SPositioningFilter& posFilter );

public :
	CR4CreateEntityManager();
	~CR4CreateEntityManager();

	CPositioningFilterRequest::Ptr		RequestPositioningQuery();
	void								ReleasePositioningQuery( CPositioningFilterRequest::Ptr query );

	Bool		SpawnAliasEntityToPosition( CR4CreateEntityHelper *const createEntityHelper, const Matrix &summonMatrix, const String &alias, const IdTag &idTag = IdTag(), TDynArray<CName> tags = TDynArray<CName>(), ISpawnEventHandler *const spawnEventHandler = nullptr, Bool important = false, Bool forceNoStreaming = false );
	Bool		SpawnAliasEntityAroundOwner( CR4CreateEntityHelper *const createEntityHelper, const String &alias, CActor *const horseOwner, const IdTag &horseIdTag = IdTag(), TDynArray<CName> tags = TDynArray<CName>(), ISpawnEventHandler *const spawnEventHandler = nullptr, Bool important = false, Bool noInteriors = false );

	void		SpawnUniqueAliasVehicle( CR4CreateEntityHelper *const createEntityHelper, Bool teleportIfSpawned, const String &alias, const IdTag &idTag = IdTag(), TDynArray<CName> tags = TDynArray<CName>(), const Matrix *inSpawnMatrix = nullptr, Bool noInteriors = true );
	void		SummonPlayerHorse( CR4CreateEntityHelper *const createEntityHelper, Bool teleportIfSpawned, const Matrix *spawnMatrix = nullptr );

	Bool		CreateEntityAsync( CCreateEntityHelper *const createEntityHelper, EntitySpawnInfo && entitySpawnInfo ) override;
};
