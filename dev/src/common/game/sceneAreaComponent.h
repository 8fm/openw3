/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "bgNpcChatPlayer.h"
#include "../engine/triggerAreaComponent.h"


enum ESceneSelectionMode
{
	SceneSelectionMode_Sequential,
	SceneSelectionMode_SequentialWithoutSkipping,
	SceneSelectionMode_Random,
};

BEGIN_ENUM_RTTI( ESceneSelectionMode );
	ENUM_OPTION_DESC( TXT( "Sequential" ),					SceneSelectionMode_Sequential );
	ENUM_OPTION_DESC( TXT( "Sequential without skipping" ),	SceneSelectionMode_SequentialWithoutSkipping );
	ENUM_OPTION_DESC( TXT( "Random" ),						SceneSelectionMode_Random );
END_ENUM_RTTI();

enum ESceneActorType
{
	SceneActorType_NewNpcs,
	SceneActorType_BackgroundNpcs,
};

BEGIN_ENUM_RTTI( ESceneActorType );
	ENUM_OPTION( SceneActorType_NewNpcs );
	ENUM_OPTION( SceneActorType_BackgroundNpcs );
END_ENUM_RTTI();

class CSceneAreaComponentChatPlayer : public CChatPlayer
{
	CScenesTableEntry* m_usedEntry;

public:
	CSceneAreaComponentChatPlayer( CScenesTableEntry & entry ) : m_usedEntry( &entry ) {}

	CScenesTableEntry& GetEntry() { return *m_usedEntry; }
};

/************************************************************************/
/* CScenesTableEntry - single scene that may be started by Scene Area   */
/************************************************************************/
struct CScenesTableEntry
{
	DECLARE_RTTI_STRUCT( CScenesTableEntry );

public:
	TSoftHandle< CStoryScene >	m_sceneFile;		//!< Scene resource
	String						m_sceneInput;		//!< Scene input name
	String						m_requiredFact;		//!< Fact that must exist when starting this scene
	String						m_forbiddenFact;	//!< Fact that can't exist when starting this scene

	EngineTime					m_timeOfUnlocking;	//!< Time at which this entry may be played again

	CScenesTableEntry() : m_timeOfUnlocking( DBL_MIN ) {}

	Bool IsActive() const;
};

BEGIN_CLASS_RTTI( CScenesTableEntry )
	PROPERTY_EDIT( m_sceneFile, TXT("Scene resource") );
	PROPERTY_EDIT( m_sceneInput, TXT("Scene input name") );
	PROPERTY_EDIT( m_requiredFact, TXT("Fact that must exist when starting this scene") );
	PROPERTY_EDIT( m_forbiddenFact, TXT("Fact that can't exist when starting this scene") );
END_CLASS_RTTI();

/************************************************************************/
/* CSceneAreaComponent - area that starts scenes when player is inside  */
/************************************************************************/
class CSceneAreaComponent : public CTriggerAreaComponent
{
	DECLARE_ENGINE_CLASS( CSceneAreaComponent, CTriggerAreaComponent, 0 );

protected:
	ESceneSelectionMode					m_sceneSelectionMode;		//!< Scenes selection mode
	EArbitratorPriorities				m_scenesPriority;			//!< Priority of scenes started via this input
	Float								m_intervalBetweenScenes;	//!< Interval between finishing one scene and starting another
	Uint32								m_maxConcurrentScenes;		//!< How many scenes may be played concurrently
	TDynArray< CScenesTableEntry >		m_scenes;					//!< Scenes that can be started when player is in this area
	ESceneActorType						m_actorsType;
	

	// Runtime data
	EngineTime							m_timeOfNextScene;			//!< Time at which next scene can be started
	Uint32								m_numScenesInProgress;		//!< How many scenes are played now
	Int32									m_lastPlayedScene;			//!< Index of last played scene
	Bool								m_sceneStartIsPending;		//!< Is current scene waiting with starting till being async loaded?
	TDynArray< CWayPointComponent* >	m_dialogsets;				//!< Random positions defined in trigger entity that are selected during scenes
	Bool								m_isInTrigger;
	Bool								m_isInTick;
	TDynArray< CSceneAreaComponentChatPlayer* >	m_chatPlayers;

public:
	CSceneAreaComponent()
		: m_sceneSelectionMode( SceneSelectionMode_Sequential )
		, m_scenesPriority( BTAP_Idle )
		, m_intervalBetweenScenes( 20.f )
		, m_maxConcurrentScenes( 1 )
		, m_actorsType( SceneActorType_NewNpcs )
		, m_isInTrigger( false )
		, m_isInTick( false )
	{
		m_color = Color( 255, 155, 255 );
	}

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	// Something have entered zone
	virtual void EnteredArea( CComponent* component );

	// Something have exited zone
	virtual void ExitedArea( CComponent* component );

	// Main component update
	virtual void OnTick( Float timeDelta );

	Int32 GetNextSceneToPlay( Int32 lastScene );

	void SceneEnded( CScenesTableEntry & entry );

	Bool IsRaining();

private:
	Bool ValidateNPCsPosition( const CStorySceneInput * input );
	Bool StartSceneForNewNpcsActors( const CStorySceneInput * input, CScenesTableEntry & entry );
	Bool StartSceneForBackgroundNpcsActors( const CStorySceneInput * input, CScenesTableEntry & entry );

	void UpdateChatPlayers( Float dt );

	Bool CanRemoveFromTick() const;
	void RemoveFromTick();
};

BEGIN_CLASS_RTTI( CSceneAreaComponent );
	PARENT_CLASS( CTriggerAreaComponent );
	PROPERTY_EDIT( m_sceneSelectionMode, TXT("Scenes selection mode") );
	PROPERTY_EDIT( m_scenesPriority, TXT("Priority of scenes started via this input") );
	PROPERTY_EDIT( m_intervalBetweenScenes, TXT("Interval between finishing one scene and starting another (seconds)") );
	PROPERTY_EDIT( m_maxConcurrentScenes, TXT("How many scenes may be played concurrently") );
	PROPERTY_EDIT( m_scenes, TXT("Scenes that can be started when player is in this area") );
	PROPERTY_EDIT( m_actorsType, TXT("") );
END_CLASS_RTTI();
