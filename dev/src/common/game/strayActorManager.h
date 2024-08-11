#pragma once

//////////////////////////////////////////////////////////////////////////
// CStrayActorManager
// This class manages actor that are not linked to an encounter or to a community ( and not the player )
// Such actors can be : Actors summoned by other actors. Or actors spawned by an encounter and stolen by the player ( horses )
// Stray actors will be streamed out when far away from the player and not in sight
class CStrayActorManager : public IGameSystem, IActorTerminationListener, IGameSaveSection
{
	DECLARE_ENGINE_CLASS( CStrayActorManager, IGameSystem, 0 );

public :
	CStrayActorManager();

	// Adds the actor to the stray actor list if he is not there already
	// If the actor is part of an encounter it will be detached from it
	// If the actor implement a guard area behavior it will be removed
	Bool ConvertToStrayActor( CActor* const actor );

	// IGameSystem interface
	virtual void Initialize() override;
	virtual void Shutdown() override;
	virtual void Tick( Float timeDelta ) override;
	virtual void OnGameEnd( const CGameInfo& gameInfo ) override;
	virtual void OnWorldEnd( const CGameInfo& gameInfo ) override;

	// IActorTerminationListener interface
	void OnDetach( CActor* actor ) override;
	void OnDeath( CActor* actor ) override {}

	// IGameSaveSection interface
	virtual void OnGameStart( const CGameInfo& gameInfo ) override;
	virtual bool OnSaveGame( IGameSaver* saver ) override;

	void Load( IGameLoader* loader );

private :
	class CStrayActorData
	{
	public:
		CStrayActorData();
		CStrayActorData( CActor* const actor );
		CStrayActorData( IGameLoader* loader );

		Bool operator==( const CStrayActorData& strayActorData ) const;
		Bool operator==( const CEntity* entity ) const;
		Bool operator!=( const CStrayActorData& strayActorData ) const;

		void Save( IGameSaver* saver ) const;

		EntityHandle	m_entityHandle;
		GameTime		m_registrationTime;
		Int32			m_secondsPast;
	};

	Bool IsTooOld( const CStrayActorData& strayActorData, const GameTime& currentTime, Int32 maxTimeHoursToKeep ) const;
	Bool IsTooFarAway( const CEntity* const entity, Float maxDistanceSq ) const;
	
	TDynArray< CStrayActorData > m_strayActorDataList;

	ASSIGN_GAME_SYSTEM_ID( GS_StrayActorsManager );
};

BEGIN_CLASS_RTTI( CStrayActorManager );
	PARENT_CLASS( IGameSystem );
END_CLASS_RTTI();