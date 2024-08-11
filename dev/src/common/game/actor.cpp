/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "actor.h"

#include "../core/gameSave.h"
#include "../core/gatheredResource.h"
#include "../core/scriptingSystem.h"
#include "../core/feedback.h"

#include "../engine/animatedIterators.h"
#include "../engine/animGlobalParam.h"
#include "../engine/animSlotsParam.h"
#include "../engine/behaviorGraphAnimationSlotNode.h"
#include "../engine/behaviorGraphNotifier.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/characterControllerParam.h"
#include "../engine/soundEntityParam.h"
#include "../engine/soundStartData.h"
#include "../engine/appearanceComponent.h"
#include "../engine/clipMap.h"
#include "../engine/morphedMeshManagerComponent.h"
#include "../engine/renderCommands.h"
#include "../engine/physicsCharacterWrapper.h"
#include "../engine/physicsCharacterVirtualController.h"
#include "../engine/localizationManager.h"
#include "../engine/mimicComponent.h"
#include "../engine/animationManager.h"
#include "../engine/visualDebug.h"
#include "../engine/tickManager.h"
#include "../engine/soundSystem.h"

#include "actorsManager.h"
#include "actorSpeech.h"
#include "actorAnimationEventFilter.h"
#include "aiHistory.h"
#include "aiParameters.h"
#include "aiParametersSpawnList.h"
#include "aiParamInjectHandler.h"
#include "aiPositionPrediction.h"
#include "aiPresetParam.h"
#include "aiRedefinitionParameters.h"
#include "attackRange.h"
#include "baseDamage.h"
#include "behTree.h"
#include "behTreeMachine.h"
#include "behTreeNode.h"
#include "behTreeNodePlayScene.h"
#include "behTreeInstance.h"
#include "behTreeDynamicNodeEvent.h"
#include "behTreeScriptedNode.h"
#include "behTreeNodeArbitrator.h"
#include "commonGame.h"
#include "nodeStorage.h"
#include "characterStats.h"
#include "definitionsManager.h"
#include "entityParams.h"
#include "factsDB.h"
#include "headManagerComponent.h"
#include "inventoryComponent.h"
#include "movableRepresentationPhysicalCharacter.h"
#include "movingAgentComponent.h"
#include "movingPhysicalAgentComponent.h"
#include "movingPhysicalAgentComponent.h"
#include "reactionsManager.h"
#include "wayPointComponent.h"
#include "questsSystem.h"
#include "questExternalScenePlayer.h"
#include "storySceneComponent.h"
#include "storySceneInput.h"
#include "storySceneVoicetagMapping.h"
#include "storyScenePlayer.h"
#include "storySceneSystem.h"
#include "storySceneActorMap.h"
#include "voicesetPlayer.h"
#include "behTreeWorkData.h"
#include "reactionSceneActor.h"
#include "strayActorManager.h"
#include "movableRepresentationPathAgent.h"
#include "encounter.h"
#include "entityPool.h"
#include "storySceneIncludes.h"
#include "itemIterator.h"


RED_DEFINE_STATIC_NAME(IsDodging);
RED_DEFINE_STATIC_NAME( Health );
RED_DEFINE_STATIC_NAME( dynamicTemplate );
RED_DEFINE_STATIC_NAME( OnPushEffects );
RED_DEFINE_STATIC_NAME( DrunkItem );
RED_DEFINE_STATIC_NAME( InjuredItem );
RED_DEFINE_STATIC_NAME( SneakItem );
RED_DEFINE_STATIC_NAME( Torch );
RED_DEFINE_STATIC_NAME( opponent_weapon );
RED_DEFINE_STATIC_NAME( opponent_bow );
RED_DEFINE_STATIC_NAME( ex_push_f );
RED_DEFINE_STATIC_NAME( ex_push_b );
RED_DEFINE_STATIC_NAMED_NAME( Pick_02, "Pick 02" );
RED_DEFINE_STATIC_NAME( OnProcessRequiredItemsFinish )


IMPLEMENT_RTTI_ENUM( EPushingDirection );
IMPLEMENT_RTTI_ENUM( EExplorationState );
IMPLEMENT_RTTI_ENUM( EPathEngineAgentType );
IMPLEMENT_RTTI_ENUM( EAIEventType );
IMPLEMENT_ENGINE_CLASS( CActor );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
RED_DISABLE_WARNING_MSC( 4355 )
// ACHTUNG!!! 'this' points to not fully constructed object

CActor::CActor()
	: m_agentComponent( NULL )
	, m_morphedMeshManagerComponent( NULL )
	, m_actorGroups( PEAT_TallNPCs )
	, m_action( NULL )
	, m_actionMoveTo( this )
	, m_actionRotateTo( this )
	, m_actionAnimation( this )
	, m_actionRaiseEvent( this )
	, m_actionSlideTo( this )
	, m_actionMoveOnCurveTo( this )
	, m_actionWork( this )
	, m_actionExploration( this )
	, m_actionMoveToDynamicNode( this )
	, m_actionAnimatedSlideTo( this )
	, m_actionCustomSteer( this )
	, m_actionMatchTo( this )
	, m_actionMoveOutsideNavdata( this )
	, m_latentActionType( ActorAction_None )
	, m_latentActionResult( ActorActionResult_InProgress )
	, m_latentActionIndex( 0 )
	, m_alive( true )
	, m_externalControl( false )
	, m_speech( NULL )
	, m_isInNonGameplayScene( false )
#ifndef NO_ERROR_STATE
	, m_dbgIsErrorState( false )
	, m_dbgErrorState( String::EMPTY )
#endif
	, m_hasInteractionVoicesetScene( false )
	, m_isAIControlled( false )
	, m_isAttackableByPlayer( true )
	, m_attackTarget( NULL )
	, m_attackTargetSetTime( 0.0f )
	, m_currentAttitudeGroup( CName::NONE )
	, m_baseAttitudeGroup( CName::NONE )
	, m_mimicComponent( NULL )
	, m_actorAnimState( AAS_Default )
	, m_cachedDisplayName( String::EMPTY )
	, m_movementType( EX_Normal )
	, m_behTreeMachine( nullptr )
	, m_animationEventFilter( nullptr )
	, m_aimOffset( 0 )
	, m_barOffset( 0.40f )
	, m_frontPushAnim( CNAME( ex_push_f ) )
	, m_backPushAnim( CNAME( ex_push_b ) )
	, m_pushController( NULL )
	, m_cachedSoundParams( NULL )
	, m_lockedByScene( false )
	, m_isUsingExploration( false )
	, m_usesRobe( false )
	, m_isCollidable( true )
	, m_isVisibileFromFar( false )
	, m_useHiResShadows( false )
	, m_useGroupShadows( true )
	, m_isInFFMiniGame( false )
	, m_isLookAtLevelForced( false )
	, m_forcedLookAtLevel( LL_Body )
	, m_useAnimationEventFilter( false )
	, m_losTestBoneIndex( -1 )
	, m_torsoBoneIndex( -1 )
	, m_headBoneIndex( -1 )
	, m_attackableByPlayerDisabledTime( EngineTime::ZERO )
	, m_voicesetPlayer( NULL )
	, m_pelvisBoneName( CNAME( pelvis ) )
	, m_torsoBoneName( CNAME( torso2 ) )
	, m_headBoneName( CNAME( head ) )
	, m_entityGroup( NULL )
	, m_appearanceChanged( false )
	, m_forceAppearanceSoundUpdate( false )
#ifndef NO_EDITOR
	, m_debugAttackRange(NULL)
	, m_debugEnableTraceDraw(true)
#endif
	, m_encounterGroupUsedToSpawn( -1 )
	, m_freezeOnFailedPhysicalRequest( false )
{}

CActor::~CActor()
{
	if(!m_soundListenerOverride.Empty())
	{
		GSoundSystem->UnregisterListenerOverride(m_soundListenerOverride);
	}
}

void CActor::SetHiResShadows( Bool flag )
{
	if ( flag != m_useHiResShadows )
	{
		m_useHiResShadows = flag;

		// Propagate to renderer
		if ( m_entityGroup != NULL )
		{
			( new CRenderCommand_SetEntityGroupHiResShadows( m_entityGroup, flag ) )->Commit();
		}
	}
}

void CActor::SetGroupShadows( Bool flag )
{
	if ( flag != m_useGroupShadows )
	{
		m_useGroupShadows = flag;

		// Propagate to renderer
		if ( m_entityGroup != NULL )
		{
			( new CRenderCommand_SetEntityGroupShadows( m_entityGroup, flag ) )->Commit();
		}
	}
}

Bool CActor::IsVisibleFromFar() const
{
	return m_isVisibileFromFar;
}

void CActor::SetIsVisibleFromFar( Bool state )
{
	m_isVisibileFromFar = state;
}

#ifndef NO_EDITOR
Bool CActor::SetAndCheckHiResShadows( Bool flag )
{
	if ( flag != m_useHiResShadows )
	{
		m_useHiResShadows = flag;

		// Propagate to renderer
		if ( m_entityGroup != NULL )
		{
			( new CRenderCommand_SetEntityGroupHiResShadows( m_entityGroup, flag ) )->Commit();
			return true;
		}
	}

	return false;
}
#endif

Bool CActor::UseHiResShadows() const
{
	return m_useHiResShadows;
}

CAnimatedComponent* CActor::GetRootAnimatedComponent() const
{
	CAnimatedComponent* ac = GetMovingAgentComponent();
	if( ac )
	{
		return ac;
	}
	else
	{
		return TBaseClass::GetRootAnimatedComponent();
	}
}

CMovingAgentComponent* CActor::FindMovingAgentComponent() const
{
	// Linear search (it should be first either way)
	for ( Uint32 i=0; i<m_components.Size(); i++ )
	{
		CMovingAgentComponent* mc = Cast< CMovingAgentComponent >( m_components[i] );
		if ( mc )
		{
			m_agentComponent = mc;
			return mc;
		}
	}

	// Not found
	return NULL;
}

CMovingAgentComponent* CActor::GetMovingAgentComponent() const
{
	if ( !m_agentComponent )
	{
		return FindMovingAgentComponent();
	}

	// Return cached shit
	return m_agentComponent;
}

CMorphedMeshManagerComponent* CActor::FindMorphedMeshManagerComponent() const
{
	// Linear search (it should be first either way)
	for ( Uint32 i=0; i<m_components.Size(); i++ )
	{
		if ( CMorphedMeshManagerComponent* mmmc = Cast< CMorphedMeshManagerComponent >( m_components[i] ) )
		{
			m_morphedMeshManagerComponent = mmmc;
			return mmmc;
		}
	}

	// Not found
	return NULL;
}

CMorphedMeshManagerComponent* CActor::GetMorphedMeshManagerComponent() const
{
	if ( !m_morphedMeshManagerComponent )
	{
		return FindMorphedMeshManagerComponent();
	}

	// Return cached
	return m_morphedMeshManagerComponent;
}

void CActor::SetAttackableByPlayerRuntime( Bool flag, Float timeout )
{
	if( flag )
	{
		m_attackableByPlayerDisabledTime = EngineTime::ZERO;
	}
	else
	{
		m_attackableByPlayerDisabledTime = GGame->GetEngineTime() + timeout;
	}
}

void CActor::SetExternalControl( Bool flag )
{
	if( flag )
	{
		AI_EVENT_START( this, EAIE_Misc, TXT( "SetExternalControl" ), String::Printf( TXT( "Relinquishing control over actor %s" ), GetName().AsChar() ) );

		// Stop behavior tree machine
		CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
		if( behTreeMachine )
		{
			behTreeMachine->Stop();
		}

		// Cancel action
		ActionCancelAll();

		// Clear rotation target
		ClearRotationTarget();

		// End and kill state
		KillCurrentState();
		KillThread();

		// Block state machine
		BlockStateMachine( true );
	}
	else
	{
		AI_EVENT_END( this, EAIE_Misc, EAIR_Success );

		CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
		if( behTreeMachine )
		{
			behTreeMachine->Restart();
		}

		// Unblock state machine
		BlockStateMachine( false );
	}

	m_externalControl = flag;
}

CMovingAgentComponent* CActor::GetAgent() const
{
	return GetMovingAgentComponent();
}

Float CActor::GetRadius() const
{
	CMovingAgentComponent* agent = GetMovingAgentComponent();
	if( agent )
	{
		return agent->GetRadius();
	}

	return 1.0f;
}

Bool CActor::CanStealOtherActor( const CActor* const other ) const
{
	CEncounter* myEncounter = nullptr;
	for ( Uint32 i = 0; i < m_terminationListeners.Size(); ++i  )
	{
		myEncounter = m_terminationListeners[i]->AsEncounter();
		if ( myEncounter )
		{
			break;
		}
	}
	
	return !myEncounter || !other->m_terminationListeners.Exist( myEncounter );
}

Bool CActor::IsMoving() const
{
	return ( m_action && ( m_action->GetType() == ActorAction_Moving || ( m_action->GetType() == ActorAction_DynamicMoving ) || ( m_action->GetType() == ActorAction_MovingOutNavdata ) ) );
}

Bool CActor::IsWorking() const
{
	return ( m_action && ( m_action->GetType() == ActorAction_Working ) );
}

Bool CActor::IsInQuestScene() const
{
	return false;
}

void CActor::SetHideInGame( Bool hideInGame, Bool immediate, EHideReason hideReason )
{
	Bool stateChanged;
	if ( hideInGame )
	{
		stateChanged = m_hideReason == 0;
		m_hideReason |= hideReason;
	}
	else
	{
		auto lastVal = m_hideReason;
		m_hideReason &= ~hideReason;
		stateChanged = ( m_hideReason == 0 && lastVal != 0 );

		// HACK! Unfortunately, Drawable component's transform is only updated when it has render proxy. So there can be a situation when,
		// entity is moving, but drawable component is 'staying' in original place.
		// Setting this flag will cause to immediately update drawable component transform after render proxy is created.
		for( EntityWithItemsComponentIterator< CDrawableComponent > it( this ); it; ++it )
		{
			CDrawableComponent* dc = *it;
			dc->SetMissedUpdateTransformFlag();
		}
	}
	if ( stateChanged )
	{
		TBaseClass::SetHideInGame( hideInGame, immediate, hideReason );
	}
}

const Vector& CActor::GetMoveDestination() const
{
	if( IsMoving() )
	{
		return m_actionMoveTo.GetTarget();
	}
	else
	{
		return Vector::ZEROS;
	}
}

Vector CActor::GetPositionOrMoveDestination() const
{
	if( IsMoving() )
	{
		return m_actionMoveTo.GetTarget();
	}
	else
	{
		return GetWorldPosition();
	}
}

Vector CActor::GetBarPosition() const
{
	if ( GetHeadBone() != -1 )
	{
		return GetHeadPosition() + Vector3( 0, 0, m_barOffset );
	}
	else
	{
		return GetTorsoPosition() + Vector3( 0, 0, m_barOffset );
	}
}

Bool CActor::IsInCombat() const
{
	if ( m_behTreeMachine )
	{
		CBehTreeInstance* instance = m_behTreeMachine->GetBehTreeInstance();
		if ( instance )
		{
			return instance->IsInCombat();
		}
	}
	return false;
}

Vector CActor::GetLOSTestPosition() const
{
	const CAnimatedComponent* animated = GetRootAnimatedComponent();
	if( m_losTestBoneIndex != -1 && animated )
	{
		return animated->GetBoneMatrixWorldSpace( m_losTestBoneIndex ).GetTranslation();
	}
	else
	{
		Vector pos = GetWorldPosition();
		pos.Z += 0.5f;
		return pos;
	}
}

Vector CActor::GetTorsoPosition() const
{
	CAnimatedComponent* animated = GetRootAnimatedComponent();
	if( m_torsoBoneIndex != -1 && animated )
	{
		return animated->GetBoneMatrixWorldSpace( m_torsoBoneIndex ).GetTranslation();
	}
	else
	{
		Vector pos = GetWorldPosition();
		pos.Z += 1.5f;
		return pos;
	}
}

Int32 CActor::GetSceneHeadBone() const
{
	return m_headBoneIndex;
}

void CActor::CacheBoneIndices()
{
	// Reset
	m_losTestBoneIndex	= -1;
	m_headBoneIndex		= -1;
	m_torsoBoneIndex	= -1;

	CAnimatedComponent* animated = GetRootAnimatedComponent();
	if( !animated )
		return;

	// Get skeleton data provider, it will help us find the bone by name
	const ISkeletonDataProvider* provider = animated ? animated->QuerySkeletonDataProvider() : NULL;
	if ( provider )
	{
		// Find bone
		m_losTestBoneIndex = provider->FindBoneByName( m_pelvisBoneName );
		if( m_losTestBoneIndex == -1 )
		{
			// Bone not found
			WARN_GAME( TXT("CacheBoneIndices: Bone 'pelvis' not found, actor '%ls'"), GetName().AsChar() );
		}

		m_torsoBoneIndex = provider->FindBoneByName( m_torsoBoneName );
		if( m_torsoBoneIndex == -1 )
		{
			// Bone not found
			WARN_GAME( TXT("CacheBoneIndices: Bone 'torso' not found, actor '%ls'"), GetName().AsChar() );
		}

		m_headBoneIndex = provider->FindBoneByName( m_headBoneName );
		if( m_headBoneIndex == -1 )
		{
			// Bone not found
			WARN_GAME( TXT("CacheBoneIndices: Bone 'head' not found, actor '%ls'"), GetName().AsChar() );
		}
	}
	else
	{
		// This is not normal not to have SkeletonData provider in animation component
		WARN_GAME( TXT("CacheLOSTestBoneIndex: No skeleton data provider in AnimationComponent, actor '%ls'"), GetName().AsChar() );
	}
}

String CActor::GetDisplayName() const
{
	if( !m_cachedDisplayName.Empty() )
	{
		return m_cachedDisplayName;
	}

	const CAlternativeDisplayName* param = nullptr;
	if( m_template )
	{
		param = static_cast< const CAlternativeDisplayName* >( m_template->FindGameplayParam( CAlternativeDisplayName::GetStaticClass() ) );
	}
	if ( param )
	{
		CFactsDB *factsDB = GCommonGame ? GCommonGame->GetSystem< CFactsDB >() : NULL;	
		const LocalizedString& locString = factsDB && factsDB->DoesExist( param->GetFactID() ) ? param->GetAltName() : m_displayName;
		{
			if ( locString.Load() )
			{
				return locString.GetString();
			}			
		}		
		return String::EMPTY;
	}

	if ( m_displayName.Load() )
	{
		String displayName = m_displayName.GetString();
		if ( displayName.Empty() )
		{
			// the last resort
			displayName = CEntity::GetDisplayName();
		}
		m_cachedDisplayName = displayName;
	}
	return m_cachedDisplayName;
}

void CActor::SetupAIParametersList( const EntitySpawnInfo& info, SAIParametersSpawnList& outList )
{
	EntitySpawnInfo::HandlerIterator< CAiParamInjectHandler > it( info );

	while ( it )
	{
		CAiParamInjectHandler* aiParam = *it;
		aiParam->InjectAIParams( outList );
		it.Next();
	}
}


void CActor::OnCreatedAsync( const EntitySpawnInfo& info )
{
	if ( !info.m_previewOnly )
	{
		THandle< CAIBaseTree > aiBaseTree;
		TDynArray< CAIPresetParam * > aiPresetsList;
		GetAITemplateParams( GetEntityTemplate(), aiBaseTree, &aiPresetsList );

		//[ Step ] Creating ai base tree
		if ( aiBaseTree == nullptr )
		{
			// TODO - do not use early return in code with TBaseClass
			TBaseClass::OnCreatedAsync( info );
			return;
		}
		if ( !m_behTreeMachine )
		{
			m_behTreeMachine = CreateObject< CBehTreeMachine >( this );
		}

		m_behTreeMachine->SetAIDefinition( aiBaseTree );
		for ( Uint32 i = 0; i < aiPresetsList.Size(); ++i )
		{
			CAIPresetParam *const aiPresetParam = aiPresetsList[ i ];
			const auto& redefinitions = aiPresetParam->GetRedefinitionParameters();
			for( Uint32 j = redefinitions.Size(); j > 0 ; --j )
			{
				IAIParameters* def = redefinitions[j-1].Get();
				if ( def )
				{
					m_behTreeMachine->InjectAIParameters( def );
				}
			}
		}
		CAIPresetsTemplateParam *const aiPresetTemplateParam = GetEntityTemplate()->FindParameter< CAIPresetsTemplateParam >( );
		if ( aiPresetTemplateParam )
		{
			const TDynArray< THandle< ICustomValAIParameters > > & customValAIParametersArray = aiPresetTemplateParam->GetCustomValParameters();
			for( Uint32 j = customValAIParametersArray.Size(); j > 0 ; --j )
			{
				ICustomValAIParameters* val = customValAIParametersArray[j-1].Get();
				if ( val )
				{
					m_behTreeMachine->InjectAIParameters( val );
				}
			}
		}

		SAIParametersSpawnList list;

		SetupAIParametersList( info, list );

		m_behTreeMachine->Initialize( list );

		m_encounterGroupUsedToSpawn = info.m_encounterEntryGroup;
	}
	

	TBaseClass::OnCreatedAsync( info );
}

void CActor::OnRestoredFromPoolAsync( const EntitySpawnInfo& info )
{
	if ( m_behTreeMachine )
	{
		SAIParametersSpawnList list;

		SetupAIParametersList( info, list );
		
		m_behTreeMachine->OnReattachedAsync( list );
	}
	m_encounterGroupUsedToSpawn = info.m_encounterEntryGroup;
	TBaseClass::OnRestoredFromPoolAsync( info );
}

void CActor::OnRestoredFromPool( CLayer* layer, const EntitySpawnInfo& info )
{
	if ( m_behTreeMachine )
	{
		SAIParametersSpawnList list;

		SetupAIParametersList( info, list );

		m_behTreeMachine->OnReattached( list );
	}
	TBaseClass::OnRestoredFromPool( layer, info );
}


void CActor::OnUninitialized()
{
	TBaseClass::OnUninitialized();

	// Destroy behavior tree machine
	if( m_behTreeMachine )
	{
		m_behTreeMachine->Uninitialize();
		m_behTreeMachine->Discard();
		m_behTreeMachine = nullptr;
	}

	m_visualDebug = nullptr;
}

void CActor::OnAttached( CWorld* world )
{
	//if ( m_spawnHidden )
	//{
	//	m_objectFlags |= NF_HideInGame;
	//}

	// Pass to the base class
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CActor_OnAttached );

	// Add actor to tick manager
	world->GetTickManager()->AddEntity( this );

	// Register using the voice tags
	if ( GetVoiceTag() != CName::NONE )
	{
		CStorySceneSystem* sceneSystem = GCommonGame->GetSystem< CStorySceneSystem >();
		if ( sceneSystem != NULL )
		{
			CStorySceneActorMap* sceneActorMap = sceneSystem->GetActorMap();
			if ( sceneActorMap != NULL )
			{
				sceneActorMap->RegisterSpeaker( this, GetVoiceTag() );
			}
		}
	}


	if( m_useAnimationEventFilter )
	{
		m_animationEventFilter = new CAnimationEventFilter();
	}

	
	if( m_behTreeMachine )
	{
		m_behTreeMachine->OnAttached();
		m_behTreeMachine->Restart();
	}

	// Create the entity grouping proxy for the renderer
	if ( GRender )
	{
		// Create entity group
		ASSERT( !m_entityGroup );
		m_entityGroup = GRender->CreateEntityGroup();

		// make sure that the entity group has the valid flags
		{
			if ( m_useHiResShadows )
			{
				( new CRenderCommand_SetEntityGroupHiResShadows( m_entityGroup, true ) )->Commit();
			}

			if ( !m_useGroupShadows )
			{
				( new CRenderCommand_SetEntityGroupShadows( m_entityGroup, false ) )->Commit();
			}
		}
	}

	ResetBaseAttitudeGroup( false );

#ifndef NO_EDITOR
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_ActorLodInfo );
#endif

	NotifyEncounterAboutAttach();	
}

void CActor::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );

	if ( IsInGame() )
	{
		GCommonGame->GetActorsManager()->Add( this );

		// create a push animation controller
		if ( m_pushController == NULL )
		{
			CMovingAgentComponent* mac = GetMovingAgentComponent();
			if ( mac )
			{
				CBehaviorGraphStack* stack = mac->GetBehaviorStack();
				if ( stack )
				{
					m_pushController = new PushReactionController( stack );
				}

				// set basic movement behavior flags
				mac->SetCollidable( m_isCollidable );
			}
		}
	}

	if( m_voiceToRandomize.Empty() ) return;

	CSoundEmitterComponent* soundEmitterComponent = GetSoundEmitterComponent( true );
	if( !soundEmitterComponent ) return;

	// For cooked resources we assume the voicesToRandomize is always filled either directly
	// or from the cooker's automatic filling
	if ( GetEntityTemplate() == nullptr || GetEntityTemplate()->IsCooked() )
	{
		if ( !m_voiceToRandomize.Empty() )
		{
			const Int32 bone = GetHeadBone();
			const Uint32 randomized = GEngine->GetRandomNumberGenerator().Get< Uint32 >( 0, m_voiceToRandomize.Size() );
			soundEmitterComponent->SoundEvent( m_voiceToRandomize[ randomized ].AsChar(), bone );
		}
	}
	else // for non-cooked resources, we need to scan for the proper voiceToRandomize manually
	{
		TDynArray< StringAnsi >* voices = nullptr;
		struct {
			Bool Scan( CEntityTemplate* tpl, TDynArray< StringAnsi >*& voices )
			{
				CActor* tplActor = Cast< CActor >( tpl->GetEntityObject() );
				if ( tplActor != nullptr )
				{
					if ( !tplActor->m_voiceToRandomize.Empty() )
					{
						voices = &tplActor->m_voiceToRandomize;
						return true;
					}
				}
				for ( THandle< CEntityTemplate > subtpl : tpl->GetIncludes() )
				{
					if ( subtpl.IsValid() )
					{
						if ( Scan( subtpl.Get(), voices ) )
						{
							return true;
						}
					}
				}
				return false;
			}
		} local;
		if ( local.Scan( GetEntityTemplate(), voices ) )
		{
			if ( !voices->Empty() )
			{
				const Int32 bone = GetHeadBone();
				const Uint32 randomized = GEngine->GetRandomNumberGenerator().Get< Uint32 >( 0, voices->Size() );
				soundEmitterComponent->SoundEvent( (*voices)[ randomized ].AsChar(), bone );
			}
		}
	}
}

void CActor::OnDetached( CWorld* world )
{
#ifndef NO_EDITOR
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_ActorLodInfo );
#endif

	ReportTerminationDetach();

	// Stop all scenes that used the actor, specifying despawn as reason
	StopAllScenes( SCR_ACTOR_DESPAWN );

	// Remove all debug listeners
	m_aiDebugListeners.Clear();

	if( m_behTreeMachine )
	{
		m_behTreeMachine->OnDetached();
	}

	m_attackers.ClearFast();

	// release the push animation controller
	if ( m_pushController )
	{
		//if ( GetMovingAgentComponent() && GetMovingAgentComponent()->IsAttached() )
		//{
		//	m_pushController->StopAnimation();
		//}

		delete m_pushController;
		m_pushController = NULL;
	}

	// Delete voiceset player
	delete m_voicesetPlayer;
	m_voicesetPlayer = NULL;

	if( m_animationEventFilter )
	{
		delete m_animationEventFilter;
		m_animationEventFilter = nullptr;
	}

	// Pass to base class
	TBaseClass::OnDetached( world );

	// Cancel active speech
	CancelSpeech();
	
	if ( m_action )
	{
		m_action->Stop();
		m_action = NULL;
	}

	// Reset cached component  - honestly we don't need this component!
	m_agentComponent = NULL;

	// Remove from the main tick group
	world->GetTickManager()->RemoveEntity( this );

	// Unregister from the speaker list
	CStorySceneSystem* sceneSystem = GCommonGame->GetSystem< CStorySceneSystem >();
	if ( sceneSystem != NULL )
	{
		CStorySceneActorMap* sceneActorMap = sceneSystem->GetActorMap();
		if ( sceneActorMap != NULL )
		{
			sceneActorMap->UnregisterSpeaker( this );
		}
	}

	// Release entity group - note that there still be components registered to this entity group (inventory stuff).
	// This is not a problem since the entity group is a soft object that only groups rendering proxies and is also internally refcounted.
	if ( m_entityGroup != NULL )
	{
		// do not leave the shadows turned on
		( new CRenderCommand_SetEntityGroupHiResShadows( m_entityGroup, false ) )->Commit();

		// release local reference, NOTE: ther may still existing other references
		m_entityGroup->Release();
		m_entityGroup = NULL;
	}

	CActorsManager* actorsManager = GCommonGame->GetActorsManager();
	if ( actorsManager )
	{
		actorsManager->Remove( this );
	}

	m_speechQueue.Cleanup();

	NotifyEncounterAboutDetach();

	// Be sure that we reset all flags before actor might go to entity pool
	SCENE_ASSERT( !m_lockedByScene );
	SCENE_ASSERT( !m_isInNonGameplayScene );
	SCENE_ASSERT( !m_isInGameplayScene );
	if ( m_lockedByScene )
	{
		m_lockedByScene = false;
	}
	if ( m_isInNonGameplayScene )
	{
		m_isInNonGameplayScene = false;
	}
	if ( m_isInGameplayScene )
	{
		m_isInGameplayScene = false;
	}
}

void CActor::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if( file.IsGarbageCollector() )
	{
		if( m_visualDebug )
		{
			file << m_visualDebug;
		}

		if ( m_speech )
		{
			file << *m_speech;
		}

		if ( m_action )
		{
			m_action->OnGCSerialize( file );
		}
	}
}

void CActor::OnTick( Float timeDelta )
{
	TBaseClass::OnTick( timeDelta );

	PC_SCOPE_PIX( ActorTick );

	if ( m_voicesetPlayer )
	{
		if ( m_voicesetPlayer->IsSceneLoaded() )
		{
			m_voicesetPlayer->PlayVoiceset( this );

			delete m_voicesetPlayer;
			m_voicesetPlayer = NULL;
		}
	}

	if ( m_speech != NULL )
	{
		m_speech->Update( timeDelta );
		if ( m_speech->IsFinished() == true )
		{
			ProceedSpeechFromQueue();
		}
	}

	UpdateLookAt( timeDelta );

	if( IsInGame() )
	{
		// Update action
		UpdateActions( timeDelta );
	}
	else if ( m_action != NULL && m_action->GetType() == ActorAction_ChangeEmotion )
	{
		// Change emotion actor should be updated even when not in game (e.g. in preview)
		UpdateActions( timeDelta );
	}

	// This cant be done in OnAppearanceChanged, because it seems, an actor can be in the middle of
	// being destroyed then, which causes a crash.
	// ATM this is called to update CR4HumanoidCombatComponent from scripts, which is used for humanoid combat and
	// modyfying sounds of npcs according to their armor (for movement and combat). 
	if(m_forceAppearanceSoundUpdate)
	{
		m_appearanceChanged = false;
		m_forceAppearanceSoundUpdate = false;
		CallEvent( CNAME( OnForceUpdateSoundInfo ) );
	}
	if(	m_appearanceChanged)
	{
		m_appearanceChanged = false;
		CallEvent( CNAME( OnAppearanceChanged ) );
	}

	//EP2 hack; we can't edit the data from the main game so let'sassume the player is Geralt
	if(IsPlayer())
	{
		m_soundListenerOverride = TXT("Geralt");
	}

	if(!m_soundListenerOverride.Empty())
	{
		GSoundSystem->RegisterListenerOverride(m_soundListenerOverride, GetWorldPosition(), GetWorldUp(), GetWorldForward());
	}
}

void CActor::OnPropertyPreChange( IProperty* property )
{
	TBaseClass::OnPropertyPreChange( property );
}

void CActor::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
}

Bool CActor::IsVisibleForMonsters() const
{
	if ( m_storyScenes.Empty() ) return false;

	for ( TDynArray< CStorySceneController* >::const_iterator sscI = m_storyScenes.Begin();
		  sscI != m_storyScenes.End();
		  ++sscI )
	{
		if ( (*sscI)->GetPlayer() && (*sscI)->GetPlayer()->IsGameplay() )
		{
			return false;
		}
	}

	return true;
}

Bool CActor::IsDoingSomethingMoreImportant( Int32 newPriority, String* errorMessage /* = NULL */ ) const
{
	for ( Uint32 i = 0; i < m_storyScenes.Size(); ++i )
	{
		if ( m_storyScenes[i] != NULL && (Int32)m_storyScenes[i]->m_priority > newPriority && m_storyScenes[i]->IsPaused() == false )
		{
			if ( errorMessage != NULL )
			{
				if ( m_storyScenes[ i ]->GetPlayer() )
				{
					*errorMessage = String::Printf( TXT( "Actor %s is playing other scene: [%s]" ), GetName().AsChar(), m_storyScenes[ i ]->GetPlayer()->GetName().AsChar() );
				}
				else
				{
					*errorMessage = String::Printf( TXT( "Actor %s is laying other scene, no scene player so name not known :( " ), GetName().AsChar() );
				}
			}
			return true;
		}
	}
	{
		SArbitratorQueryData arbitratorQuery = SArbitratorQueryData( IBehTreeNodeDefinition::Priority( newPriority ) );
		SignalGameplayEvent( CNAME( AI_ArbitratorQuery ), &arbitratorQuery, SArbitratorQueryData::GetStaticClass() );
		if ( !arbitratorQuery.m_queryResult )
		{
			if ( errorMessage != NULL )
			{
				*errorMessage = String::Printf( TXT( "Actor %s has a more important AI action" ), GetName().AsChar() );
			}
			return true;
		}
	}

	return false;
}

Bool CActor::CanPlayQuestScene( Bool noLog ) const
{
	String error;
	Bool isPlayingOtherScene = IsDoingSomethingMoreImportant( BTAP_FullCutscene, &error );
	if ( isPlayingOtherScene == true  && !noLog )
	{
		LOG_GAME( error.AsChar() );
	}
	return isPlayingOtherScene == false;
}

Bool CActor::HasInteractionScene() const
{
	if ( GCommonGame != NULL && GCommonGame->GetSystem< CQuestsSystem >() != NULL )
	{
		return GCommonGame->GetSystem< CQuestsSystem >()->AreThereAnyDialogsForActor( *this );
	}
	return false;
}

Bool CActor::CanTalk( Bool ignoreCurrentSpeech /* = false */ ) const
{
	// there are some problems with unstreaming, which causes 'talk' interactions not to appear because of this commented condition
	//Bool isStreaming = GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetCurrentStreamingTask();
	if( CanPlayQuestScene( true ) && /*!isStreaming &&*/ !IsUsingExploration() && WasVisibleLastFrame() && m_displayName.Load() )
	{
		if( HasInteractionScene() )
		{
			return true;
		}
		else if( HasInteractionVoicesetScene() && ( ignoreCurrentSpeech || m_speech == NULL ) )
		{
			return true;
		}
	}
	return false;
}

Bool CActor::AttachEntityToBone( CEntity* entity, CName boneName )
{
	ASSERT( entity && boneName );

	if ( !entity || !boneName )
	{
		return false;
	}

	// Find bone index (for check)
	CAnimatedComponent* animComponent = GetRootAnimatedComponent();
	if ( animComponent == NULL )
	{
		ITEM_ERR( TXT("CActor::AttachEntityToBone - actor %s is missing root animated component !!! Grabbing will fail !!!"), GetFriendlyName().AsChar() );
		return false;
	}

	const ISkeletonDataProvider* provider = animComponent->QuerySkeletonDataProvider();
	if ( !provider )
	{
		ITEM_ERR( TXT("CActor::AttachEntityToBone - skeleton data provider query for actor's %s animated component failed !!! Grabbing will fail !!!"), GetFriendlyName().AsChar() );
		return false;
	}

	Int32 boneIdx = provider->FindBoneByName( boneName.AsString().AsChar() );
	if ( boneIdx == -1 )
	{
		ITEM_ERR( TXT("CActor::AttachEntityToBone - actor %s is missing target bone %s !!! Grabbing will fail !!!"), GetFriendlyName().AsChar(), boneName.AsString().AsChar() );
		return false;
	}

	// Reset item position
	entity->SetPosition( Vector::ZERO_3D_POINT );
	entity->SetRotation( EulerAngles::ZEROS );

	// Create attachment spawn info
	HardAttachmentSpawnInfo info;
	info.m_parentSlotName = boneName;

	// Iterate all root components and attach them to bone by applying attachment spawn info
	Uint32 numComponentsAttached = 0;
	for ( CComponent* current : entity->GetComponents() )
	{
		CComponent* rootComponent = current;
		if ( rootComponent->GetParentAttachments().Empty() )
		{
			// Reset root components position
			rootComponent->SetPosition(Vector::ZERO_3D_POINT);
			rootComponent->SetRotation(EulerAngles::ZEROS);

			// Attach
			animComponent->Attach( rootComponent, info );
			++numComponentsAttached;
		}
	}

	if ( numComponentsAttached == 0 )
	{
		ITEM_ERR( TXT("CActor::AttachEntityToBone - 0 components attached !!! Grabbing may be bugged !!! ") );
		ITEM_ERR( TXT("CActor::AttachEntityToBone - actor %s, bone name %s"), GetFriendlyName().AsChar(), boneName.AsString().AsChar() );
	}

	// update stuff
	entity->ForceUpdateTransformNodeAndCommitChanges();
	entity->ForceUpdateBoundsNode();

	return true;
}

void CActor::DetachEntityFromSkeleton( CEntity* entity )
{
	if( entity == NULL )
	{
		return;
	}

	CAnimatedComponent* animComponent = GetRootAnimatedComponent();
	if ( animComponent == NULL )
	{
		ITEM_ERR( TXT("CActor::DetachEntityFromSkeleton - actor %s is missing root animated component !!! Putting will fail !!!"), GetFriendlyName().AsChar() );
		return;
	}

	Uint32 numAttachmentsBroken = 0;

	// Iterate all root components of the item entity, and break their parent attachments
	for ( CComponent* component : entity->GetComponents() )
	{
		TStaticArray< IAttachment*, 32 > attArray;

		for ( IAttachment* attachment : component->GetParentAttachments() )
		{
			attArray.PushBack( attachment );
		}

		// Deffered break to avoid iterator corruption
		for ( Uint32 i=0; i<attArray.Size(); i++ )
		{
			if ( attArray[i] )
			{
				attArray[i]->Break();
				++numAttachmentsBroken;
			}
		}
	}
	if ( numAttachmentsBroken == 0 )
	{
		ITEM_ERR( TXT("CActor::DetachEntityFromSkeleton - no attachments were broken !!! putting may be bugged !!!") );
		ITEM_ERR( TXT("CActor::AttachEntityToBone - actor %s"), GetFriendlyName().AsChar() );
	}
}

void CActor::OnGrabItem( SItemUniqueId itemId, CName slot )
{
	TBaseClass::OnGrabItem( itemId, slot );
	CInventoryComponent* inventoryComponent = m_inventoryComponent.Get();
	const SInventoryItem* item = inventoryComponent->GetItem( itemId );

	if( !item )
	{
		RED_ASSERT( false, TXT( "There are two or more inventory components in this entity?" ) );
		return;
	}

	if ( slot == CNAME( r_weapon ) || slot == CNAME( l_weapon ) )
	{
		CallEvent( CNAME( OnEquippedItem ), item->GetCategory(), slot );
	}

	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() );

	inventoryComponent->OnGrabItem( itemId );

	if ( m_stats && itemDef->IsAbilityEnabledOnHold() )
	{
		m_stats->ApplyItemAbilities( *item );
	}

	if ( itemDef->GetActorAnimState( IsA< CPlayer >() ) )
	{
		EActorAnimState animState = CDefinitionsManager::MapNameToActorAnimState( itemDef->GetActorAnimState( IsA< CPlayer >() ) );
		if ( animState > GetActorAnimState() )
		{
			SetActorAnimState( animState );
		}
	}

	if ( itemDef->IsWeapon() )
	{
		OnDrawWeapon();
	}
}

void CActor::OnPutItem( SItemUniqueId itemId, Bool emptyHand )
{
	TBaseClass::OnPutItem( itemId, emptyHand );
	CInventoryComponent* inventoryComponent = m_inventoryComponent.Get();
	const SInventoryItem* item = inventoryComponent->GetItem( itemId );

	if( !item )
	{
		RED_ASSERT( false, TXT( "There are two or more inventory components in this entity?" ) );
		return;
	}

	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() );

	if ( emptyHand )
	{
		CName slot = ( itemDef != nullptr ) ? itemDef->GetHoldSlot( IsPlayer() ) : CName::NONE;
		if ( slot == CNAME( r_weapon ) || slot == CNAME( l_weapon ) )
		{
			CallEvent( CNAME( OnHolsteredItem ), item->GetCategory(), slot );
		}
	}

	inventoryComponent->OnPutItem( itemId );

	if ( m_stats && itemDef->IsAbilityEnabledOnHold() )
	{
		m_stats->RemoveItemAbilities( *item );
	}

	EActorAnimState animState = CDefinitionsManager::MapNameToActorAnimState( itemDef->GetActorAnimState( IsA< CPlayer >() ) );

	if ( animState == GetActorAnimState() )
	{
		// We are putting item that causes current anim state, so it's time to set it to default
		SetActorAnimState( AAS_Default );
	}
}

void CActor::OnMountItem( SItemUniqueId itemId, Bool wasHeld )
{
	TBaseClass::OnMountItem( itemId, wasHeld );
	const SInventoryItem* item = m_inventoryComponent.Get()->GetItem( itemId );

	if( !item )
	{
		RED_ASSERT( false, TXT( "There are two or more inventory components in this entity?" ) );
		return;
	}

	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() );
	if ( m_stats && itemDef && itemDef->IsAbilityEnabledOnMount() )
	{
		m_stats->ApplyItemAbilities( *item );
	}
}

void CActor::OnEnhanceItem( SItemUniqueId enhancedItemId, Int32 slotIndex )
{
	TBaseClass::OnEnhanceItem( enhancedItemId, slotIndex );
	const SInventoryItem* item = m_inventoryComponent.Get()->GetItem( enhancedItemId );

	if( !item )
	{
		RED_ASSERT( false, TXT( "There are two or more inventory components in this entity?" ) );
		return;
	}

	if ( m_stats && item->AreAbilitiesActive() )
	{
		m_stats->ApplyItemSlotAbilities( *item, slotIndex );
	}
}

void CActor::OnRemoveEnhancementItem( SItemUniqueId enhancedItemId, Int32 slotIndex )
{
	TBaseClass::OnRemoveEnhancementItem( enhancedItemId, slotIndex );
	const SInventoryItem* item = m_inventoryComponent.Get()->GetItem( enhancedItemId );

	if( !item )
	{
		RED_ASSERT( false, TXT( "There are two or more inventory components in this entity?" ) );
		return;
	}

	if ( m_stats && item->AreAbilitiesActive() )
	{
		m_stats->RemoveItemSlotAbilities( *item, slotIndex );
	}
}

void CActor::OnUnmountItem( SItemUniqueId itemId )
{
	TBaseClass::OnUnmountItem( itemId );
	const SInventoryItem* item = m_inventoryComponent.Get()->GetItem( itemId );

	if( !item )
	{
		RED_ASSERT( false, TXT( "There are two or more inventory components in this entity?" ) );
		return;
	}

	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() );

	if ( m_stats && itemDef && itemDef->IsAbilityEnabledOnMount() )
	{
		m_stats->RemoveItemAbilities( *item );
	}
}

void CActor::OnAddedItem( SItemUniqueId itemId )
{
	TBaseClass::OnAddedItem( itemId );

	CInventoryComponent* inventory = GetInventoryComponent();

	if( !inventory )
	{
		RED_ASSERT( false, TXT( "No inventory component while adding item" ) );
		return;
	}

	const SInventoryItem* item = inventory->GetItem( itemId );
	if ( !item )
	{
		WARN_GAME( TXT("%s has more than one inventory component, FIX THIS NOW !!!!!!!!"), GetFriendlyName().AsChar() );
		return;
	}
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() );
	if ( nullptr == itemDef )
	{
		WARN_GAME( TXT("Item %s have no definition. Expect trouble."), item->GetName().AsChar() );
	}
	else if ( m_stats && itemDef->IsAbilityEnabledOnHidden() )
	{
		m_stats->ApplyItemAbilities( *item );
	}
}

void CActor::OnRemovedItem( SItemUniqueId itemId )
{
	TBaseClass::OnRemovedItem( itemId );

	const SInventoryItem* item = m_inventoryComponent.Get()->GetItem( itemId );
	const SItemDefinition* itemDef = GCommonGame->GetDefinitionsManager()->GetItemDefinition( item->GetName() );

	if ( m_stats && itemDef && itemDef->IsAbilityEnabledOnHidden() )
	{
		m_stats->RemoveItemAbilities( *item );
	}
}

void CActor::OnItemAbilityAdded( SItemUniqueId itemId, CName ability )
{
	TBaseClass::OnItemAbilityAdded( itemId, ability );

	CInventoryComponent *inventoryComponent = m_inventoryComponent.Get();
	if ( inventoryComponent == NULL )
	{
		ITEM_ERR( TXT("CActor::OnItemAbilityAdded - actor %s doesn't have inventory component!!!"), GetFriendlyName().AsChar() );
		return;
	}

	const SInventoryItem* item = inventoryComponent->GetItem( itemId );

	if ( m_stats && item->AreAbilitiesActive() )
	{
		m_stats->ApplyItemAbility( *item, ability );
	}
}

void CActor::OnItemAbilityRemoved( SItemUniqueId itemId, CName ability )
{
	TBaseClass::OnItemAbilityRemoved( itemId, ability );

	CInventoryComponent *inventoryComponent = m_inventoryComponent.Get();
	if ( inventoryComponent == NULL )
	{
		ITEM_ERR( TXT("CActor::OnItemAbilityRemoved - actor %s doesn't have inventory component!!!"), GetFriendlyName().AsChar() );
		return;
	}

	const SInventoryItem* item = inventoryComponent->GetItem( itemId );

	if ( m_stats && item->AreAbilitiesActive() )
	{
		m_stats->RemoveItemAbility( *item, ability );
	}
}

void CActor::IssueRequiredItems( const SActorRequiredItems& info, Bool instant /*= false*/, IssueRequiredItemsInfo* res /*= nullptr*/ )
{
	m_requiredItemsState = info;
	ProcessRequiredItemsState( instant, res );
}

void CActor::SetRequiredItems( const SActorRequiredItems& info )
{
	m_requiredItemsState = info;
}

void CActor::ProcessRequiredItemsState( Bool instant /*= false*/, IssueRequiredItemsInfo* res /*= nullptr*/  )
{
	Uint32 size = m_requiredItemsState.m_slotAndItem.Size();
	for( Uint32 i = 0; i < size; ++i )
	{
		ProcessRequiredItem( m_requiredItemsState.m_slotAndItem[i].m_second, m_requiredItemsState.m_slotAndItem[i].m_first, instant, res );
	}
}

void CActor::StoreRequiredItems( SActorRequiredItems& info )
{
	if ( CInventoryComponent* inventory = m_inventoryComponent.Get() )
	{
		for ( auto it = info.m_slotAndItem.Begin(), end = info.m_slotAndItem.End(); it != end; ++it )
		{
			const SItemUniqueId current = inventory->GetItemIdHeldInSlot( it->m_first );
			if ( const SInventoryItem* currentItem = inventory->GetItem( current ) )
			{
				it->m_second = currentItem->GetName();
			}
		}
	}
}

void CActor::ProcessRequiredItem( CName required, CName slot, Bool instant /*= false*/, IssueRequiredItemsInfo* res /*= nullptr*/ )
{
	CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
	CInventoryComponent* inventory = GetInventoryComponent();
	ASSERT( defMgr);

	if ( !defMgr || !inventory || required == CNAME( Any ))
	{
		return;
	}
	SItemUniqueId current = inventory->GetItemIdHeldInSlot( slot );

	if ( !required )
	{
		if ( current )
		{
			inventory->HolsterItem( current, instant );
		}
		else
		{
			SItemEntityManager::GetInstance().CancelLatentActionsForActor( this, slot );
		}
		return;
	}

	if ( defMgr->CategoryExists( required ) )
	{
		// Left hand category requirement only, check it
		const SInventoryItem* currentItem = inventory->GetItem( current );
		if ( !currentItem || currentItem->GetCategory() != required )
		{
			SItemUniqueId foundItemId;
			if ( res && res->m_usePriorityForSceneItems )
			{
				foundItemId = inventory->GetItemByCategoryForScene( required );
			}
			else
			{
				CInventoryComponent::SFindItemInfo findItemInfo;
				findItemInfo.m_category = required;
				findItemInfo.m_mountOnly = true;
				findItemInfo.m_holdSlot = slot;

				foundItemId = inventory->FindItem( findItemInfo );
				if ( !foundItemId )
				{
					findItemInfo.m_mountOnly = false;
					foundItemId = inventory->FindItem( findItemInfo );
				}
	
			}

			if ( foundItemId )
			{
				inventory->DrawItem( foundItemId, instant );
				return;
			}	
			else
			{
				ITEM_WARN( TXT("Failed to find item suitable for requirement: %s"), required.AsString().AsChar() );
				if( res )
				{
					res->m_success = false;
				}
				return;
			}
		}			
	}

	//search item by tag
	const SInventoryItem* currentItem = inventory->GetItem( current );
	if ( !currentItem || !currentItem->GetTags().Exist( required ) )
	{
		CInventoryComponent::SFindItemInfo findItemInfo;
		findItemInfo.m_mountOnly = true;
		findItemInfo.m_holdSlot = slot;
		findItemInfo.m_itemTag = required;
		findItemInfo.m_eligibleToMount = res && res->m_usePriorityForSceneItems;
		SItemUniqueId foundItemId = inventory->FindItem( findItemInfo )  ;
		if ( !foundItemId )
		{
			findItemInfo.m_mountOnly = false;
			foundItemId = inventory->FindItem( findItemInfo );
		}
		if ( foundItemId )
		{
			inventory->DrawItem( foundItemId, instant );
			return;
		}		
	}

	//Request contains item name
	const SItemDefinition* itemDef = defMgr->GetItemDefinition( required );
	if ( itemDef )
	{
		if ( itemDef->GetHoldSlot( IsA< CPlayer >() ) != slot )
		{
			ITEM_ERR( TXT("Item %s issued as required for %s slot, while having %s slot specified in definition"), required.AsChar(), slot.AsChar(), itemDef->GetHoldSlot( IsA< CPlayer >() ).AsChar() );
			if( res )
			{
				res->m_success = false;
			}
		}
		else
		{
			SItemUniqueId itemId = inventory->GetItemId( required );
			if ( !itemId )
			{
				inventory->AddItem( required );
				itemId = inventory->GetItemId( required );
				if( res )
				{
					res->m_spawnedItems.PushBack( itemId );
				}
			}
			inventory->DrawItem( itemId, instant );
		}
	}
}

void CActor::OnCollectAnimationSyncTokens( CName animationName, TDynArray< CAnimationSyncToken* >& tokens ) const
{
	TBaseClass::OnCollectAnimationSyncTokens( animationName, tokens );

	if ( CInventoryComponent* inventory = GetInventoryComponent() )
	{
		inventory->OnCollectAnimationSyncTokens( animationName, tokens );
	}

	//if ( CMimicComponent* mimic = GetMimicComponent() )
	//{
	//	mimic->OnCollectAnimationSyncTokens( animationName, tokens );
	//}
}

void CActor::OnProcessBehaviorPose( const CAnimatedComponent* poseOwner, const SBehaviorGraphOutput& pose )
{
	TBaseClass::OnProcessBehaviorPose( poseOwner, pose );

	if ( poseOwner == GetRootAnimatedComponent() )
	{
		// Reset look at level after behavior sampling
		m_lookAtController.SetLevel( LL_Body );
	}
}

void CActor::OnInitialized()
{
	TBaseClass::OnInitialized();

	PC_SCOPE_PIX( Actor_OnInitialized );

	// Cache LOS test bone
	CacheBoneIndices();

	if( IsInGame() )
	{
		// Create visual debug
		m_visualDebug = CreateObject<CVisualDebug>( this );

		if ( m_behTreeMachine )
		{
			m_behTreeMachine->OnSpawn();
		}
	}
	else
	{
		m_visualDebug = NULL;
	}

	m_lookAtController.FindAndCacheStaticLookAtParam( GetEntityTemplate() );
}

void CActor::OnAttachmentCreated()
{
	TBaseClass::OnAttachmentCreated();

	CMovingAgentComponent * mac = GetMovingAgentComponent();
	if ( mac )
	{
		mac->Teleport( Vector::ZERO_3D_POINT, &EulerAngles::ZEROS );
	}
}

void CActor::OnAttachmentBroken()
{
	TBaseClass::OnAttachmentBroken();
	
	CMovingAgentComponent * mac = GetMovingAgentComponent();
	if ( mac )
	{
		const EulerAngles rotation = GetWorldRotation();
		mac->Teleport( GetWorldPosition(), &rotation );
	}
}

void CActor::NotifyEncounterAboutAttach()
{
	if( !IsInGame() )
	{
		return;
	}

	if( m_encounterGroupUsedToSpawn < 0 )
	{
		return;
	}

	GCommonGame->GetEncounterSpawnGroup().RegisterActorInGroup( m_encounterGroupUsedToSpawn );
}

void CActor::NotifyEncounterAboutDetach()
{
	if( !IsInGame() )
	{
		return;
	}

	if( m_encounterGroupUsedToSpawn < 0 )
	{
		return;
	}

	GCommonGame->GetEncounterSpawnGroup().UnregisterActorFromGroup( m_encounterGroupUsedToSpawn );
}

Bool CActor::Teleport( const Vector& position, const EulerAngles& rotation )
{
	// VERY UGLY HACK for teleporting with mount
	if ( m_transformParent )
	{
		CNode *node = m_transformParent->GetParent();
		CComponent *comp = Cast< CComponent >( node );
		CEntity* entity = NULL;
		if ( comp )
		{
			CGameplayEntity* ge = Cast< CGameplayEntity >( comp->GetEntity() );
			if ( ge && ge->HasGameplayFlags( FLAG_HasVehicle ) )
			{
				entity = ge;
			}
			if ( !entity )
			{
				if ( comp->GetEntity()->GetTags().HasTag( CName( TXT("vehicle") ) ) )
				{
					entity = comp->GetEntity();
				}
			}
		}
		else
		{
			entity = Cast< CGameplayEntity >( node );
		}
		if ( entity )
		{
			entity->Teleport( position, rotation );
			Int32 componentsCount = (Int32)entity->GetComponents().Size();
			for ( Int32 i = componentsCount - 1; i >= 0; --i )
			{
				CComponent* component = entity->GetComponents()[ i ];
				if ( !component ) continue;
				CPhysicsWrapperInterface* wrapper = component->GetPhysicsRigidBodyWrapper();
				if( !wrapper ) continue;

				wrapper->SetFlag( PRBW_PoseIsDirty, true );
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	// Use the MAC to move the entity
	CMovingAgentComponent * mac = GetMovingAgentComponent();
	if ( mac )
	{
		// We have to reset IK after teleport..
		mac->AccessAnimationProxy().HACK_SetJustTeleportedFlag();

		mac->Teleport( position, &rotation );
	}
	else
	{
		if ( TBaseClass::Teleport( position, rotation ) )
		{
			return false;
		}
	}

	return true;
}

Bool CActor::Teleport( CNode* node, Bool applyRotation /*= true*/, const Vector& offset /*= Vector::ZEROS*/ )
{
	ASSERT( node );

	if ( applyRotation )
	{
		// Use only Yaw from rotation. We don't want to roll or pitch character.
		EulerAngles rotation( EulerAngles::ZEROS );
		rotation.Yaw = node->GetWorldRotation().Yaw;
		return Teleport( node->GetWorldPosition() + offset, rotation );
	}
	else
	{
		return Teleport( node->GetWorldPosition() + offset, GetWorldRotation() );
	}
}

void CActor::SetPosition( const Vector& position )
{
	// Calling set position on actor is illegal while in game
	if ( GGame->IsActive() && GetLayer() && GetLayer()->GetWorld() == GGame->GetActiveWorld() )
	{
		WARN_GAME( TXT("Calling SetPosition on actor '%ls' while in game is ILLEGAL. Use Teleport."), GetFriendlyName().AsChar() );
		return;
	}

	// Pass to base class
	TBaseClass::SetPosition( position );
}

void CActor::OnUpdateTransformEntity()
{
	// Pass to base class
	TBaseClass::OnUpdateTransformEntity();

	GCommonGame->RegisterEntityForActorManagerUpdate( this );
}

// For new event system
void CActor::ProcessAnimationEvent( const CAnimationEventFired* event )
{
	// Pass to base class
	TBaseClass::ProcessAnimationEvent( event );

	if ( event->ReportToAI() )
	{
		if( CBehTreeMachine* behTreeMachine = GetBehTreeMachine() )
		{
			behTreeMachine->ProcessAnimationEvent( event->m_extEvent, event->m_type, event->m_animInfo );
		}
	}
}

CName CActor::GetVoiceTag() const
{
	return m_voiceTag;
}

void CActor::OnSaveGameplayState( IGameSaver* saver )
{
	TBaseClass::OnSaveGameplayState( saver );

	CBehTreeMachine* behTreeMaching = GetBehTreeMachine();
	CBehTreeInstance* behTreeInstance = behTreeMaching ? behTreeMaching->GetBehTreeInstance() : nullptr;

	if ( behTreeInstance )
	{
		CGameSaverBlock block( saver, CNAME( ai ) );
		behTreeInstance->SaveState( saver );
	}

	{
		CGameSaverBlock block( saver, CNAME( at ) );

		// base attitude group
		saver->WriteValue( CNAME( a ), m_baseAttitudeGroup );

		// temporary groups
		Uint32 tmpGroupsCount = m_temporaryAttitudeGroups.Size();
		saver->WriteValue( CNAME( c ), tmpGroupsCount );
		for ( const auto& group : m_temporaryAttitudeGroups )
		{
			CName groupName = group.m_second;
			Uint32 priority = group.m_first;
			saver->WriteValue( CNAME( name ), groupName );
			saver->WriteValue( CNAME( p ), priority );
		}
	}

	// tags are often modified from scripts, etc.
	// let's just save all of them for unmanaged actors (they are stored in spandata either way for managed ones)
	if ( false == IsManaged() )
	{
		CGameSaverBlock block( saver, CNAME( tags ) );
		saver->WriteValue( CNAME( tags ), m_tags );
	}

	saver->WriteValue( CNAME( a ), m_alive );
}

void CActor::OnLoadGameplayState( IGameLoader* loader )
{
	TBaseClass::OnLoadGameplayState( loader );

	if ( m_behTreeMachine )
	{
		m_behTreeMachine->OnSpawn();
	}
	CBehTreeInstance* behTreeInstance = m_behTreeMachine ? m_behTreeMachine->GetBehTreeInstance() : nullptr;

	if ( behTreeInstance )
	{
		CGameSaverBlock block( loader, CNAME( ai ) );
		behTreeInstance->LoadState( loader );
	}

	{
		CGameSaverBlock block( loader, CNAME( at ) );

		// base attitude group
		loader->ReadValue( CNAME( a ), m_baseAttitudeGroup );

		// temporary groups
		m_temporaryAttitudeGroups.Clear();
		Uint32 tmpGroupsCount = 0;
		loader->ReadValue( CNAME( c ), tmpGroupsCount );
		for ( Uint32 i = 0; i < tmpGroupsCount; i++ )
		{
			CName groupName = CName::NONE;
			Uint32 priority = 0;
			loader->ReadValue( CNAME( name ), groupName );
			loader->ReadValue( CNAME( p ), priority );
			if ( groupName != CName::NONE )
			{
				m_temporaryAttitudeGroups.Insert( priority, groupName );
			}
		}
	}

	// tags are often modified from scripts, etc.
	// let's just save all of them for unmanaged actors (they are stored in spandata either way for managed ones)
	if ( false == IsManaged() )
	{
		CGameSaverBlock block( loader, CNAME( tags ) );
		TagList tags;
		loader->ReadValue( CNAME( tags ), tags );
		if ( false == tags.Empty() )
		{
			SetTags( tags );
		}
	}

	if ( loader->GetSaveVersion() >= SAVE_VERSION_ACTOR_ALIVE_SAVING )
	{
		loader->ReadValue( CNAME( a ), m_alive );

		if ( !m_alive )
		{
			SignalGameplayEvent( CNAME( OnDeath ) );
		}
	}
}

void CActor::OnAppearanceChanged( const CEntityAppearance& appearance )
{
	TBaseClass::OnAppearanceChanged( appearance );

	m_usesRobe = appearance.GetUsesRobe();

	CName voiceTag;

	if ( m_template.IsValid() )
	{
		voiceTag = m_template->GetApperanceVoicetag( appearance.GetName() );
	}

	// Process only if voice tag changes
	if ( m_voiceTag != voiceTag )
	{
		CStorySceneActorMap* sceneActorMap = NULL;
		CStorySceneSystem* sceneSystem = GCommonGame->GetSystem< CStorySceneSystem >();
		if ( sceneSystem != NULL )
		{
			sceneActorMap = sceneSystem->GetActorMap();
		}

		//// Remove actor from the scene actor map
		if ( sceneActorMap != NULL && m_voiceTag )
		{
			sceneActorMap->UnregisterSpeaker( this );
		}

		// Save
		m_voiceTag = voiceTag;

		// CODE SPLIT - refactor actor map while moving scene system
		//// Add actor to the scene actor map
		if ( sceneActorMap != NULL && m_voiceTag )
		{
			sceneActorMap->RegisterSpeaker( this, GetVoiceTag() );
		}
	}

	String componentName = TXT("voiceset_") + GetVoiceTag().AsString();
	m_hasInteractionVoicesetScene = ( Cast<CStorySceneComponent>( FindComponent( componentName , false ) ) != NULL );

	//some data fuckups in appearances
	///m_hasInteractionVoicesetScene = appearance.HasInteractionVoicesetScene();

	{
		ComponentIterator< CMimicComponent > it ( this );
		if ( it )
		{
			SetMimicComponent( *it );
		}
		else
		{
			// Mimic component doesn't have to be in appearance, it can also be attached as an inventory item ( see Geralt)
			// if actor has a head component mimic component will be set by itself, appearance doesn't have any head
			ComponentIterator< CHeadManagerComponent > head ( this );
			if ( !head )
			{
				SetMimicComponent( nullptr );
			}
		}
	}

	ComponentIterator< CInventoryComponent > inventory( this );
	if ( inventory )
	{
		(*inventory)->OnAppearanceChanged( true );
	}

	NotifyScenesAboutChangingApperance();

	CAppearanceComponent* appearanceComponent = GetAppearanceComponent();
	if ( appearanceComponent )
	{
		if ( appearanceComponent->CheckShouldSave() )
		{
			appearanceComponent->SetShouldSave( true );
		}
	}

	m_appearanceChanged = true;
}

Bool CActor::IsRotatedTowards( const CNode* node, Float maxAngle ) const
{
	Vector direction = node->GetWorldPosition() - GetWorldPosition();
	float yaw = EulerAngles::YawFromXY( direction.X, direction.Y );
	float angle = DistanceBetweenAnglesAbs( GetRotation().Yaw, yaw );

	if ( angle <= maxAngle )
	{
		return true;
	}

	return false;
}

Bool CActor::IsRotatedTowards( const Vector& point , Float maxAngle ) const
{
	Vector direction = point - GetWorldPosition();
	direction.Z = 0.0f;
	float yaw = RAD2DEG( MATan2( direction.Y, direction.X ) ) - 90.0f;
	float angle = DistanceBetweenAnglesAbs( GetRotation().Yaw, yaw );

	if ( angle <= maxAngle )
	{
		return true;
	}

	return false;
}

void CActor::Kill( Bool forced /*= false*/ )
{
	THandle< CActor > nullActor;
	CallFunction( this, CNAME( InterfaceKill ), forced, nullActor );
}

void CActor::Stun( Bool forced /*= false*/ )
{
	THandle< CActor > nullActor;
	CallFunction( this, CNAME( InterfaceStun ), forced, nullActor );
}

void CActor::EmptyHands( Bool drop /*= false*/ )
{
	EmptyHand( CNAME( r_weapon ), drop );
	EmptyHand( CNAME( l_weapon ), drop );
}

void CActor::EmptyHand( CName itemSlot, Bool drop /*= false */ )
{
	CInventoryComponent *inventoryComponent = m_inventoryComponent.Get();
	if ( inventoryComponent == NULL )
	{
		ITEM_ERR( TXT("CActor::EmptyHands - actor %s doesn't have inventory component!!!"), GetFriendlyName().AsChar() );
		return;
	}
	SItemUniqueId itemId = inventoryComponent->GetItemIdHeldInSlot( itemSlot );
	if ( itemId != SItemUniqueId::INVALID )
	{		
		if ( drop )
		{
			inventoryComponent->DropItem( itemId, false );
		}
		else
		{			
			inventoryComponent->UnMountItem( itemId, true );
		}
	}
}

Bool CActor::HasLatentItemAction() const
{
	return SItemEntityManager::GetInstance().HasActorAnyLatentAction( this );
}

Bool CActor::DrawItem( SItemUniqueId itemId, Bool instant )
{
	CInventoryComponent *inventoryComponent = m_inventoryComponent.Get();
	if( inventoryComponent == NULL || itemId == SItemUniqueId::INVALID )
	{
		return false;
	}
	return m_inventoryComponent.Get()->DrawItem( itemId, instant );
}

Bool CActor::HolsterItem( SItemUniqueId itemId, Bool instant )
{
	CInventoryComponent *inventoryComponent = m_inventoryComponent.Get();
	if( inventoryComponent == NULL || itemId == SItemUniqueId::INVALID )
	{
		return false;
	}

	return m_inventoryComponent.Get()->HolsterItem( itemId, instant );
}

Bool CActor::DrawWeaponAndAttack( SItemUniqueId itemId )
{
	CInventoryComponent *inventoryComponent = m_inventoryComponent.Get();
	if( inventoryComponent == NULL || itemId == SItemUniqueId::INVALID )
	{
		return false;
	}

	return  m_inventoryComponent.Get()->DrawWeaponAndAttack( itemId );
}

void CActor::SetAnimationTimeMultiplier( Float mult )
{
	for ( ComponentIterator< CAnimatedComponent > it( this ); it; ++it )
	{
		(*it)->SetTimeMultiplier( mult );
	}
}

Float CActor::GetAnimationTimeMultiplier() const
{
	CAnimatedComponent* animated = GetRootAnimatedComponent();
	if ( !animated )
		return 1.f;

	return animated->GetTimeMultiplier();
}

void CActor::ClearRotationTarget() const
{
	CMovingAgentComponent* mac = GetMovingAgentComponent();
	if( mac )
	{
		mac->ClearRotationTarget();
	}
}

void CActor::SetRotationTarget( const Vector& position, Bool clamping ) const
{
	CMovingAgentComponent* mac = GetMovingAgentComponent();
	if( mac )
	{
		mac->SetRotationTarget( position, clamping );
	}
}

void CActor::SetRotationTarget( const THandle< CNode >& node, Bool clamping ) const
{
	CMovingAgentComponent* mac = GetMovingAgentComponent();
	if( mac )
	{
		mac->SetRotationTarget( node, clamping  );
	}
}

const CAIAttackRange* CActor::GetAttackRange( const CName& attackRangeName /*= CName::NONE */ ) const
{
	return CAIAttackRange::Get( this, attackRangeName );
}

Bool CActor::InAttackRange( const CGameplayEntity* entity, const CName& rangeName /*= CName::NONE */ ) const
{
	const CAIAttackRange* attackRange = CAIAttackRange::Get( this, rangeName );
	if( !attackRange )
	{
		SET_ERROR_STATE( this, TXT("Attack range error") );
		WARN_GAME( TXT("CActor::InAttackRange error with attack range - '%ls'"), rangeName.AsString().AsChar() );
		return false;
	}
	return attackRange->Test( this, entity );
}

void CActor::OnProcessRequiredItemsFinish()
{
	CallEvent( CNAME( OnProcessRequiredItemsFinish ) );
}

//////////////////////////////////////////////////////////////
void CActor::NotifyScenesAboutChangingApperance()
{
	for ( CStorySceneController* c : m_storyScenes )
	{
		if ( c && c->GetPlayer() )
		{
			c->GetPlayer()->OnActorAppearanceChanged( this );
		}
	}
}

// ----------------------------------------------------------------------------

Bool CActor::WasInventoryVisibleLastFrame() const
{
	if ( const CInventoryComponent* inv = GetInventoryComponent() )
	{
		for ( const SInventoryItem& item : inv->GetItems() )
		{
			if ( item.IsMounted() && item.GetItemEntity() && item.GetItemEntity()->WasVisibleLastFrame() )
			{
				return true;
			}
		}
	}
	return false;
}

// ----------------------------------------------------------------------------
// Actions
// ----------------------------------------------------------------------------
void CActor::ActionCancelAll()
{
	// Stop current action
	if ( m_action )
	{
		m_action->Stop();

		OnActionEnded( m_action->GetType(), false );

		m_action = NULL;

		m_latentActionResult = ActorActionResult_Failed;
		m_latentActionType   = ActorAction_None;

		// We need to zero the index here - calling this method means that we're canceling all actions
		// that there are.
		// However - if we're calling it from a starting latent action, the index will get overwritten
		// to an actual one next ( or it should be ) - so no worries there... Ok - actually it's a huge
		// hack, but I've no idea how to refactor it well in such a short time, so I'm leaving these comments here.
		m_latentActionIndex  = 0;
	}
}

void CActor::ActionEnded( EActorActionType actionType, EActorActionResult result )
{
	// Update latent function state
	if ( m_latentActionType == actionType )
	{
		ASSERT( result == ActorActionResult_Failed || result == ActorActionResult_Succeeded );
		m_latentActionResult	= result;
		m_latentActionType		= ActorAction_None;

		OnActionEnded( actionType, result == ActorActionResult_Succeeded );
	}
	else
	{
		// ups - we're trying to end an action that's not currently active!!!
		ASSERT( false && "Trying to end an action that's not currently active" );
	}
}

void CActor::UpdateActions( Float timeDelta )
{
	PC_SCOPE( AI_UpdateActorActions );

	if ( m_action )
	{
		// Measure formation update time
		CTimeCounter timer;
		EActorActionType currActionType = m_action->GetType();

		if( m_action->Update( timeDelta ) == false )
		{
			m_action = NULL;

			// at this point 'ActionEnded' method should have been already called
			ASSERT( m_latentActionType == ActorAction_None && "'ActionEnded' hasn't been called, and yet an action has ended!" );

			// Variable 'm_latentActionIndex' used to be zeroed here.
			// However - all latent action invocations overwrite
			// this flag with the new value each time a new action gets called, and verify its contents
			// whenever a subsequent call to the latent action is made to verify some other
			// action didn't take the previous action's place.
			//
			// CASE I - latent action gets called and executed till the end uninterrupted
			//   1.) New index will be assigned to the 'm_latentActionIndex' variable
			//   2.) With each subsequent call, the index stored in the thread will be the equal to the one in the variable
			//   3.) When the method ends, 'ActionEnded' method will be called, setting the finish flag
			//   4.) The subsequent latent action call will verify that the action has ended and that it's latent action idx is
			//       the same as the one stored in the host thread - therefore the action will be considered to have finished uncanceled.
			//
			// CASE II - some other action takes place of a previously called one
			//   1.) New index will be assigned to the 'm_latentActionIndex' variable
			//   2.) The value stored in the script thread will be compared to it and the comparison will fail and the latent method
			//       will get finished returning false.
			//
			// Moreover - the 'm_latentActionIndex' is now changed only from within the latent action methods - which enforces the SRP principle.
		}
#ifndef NO_EDITOR
		// Was it slow ?
		const Float timeElapsed = timer.GetTimePeriod();
		if ( timeElapsed > 0.005f )
		{
			CEnum* actionTypeEnum = SRTTI::GetInstance().FindEnum( CNAME( EActorActionType ) );
			CName enumValName;
			actionTypeEnum->FindName( currActionType, enumValName );
			GScreenLog->PerfWarning( timeElapsed, TXT("ACTION"), TXT("Action '%ls' update was slow for '%ls'"), enumValName.AsString().AsChar(), GetFriendlyName().AsChar() );
		}
#endif
	}
}

void CActor::OnActionStarted( EActorActionType actionType )
{
	CEnum* actionTypeEnum = SRTTI::GetInstance().FindEnum( CNAME( EActorActionType ) );
	CName enumValName;
	actionTypeEnum->FindName( actionType, enumValName );

	ASSERT( m_action != NULL );
	String description = m_action->GetDescription();
	AI_EVENT_START( this, EAIE_Action, enumValName.AsString(), String::Printf( TXT("Actor %s started executing an action.\n%s"), GetName().AsChar(), description.AsChar() ) );


	// Pass to script
	{
		static CName eventName( TXT("OnActionStarted") );
		CallEvent( eventName, actionType );
	}

	// take look at into consideration
	UpdateLookAt( 0 );
}

void CActor::OnActionEnded( EActorActionType actionType, Bool result )
{
	AI_EVENT_END( this, EAIE_Action, result ? EAIR_Success : EAIR_Failure );
}

Bool CActor::CanPerformActionFromScript( CScriptStackFrame& stack ) const
{
	// Action not allowed when controlled externally
	if( IsExternalyControlled() )
	{
		SET_ERROR_STATE( this, TXT( "Trying to start action while externally controlled, callstack in log" ) );
		SCRIPT_LOG( stack, TXT( "Trying to start action while externally controlled, '%" ) RED_PRIWs TXT( "'" ), GetName().AsChar() );
		SCRIPT_DUMP_STACK_TO_LOG( stack );
		return false;
	}

	if( m_isAIControlled )
	{
		if( ( !stack.GetContext() ) || ( !stack.GetContext()->IsA< IBehTreeTask >() ) )
		{
			// What if the call stack looks like that :
			// ::BehTreeNodeMain
			// ::ComponentLatentFunction
			// ::CanPerformActionFromScript
			CScriptStackFrame* stackFrame = stack.m_parent;
			Bool isCallFromAI = false;
			while ( stackFrame && stackFrame->GetContext() )
			{
				if ( stackFrame->GetContext()->IsA< IBehTreeTask >() )
				{
					isCallFromAI = true;
					break;
				}
				stackFrame = stackFrame->m_parent;
			}
			if ( !isCallFromAI )
			{
				SET_ERROR_STATE( this, TXT( "Trying to start action while tree running, callstack in log" ) );
				SCRIPT_LOG( stack, TXT( "Trying to start action while tree running, '%" ) RED_PRIWs TXT( "'" ), GetName().AsChar() );
				SCRIPT_DUMP_STACK_TO_LOG( stack );
				return false;
			}
		}
	}

	return true;
}

Bool CActor::UseItem( SItemUniqueId itemId, Bool destroyItem )
{
	ASSERT( m_inventoryComponent.Get() );

	Bool result = ( CR_EventSucceeded == CallEvent( CNAME(OnItemUse), itemId ) );
	if ( destroyItem )
	{
		result &= m_inventoryComponent.Get()->RemoveItem( itemId );
	}
	return result;
}

void CActor::GenerateAIDebugInfo( CRenderFrame* frame, Uint32& lineCounter )
{
#ifdef EDITOR_AI_DEBUG
	if ( m_behTreeMachine && !m_behTreeMachine->IsStopped() )
	{
		String fullText( this->GetName().AsChar() );
		const Vector& pos = GetWorldPositionRef();
		const Uint32 MAX_LINE_LENGTH = 80;
		String text;
		m_behTreeMachine->DescribeAIState( text );
		Uint32 line = 0;
		while ( text.GetLength() > MAX_LINE_LENGTH )
		{
			size_t index = 0 ;
			if ( !text.FindCharacter( TXT('>'), index, MAX_LINE_LENGTH, false ) || index < MAX_LINE_LENGTH / 2 )
			{
				index = MAX_LINE_LENGTH;
			}
			String lineText = text.LeftString( index - 1 );
			text = text.RightString( text.Size() - index );
			frame->AddDebugText( pos, lineText, -200, lineCounter++, false );

			fullText += lineText;
		}
		if ( !text.Empty() )
		{
			frame->AddDebugText( pos, text, -200, lineCounter++, false );
			fullText += text;
		}

		//if ( !GGame->IsActivelyPaused() && !GGame->IsPaused() )
		//{
		//	RED_LOG( RED_LOG_CHANNEL(AITreeHistory), fullText.AsChar() );
		//}
	}
#endif
}
void CActor::GenerateAITicketInfo( CRenderFrame* frame, Uint32& lineCounter )
{
	CBehTreeInstance* instance = m_behTreeMachine ? m_behTreeMachine->GetBehTreeInstance() : nullptr;
	if ( instance )
	{
		TDynArray< String > textArr;
		instance->DescribeTicketsInfo( textArr );
		const Vector& pos = GetWorldPositionRef();
		for ( auto it = textArr.Begin(), end = textArr.End(); it != end; ++it )
		{
			frame->AddDebugText( pos, *it, -200, lineCounter++, false );
		}
		
	}
}

void CActor::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if( frame->GetFrameInfo().IsShowFlagOn( SHOW_ActorLodInfo ) && m_LOD.m_desiredLOD )
	{
		const Uint32 lodIndex = m_LOD.m_desiredLOD->m_index;

		Color textCol = Color::GRAY;
		switch ( lodIndex )
		{
		case 0: textCol = Color::WHITE; break;
		case 1: textCol = Color::GREEN; break;
		case 2: textCol = Color::BLUE; break;
		case 3: textCol = Color::RED; break;
		case 4: textCol = Color::YELLOW; break;
		}
		const String text = String::Printf( TXT("LOD: %d"), lodIndex );
		frame->AddDebugText( GetWorldPosition(), text, true, textCol, Color::BLACK, nullptr );
	}
}

void CActor::GenerateDebugFragments( CRenderFrame* frame )
{
	const CRenderFrameInfo& info = frame->GetFrameInfo();

	if( frame->GetFrameInfo().IsShowFlagOn( SHOW_AIRanges ) )
	{
		CMovingPhysicalAgentComponent * mpac = Cast<CMovingPhysicalAgentComponent>( GetMovingAgentComponent() );
		if( mpac )
		{
			mpac->OnGenerateEditorFragments( frame , SHOW_AIRanges );
		}
#ifndef NO_EDITOR
		if( m_debugAttackRange )
		{
			m_debugAttackRange->OnGenerateDebugFragments( this, frame );
		}
#endif
	}

#ifndef NO_ERROR_STATE
	if ( info.IsShowFlagOn( SHOW_ErrorState ) )
	{
		if ( IsInErrorState() && ! GetErrorState().Empty() )
		{
			frame->AddDebugText( GetWorldPosition() + Vector(0.f,0.f,1.8f), GetErrorState(), true, Color::RED, Color::BLACK );
		}
	}
#endif

	if( info.IsShowFlagOn( SHOW_AI ) )
	{
		if( IsInGame() )
		{
			CVisualDebug* visualDebug = GetVisualDebug();
			if( visualDebug )
			{
				CallFunction( this, CNAME( UpdateVisualDebug ) );
				visualDebug->Render( frame, GetLocalToWorld() );
			}
		}

		if ( m_action )
		{
			m_action->GenerateDebugFragments( frame );
		}
	}

	if ( info.IsShowFlagOn( SHOW_AIBehaviorDebug ) )
	{
		CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
		CBehTreeInstance* ai = behTreeMachine ? behTreeMachine->GetBehTreeInstance() : NULL;
		if ( ai )
		{
			IBehTreeNodeInstance* node = ai->GetInstanceRootNode();
			if ( node && node->IsActive() )
			{
				node->PropagateDebugFragmentsGeneration( frame );
			}
		}
	}

	Uint32 lineCounter = 0;
#ifdef EDITOR_AI_DEBUG
	if ( info.IsShowFlagOn( SHOW_AIBehTree ) )
	{
		GenerateAIDebugInfo( frame, lineCounter );
	}
#endif
	if ( info.IsShowFlagOn( SHOW_AITickets ) )
	{
		GenerateAITicketInfo( frame, lineCounter );
	}

	if ( info.IsShowFlagOn( SHOW_Behavior ) )
	{
		m_lookAtController.GenerateDebugFragments( frame, GetLocalToWorld() );
	}
}

Bool CActor::PlayEffect( const CName& effectName, const CName& boneName /*= CName::NONE*/, const CNode* targetNode )
{
	return TBaseClass::PlayEffect( effectName, boneName, targetNode );
}

Bool CActor::PlayEffectForAnimation( CName animationName, Float startTime /*=0.0f*/ )
{
	return TBaseClass::PlayEffectForAnimation( animationName, startTime );
}

void CActor::ApplyAppearance( const CName &appearanceName )
{
	CAppearanceComponent* appearanceComponent = GetAppearanceComponent();
	if ( appearanceComponent )
	{
		appearanceComponent->ApplyAppearance( appearanceName );
	}
	else
	{
		WARN_GAME( TXT("Appearance change to '%ls' requested, but the actor has no appearance component"), appearanceName.AsChar() );
	}
}

void CActor::SetCrossSafeZoneEnabled( Bool flag )
{
	CMovingAgentComponent* movingAgentComponent = GetMovingAgentComponent();
	ASSERT( movingAgentComponent );

	if ( movingAgentComponent )
	{
		movingAgentComponent->EnableCharacterCollisions( !flag );
		movingAgentComponent->ForceEntityRepresentation( flag, CMovingAgentComponent::LS_CActor );

		//movingAgentComponent->SetMotionEnabled( !flag, CMovingAgentComponent::LS_CActor );
	}
}

Bool CActor::IsRagdollObstacle() const
{
	if ( GetMovingAgentComponent() )
	{
		return GetMovingAgentComponent()->IsRagdollObstacle();
	}
	return false;
}

void CActor::ResetClothAndDangleSimulation()
{
	TBaseClass::ResetClothAndDangleSimulation();
	if( GetMovingAgentComponent() )
	{
		GetMovingAgentComponent()->SetTeleportDetectorForceUpdateOneFrame();							// this flag will be cleared when executed
		for ( EntityWithItemsComponentIterator< CComponent > it( this ); it; ++it )
		{
			CComponent* c = *it;
			c->OnResetClothAndDangleSimulation();
		}
	}
}


Bool CActor::SetMovementType( EExplorationState state )
{
	DeactivateMovementType( m_movementType );
	if ( ActivateMovementType( state ) )
	{
		// new state was successfully activated
		m_movementType = state;
		return true;
	}
	else
	{
		// couldn't activate the new state - activate the old one
		ActivateMovementType( m_movementType );
		return false;
	}
}

Bool CActor::ActivateMovementType( EExplorationState state )
{
	CInventoryComponent* inventory = GetInventoryComponent();
	if ( !inventory )
	{
		return false;
	}

	SItemUniqueId itemId;
	switch( state )
	{
	case EX_Normal:
		{
			return true;
		}

	case EX_Drunk:
		{
			itemId = inventory->GetItemId( CNAME( DrunkItem ) );
			if ( itemId == SItemUniqueId::INVALID )
			{
				inventory->AddItem( CNAME( DrunkItem ) );
				itemId = inventory->GetItemId( CNAME( DrunkItem ) );
			}
			break;
		}

	case EX_Injured:
		{
			itemId = inventory->GetItemId( CNAME( InjuredItem ) );
			if ( itemId == SItemUniqueId::INVALID )
			{
				inventory->AddItem( CNAME( InjuredItem ) );
				itemId = inventory->GetItemId( CNAME( InjuredItem ) );
			}
			break;
		}

	case EX_Stealth:
		{
			itemId = inventory->GetItemId( CNAME( SneakItem ) );
			if ( itemId == SItemUniqueId::INVALID )
			{
				inventory->AddItem( CNAME( SneakItem ) );
				itemId = inventory->GetItemId( CNAME( SneakItem ) );
			}
			break;
		}

	case EX_WithWeapon:
		{
			itemId = inventory->GetItemByCategory( CNAME( opponent_weapon ), false );
			break;
		}

	case EX_WithRangedWeapon:
		{
			itemId = inventory->GetItemByCategory( CNAME( opponent_bow ), false );
			break;
		}

	case EX_CarryTorch:
		{
			itemId = inventory->GetItemId( CNAME( Torch ) );
			if ( itemId == SItemUniqueId::INVALID )
			{
				inventory->AddItem( CNAME( Torch ) );
				itemId = inventory->GetItemId( CNAME( Torch ) );
			}
			break;
		}
	case EX_WithPickaxe:
		{
			itemId = inventory->GetItemId( CNAME( Pick_02 ) );
			if ( itemId == SItemUniqueId::INVALID )
			{
				inventory->AddItem( CNAME( Pick_02 ) );
				itemId = inventory->GetItemId( CNAME( Pick_02 ) );
			}
			break;
		}
	}

	if ( itemId != SItemUniqueId::INVALID )
	{
		CInventoryComponent::SMountItemInfo mountInfo;
		mountInfo.m_toHand = true;
		inventory->MountItem( itemId, mountInfo );
		return true;
	}
	else
	{
		WARN_GAME( TXT( "Couldn't mount an exploration item" ) );
		return false;
	}
}

void CActor::DeactivateMovementType( EExplorationState state )
{
	CInventoryComponent* inventory = GetInventoryComponent();
	if ( !inventory )
	{
		return;
	}

	SItemUniqueId itemId;
	switch( state )
	{
	case EX_Drunk:
		{
			itemId = inventory->GetItemId( CNAME( DrunkItem ) );
			break;
		}

	case EX_Injured:
		{
			itemId = inventory->GetItemId( CNAME( InjuredItem ) );
			break;
		}

	case EX_Stealth:
		{
			itemId = inventory->GetItemId( CNAME( SneakItem ) );
			break;
		}

	case EX_WithWeapon:
		{
			itemId = inventory->GetItemByCategory( CNAME( opponent_weapon ) );
			break;
		}

	case EX_WithRangedWeapon:
		{
			itemId = inventory->GetItemByCategory( CNAME( opponent_bow ) );
			break;
		}

	case EX_CarryTorch:
		{
			itemId = inventory->GetItemId( CNAME( Torch ) );
			break;
		}

	case EX_WithPickaxe:
		{
			itemId = inventory->GetItemId( CNAME( Pick_02 ) );
			break;
		}
	}

	inventory->UnMountItem( itemId );
}

#ifndef NO_ERROR_STATE

void CActor::SetErrorState( const String &description ) const
{
	m_dbgErrorState = description;
	m_dbgIsErrorState = true;
}

void CActor::SetErrorState( const Char* description ) const
{
	m_dbgErrorState = description;
	m_dbgIsErrorState = true;
}

Bool CActor::IsInErrorState() const
{
	return m_dbgIsErrorState;
}

const String &CActor::GetErrorState() const
{
	return m_dbgErrorState;
}

#endif // NO_ERROR_STATE

Bool CActor::IsGameplayLODable()
{
	CMovingAgentComponent* movingAgentComponent = GetMovingAgentComponent();
	CAnimatedComponent* animatedComponent = movingAgentComponent ? movingAgentComponent : TBaseClass::GetRootAnimatedComponent();
	return animatedComponent ? animatedComponent->IsGameplayLODable() : true;
}

void CActor::UpdateLODLevel( const SActorLODInstanceInfo* instanceInfo )
{
	CMovingAgentComponent* movingAgentComponent = GetMovingAgentComponent();
	CAnimatedComponent* animatedComponent = movingAgentComponent ? movingAgentComponent : TBaseClass::GetRootAnimatedComponent();

	// Is this actor LODable at all (at the moment)?
	// Note: This can change at runtime depending on e.g. whether an actor is part of the scene/cutscene

	const Bool isLODable = IsGameplayLODable();
	const SActorLODConfig* desiredLOD = m_LOD.m_desiredLOD;

	// Actor visibility change
	const Bool shouldBeHiddenDueToDistance = !m_isVisibileFromFar && isLODable && desiredLOD->m_hide;
	if ( IsHiddentInGame( HR_Lod ) != shouldBeHiddenDueToDistance )
	{
		SetHideInGame( shouldBeHiddenDueToDistance, false, HR_Lod );
	}

	// Animated component specific

	if ( animatedComponent )
	{
		const Bool isLongInvisible = m_LOD.m_timeInvisible >= GGame->GetGameplayConfig().m_LOD.m_actorInvisibilityTimeThreshold;
		const Float overrideBudgetedTickDistance = animatedComponent->GetOverrideBudgetedTickDistance();
		const Bool disableTickBudgetingDueToDistance = overrideBudgetedTickDistance != 0.0f && instanceInfo->m_distanceSqr < Red::Math::MSqr( overrideBudgetedTickDistance );

#if 0 // Can't suppress animation because it stops sending animation events too; in some cases we need events even if actors are far away or invisible

		// Animation suppression due to visibility

		const Bool shouldSuppressAnimationDueToVisibility = isLODable && desiredLOD->m_suppressAnimatedComponentIfNotVisible && isLongInvisible;
		if ( animatedComponent->IsAnimationSuppressed( CAnimatedComponent::AnimationSuppressionReason_Visibility ) != shouldSuppressAnimationDueToVisibility )
		{
			animatedComponent->SuppressAnimation( shouldSuppressAnimationDueToVisibility, CAnimatedComponent::AnimationSuppressionReason_Visibility );
		}

		// Animation suppression due to distance

		const Bool shouldSuppressAnimatedComponentDueToDistance = isLODable && desiredLOD->m_suppressAnimatedComponent;
		if ( animatedComponent->IsAnimationSuppressed( CAnimatedComponent::AnimationSuppressionReason_Distance ) != shouldSuppressAnimatedComponentDueToDistance )
		{
			animatedComponent->SuppressAnimation( shouldSuppressAnimatedComponentDueToDistance, CAnimatedComponent::AnimationSuppressionReason_Distance );
		}
#endif
		// Budgeting tick due to visibility

		const Bool shouldBudgetAnimationDueToVisibility = isLODable && desiredLOD->m_budgetAnimatedComponentTickIfNotVisible && isLongInvisible;
		if ( animatedComponent->IsTickBudgeted( CComponent::BR_Visibility ) != shouldBudgetAnimationDueToVisibility )
		{
			animatedComponent->SetTickBudgeted( shouldBudgetAnimationDueToVisibility, CComponent::BR_Visibility );
		}

		// Budgeting tick due to distance

		const Bool shouldBudgetAnimatedComponentTickDueToDistance = isLODable && desiredLOD->m_budgetAnimatedComponentTick && !disableTickBudgetingDueToDistance;
		if ( animatedComponent->IsTickBudgeted( CComponent::BR_Distance ) != shouldBudgetAnimatedComponentTickDueToDistance )
		{
			animatedComponent->SetTickBudgeted( shouldBudgetAnimatedComponentTickDueToDistance, CComponent::BR_Distance );
		}

		// Animated component frame skip

		const Uint32 expectedUpdateSkip = isLODable ? desiredLOD->m_animatedComponentUpdateFrameSkip : 0;
		if ( animatedComponent->GetSkipUpdateAndSampleFrames() != expectedUpdateSkip )
		{
			animatedComponent->SetSkipUpdateAndSampleFrames( expectedUpdateSkip );
		}

		// Moving agent component specific

		if ( movingAgentComponent )
		{
			// IK

			SAnimationProxyData& animationProxyData = movingAgentComponent->AccessAnimationProxy();

			const Bool shouldEnableIK = !isLODable || desiredLOD->m_enableIK;
			if ( animationProxyData.IsIKEnabledDueToLOD() != shouldEnableIK )
			{
				animationProxyData.EnableIKDueToLOD( shouldEnableIK );
			}

			// Set motion and/or physical representation request basing on actor's surroundings

			const CMovingAgentComponent::ERequest enablePhysReq  = movingAgentComponent->GetPathAgent()->IsOnNavdata() ? CMovingAgentComponent::Req_Off : CMovingAgentComponent::Req_On;
			const CMovingAgentComponent::ERequest disablePhysReq = instanceInfo->m_hasCollisionDataAround ? CMovingAgentComponent::Req_Off : CMovingAgentComponent::Req_On;
			const Bool conflictingRequests = m_freezeOnFailedPhysicalRequest && ( enablePhysReq == CMovingAgentComponent::Req_On && disablePhysReq == CMovingAgentComponent::Req_On );

			const Bool shouldEnableMotion = instanceInfo->m_hasNavigationDataAround && !conflictingRequests;

			movingAgentComponent->SetMotionEnabled( shouldEnableMotion, CMovingAgentComponent::LS_LOD );

			movingAgentComponent->SetPhysicalRepresentationRequest( enablePhysReq, CMovingAgentComponent::LS_LOD, disablePhysReq, CMovingAgentComponent::LS_Force );
		}
	}
}

// ----------------------------------------------------------------------------
// Push animation controller
// ----------------------------------------------------------------------------

void CActor::PushInDirection( const Vector& pusherPos, const Vector& pushDir, Float speed, Bool playAnimation, Bool applyRotation )
{
	CMovingAgentComponent* myMac = GetMovingAgentComponent();
	if ( !myMac || pushDir.Mag3() < 1e-1 )
	{
		return;
	}

	myMac->RelaxRotationTarget( 2.0f );

	// calculate the pushing factors
	Vector myPos = myMac->GetWorldPosition();
	Float destYaw = pushDir.Normalized2().ToEulerAngles().Yaw;
	Float rot = EulerAngles::AngleDistance( myMac->GetWorldYaw(), destYaw );
	Float facingAngle = Vector::Dot2( myMac->GetWorldForward(), pusherPos - myPos );

	EPushingDirection animDirection;
	if ( facingAngle > 0 )
	{
		rot = -rot;
		animDirection = EPD_Front;
	}
	else
	{
		animDirection = EPD_Back;
	}

	if ( applyRotation == false )
	{
		// don't rotate the agent if the user doesn't want it
		rot = 0.0f;
	}

	if ( m_action )
	{
		// there's an action in progress - make it decide what to do
		m_action->OnBeingPushed( pushDir, rot, speed, animDirection );
	}
	else
	{
		// actor's not performing any action - so just slide it
		myMac->Slide( pushDir, EulerAngles( 0, 0, rot ), &speed );
		CallEvent( CNAME( OnPushEffects ), animDirection );
	}

	if ( playAnimation )
	{
		PlayPushAnimation( animDirection );
	}
}

void CActor::PlayPushAnimation( EPushingDirection animDirection )
{
	CName animName;
	if ( animDirection == EPD_Front )
	{
		animName = m_frontPushAnim;
	}
	else
	{
		animName = m_backPushAnim;
	}

	if ( m_pushController )
	{
		m_pushController->PlayAnimation( animName );
	}
}

CActor::PushReactionController::PushReactionController( CBehaviorGraphStack* behaviorStack )
	: m_behaviorStack( behaviorStack )
	, m_animationBeingPlayed( false )
{}

void CActor::PushReactionController::PlayAnimation( CName animName )
{
	if ( m_animationBeingPlayed )
	{
		return;
	}

	// Setup slot
	SBehaviorSlotSetup slotSetup;
	slotSetup.m_blendInType = BTBM_Destination;
	slotSetup.m_blendIn = 0.2f;
	slotSetup.m_blendOutType = BTBM_Source;
	slotSetup.m_blendOut = 0.2f;
	slotSetup.m_motionEx = false;
	slotSetup.m_listener = this;

	// Play animation
	m_animationBeingPlayed = m_behaviorStack->PlaySlotAnimation( CNAME( REACTION_SLOT ), animName, &slotSetup );
}

void CActor::PushReactionController::StopAnimation()
{
	m_behaviorStack->StopSlotAnimation( CNAME( REACTION_SLOT ) );
	m_animationBeingPlayed = false;
}

void CActor::PushReactionController::OnSlotAnimationEnd( const CBehaviorGraphAnimationBaseSlotNode * sender, CBehaviorGraphInstance& instance, ISlotAnimationListener::EStatus status )
{
	m_animationBeingPlayed = false;
}

void CActor::CacheSoundParams( Bool force /* = false */ )
{
	if( ! force && m_cachedSoundParams != NULL )
	{
		return;
	}

	if ( m_template.IsValid() )
	{
		m_cachedSoundParams = m_template->FindParameter< CSoundEntityParam >( true );
	}
}

void CActor::MuteHeadAudio( Bool mute )
{
	if( m_headBoneIndex != -1 )
	{
		if( CSoundEmitterComponent* soundEmitterComponent = GetSoundEmitterComponent( false ) )
		{
			soundEmitterComponent->SoundParameter( "vo_head_mute", mute ? 0.0f : 1.0f, 0.3f, m_headBoneIndex );
		}
	}
}

void CActor::PredictWorldPosition( Float inTime, Vector& outPosition ) const
{
	if ( inTime > 0.f )
	{
		// ai prediction
		{
			SAIPositionPrediction positionPreditionParam( inTime );
			SignalGameplayEvent( SAIPositionPrediction::EventName(), &positionPreditionParam, SAIPositionPrediction::GetStaticClass() );
			if ( positionPreditionParam.m_outIsHandled )
			{
				outPosition = positionPreditionParam.m_outPosition;
				return;
			}
		}

		// animation prediction
		if (CMovingAgentComponent* mac = GetMovingAgentComponent())
		{
			outPosition = mac->PredictWorldPosition( inTime );
			return;
		}
	}
	outPosition = GetWorldPosition();
}

CAppearanceComponent* CActor::GetAppearanceComponent() const
{
	return CAppearanceComponent::GetAppearanceComponent( this );
}

CName CActor::GetAppearance() const
{
	CAppearanceComponent* apperanceComponent = GetAppearanceComponent();
	return apperanceComponent ? apperanceComponent->GetAppearance() : CName::NONE;
}

Bool CActor::IsAttackedBy( CActor* attacker ) const
{
	for( Uint32 i = 0, n = m_attackers.Size(); i < n; ++i )
	{
		if( m_attackers[i] == attacker )
		{
			return true;
		}
	}

	return false;
}

Bool CActor::RegisterAttacker( CActor* attacker, Bool registration )
{
	if( registration )
	{
		return m_attackers.PushBackUnique( attacker );
	}
	else
	{
		return m_attackers.Remove( attacker );
	}
}

CActor* CActor::GetAttacker(Uint32 index) const
{
	if( index < m_attackers.Size() )
	{
		return m_attackers[index];
	}

	return NULL;
}

void CActor::GetStorageBounds( Box& box ) const
{
	box.Min = box.Max = GetWorldPositionRef();

	if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( GetMovingAgentComponent() ) )
	{
		if ( const CMRPhysicalCharacter* character = mpac->GetPhysicalCharacter() )
		{
			const Float radius = ::Max( character->GetCombatRadius(), mpac->GetAvoidanceRadius() );
			const Float height = character->GetCurrentHeight();
			const Vector& worldPosition = GetWorldPositionRef();

			box.Min = worldPosition - Vector( radius, radius, 0.0f );
			box.Max = worldPosition + Vector( radius, radius, height );
		}		
	}

	if ( const CEntityTemplate* entityTemplate = GetEntityTemplate() )
	{
		if ( const CAttackableArea* params = entityTemplate->FindGameplayParamT< CAttackableArea >() )
		{
			Box arBox;
			const Vector cylinderWorldPos = m_localToWorld.TransformPoint( params->GetOffset() );
			arBox.Min = cylinderWorldPos - Vector( params->GetRadius(), params->GetRadius(), 0.0f );
			arBox.Max = cylinderWorldPos + Vector( params->GetRadius(), params->GetRadius(), params->GetHeight() );
			box.AddBox( arBox );
		}
	}
}

void CActor::ReportTerminationDeath()
{
	for( Int32 i = m_terminationListeners.Size() - 1; i >= 0 ; --i )
	{
		m_terminationListeners[i]->OnDeath( this );
	}
	GCommonGame->GetSystem< CStrayActorManager >()->ConvertToStrayActor( this );
}

void CActor::ReportTerminationDetach()
{
	for( Int32 i = m_terminationListeners.Size() - 1; i >= 0 ; --i )
	{
		m_terminationListeners[i]->OnDetach( this );
	}
	m_terminationListeners.ClearFast();

	if( CheckEntityFlag( EF_Poolable ) )
	{
		GCommonGame->GetEntityPool()->SignalEntityUnspawn( this );
	}
}


void CActor::ReportTerminationPrePooling()
{
	for( Int32 i = m_terminationListeners.Size() - 1; i >= 0 ; --i )
	{
		m_terminationListeners[i]->OnPrePooling( this );
	}
}

Bool CActor::RegisterTerminationListener( Bool registerListener, IActorTerminationListener* listener )
{
	Int32 index = -1;
	for( Uint32 i = 0; i < m_terminationListeners.Size(); ++i )
	{
		if( m_terminationListeners[i] == listener )
		{
			index = i;
			break;
		}
	}

	if( registerListener )
	{
		if ( index < 0)
		{
			m_terminationListeners.PushBack( listener );
			return true;
		}
	}
	else
	{
		if( index >= 0 )
		{
			m_terminationListeners.RemoveAt( index );
			return true;
		}
	}

	return false;
}

Bool CActor::GetAnimCombatSlots( const CName& animSlotName, TDynArray< Matrix >& slots, Uint32 slotsNum,
	const Matrix& mainEnemyMatrix ) const
{
	if( !m_template )
	{
		WARN_GAME( TXT("CActor::GetAnimCombatSlots: entity template is detached, cannot acces animation slots params.") );
		return false;
	}

	TDynArray< CAnimSlotsParam* > params;
	m_template->GetAllParameters( params );

	const CAnimationSlots* animSlots = NULL;
	for( Uint32 i = 0; i < params.Size(); ++i )
	{
		animSlots = params[i]->FindAnimationSlots( animSlotName );
		if( animSlots )
		{
			break;
		}
	}

	if( !animSlots )
	{
		WARN_GAME( TXT("CActor::GetAnimCombatSlots: animation slots not found! ADD THIS ANIMATION (%s) TO ANIMATION SLOTS NOW!!! DO IT!!!"), animSlotName.AsString().AsChar() );
		return false;
	}

	if( animSlots->m_transforms.Size() < slotsNum )
	{
		WARN_GAME( TXT("CActor::GetAnimCombatSlots: not enough slots(%i) in animation!"), slotsNum );
		return false;
	}

	// Get slots transform
	for ( Uint32 i = 0; i < slotsNum; ++i )
	{
		Matrix mat = Matrix::Mul( mainEnemyMatrix, animSlots->m_transforms[i] );
		slots.PushBack( mat );
	}

	return true;
}

// ----------------------------------------------------------------------------
// AI event handling and debug
// ----------------------------------------------------------------------------

void CActor::AttachAIDebugListener( IAIDebugListener& listener )
{
	m_aiDebugListeners.PushBackUnique( &listener );
}

void CActor::DetachAIDebugListener( IAIDebugListener& listener )
{
	m_aiDebugListeners.Remove( &listener );
}

void CActor::OnAIEvent( EAIEventType type, EAIEventResult result, const String& name, const String& description )
{
	Float time = (Float)GGame->GetEngineTime();

	for ( TDynArray< IAIDebugListener* >::iterator it = m_aiDebugListeners.Begin(); it != m_aiDebugListeners.End(); ++it )
	{
		(*it)->OnAIEvent( type, result, name, description, time );
	}
}

void CActor::OnAIEventStart( EAIEventType type, const String& name, const String& description )
{
	Float startTime = (Float)GGame->GetEngineTime();

	for ( TDynArray< IAIDebugListener* >::iterator it = m_aiDebugListeners.Begin(); it != m_aiDebugListeners.End(); ++it )
	{
		(*it)->OnAIEventStart( type, name, description, startTime );
	}
}

void CActor::OnAIEventEnd( EAIEventType type, EAIEventResult result )
{
	Float endTime = (Float)GGame->GetEngineTime();
	for ( TDynArray< IAIDebugListener* >::iterator it = m_aiDebugListeners.Begin(); it != m_aiDebugListeners.End(); ++it )
	{
		(*it)->OnAIEventEnd( endTime, type, result );
	}
}

CActor* CActor::GetTarget() const
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( behTreeMachine && behTreeMachine->GetBehTreeInstance() )
	{
		return behTreeMachine->GetBehTreeInstance()->GetCombatTarget().Get();
	}
	return NULL;
}

CActor* CActor::GetScriptTarget() const
{
	return GetTarget();
}

void CActor::ForceTargetUpdate()
{
}

void CActor::OnCombatModeSet( Bool b )
{
	CallFunction( this, CNAME( OnCombatModeSet ), b );
}

void CActor::SignalGameplayEvent( CName name, void* additionalData, IRTTIType* additionalDataType ) const
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( behTreeMachine )
	{
		behTreeMachine->OnGameplayEvent( name, additionalData, additionalDataType );
	}
}

void CActor::SignalGameplayEvent( CName name, CName param ) const
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( behTreeMachine )
	{
		SGameplayEventParamCName eventParamCName( param );
		void* pEventParamCName = static_cast< void* >( &eventParamCName );
		behTreeMachine->OnGameplayEvent( name, pEventParamCName, SGameplayEventParamCName::GetStaticClass() );
	}
}

void CActor::SignalGameplayEvent( CName name, Int32 param ) const
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( behTreeMachine )
	{
		SGameplayEventParamInt eventParamInt( param );
		void* pEventParamInt = static_cast< void* >( &eventParamInt );
		behTreeMachine->OnGameplayEvent( name, pEventParamInt, SGameplayEventParamInt::GetStaticClass() );
	}
}

void CActor::SignalGameplayEvent( CName name, Float param ) const
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( behTreeMachine )
	{
		SGameplayEventParamFloat eventParamFloat( param );
		void* pEventParamFloat = static_cast< void* >( &eventParamFloat );
		behTreeMachine->OnGameplayEvent( name, pEventParamFloat, SGameplayEventParamFloat::GetStaticClass() );
	}
}

void CActor::SignalGameplayEvent( CName name, CObject* param ) const
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( behTreeMachine )
	{
		SGameplayEventParamObject eventParamObject( param );
		void* pEventParamObject = static_cast< void* >( &eventParamObject );
		behTreeMachine->OnGameplayEvent( name, pEventParamObject, SGameplayEventParamObject::GetStaticClass() );
	}
}

//! Signals event that is directly passed to AI system with a CName parameter
CName CActor::SignalGameplayEventReturnCName( CName name, CName defaultVal ) const
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( behTreeMachine )
	{
		SGameplayEventParamCName eventData( defaultVal );
		behTreeMachine->OnGameplayEvent( name, &eventData, SGameplayEventParamCName::GetStaticClass() );
		return eventData.m_value;
	}
	return defaultVal;
}

//! Signals event that is directly passed to AI system with an int parameter
Int32 CActor::SignalGameplayEventReturnInt( CName name, Int32 defaultVal ) const
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( behTreeMachine )
	{
		SGameplayEventParamInt eventData( defaultVal );
		behTreeMachine->OnGameplayEvent( name, &eventData, SGameplayEventParamInt::GetStaticClass() );
		return eventData.m_value;
	}
	return defaultVal;
}

//! Signals event that is directly passed to AI system with a float parameter
Float CActor::SignalGameplayEventReturnFloat( CName name, Float defaultVal ) const
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( behTreeMachine )
	{
		SGameplayEventParamFloat eventData( defaultVal );
		behTreeMachine->OnGameplayEvent( name, &eventData, SGameplayEventParamFloat::GetStaticClass() );
		return eventData.m_value;
	}
	return defaultVal;
}

void CActor::SignalGameplayDamageEvent( CName name, CBaseDamage* additionalData )
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( behTreeMachine )
	{
		behTreeMachine->OnGameplayEvent( name, static_cast<void*>( additionalData ), additionalData->GetClass() );
	}
}

void CActor::SignalGameplayAnimEvent( CName name, CName additionalData )
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( behTreeMachine )
	{
		SGameplayEventParamCName eventParamCName( additionalData );
		void* pEventParamCName = static_cast< void* >( &eventParamCName );
		behTreeMachine->OnGameplayEvent( name, pEventParamCName, SGameplayEventParamCName::GetStaticClass() );
	}
}

Bool CActor::ForceAIBehavior( IAITree* aiTree, Int8 priority, Int16* forcedActionId, CName forceEventName, IGameDataStorage* aiState, EAIForcedBehaviorInterruptionLevel interruptionLevel )
{
	SForcedBehaviorEventData data( aiTree, priority, false, aiState, interruptionLevel );
	SignalGameplayEvent( forceEventName, &data, SForcedBehaviorEventData::GetStaticClass() );
	if ( forcedActionId )
	{
		(*forcedActionId) = data.m_uniqueActionId;
	}
	return data.m_isHandled == SForcedBehaviorEventData::OUTPUT_HANDLED;
}

Bool CActor::CancelAIBehavior( Int16 forcedActionId, CName cancelEventName, Bool fireEventAndForget )
{
	SForcedBehaviorEventData data( forcedActionId, true, fireEventAndForget );
	SignalGameplayEvent( cancelEventName, &data, SForcedBehaviorEventData::GetStaticClass() );

	if ( fireEventAndForget )
	{
		return data.m_isHandled != SForcedBehaviorEventData::OUTPUT_DELAYED;
	}
	else
	{
		return data.m_isHandled == SForcedBehaviorEventData::OUTPUT_DELAYED || data.m_isHandled == SForcedBehaviorEventData::OUTPUT_HANDLED;
	}
}

Bool CActor::ForceDynamicBehavior( IAITree* tree, CName forceEventName, Bool interrupt )
{
	SBehTreeDynamicNodeEventData data( tree, interrupt );
	SignalGameplayEvent( forceEventName, &data, SBehTreeDynamicNodeEventData::GetStaticClass() );
	return data.m_isHandled == SBehTreeDynamicNodeEventData::OUTPUT_HANDLED;
}
Bool CActor::CancelDynamicBehavior( CName forceEventName, Bool interrupt )
{
	SBehTreeDynamicNodeCancelEventData data( interrupt );
	SignalGameplayEvent( forceEventName, &data, SBehTreeDynamicNodeCancelEventData::GetStaticClass() );
	return data.m_isHandled == SBehTreeDynamicNodeCancelEventData::OUTPUT_HANDLED;
}

CBehTreeMachine* CActor::GetBehTreeMachine() const
{
	return m_behTreeMachine;
}

void CActor::onCollision( const SPhysicalCollisionInfo& info )
{
	return;
/*	if( !info.m_otherBody ) return;

	if( info.m_otherBody->IsStatic() ) return;

	THandle< CComponent > component = info.m_otherBody->GetParent();
	if( !component.Get() ) return;

	//TEMPORARY // will be replaced by filtering out collision between all physical representation from one actor instance
	CObject* componentParent = component.Get()->GetParent();
	if ( componentParent == this ) return;
	if( componentParent && componentParent->GetParent() == this ) return;
	//TEMPORARY // will be replaced by filtering out collision between all physical representation from one actor instance

	Int32 boneIndex = info.m_otherBodyIndex.m_actorIndex;
	Int32 shapeIndex = info.m_otherBodyIndex.m_shapeIndex;
	CallEvent( CNAME( OnContactEvent ), info.m_position, info.m_force, component, boneIndex, shapeIndex );*/
}

void CActor::SetAlive( Bool flag )
{
	m_alive = flag;
}

const Bool CActor::IsDangerous( CActor* actor )
{
	// We dislike them
	return GetAttitude( actor ) == AIA_Hostile;
}

EAIAttitude	CActor::GetAttitude( CActor* actor )
{
	if ( actor == NULL )
	{
		WARN_GAME( TXT("CActor::GetAttitude NULL actor '%ls'"), GetName().AsChar() );
		return CAttitudes::GetDefaultAttitude();
	}

	CAttitudeManager *atMan = GCommonGame->GetSystem< CAttitudeManager >();
	RED_ASSERT( atMan != nullptr, TXT( "AttitudeManager is null") );
	EAIAttitude attitude = CAttitudes::GetDefaultAttitude();

	// check if attitude between actors is defined
	if ( atMan->GetActorAttitude( this, actor, attitude ) )
	{
		return attitude;
	}

	// if not defined, check attitude between groups
	const CName srcAttitudeGroup = GetAttitudeGroup();
	const CName dstAttitudeGroup = actor->GetAttitudeGroup();
	if ( srcAttitudeGroup != CName::NONE && srcAttitudeGroup != CNAME( default ) &&
		dstAttitudeGroup != CName::NONE && dstAttitudeGroup != CNAME( default ) )
	{
		if ( atMan->GetGlobalAttitude( srcAttitudeGroup, dstAttitudeGroup, attitude ) )
		{
			return attitude;
		}
	}

	// if not defined, neutral attitude will be returned
	return CAttitudes::GetDefaultAttitude();
}

void CActor::SetAttitude( CActor* actor, EAIAttitude attitude )
{
	if ( actor != nullptr && actor != this )
	{
		CAttitudeManager *atMan = GCommonGame->GetSystem< CAttitudeManager >();
		RED_ASSERT( atMan != nullptr, TXT( "AttitudeManager is null") );
		VERIFY( atMan->SetActorAttitude( this, actor, attitude ) );
		ForceTargetUpdate();
	}
}

void CActor::ResetAttitude( CActor* actor )
{
	if ( actor != nullptr && actor != this )
	{
		CAttitudeManager *atMan = GCommonGame->GetSystem< CAttitudeManager >();
		RED_ASSERT( atMan != nullptr, TXT( "AttitudeManager is null") );
		VERIFY( atMan->ResetActorAttitude( this, actor ) );
		ForceTargetUpdate();
	}
}

void CActor::ClearAttitudes( Bool hostile, Bool neutral, Bool friendly )
{
	CAttitudeManager *atMan = GCommonGame->GetSystem< CAttitudeManager >();
	RED_ASSERT( atMan != nullptr, TXT( "AttitudeManager is null") );
	atMan->RemoveActorAttitudes( this, hostile, neutral, friendly );
}

Bool CActor::GetAttitudeMap( TActorAttitudeMap &attitudeMap )
{
	CAttitudeManager *atMan = GCommonGame->GetSystem< CAttitudeManager >();
	RED_ASSERT( atMan != nullptr, TXT( "AttitudeManager is null") );
	return atMan->GetAttitudeMapForActor( this, attitudeMap );
}

void CActor::SetBaseAttitudeGroup( CName groupName )
{
	m_baseAttitudeGroup = groupName;
	UpdateCurrentAttitudeGroup();
}

void CActor::ResetBaseAttitudeGroup( Bool force )
{
	if ( force )
	{
		m_baseAttitudeGroup = CName::NONE;
	}
	LoadBaseAttitudeGroup();
	UpdateCurrentAttitudeGroup();
}

void CActor::LoadBaseAttitudeGroup()
{
	// don't load base attitude group from template if it was already loaded from save
	if ( m_baseAttitudeGroup == CName::NONE && m_template )
	{
		CAIProfile* profile = m_template->FindParameter< CAIProfile >( false, SAttitudesAIProfilePred() );
		if ( profile != nullptr )
		{
			m_baseAttitudeGroup = profile->GetAttitudeGroup();
		}
		else
		{
			profile = m_template->FindParameter< CAIProfile >( true, SAttitudesAIProfilePred() );
			if ( profile != nullptr )
			{
				m_baseAttitudeGroup = profile->GetAttitudeGroup();
			}
		}
	}
}

void CActor::SetTemporaryAttitudeGroup( CName groupName, Int32 priority )
{
	TTemporaryAttitudeGroups::iterator it = m_temporaryAttitudeGroups.Find( priority );
	if ( it != m_temporaryAttitudeGroups.End() )
	{
		it->m_second = groupName;
	}
	else
	{
		m_temporaryAttitudeGroups.Insert( priority, groupName );
	}
	UpdateCurrentAttitudeGroup();
}

void CActor::ResetTemporaryAttitudeGroup( Int32 priority )
{
	TTemporaryAttitudeGroups::iterator it = m_temporaryAttitudeGroups.Find( priority );
	if ( it != m_temporaryAttitudeGroups.End() )
	{
		m_temporaryAttitudeGroups.Erase( it );
		UpdateCurrentAttitudeGroup();
	}
}

Bool CActor::OnPoolRequest()
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	CBehTreeInstance* ai = behTreeMachine ? behTreeMachine->GetBehTreeInstance() : nullptr;
	if ( ai && !ai->OnPoolRequest() )
	{
		return false;
	}
	if( IsInNonGameplayScene() )
	{
		return false;
	}
	ReportTerminationPrePooling();
	ActionCancelAll();
	m_speechQueue.Cleanup();
	return TBaseClass::OnPoolRequest();
}

void CActor::UpdateCurrentAttitudeGroup()
{
	CName previousAttitudeGroup = m_currentAttitudeGroup;
	if ( m_temporaryAttitudeGroups.Size() > 0 )
	{
		m_currentAttitudeGroup = m_temporaryAttitudeGroups.Begin()->m_second;
	}
	else
	{
		m_currentAttitudeGroup = m_baseAttitudeGroup;
	}
	if ( m_currentAttitudeGroup != previousAttitudeGroup )
	{
		SignalGameplayEvent( CNAME( NoticedObjectReevaluation ) );
	}
}


Bool  CActor::IsPlayingChatScene()
{
	if ( IsInGameplayScene() )
	{
		return true;
	}

	CReactionSceneActorComponent* actorCmp = FindComponent< CReactionSceneActorComponent >();
	if ( actorCmp && actorCmp->GetRole() )
	{
		return true;
	}

	return false;
}

Bool CActor::IsConsciousAtWork()
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( !behTreeMachine )
	{
		return false;
	}
	CBehTreeInstance* ai = behTreeMachine->GetBehTreeInstance();
	if ( !ai )
	{
		return false;
	}
	CBehTreeWorkData* workData = ai->GetTypedItem< CBehTreeWorkData >( CBehTreeWorkData::GetStorageName() );
	return workData && workData->IsBeingPerformed() && workData->GetIsConscious();
}

Int32 CActor::GetCurrentJTType()
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( !behTreeMachine )
	{
		return false;
	}
	CBehTreeInstance* ai = behTreeMachine->GetBehTreeInstance();
	if ( !ai )
	{
		return false;
	}
	CBehTreeWorkData* workData = ai->GetTypedItem< CBehTreeWorkData >( CBehTreeWorkData::GetStorageName() );
	return workData ? workData->GetJTType() : 0;
}

Bool CActor::IsSittingAtWork()
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( !behTreeMachine )
	{
		return false;
	}
	CBehTreeInstance* ai = behTreeMachine->GetBehTreeInstance();
	if ( !ai )
	{
		return false;
	}
	CBehTreeWorkData* workData = ai->GetTypedItem< CBehTreeWorkData >( CBehTreeWorkData::GetStorageName() );
	return workData && workData->IsBeingPerformed() && workData->GetIsSitting();
}


Bool CActor::IsAtWork( Bool orWasJustWorking )
{
	CBehTreeMachine* behTreeMachine = GetBehTreeMachine();
	if ( !behTreeMachine )
	{
		return false;
	}
	CBehTreeInstance* ai = behTreeMachine->GetBehTreeInstance();
	if ( !ai )
	{
		return false;
	}
	CBehTreeWorkData* workData = ai->GetTypedItem< CBehTreeWorkData >( CBehTreeWorkData::GetStorageName() );
	if( !workData )
	{
		return false;
	}

	if( workData->IsBeingPerformed() )
	{
		return true;
	}	
	return orWasJustWorking && workData->WasJustWorking();
}

Bool CActor::CanUseChatInCurrentAP()
{
	CBehTreeWorkData*  workData = GetBehTreeMachine()->GetBehTreeInstance()->GetTypedItem< CBehTreeWorkData >( CBehTreeWorkData::GetStorageName() );
	return workData && workData->IsBeingPerformed() && workData->GetIsConscious();
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CPlaySoundOnActorRequest );

CPlaySoundOnActorRequest::CPlaySoundOnActorRequest()
	: m_fadeTime( 0.0f )
	, m_executed( false )
{
}

void CPlaySoundOnActorRequest::Execute( CGameplayEntity* entity )
{
	if ( !m_executed )
	{
		CSoundEmitterComponent* soundEmitterComponent = entity->GetSoundEmitterComponent();
		if( !soundEmitterComponent ) return;

		Int32 bone = entity->GetRootAnimatedComponent()->FindBoneByName( m_boneName );
		soundEmitterComponent->SoundEvent( m_soundName.AsChar(), bone );

		m_executed = true;
	}
}

String CPlaySoundOnActorRequest::OnStateChangeRequestsDebugPage() const
{
	return String::Printf( TXT( "CPlaySoundOnActorRequest %s - bone '%ls', sound '%s" )
		, m_executed ? TXT( "[PLAYED]" ) : TXT( "" )
		, m_boneName.AsString().AsChar()
		, m_soundName.AsChar() );
}

void CPlaySoundOnActorRequest::funcInitialize( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, boneName, CName::NONE );
	GET_PARAMETER( String, soundName, String() );
	GET_PARAMETER_OPT( Float, fadeTime, 0.0f );
	FINISH_PARAMETERS;

	m_boneName = boneName;
	m_soundName = UNICODE_TO_ANSI( soundName.AsChar() );
	m_fadeTime = fadeTime;
}

///////////////////////////////////////////////////////////////////////////////

void CActor::GetAITemplateParams( CEntityTemplate*const entityTemplate, THandle< CAIBaseTree >& outAIBaseTree, TDynArray< CAIPresetParam* > * outAIPresetsList )
{
	ASSERT( outAIPresetsList == nullptr || outAIPresetsList->Size() == 0 );
	
	// [ Step ] Backward compat ai tree
#ifdef AI_WIZARD_BACKWARD_COMPAT
	struct Local
	{
		static Bool Pred( CAIProfile* aiProfile )
		{
			return ( aiProfile->GetAiResource() != NULL );
		}
	};
	CAIProfile* aiProfile = entityTemplate->FindParameter< CAIProfile >( false, Local::Pred );
	CAIProfile* deepAIProfile = entityTemplate->FindParameter< CAIProfile >( true, Local::Pred );
	if( aiProfile ) // if we have a local ai profile we need to get ai resource from it
	{
		*outAIBaseTree = aiProfile->GetAiResource();
	}
#endif
	// [ Step ] Getting all CAITemplateParam ( basically all ai wisard and ai tree related templates )
	TDynArray< CGameplayEntityParam* > aiTemplateParamList;
	entityTemplate->CollectGameplayParams( aiTemplateParamList, CAITemplateParam::GetStaticClass() );

	// [ Step ] Filling aiPresetsList and  aiBaseTree
	for( Uint32 i = 0; i < aiTemplateParamList.Size(); ++i )
	{
		CAITemplateParam* aiTemplateParam = static_cast<CAITemplateParam*>(aiTemplateParamList[ i ]);
		if ( aiTemplateParam->IsA< CAIBaseTreeTemplateParam >() )
		{
			CAIBaseTreeTemplateParam *const aiBaseTreeTemplateParam = static_cast<CAIBaseTreeTemplateParam*>(aiTemplateParam);

			if ( !outAIBaseTree ) // only if backward compat does yield ai tree
			{
				outAIBaseTree = aiBaseTreeTemplateParam->m_aiBaseTree;
			}
		}
		else
		{
			AI_ASSERT( aiTemplateParam->IsA< CAIPresetParam >() );
			if ( aiTemplateParam->IsA< CAIPresetParam >() && outAIPresetsList )
			{
				CAIPresetParam *const aiPresetParam = static_cast<CAIPresetParam*>(aiTemplateParam);
				outAIPresetsList->PushBack( aiPresetParam );
			}
		}
	}
#ifdef AI_WIZARD_BACKWARD_COMPAT
	if( outAIBaseTree && deepAIProfile ) // if we have no aibase tree yet take deep one
	{
		outAIBaseTree = deepAIProfile->GetAiResource();
	}
#endif
}

void CActor::funcApplyItemAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	Bool applied = false;
	CInventoryComponent* inventoryComponent = m_inventoryComponent.Get();
	if ( inventoryComponent != nullptr && m_stats != nullptr )
	{
		const SInventoryItem* item = inventoryComponent->GetItem( itemId );
		if ( item != nullptr )
		{
			applied = m_stats->ApplyItemAbilities( *item );
		}
	}

	RETURN_BOOL( applied );
}

void CActor::funcRemoveItemAbilities( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SItemUniqueId, itemId, SItemUniqueId::INVALID );
	FINISH_PARAMETERS;

	Bool removed = false;
	CInventoryComponent* inventoryComponent = m_inventoryComponent.Get();
	if ( inventoryComponent != nullptr && m_stats != nullptr )
	{
		const SInventoryItem* item = inventoryComponent->GetItem( itemId );
		if ( item != nullptr )
		{
			removed = m_stats->RemoveItemAbilities( *item );
		}
	}

	RETURN_BOOL( removed );
}

void CActor::funcReportDeathToSpawnSystems( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ReportTerminationDeath();
}

void CActor::funcCanPush( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, canPush, false );
	FINISH_PARAMETERS;

#ifdef USE_PHYSX
	if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( GetMovingAgentComponent() ) )
	{
		if ( const CMRPhysicalCharacter* character = mpac->GetPhysicalCharacter() )
		{
			if ( CPhysicsCharacterWrapper* wrapper = character->GetCharacterController() )
			{
#ifdef USE_PHYSX
				wrapper->SetIfCanPush( canPush );
#endif
			}
		}		
	}
#endif // USE_PHYSX
}

void CActor::funcMuteHeadAudio( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, mute, false );
	FINISH_PARAMETERS;

	MuteHeadAudio( mute );
}

void CActor::funcSetGroupShadows( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, true );
	FINISH_PARAMETERS;

	SetGroupShadows( flag );
}

void CActor::funcForceSoundAppearanceUpdate(CScriptStackFrame& stack, void* result)
{
	FINISH_PARAMETERS;

	m_forceAppearanceSoundUpdate = true;
}

