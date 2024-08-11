
#pragma once

#include "storySceneIncludes.h"
#include "storySceneLookAtController.h"
#include "storySceneActorsEyesTracker.h"
#include "..\engine\behaviorGraphAnimationMixerSlot.h"

class CStoryScenePlayer;
class CStorySceneEventsCollector;
class IStorySceneDebugger;

struct SStorySceneGameplayActionCallbackInfo
{
	DECLARE_RTTI_STRUCT( SStorySceneGameplayActionCallbackInfo )

	Bool	m_outChangeItems;
	Bool	m_outDontUseSceneTeleport;

	Vector	m_inActorPosition;
	Vector	m_inActorHeading;
	Int32	m_inGameplayAction;
	THandle< CActor > m_inActor;

	SStorySceneGameplayActionCallbackInfo()
		: m_outChangeItems( false )
		, m_outDontUseSceneTeleport( false )
		, m_inActorPosition( Vector::ZERO_3D_POINT )
		, m_inActorHeading( Vector::EY )
		, m_inGameplayAction( 0 )
	{}
};

BEGIN_CLASS_RTTI( SStorySceneGameplayActionCallbackInfo );
	PROPERTY( m_outChangeItems );
	PROPERTY( m_outDontUseSceneTeleport );
	PROPERTY( m_inActorPosition );
	PROPERTY( m_inActorHeading );
	PROPERTY( m_inGameplayAction );
	PROPERTY( m_inActor );
END_CLASS_RTTI();

class CStorySceneEventsExecutor
{
	static const CName&	MIXER_SLOT_NAME_CAMERA;
	static const CName&	MIXER_SLOT_NAME_BODY;
	static const CName&	MIXER_SLOT_NAME_BODY_GAMEPLAY;
	static const CName&	MIXER_SLOT_NAME_MIMICS_LAYER_EYES;
	static const CName&	MIXER_SLOT_NAME_MIMICS_LAYER_POSE;
	static const CName&	MIXER_SLOT_NAME_MIMICS_LAYER_ANIM;
	static const CName&	MIXER_SLOT_NAME_MIMICS_LAYER_OVERRIDE;

	template< typename T >
	class ActorSignalCollector
	{
	public:
		TDynArray< CEntity* >	m_actors;
		TDynArray< T >			m_signals;

		void Add( CEntity* entity, const T& signal )
		{
			const Uint32 size = m_actors.Size();
			SCENE_ASSERT( size == m_signals.Size() );

			for ( Uint32 i=0; i<size; ++i )
			{
				if ( m_actors[ i ] == entity )
				{
					if ( signal > m_signals[ i ] )
					{
						m_signals[ i ] = signal;
					}
				}
			}

			m_actors.PushBack( entity );
			m_signals.PushBack( signal );
		}

		void Set( CEntity* entity, const T& signal )
		{
			const Uint32 size = m_actors.Size();
			SCENE_ASSERT( size == m_signals.Size() );

			for ( Uint32 i=0; i<size; ++i )
			{
				if ( m_actors[ i ] == entity )
				{
					m_signals[ i ] = signal;
				}
			}

			m_actors.PushBack( entity );
			m_signals.PushBack( signal );
		}

		Bool Get( CEntity* entity, T& val ) const
		{
			const Uint32 size = m_actors.Size();
			SCENE_ASSERT( size == m_signals.Size() );

			for ( Uint32 i=0; i<size; ++i )
			{
				if ( m_actors[ i ] == entity )
				{
					val = m_signals[ i ];
					return true;
				}
			}
			return false;
		}
	};

	template< typename K, typename T >
	struct ActorCachedData
	{
		K	m_id;
		T	m_data;
	};

	struct ActorAnimState
	{
		SStorySceneActorAnimationState	m_statePrev;
		SStorySceneActorAnimationState	m_stateCurr;

		CName							m_animationBodyNamePrev;
		CName							m_animationBodyNameCurr;
		Float							m_bodyWeight;

		CName							m_animationMimicsEyesNamePrev;
		CName							m_animationMimicsEyesNameCurr;
		CName							m_animationMimicsPoseNamePrev;
		CName							m_animationMimicsPoseNameCurr;
		CName							m_animationMimicsAnimNamePrev;
		CName							m_animationMimicsAnimNameCurr;
		Float							m_mimicsPoseWeightPrev;
		Float							m_mimicsPoseWeightCurr;
		Float							m_mimicsWeight;

		CName							m_lookAtBodyAnimationPrev;
		CName							m_lookAtHeadAnimationPrev;
		CName							m_lookAtBodyAnimationCurr;
		CName							m_lookAtHeadAnimationCurr;

		CGUID							m_prevID;
		CGUID							m_currID;

		CGUID							m_prevMimicID;
		CGUID							m_currMimicID;

		ActorAnimState() : m_bodyWeight( 1.f ), m_mimicsWeight( 1.f ), m_mimicsPoseWeightPrev( 1.f ), m_mimicsPoseWeightCurr( 1.f ), m_prevID( CGUID::ZERO ), m_currID( CGUID::ZERO ) {}
	};

	struct ActorEffectState
	{
		Bool operator == ( const ActorEffectState& rhs ) const
		{
			return	m_name == rhs.m_name &&
					m_persistAcrossSections == rhs.m_persistAcrossSections;
		}

		CName	m_name;
		Bool	m_persistAcrossSections;
	};

	struct LightPropState
	{
		SStorySceneAttachmentInfo  m_attachment;
		EngineTransform            m_transform;
	};

	struct ActorPlacement
	{
		Matrix							m_placementWS;
		Float							m_timeStampAbsEvt;
		Int32							m_timeStampAbsAcc;
		Float							m_timeStampAbsAccVal;
		Bool							m_dontTeleportMe;
		Bool							m_switchedToGameplayMode;
		Bool							m_switchedToGameplayModeDone;

		ActorPlacement() : m_placementWS( Matrix::IDENTITY ), m_timeStampAbsEvt( 0.f ), m_timeStampAbsAccVal( 0.f ), m_timeStampAbsAcc( 0 ), m_dontTeleportMe( false ), m_switchedToGameplayMode( false ), m_switchedToGameplayModeDone( false ) {}
	};

	struct CameraAnimState
	{
		CName							m_animationNameCurr;
		CName							m_animationNamePrev;
		Float							m_animationWeightCurr;
		Float							m_animationWeightPrev;
		Float							m_blendWeight;

		CameraAnimState() : m_animationWeightCurr( 1.f ), m_animationWeightPrev( 1.f ), m_blendWeight( 1.f ) {}
	};

	struct SDoorStateChange
	{
		Bool operator==( const SDoorStateChange& other ) const
		{
			return m_doorTag == other.m_doorTag && m_doorComponent.GetConst() == other.m_doorComponent.GetConst();
		}

		CName								m_doorTag;
		THandle<CDoorComponent>				m_doorComponent;
		Bool								m_opened;
	};

	struct SSynchronizedItem
	{
		SSynchronizedItem() : m_isActive( false ) {}

		Bool							m_isActive;
		CName							m_parentActor;
		CName							m_itemName;
		CBehaviorMixerSlotInterface		m_itemMixer;
	};

	typedef ActorCachedData< CEntity*, LightPropState >					TLightPropState;
	typedef TDynArray< TLightPropState >								TLightsPropStates;

	typedef ActorCachedData< CEntity*, ActorPlacement >					TActorPlacement;
	typedef TDynArray< TActorPlacement >								TActorsPlacement;

	typedef ActorCachedData< CEntity*, Matrix >							TActorTransform;
	typedef TDynArray< TActorTransform >								TActorsTransform;

	typedef ActorCachedData< CEntity*, ActorAnimState >					TActorAnimState;
	typedef TDynArray< TActorAnimState>									TActorsAnimStates;

	typedef ActorCachedData< CName, CBehaviorMixerSlotInterface >		TActorMixer;
	typedef TDynArray< TActorMixer >									TActorsMixers;

	typedef ActorCachedData< CEntity*, CStorySceneLookAtController >	TActorLookAt;
	typedef TDynArray< TActorLookAt >									TActorLookAts;

	typedef ActorCachedData< CEntity*, CStorySceneActorsEyesTracker >	TActorsEyesTracker;
	typedef TDynArray< TActorsEyesTracker >								TActorsEyesTrackers;

	typedef ActorCachedData< CEntity*, TDynArray< ActorEffectState > >	TActorActiveEffects;
	typedef TDynArray< TActorActiveEffects >							TActorsEffects;

	typedef ActorCachedData< CEntity*, TDynArray< SItemUniqueId > >		TActorHiddenItems;
	typedef TDynArray< TActorHiddenItems >								TActorsHiddenItems;

	typedef TDynArray< SDoorStateChange >								TDoorStateChanges;

	typedef TDynArray< TPair< CName, SItemUniqueId > >					TSpawnedItems;

	typedef ActorSignalCollector< Float >								TActorSignalsDisableLookAtBLP;
	typedef ActorSignalCollector< SAnimationMixerVisualState >			TActorSignalsMixerPrevState;

private:
	TActorsMixers								m_actorsBodyMixers;
	TActorsMixers								m_actorsGmplBodyMixers;
	TActorsMixers								m_actorsMimicsMixers[ DML_Last ];
	TActorsAnimStates							m_actorsAnimStates;
	TActorsPlacement							m_actorsPlacements;
	TActorsTransform							m_actorsTransforms;
	CBehaviorMixerSlotInterface					m_cameraMixer;
	CameraAnimState								m_cameraAnimState;
	TActorLookAts								m_actorsLookAts;
	TActorsEyesTrackers							m_actorsEyesTrackers;
	TActorsEffects								m_activeEffects;
	TLightsPropStates							m_lightsStates;
	TActorsHiddenItems							m_hiddenItems;
	TDoorStateChanges							m_changedDoors;
	TSpawnedItems								m_spawnedItems;
	TEnvManagerAreaEnvId						m_changedEnvs;
	TEnvManagerAreaEnvId						m_blendOutEnv;
	SSynchronizedItem							m_syncItem;

	const THashMap< CName, THandle< CEntity > >* m_actorsMap; // TODO remove this THashMap
	const THashMap< CName, THandle< CEntity > >* m_propsMap; 
	const THashMap< CName, THandle< CEntity > >* m_lightsMap; 

	Int32										m_updateID;

public:
	CStorySceneEventsExecutor();

	void Init( const THashMap< CName, THandle< CEntity > >* actorMap, const THashMap< CName, THandle< CEntity > >* propMap, const THashMap< CName, THandle< CEntity > >* lightMap, CCamera* camera );
	void Deinit( CStoryScenePlayer* player );

	void Execute( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );

	void ForceResetForAllEntities( const CStorySceneDialogsetInstance* dialogset, Bool forceDialogset, Bool isGameplay, Bool wasGameplay, Bool isCutscene, Bool wasCutscene, CStoryScenePlayer* player, IStorySceneDebugger* debugger );
	void DeactivateCustomEnv( CStoryScenePlayer* player, Float blendTime = 0.f );

	void OnAppearanceChanged( const CStoryScenePlayer* player, const CName& actorID );

	Bool IsEntityPositionControlledByScene( CEntity* entity );

	void RegisterSpawnedItems( TDynArray< TPair< CName, SItemUniqueId > >& spawnedItems ) { m_spawnedItems.PushBack( spawnedItems ); }
	void RegisterSpawnedItem( TPair< CName, SItemUniqueId > spawnedItem ){ m_spawnedItems.PushBack( spawnedItem ); }
public:
	// DIALOG_TOMSIN_TODO - to tez editor only
	Bool GetCurrentActorIdleAnimation( const CName& actor, CName& out ) const;

#ifndef NO_EDITOR
	Bool GetCurrentActorState( const CName& actor, SStorySceneActorAnimationState& out ) const;
	Bool GetPreviousActorState( const CName& actor, SStorySceneActorAnimationState& out ) const;
	Bool GetCurrentActorAnimationMimicState( CName actor, CName& mimicEmoState, CName& mimicLayerEyes, CName& mimicLayerPose, CName& mimicLayerAnim, Float& poseWeight ) const;
	Bool GetCurrentLightState( CName lightId, SStorySceneAttachmentInfo& out, EngineTransform& outPos ) const;
	void GetActorAnimationNames( const CName& actor, TDynArray< CName >& out ) const;
	Bool GetActorCurrIdleAnimationNameAndTime( CName actorId, CName& animName, Float& animTime ) const;
	Bool GetActorCurrAnimationTime( CName actorId, CName animName, Float& animTime ) const;
#endif

private:
	void ForceResetActorState( const CName& actorId, CEntity* entity, const CStorySceneDialogsetInstance* dialogset, Bool forceDialogset, Bool isGameplay, Bool wasGameplay, Bool isCutscene, Bool wasCutscene, CStoryScenePlayer* player, CStorySceneEventsCollector& collector );
	void ForceResetPropState( const CName& propId, CEntity* e, const CStorySceneDialogsetInstance* dialogset, Bool forceDialogset, CStoryScenePlayer* player, CStorySceneEventsCollector& collector );
	void ResetActiveEffectsOnEnt( CName id, CEntity* ent, CStorySceneEventsCollector& collector  );
	
	TActorMixer* CreateActorBodyMixer( const CName& mixerName, TActorsMixers& mixers, const CName& actorId, CEntity* actor );
	TActorMixer* CreateActorBodyMixer( const CName& actorId, CEntity* actor );
	TActorMixer* CreateActorMimicsMixer( const CName& actorId, CEntity* actor, EDialogMimicsLayer layer );
	TActorMixer* FindActorBodyMixer( const CName& actorId, const CEntity* e );
	void ClearSlotsForActor( const CName& actorId );
	void ClearAllSlots( TActorsMixers& mixers );
	void ClearAllSlots();

	Bool HasActorSceneBehavior( const CEntity* e ) const;
	THandle< CEntity > FindActorByType( CName actorID, Int32 type ) const ; 

private:
	void OpenSyncItemMixer( CName actorId );
	void CloseSyncItemMixers();

	void SetSyncItemIdleAnimToSample( CName actorId, const SAnimationFullState& animationA, const SAnimationFullState& animationB, Float blendWeight, Bool canRandAnimStartTime = false, CAnimationMixerAnimSynchronizer* synchronizer = nullptr );
	void AddSyncItemAnimToSample( CName actorId, const SAnimationFullState& animation, EAnimationType type );

	void RemoveSyncItemPose( CName actorId, Uint32 poseId );
	void RemoveAllSyncItemPoses( CName actorId );
	void AddSyncItemPoseToSample( CName actorId, Uint32 poseId, const SAnimationMappedPose& pose );
	
	Bool IsSyncItemValid( CName actorId );
private:
	Bool FillActorsPlacements( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	Bool FillActorsMotions( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessActorsTransforms( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessBodyAnimations( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, const CStorySceneAnimationContainer& c, IStorySceneDebugger* debugger, TActorSignalsDisableLookAtBLP& signalsDisableLookAtBLP );
	void ProcessMimicsAnimations( const CStorySceneEventsCollector& collector, const CStorySceneAnimationContainer& c, IStorySceneDebugger* debugger );
	void ProcessChangeStates( const CStorySceneEventsCollector& collector, const CStorySceneAnimationContainer& c, IStorySceneDebugger* debugger );
	Bool ProcessChangeGameStates( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector );
	void ProcessCameras( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessVisibility( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessAppearances( const CStorySceneEventsCollector& collector );
	void ProcessItems( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessMorph( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessCloth( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessPlayDialogLineEvents( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessActorsLookAts_Pre( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger, TActorSignalsMixerPrevState& signalsMixerPrevState );
	void ProcessActorsLookAts_Post( const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger, const TActorSignalsDisableLookAtBLP& signalsDisableLookAtBLP, const TActorSignalsMixerPrevState& signalsMixerPrevState );
	void ProcessActorsLookAts_ResampleHeadBone( CEntity* target, Int32 headBoneIdx, TActorMixer* mixerTarget, const TActorSignalsMixerPrevState& signalsMixerPrevState );
	void ProcessPropsTransforms( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessLightProperties( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector );
	void ProcessFade( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessPropsAttachEvents( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessUnfrozenActors();
	void ProcessEffects( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessDebugComments( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessActorsPrepareToAndFinishTalk( const CStorySceneEventsCollector& collector );
	void ProcessTimeModifiers( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void SpawnPropsOnDemand( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessDoorsEvents( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcessEnvChanges( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );	
	void ProcessHiresShadows( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector );
	void ProcessMimicLod( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector );
	void ProcessActorLodOverrides( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector );
	void OpenMixers( TActorsMixers& mixers );
	void CloseMixers( TActorsMixers& mixers );
	void ForceResetWithRelaxedClothState( CActor* a, Bool cloth, Bool dangle );
	void ProcessVideoOverlay( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
	void ProcesItemSync( CStoryScenePlayer* player, const CStorySceneEventsCollector& collector, IStorySceneDebugger* debugger );
private:
	template< typename K, typename T >
	ActorCachedData< K, T >* FindActorCachedData( const K& id, TDynArray< ActorCachedData< K, T > >& dataArr )
	{
		const Uint32 size = dataArr.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const K& k = dataArr[ i ].m_id;
			if ( k == id )
			{
				return &(dataArr[ i ]);
			}
		}

		return nullptr;
	}

	template< typename K, typename T >
	const ActorCachedData< K, T >* FindActorCachedData( const K& id, const TDynArray< ActorCachedData< K, T > >& dataArr ) const
	{
		const Uint32 size = dataArr.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const K& k = dataArr[ i ].m_id;
			if ( k == id )
			{
				return &(dataArr[ i ]);
			}
		}

		return nullptr;
	}

	template< typename K, typename T >
	ActorCachedData< K, T >* CreateActorCachedData( const K& id, TDynArray< ActorCachedData< K, T > >& dataArr )
	{
		const Uint32 index = (Uint32)dataArr.Grow( 1 );
		ActorCachedData< K, T >& t = dataArr[ index ];
		t.m_id = id;
		t.m_data = T();

		return &t;
	}

	template< typename K, typename T >
	ActorCachedData< K, T >* FindOrCreateActorCachedData( const K& id, TDynArray< ActorCachedData< K, T > >& dataArr )
	{
		ActorCachedData< K, T >* t = FindActorCachedData( id, dataArr );
		if ( !t )
		{
			t = CreateActorCachedData( id, dataArr );
		}
		return t;
	}

	template< typename K, typename T >
	ActorCachedData< K, T >* CreateActorCachedDataAndInit( const K& id, TDynArray< ActorCachedData< K, T > >& dataArr )
	{
		const Uint32 index = (Uint32)dataArr.Grow( 1 );
		ActorCachedData< K, T >& t = dataArr[ index ];
		t.m_id = id;
		t.m_data = T();
		t.m_data.Init( id );

		return &t;
	}
};
