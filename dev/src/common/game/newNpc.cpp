/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "newNpc.h"

#include "../core/scriptingSystem.h"

#include "aiLog.h"
#include "behTreeGuardAreaData.h"
#include "behTreeMachine.h"
#include "behTreeInstance.h"
#include "communityUtility.h"
#include "communityAgentStub.h"
#include "communitySystem.h"
#include "newNpcStateIdle.h"
#include "questsSystem.h"
#include "sceneStopReasons.h"
#include "questExternalScenePlayer.h"
#include "reactionAction.h"
#include "interestPointComponent.h"
#include "reactionsManager.h"
#include "reactionCondition.h"
#include "behTreeWorkData.h"
#include "reactionSceneActor.h"

#include "../engine/gameTimeManager.h"
#include "../core/gameSave.h"
#include "../engine/dynamicLayer.h"
#include "../engine/layerInfo.h"
#include "../engine/areaComponent.h"
#include "../engine/renderFrame.h"
#include "../../games/r4/monsterParam.h"
#include "../core/dataError.h"

#ifdef USE_ANSEL
#include "../engine/mimicComponent.h"
#include "../engine/meshTypeComponent.h"

extern Bool isAnselTurningOn;
extern Bool isAnselTurningOff;
#endif // USE_ANSEL

IMPLEMENT_ENGINE_CLASS( CNewNPC );

RED_DISABLE_WARNING_MSC( 4355 )
// ACHTUNG!!! 'this' points to not fully constructed object

RED_DEFINE_STATIC_NAME( OnLevelUpscalingChanged )


NPCQuestLock::NPCQuestLock( Bool lockState, const CQuestPhase* phase, const CQuestGraphBlock* block )
	: m_lockState( lockState )
	, m_phase( ( CQuestPhase* ) phase )
	, m_block( ( CQuestGraphBlock* ) block )
{}

NPCQuestLock::~NPCQuestLock()
{
}

CNewNPC::CNewNPC()
	: m_aiEnabled( true )
	, m_isCurrentlyWorkingInAP( false )
	, m_isStartingInAP( false )
	, m_suppressBroadcastingReactions( false )
	, m_senses( this )
	, m_lastFindAPResult( FAPR_Default )
	, m_defaultSchedule( NULL )
	, m_questLockCount( 0 )	
	, m_isInInterior( 0 )
	, m_monsterCategory(-1)
	, m_isAfraid(false)
#ifdef EDITOR_AI_DEBUG
	, m_reactionsDebugListener( NULL )
#endif
	, m_didChangeMimicProperties( false )
{
	m_defaultSchedule = new NewNPCSchedule();
	m_scheduleProxy.Set( *m_defaultSchedule );
	m_entityStaticFlags &= ~ESF_Streamed;
	SetDynamicFlag( EDF_AlwaysTick );
}

CNewNPC::~CNewNPC()
{
	delete m_defaultSchedule;
	m_defaultSchedule = NULL;

	m_questLocksHistory.ClearPtr();
}

void CNewNPC::SetAfraid(Bool val)
{
	m_isAfraid = val;
}

void CNewNPC::SetWorkSchedule( NewNPCSchedule& schedule )
{
	m_scheduleProxy.Set( schedule );
}

void CNewNPC::ClearWorkSchedule()
{
	m_scheduleProxy.Set( *m_defaultSchedule );
}

void CNewNPC::OnInitialized()
{
	TBaseClass::OnInitialized();

	PC_SCOPE_PIX( NPC_OnInitialized );

	if( IsInGame() )
	{
		// Initialize senses
		InitializeSenses();

		// Cache reactions
		CacheReactions();

		if( !m_template )
		{
			m_aiEnabled = false;
			SET_ERROR_STATE( this, String::Printf( TXT("NPC with NULL template %s, AI disabled!"), GetFriendlyName().AsChar() ) );
			ERR_GAME( TXT("NPC with NULL template %s, AI disabled!"), GetFriendlyName().AsChar() );
		}
	}
}

void CNewNPC::OnUninitialized()
{
	TBaseClass::OnUninitialized();

	// Clear reactions cache
	m_reactionsCache.Clear();
}

void CNewNPC::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CNewNpc_OnAttached );

	if( IsInGame() )
	{
		CWorld* world = GGame->GetActiveWorld();
		if( world && GetLayer() != world->GetDynamicLayer() )
		{
			m_aiEnabled = false;
			SET_ERROR_STATE( this, String::Printf( TXT("NPC on static layer %s, AI disabled!"), GetFriendlyName().AsChar() ) );
			ERR_GAME( TXT("NPC on static layer %s, AI disabled!"), GetFriendlyName().AsChar() );
		}

		// reset flags
		m_isCurrentlyWorkingInAP = false;
	}
}

void CNewNPC::CallCommunityInitializers( Bool storyPhaseInitializers )
{
	// HACK: this should be called when the agent's LOD changes on some kind of a callback - and definitely
	// from the stub, the responsibility of which is to initialize an NPC that hatched from it

	const SAgentStub* stub = GCommonGame->GetSystem< CCommunitySystem >()->FindStubForNPC( this );
	if( stub )
	{
		CCommunityInitializers*	initializers = stub->GetInitializers( storyPhaseInitializers );
		if( initializers )
		{
			Bool entityRestored = CheckDynamicFlag( EDF_RestoredFromLayerStorage );
			const Uint32 s = initializers->m_initializers.Size();
			for( Uint32 i=0; i<s; i++ )
			{
				ISpawnTreeInitializer* init = initializers->m_initializers[i];
				if( init )
				{
					if( storyPhaseInitializers || entityRestored == false || init->CallActivateOnRestore() )
					{
						ISpawnTreeInitializer::EOutput res = init->Activate( this, nullptr, nullptr, ISpawnTreeInitializer::EAR_Steal );
						if( res != ISpawnTreeInitializer::OUTPUT_SUCCESS )
						{
							AI_LOG( TXT("ERROR: Community initializer '%ls' for npc '%ls' failed!"), init->GetClass()->GetName().AsString().AsChar(), GetName().AsChar() );
						}
					}
				}
			}
		}
	}
}

Bool CNewNPC::CanStealOtherActor( const CActor* const other ) const
{
	const CCommunitySystem* communitySystem = GCommonGame->GetSystem< CCommunitySystem >();
	return communitySystem->IsNPCInCommunity( this ) ? true : CActor::CanStealOtherActor( other );
}

void CNewNPC::OnAttachFinished( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttachFinished( world );

	if( IsInGame() )
	{
		// Spawn initializers
		CallCommunityInitializers( false );

		// Story phase initializers
		CallCommunityInitializers( true );
	}
}

void CNewNPC::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );

	// Inform reaction manager about detach
	CReactionsManager* reactionsMgr = GCommonGame->GetReactionsManager();
	if( reactionsMgr )
	{
		reactionsMgr->OnNPCDetached( this );
	}
}

void CNewNPC::OnTick( Float timeDelta )
{
	ASSERT( IsAttached() );

	if( IsInGame() )
	{
		Bool doUpdate = IsAIEnabled() && !IsExternalyControlled();

		{
			PC_SCOPE_PIX( TickNPC1 );

			if( doUpdate )
			{
				// Update NPC senses
				{
					PC_SCOPE( AI_Senses_Total );
					UpdateSenses( timeDelta );
				}
			}
		}

		// Pass to base class
		TBaseClass::OnTick( timeDelta );

		{
			PC_SCOPE_PIX( TickNPC2 );

			if( doUpdate )
			{
				// Process delayed interest points
				ProcessDelayedInterestPoints();

				// Update reactions
				ProcessReactions();

				// Update NPC state machine
				UpdateStateMachine( timeDelta );
			}
		}
	}
	else
	{
		// Pass to base class
		TBaseClass::OnTick( timeDelta );
	}
}

//! LOD Tick
#ifdef USE_ANSEL
void CNewNPC::HACK_ANSEL_OnLODTick()
{
	if ( !m_didChangeMimicProperties )
	{
		if ( isAnselTurningOn )
		{
			if ( CMimicComponent* mimicComp = FindComponent< CMimicComponent >() )
			{
				mimicComp->MimicHighOn();
				mimicComp->ForceUpdateTransformNodeAndCommitChanges();
			}

			// bumping autohide distance for character visuals
			for ( CComponent* component : GetComponents() )
			{
				if ( CDrawableComponent* drawable = SafeCast< CDrawableComponent >( component ) )
				{
					drawable->SetForceNoAutohide( true );
				}
			}

			m_didChangeMimicProperties = true;
			ForceUpdateTransformNodeAndCommitChanges();
		}
		else if ( isAnselTurningOff )
		{
			if ( CMimicComponent* mimicComp = FindComponent< CMimicComponent >() )
			{
				mimicComp->MimicHighOff();
				mimicComp->ForceUpdateTransformNodeAndCommitChanges();
			}

			// bringing back regular autohide for character visuals
			for ( CComponent* component : GetComponents() )
			{
				if ( CDrawableComponent* drawable = SafeCast< CDrawableComponent >( component ) )
				{
					drawable->SetForceNoAutohide( false );
				}
			}

			m_didChangeMimicProperties = true;
			ForceUpdateTransformNodeAndCommitChanges();
		}
	}
	else
	{
		if ( !isAnselTurningOn && !isAnselTurningOff )
		{
			m_didChangeMimicProperties = false;
		}
	}

	if ( isAnselTurningOn )
	{
		ForceUpdateTransformNodeAndCommitChanges();
	}
}
#endif // USE_ANSEL

void CNewNPC::OnSerialize( IFile& file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );
}

Int32 CNewNPC::GetCurrentActionPriority() const
{
	return 0;
}

Bool CNewNPC::IsInQuestScene() const
{
	return GCommonGame && GCommonGame->GetSystem< CQuestsSystem >() ? GCommonGame->GetSystem< CQuestsSystem >()->IsNPCInQuestScene( this ) : false;
}

void CNewNPC::NoticeActor( CActor* actor )
{
	ASSERT( actor );
	if( actor != this && actor->IsAlive() && IsAlive() )
	{
		Int32 objectIndex = -1;
		if ( m_senses.OnActorNoticed( actor, m_noticedObjects, NewNPCNoticedObject::FLAG_DETECTION_FORCED, objectIndex ) )
		{
			ForceTargetUpdate();
		}
	}
}

void CNewNPC::NoticeActorInGuardArea( CActor* const actor )
{
	ASSERT( actor );
	
	const CBehTreeMachine* const machine = GetBehTreeMachine();
	CBehTreeInstance* const behTreeInstance = machine ? machine->GetBehTreeInstance() : nullptr;
	if ( behTreeInstance )
	{
		if ( !behTreeInstance->GetCombatTarget() || actor == behTreeInstance->GetCombatTarget() )
		{
			NoticeActor( actor );

			CBehTreeGuardAreaData* const guardAreaData = CBehTreeGuardAreaData::Find( behTreeInstance );
			if ( guardAreaData )
			{
				guardAreaData->NoticeTargetAtGuardArea( actor );
			}
		}
	}
}

Bool CNewNPC::OnPoolRequest()
{
	if( TBaseClass::OnPoolRequest() )
	{
		m_isInInterior = 0;
		return true;
	}

	return false;
}

void CNewNPC::ForgetActor( const CActor *const actorToForget )
{
	for( Int32 i=(Int32)m_noticedObjects.Size()-1; i>=0; i-- )
	{
		const NewNPCNoticedObject& obj	= m_noticedObjects[i];
		CActor* noticedActor			= obj.m_actorHandle.Get();
		if( noticedActor == actorToForget )
		{
			m_noticedObjects.EraseFast( m_noticedObjects.Begin() + i );
			ForceTargetUpdate();
			SignalGameplayEvent( CNAME( NoticedObjectReevaluation ) );
		}
	}
}

void CNewNPC::ForgetAllActors()
{
	if( !m_noticedObjects.Empty() )
	{
		m_noticedObjects.ClearFast();
		ForceTargetUpdate();
		SignalGameplayEvent( CNAME( NoticedObjectReevaluation ) );
	}
}


Bool CNewNPC::HasActorBeenNoticed( CActor * actor, Float time /*= 1.0f*/ )
{
	TDynArray< NewNPCNoticedObject >::iterator
		currObj = m_noticedObjects.Begin(),
		lastObj = m_noticedObjects.End();

	for (; currObj != lastObj; ++currObj )
	{
		if ( currObj->m_actorHandle.Get() == actor )
		{
			if( GGame->GetEngineTime() - currObj->m_lastNoticedTime < time )
			{
				return true;
			}
		}
	}

	return false;
}

void CNewNPC::GenerateDebugFragments( CRenderFrame* frame )
{
	TBaseClass::GenerateDebugFragments( frame );

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_AI ) )
	{
		if( !IsAIEnabled() )
		{
			static const Vector pos = Vector(0.0f,0.0f,2.8f);
			frame->AddDebugText( pos + GetWorldPosition(), TXT("AI DISABLED"), 0, Uint32(-1), false, Color::RED );
		}

		CActor* target = GetTarget();
		if( target )
		{
			Vector startPos = GetWorldPosition();
			startPos.Z += 1.0f;
			Vector endPos = target->GetWorldPosition();
			endPos.Z += 0.7f;
			frame->AddDebugLineWithArrow( startPos, endPos, 0.7f, 0.3f, 0.6f, Color::CYAN );
		}
	}

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_AISenses ) )
	{
		m_senses.GenerateDebugFragments( frame );
	}

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_Spawnset ) )
	{
		frame->AddDebugText( GetWorldPosition(), GetCurrentStateName().AsString(), true, Color::WHITE, Color::BLACK );
	}
}

void CNewNPC::DebugDraw( IDebugFrame& debugFrame ) const
{
	if ( m_action )
	{
		m_action->DebugDraw( debugFrame );
	}
}

Bool CNewNPC::FindActionPoint( TActionPointID &apIDOut /* inout */, CName &categoryNameOut /* inout */ )
{
	CActionPointManager *apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();

	// AP Manager can be NULL for example in cutscenes
	if ( apMan == NULL ) return false;

	const NewNPCScheduleProxy &schedule = GetActionSchedule();

	CName actionCategoryName = categoryNameOut;
	CLayerInfo *layerInfo = NULL;
	TagList actionPointTags;
	categoryNameOut = CName::NONE;

	SActionPointFilter apFilter;
	apFilter.m_actionPointTags = actionPointTags;
	apFilter.m_category = actionCategoryName;
	apFilter.m_layerGuid = layerInfo->GetGUID();
	apFilter.m_askingNPC = this;

	m_lastFindAPResult = GCommonGame->GetSystem< CCommunitySystem >()->FindCurrentActionPointFilter( schedule.GetTimetable(), apFilter );

	if ( layerInfo )
	{
		apMan->FindActionPoint( apIDOut, apFilter );

		m_currentActionCategory = apFilter.m_category;
		categoryNameOut = apFilter.m_category;
	}

	return apIDOut != ActionPointBadID;
}

String CNewNPC::GetLastFindAPFriendlyResult() const
{
	return CCommunityUtility::GetFriendlyFindAPStateDescription( m_lastFindAPResult );
}

Int32 CNewNPC::GetAvailableActionPointsCount( Bool onlyFree, CEntity* excludeApOwner )
{
	Int32 result = 0;

	CActionPointManager *apMan = GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager();
	const CSStoryPhaseTimetableACategoriesTimetableEntry *timetabEntry =
		CCommunityUtility::GetTimeActiveEntry< CSStoryPhaseTimetableACategoriesTimetableEntry >(
		GetActionSchedule().GetTimetable(), GGame->GetTimeManager()->GetTime() );

	if ( apMan == NULL ) return 0;
	if ( timetabEntry == NULL ) return 0;

	for ( TDynArray< CSStoryPhaseTimetableActionEntry >::const_iterator action = timetabEntry->m_actions.Begin();
		action != timetabEntry->m_actions.End();
		++action )
	{
		CLayerInfo *layerInfo = action->m_layerName.GetCachedLayer();

		if( action->m_layerName.m_layerName && !layerInfo )
		{
			continue;
		}

		CGUID apLayer = CGUID::ZERO;
		if ( layerInfo )
		{
			apLayer = layerInfo->GetGUID();
		}

		for ( TDynArray< CSStoryPhaseTimetableACategoriesEntry >::const_iterator actionCategory = action->m_actionCategories.Begin();
			actionCategory != action->m_actionCategories.End();
			++actionCategory )
		{
			if ( actionCategory->m_weight == 0.0f )
			{
				continue;
			}

			TDynArray< CActionPointComponent* > actionPoints;

			SActionPointFilter apFilter;
			apFilter.m_actionPointTags = actionCategory->m_apTags;
			apFilter.m_category = actionCategory->m_name;
			apFilter.m_layerGuid = apLayer;

			apMan->FindActionPoints( apFilter, actionPoints );

			for ( CActionPointComponent* ap : actionPoints )
			{
				const Bool doesMatchOwner = ap->GetEntity() == excludeApOwner;

				if ( ( (!onlyFree || ap->IsFree()) && ( !excludeApOwner || !doesMatchOwner ) ) || !layerInfo )	// damn this hack - null layer info can be interpreted that npc is from house template
				{
					++result;
				}
			}
		}
	}

	return result;
}

// ------------------------------------------------------------------------------
// Reactions
// ------------------------------------------------------------------------------

Bool CNewNPC::OnInterestPoint( CInterestPointInstance* interestPoint, Bool forceNoVisibilityTest /*=false*/ )
{
	if( !CanReact() )
	{
		return false;
	}

	if( m_reactionsCache.Size() == 0 )
	{
		return false;
	}

	PC_SCOPE( AI_Reactions_OnInterestPoint );

	TStaticArray< Uint32, 16 > reactionIndices;
	FindReactions( interestPoint, reactionIndices );
	if ( reactionIndices.Size() == 0 )
	{
		return false;
	}

	Vector interestPointPos = interestPoint->GetWorldPosition();
	const THandle< CNode >& nodeHandle = interestPoint->GetNode();
	const CNode* node = nodeHandle.Get();
	const CActor* actor = Cast< const CActor >( node );

	const EngineTime& currentTime = GGame->GetEngineTime();
	Bool visionInvalid = !m_senses.IsValid( AIST_Vision );

	// Immediate tests
	for( Int32 i = Int32( reactionIndices.Size() ) - 1; i >= 0; i-- )
	{
		Int32 reactionIdx = reactionIndices[i];
		const CAIReaction* reaction = GetReaction( reactionIdx );

		// Cooldown test
		Float lastTime = GetReactionLastActivationTime( reactionIdx );
		Float cooldown = reaction->m_cooldownTime;
		if( cooldown > 0.0f && lastTime != 0.0f && currentTime < lastTime + cooldown )
		{
			reactionIndices.EraseFast( reactionIndices.Begin() + i );
			continue;
		}

		// Range test
		if( !reaction->m_range.PerformTest( this, interestPointPos ) )
		{
			reactionIndices.EraseFast( reactionIndices.Begin() + i );
			continue;
		}

		// Fact test
		if( !reaction->m_factTest.PerformTest() )
		{
			reactionIndices.EraseFast( reactionIndices.Begin() + i );
			continue;
		}

		// If no sense vision fail visibility test at once
		if( visionInvalid && reaction->m_visibilityTest != VT_None )
		{
			reactionIndices.EraseFast( reactionIndices.Begin() + i );
			continue;
		}

		// Scripted conditions test - always after visibility test - so if no visibility test then test here
		if( ( (reaction->m_visibilityTest == VT_None) || ( GGame->GetEngineTime() - m_senses.GetLastTimePlayerNoticed() < 0.2f ) ) && reaction->m_condition != NULL )
		{
			// Last activation time just before custom test
			SetReactionLastActivationTime( reactionIdx, GGame->GetEngineTime() );

			if ( !reaction->m_condition->Perform( node, this, interestPoint ) )
			{
				reactionIndices.EraseFast( reactionIndices.Begin() + i );
				continue;
			}
		}
	}

	if( reactionIndices.Size() > 0 )
	{
		Bool delayedProcess = false;

		// If any reaction that passed immediate tests needs visibility test, perform delayed processing of all reactions
		if( !forceNoVisibilityTest )
		{
			for( Uint32 i = 0; i < reactionIndices.Size(); i++ )
				if ( GetReaction( reactionIndices[i] )->m_visibilityTest != VT_None )
				{
					delayedProcess = true;
					break;
				}
		}

		// If player has been noticed lately cancel delayed process
		if( delayedProcess && actor && actor->IsPlayer() )
		{
			if( GGame->GetEngineTime() - m_senses.GetLastTimePlayerNoticed() < 0.2f )
			{
				delayedProcess = false;
			}
		}

		THandle< CInterestPointInstance > ipHandle( interestPoint );

		if( delayedProcess )
		{
			Vector pos = interestPoint->GetPosition();

			const Uint32 s = reactionIndices.Size();
			for( Uint32 i = 0; i < s; i++ )
			{
				Uint32 reactionIndex = reactionIndices[i];
				const CAIReaction* reaction = GetReaction( reactionIndex );
				if( reaction->m_visibilityTest != VT_None )
				{
					Bool delayedTestRequestRes = false;
					CNewNpcSensesManager* sensesManager = GCommonGame->GetNpcSensesManager();
					if ( sensesManager != nullptr )
					{
						VisibilityQueryId queryId = sensesManager->SubmitQuery( this, GetWorldPosition(), pos );
						m_delayedVisibilityTests.Insert( TDelayedTestId( reaction, interestPoint ), queryId );
						delayedTestRequestRes = true;
					}
					if ( delayedTestRequestRes )
					{
						m_delayedInterestPoints.PushBackUnique( InterestPointReactionData( ipHandle, reactionIndex, InterestPointReactionData::IPDS_InProgress ) );
					}
					else
					{
						//ASSERT( 0 && TXT("Too much delayed visibility test requests") );
						WARN_GAME( TXT("CActor::OnInterestPoint RequestDelayedTest failed" ) );
						m_delayedInterestPoints.PushBackUnique( InterestPointReactionData( ipHandle, reactionIndex, InterestPointReactionData::IPDS_Failed ) );
					}
				}
				else
				{
					m_delayedInterestPoints.PushBackUnique( InterestPointReactionData( ipHandle, reactionIndex, InterestPointReactionData::IPDS_Passed ) );
				}
			}
		}
		else
		{
			for( Uint32 i = 0; i < reactionIndices.Size(); i++ )
				ReactTo( ipHandle, reactionIndices[i] );
		}

		return true;
	}

	return false;
}

void CNewNPC::ProcessDelayedInterestPoints()
{
	PC_SCOPE( AI_Reactions_DelayedIP );

#ifdef EDITOR_AI_DEBUG
	if( m_reactionsDebugListener )
	{
		m_reactionsDebugListener->UpdateDelayed();
	}
#endif

	Int32 s = Int32( m_delayedInterestPoints.Size() );

	// Update status
	for( Int32 i = 0; i < s; i++ )
	{
		InterestPointReactionData& data = m_delayedInterestPoints[i];

		if( data.m_delayedStatus == InterestPointReactionData::IPDS_InProgress )
		{
			CNewNpcSensesManager::EVisibilityQueryState state = CNewNpcSensesManager::QS_NotFound;
			CNewNpcSensesManager* sensesManager = GCommonGame->GetNpcSensesManager();
			if ( sensesManager != nullptr )
			{
				const CAIReaction* reaction = GetReaction( data.m_reactionIndex );
				TDelayedTestId testId = TDelayedTestId( reaction, data.m_interestPoint.Get() );
				TDelayedTests::iterator it = m_delayedVisibilityTests.Find( testId );
				if ( it != m_delayedVisibilityTests.End() )
				{
					state = sensesManager->GetQueryState( it->m_second );
				}
			}
			if ( state == CNewNpcSensesManager::QS_True )
			{
				m_delayedInterestPoints[i].m_delayedStatus = InterestPointReactionData::IPDS_Passed;
			}
			else if ( state == CNewNpcSensesManager::QS_False || state == CNewNpcSensesManager::QS_NotFound )
			{
				m_delayedInterestPoints[i].m_delayedStatus = InterestPointReactionData::IPDS_Failed;
			}
			// else state == CNewNpcSensesManager::QS_NotReady, so we're waiting for the better future
		}
	}

	THandle< CInterestPointInstance > lastInProgressIP;

	for( Int32 i = 0; i < s; i++ )
	{
		InterestPointReactionData& data = m_delayedInterestPoints[i];

		// If passed and no more important tests in progress for given IP
		if( data.m_delayedStatus == InterestPointReactionData::IPDS_Passed && data.m_interestPoint != lastInProgressIP )
		{
			// Cancel next tests for same interestpoint
			for( Int32 j = i+1; j < s; j++ )
			{
				InterestPointReactionData& dataNext = m_delayedInterestPoints[j];
				if( dataNext.m_interestPoint == data.m_interestPoint )
				{
					if( dataNext.m_delayedStatus == InterestPointReactionData::IPDS_InProgress )
					{
						const CAIReaction* reaction = GetReaction( dataNext.m_reactionIndex );
						TDelayedTestId testId = TDelayedTestId( reaction, dataNext.m_interestPoint.Get() );
						m_delayedVisibilityTests.Erase( testId );
					}

					dataNext.m_delayedStatus = InterestPointReactionData::IDPS_Remove;
				}
			}

			const CAIReaction* reaction = GetReaction( data.m_reactionIndex );
			const THandle< CNode >& nodeHandle = data.m_interestPoint.Get()->GetNode();
			const CNode* node = nodeHandle.Get();

			// Last activation time just before custom test
			SetReactionLastActivationTime( data.m_reactionIndex, GGame->GetEngineTime() );

			// Scripted conditions test
			if( (reaction->m_condition == NULL) || reaction->m_condition->Perform( node, this, data.m_interestPoint.Get() ) )
			{
				// Call script event
				ReactTo( data.m_interestPoint, data.m_reactionIndex );
			}

			data.m_delayedStatus = InterestPointReactionData::IDPS_Remove;
		}
		else if( data.m_delayedStatus == InterestPointReactionData::IPDS_Failed )
		{
			data.m_delayedStatus = InterestPointReactionData::IDPS_Remove;
		}
	}

	// Remove marked
	for( Int32 i = s - 1; i >= 0; i-- )
	{
		InterestPointReactionData::EIPDelayedStatus status = m_delayedInterestPoints[i].m_delayedStatus;
		if( status == InterestPointReactionData::IDPS_Remove )
		{
			m_delayedInterestPoints.Erase( m_delayedInterestPoints.Begin() + i );
		}
	}
}

EMonsterCategory CNewNPC::GetMonsterCategory()
{
	if(m_monsterCategory >= 0)
	{
		return (EMonsterCategory)m_monsterCategory;
	}

	m_monsterCategory = GCommonGame->GetMonsterCategoryForNpc(this);

	if(m_monsterCategory < 0)
	{
		return MC_NotSet;
	}

	return (EMonsterCategory)m_monsterCategory;
}

void CNewNPC::OnLevelUpscalingChanged( Bool upscaleEnabled )
{
	CallEvent( CNAME( OnLevelUpscalingChanged ) );
}

void CNewNPC::CacheReactions()
{
	m_reactionsCache.Clear();
	TDynArray< CAIProfile* > profiles;
	GetAIProfiles( profiles );

	for ( TDynArray< CAIProfile* >::iterator profIt = profiles.Begin();
		profIt != profiles.End(); ++profIt )
	{
		if ( !*profIt )
		{
			continue;
		}

		CAIProfile* profile = *profIt;
		const TDynArray< THandle< CAIReaction > >& reactions = profile->GetReactions();
		for ( TDynArray< THandle< CAIReaction > >::const_iterator reactIt = reactions.Begin();
			reactIt != reactions.End(); ++reactIt )
		{
			CAIReaction* reaction = (*reactIt).Get();
			if ( reaction )
			{
				m_reactionsCache.PushBack( CachedReaction( reaction ) );
			}
		}
	}
}

const CAIReaction* CNewNPC::FindReaction( CInterestPointInstance* interestPoint ) const
{
	if ( !interestPoint )
	{
		return NULL;
	}

	return FindReaction( interestPoint->GetName() );
}

const CAIReaction* CNewNPC::FindReaction( CName fieldName ) const
{
	if( fieldName == CName::NONE )
	{
		return NULL;
	}

	for ( TDynArray< CachedReaction >::const_iterator it = m_reactionsCache.Begin(); it != m_reactionsCache.End(); ++it )
	{
		const CAIReaction* reaction = (*it).m_reaction;
		if ( reaction && reaction->m_fieldName == fieldName )
		{
			return reaction;
		}
	}
	return NULL;
}

void CNewNPC::FindReactions( CInterestPointInstance* interestPoint, TStaticArray< Uint32, 16 >& outReactionIndices ) const
{
	if ( !interestPoint )
	{
		return;
	}

	return FindReactions( interestPoint->GetName(), outReactionIndices );
}

void CNewNPC::FindReactions( CName fieldName, TStaticArray< Uint32, 16 >& outReactionIndices ) const
{
	const Uint32 s = m_reactionsCache.Size();
	for ( Uint32 i = 0; i < s; i++ )
	{
		const CAIReaction* reaction = m_reactionsCache[i].m_reaction;
		if ( reaction && reaction->m_fieldName == fieldName )
		{
			outReactionIndices.PushBack( i );
		}
	}
}

void CNewNPC::ProcessReactions()
{
	PC_SCOPE( AI_Reactions_Process );

#ifdef EDITOR_AI_DEBUG
	if( m_reactionsDebugListener )
	{
		m_reactionsDebugListener->UpdateAffecting();
	}
#endif

	// If is not alive clear affecting and return
	if( !IsAlive() )
	{
		m_affectingInterestPoints.Clear();
		return;
	}

#if 1

	// Process all reactions
	for ( TDynArray< InterestPointReactionData >::iterator it = m_affectingInterestPoints.Begin(); it != m_affectingInterestPoints.End(); ++it )
	{
		ProcessReaction( it->m_reactionIndex, it->m_interestPoint.Get() );
	}

#else // Only process highest priority (by index and then by distance) reaction

	Vector currPos = GetWorldPosition();
	CInterestPointInstance* interestPoint = NULL;
	Uint32 lowestReactionIndex = NumericLimits< Uint32 >::Max();

	for ( TDynArray< InterestPointReactionData >::iterator it = m_affectingInterestPoints.Begin();
		it != m_affectingInterestPoints.End(); ++it )
	{
		const InterestPointReactionData& data = *it;
		CInterestPointInstance* ip = data.m_interestPoint.Get();
		if ( ip )
		{
			if ( data.m_reactionIndex < lowestReactionIndex )
			{
				lowestReactionIndex = data.m_reactionIndex;
				interestPoint = ip;
			}
			else if( data.m_reactionIndex == lowestReactionIndex )
			{
				Vector npcPos = GetWorldPosition();
				if( npcPos.DistanceSquaredTo( ip->GetWorldPosition() ) < npcPos.DistanceSquaredTo( interestPoint->GetWorldPosition() ) )
				{
					interestPoint = ip;
				}
			}
		}
	}

	m_affectingInterestPoints.Clear();
	if ( !interestPoint )
	{
		// no point selected - do nothing
		return;
	}

	ASSERT( lowestReactionIndex < NumericLimits< Uint32 >::Max() );

	ProcessReaction( lowestReactionIndex, interestPoint );

#endif
}

void CNewNPC::ProcessReaction( Uint32 reactionIndex, CInterestPointInstance* interestPointInstance )
{
	const CAIReaction* reaction = GetReaction( reactionIndex );

	// Set time
	SetReactionLastActivationTime( reactionIndex, GGame->GetEngineTime() );

	// perform action
	if (reaction->m_action)
	{
		PC_SCOPE( AI_Reactions_Process_Action );
		StopAllScenes( SCR_ACTOR_REACTING );

		reaction->m_action->Perform( this, interestPointInstance, reactionIndex );
	}
}

Bool CNewNPC::CanReact()
{
	CNewNPCStateBase* currentState = GetCurrentState();
	return currentState && currentState->CanReact() && m_action ? m_action->CanReact() : true;
}

void CNewNPC::ReactTo( const THandle< CInterestPointInstance >& interestPoint, Uint32 reactionIndex   )
{
	if( CanReact() )
	{
		new (m_affectingInterestPoints) InterestPointReactionData( interestPoint, reactionIndex );
	}
}

void CNewNPC::GetAIProfiles( TDynArray< CAIProfile* >& profiles ) const
{
	if( m_template )
	{
		m_template->GetAllParameters< CAIProfile >( profiles );
	}
}

// ----------------------------------------------------------------------------
// Game saves
// ----------------------------------------------------------------------------
void CNewNPC::OnLoadPlacementInfo( IGameLoader* loader )
{
	// Npc's should rely on stubs to restore their placement info
	// values are read though for backward save compatibility

	EngineTransform dummyTransform;
	Matrix			dummyLocalToWorld;

	loader->ReadValue( CNAME(transform),	dummyTransform );
	loader->ReadValue( CNAME(localToWorld), dummyLocalToWorld );
}

CActor* CNewNPC::GetNoticedObject(const Uint32 index) const
{
	if ( m_noticedObjects.Size() > index )
	{
		return m_noticedObjects[index].m_actorHandle.Get();
	}

	return NULL;
}

const CActor*const CNewNPC::IsInDanger()
{
	// If we dislike them or they dislike us, we're in danger
	for ( auto it = m_noticedObjects.Begin(), end = m_noticedObjects.End(); it != end; ++it )
	{
		CActor* actor = it->m_actorHandle.Get();
		if( actor && actor->IsAlive() && IsDangerous( actor ) )
		{
			return actor;
		}
	}
	return NULL;
}

const CActor*const CNewNPC::IsSeeingNonFriendlyNPC()
{
	for ( auto it = m_noticedObjects.Begin(), end = m_noticedObjects.End(); it != end; ++it )
	{
		CActor* actor = it->m_actorHandle.Get();
		if( actor && actor->IsAlive() && GetAttitude( actor ) != AIA_Friendly )
		{
			return actor;
		}
	}
	return NULL;
}

CBehTreeGuardAreaData* CNewNPC::GetGuardAreaData() const
{
	const CBehTreeMachine* const machine = GetBehTreeMachine();
	CBehTreeInstance* const behTreeInstance = machine ? machine->GetBehTreeInstance() : nullptr;
	return behTreeInstance ? CBehTreeGuardAreaData::Find( behTreeInstance ) : nullptr;
}

void CNewNPC::funcIsInInterior( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsInInterior() );
}


void CNewNPC::funcIsInDanger( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsInDanger() != NULL );
}

void CNewNPC::funcIsSeeingNonFriendlyNPC( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsSeeingNonFriendlyNPC() != NULL );
}

void CNewNPC::funcIsAIEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsAIEnabled() );
}

void CNewNPC::funcFindActionPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SActionPointId, apIDOut, ActionPointBadID );
	GET_PARAMETER_REF( CName, categoryNameOut, CName::NONE );
	FINISH_PARAMETERS;

	FindActionPoint( apIDOut, categoryNameOut );
}

void CNewNPC::funcGetDefaultDespawnPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, despawnPoint, Vector::ZEROS )
	FINISH_PARAMETERS;

	Bool found = false;

	const NewNPCScheduleProxy &schedule = GetActionSchedule();

	if ( GCommonGame->GetSystem< CCommunitySystem >()->FindDespawnPoint( &schedule.GetTimetable(), m_despawnTags, despawnPoint ) )
	{
		found = true;
	}
}

void CNewNPC::funcNoticeActor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actorHandle, NULL );
	FINISH_PARAMETERS;
	CActor *actor = actorHandle.Get();
	if( actor )
	{
		NoticeActor( actor );
	}
}

void CNewNPC::funcNoticeActorInGuardArea( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actorHandle, NULL );
	FINISH_PARAMETERS;
	if ( CActor* const actor = actorHandle.Get() )
	{
		NoticeActorInGuardArea( actor );
	}
}

void CNewNPC::funcGetActiveActionPoint( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	if( IsWorkingInAP() )
	{
		RETURN_STRUCT( SActionPointId, m_currentAP );
		return;
	}
	RETURN_STRUCT( SActionPointId, m_scheduleProxy.GetActiveAP() );
}

void CNewNPC::funcPlayDialog( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, forceSpawnedActors, false );
	FINISH_PARAMETERS;

	if ( !GCommonGame || !GCommonGame->GetSystem< CQuestsSystem >() )
	{
		RETURN_BOOL( false );
		return;
	}

	if ( this->GetTags().Empty() )
	{
		SET_ERROR_STATE( this, TXT( "No tags assigned - won't be able to find a default dialog scene" ) );
		WARN_GAME( TXT( "NPC does not have a tag assigned - we won't be able to find a default dialog scene for it." ) );
		RETURN_BOOL( false );
		return;
	}

	CQuestsSystem* questsSystem = GCommonGame->GetSystem< CQuestsSystem >();

	// find an interaction scene for this dialog
	const TDynArray< CName >& tags = this->GetTags().GetTags();
	Uint32 count = tags.Size();
	Bool started = false;
	for ( Uint32 i = 0; i < count; ++i )
	{
		started = questsSystem->GetInteractionDialogPlayer()->StartDialog( tags[ i ] );
		if ( started )
		{
			break;
		}
	}

	RETURN_BOOL( started );
}

void CNewNPC::funcGetPerceptionRange( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( m_senses.IsValid( AIST_Vision ) )
	{
		RETURN_FLOAT( m_senses.GetRangeMax( AIST_Vision ) );
	}
	else
	{
		RETURN_FLOAT( -1.0f );
	}
}

void CNewNPC::funcSetWristWrestlingParams( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, minWidth, 0 );
	GET_PARAMETER( Int32, maxWidth, 0 );
	GET_PARAMETER( EAIMinigameDifficulty, difficulty, AIMD_Normal );
	FINISH_PARAMETERS;

	CAIProfile* profile = GetEntityTemplate()->FindParameter< CAIProfile >( false );
	if ( profile )
	{
		SAIMinigameParams *minigameParams = profile->GetMinigameParams();
		ASSERT( minigameParams );
		CAIMinigameParamsWristWrestling* wwParams = minigameParams->GetWristWrestlingParams();
		if ( wwParams )
		{
			wwParams->m_hotSpotMinWidth = minWidth;
			wwParams->m_hotSpotMaxWidth = maxWidth;
			wwParams->m_gameDifficulty = difficulty;
			RETURN_BOOL( true );
		}
	}
	else
	{
		RETURN_BOOL( false );
	}
}

void CNewNPC::funcGetReactionScript( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, index, 0 );
	FINISH_PARAMETERS;
	const CReactionScript* script = NULL;
	if( index >= 0 && index < Int32( m_reactionsCache.Size() ) )
	{
		script = Cast< const CReactionScript >( m_reactionsCache[ index ].m_reaction->m_action );
	}
	else
	{
		WARN_GAME( TXT("No reaction script for index %d, actor '%ls'"), index, GetName().AsChar() );
	}

	RETURN_OBJECT( const_cast< CReactionScript* >( script ) );
}

void CNewNPC::funcForgetActor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actorHandle, NULL );
	FINISH_PARAMETERS;

	CActor *actor = actorHandle.Get();
	if ( actor )
	{
		ForgetActor( actor );
	}
}

void CNewNPC::funcForgetAllActors( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ForgetAllActors();
}

void CNewNPC::funcGetNoticedObject( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, index, 0 );
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetNoticedObject( index ) );
}

void CNewNPC::funcForceKnowledgeUpdate( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	m_senses.ForceUpdateKnowledge();
}

void CNewNPC::funcIsNoticedObjectVisible( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, index, 0 );
	FINISH_PARAMETERS;

	Bool isVisible( false );

	if ( m_noticedObjects.Size() > index )
	{
		isVisible = m_noticedObjects[index].IsVisible();
	}

	RETURN_BOOL( isVisible );
}

void CNewNPC::funcGetLastKnownPosition	( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, index, 0 );
	FINISH_PARAMETERS;

	Vector3 knownPosition;

	if ( m_noticedObjects.Size() > index )
	{
		knownPosition = m_noticedObjects[index].GetKnownPosition();
	}

	RETURN_STRUCT( Vector, knownPosition );
}

void CNewNPC::funcIfCanSeePlayer	( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool isVisible( false );

	CPlayer* player = GCommonGame->GetPlayer();

	for( Uint32 i = 0; i<m_noticedObjects.Size(); ++i )
	{
		if( player ==  m_noticedObjects[ i ].m_actorHandle.Get() )
		{
			isVisible = m_noticedObjects[i].IsVisible();
			break;
		}		
	}

	RETURN_BOOL( isVisible );

}

void CNewNPC::funcGetGuardArea( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const CBehTreeGuardAreaData* const guardAreaData = GetGuardAreaData();
	const CAreaComponent* const guardArea = guardAreaData ? guardAreaData->GetGuardArea() : nullptr;
	
	RETURN_HANDLE( CAreaComponent, guardArea );
}

void CNewNPC::funcSetGuardArea( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CAreaComponent >, areaHandle, nullptr );
	FINISH_PARAMETERS;
	CAreaComponent *const areaComponent = areaHandle.Get();
	if ( areaComponent )
	{
		CBehTreeGuardAreaData* const guardAreaData = GetGuardAreaData();
		if ( guardAreaData )
		{
			guardAreaData->SetGuardArea( areaComponent );
		}
	}
}

void CNewNPC::funcDeriveGuardArea( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNewNPC >, source, nullptr );
	FINISH_PARAMETERS;
	Bool success = false;
	if ( source )
	{
		if ( CBehTreeGuardAreaData* sourceGuardAreaData = source->GetGuardAreaData() )
		{
			if ( CBehTreeGuardAreaData* targetGuardAreaData = GetGuardAreaData() )
			{
				CAreaComponent* guardArea = sourceGuardAreaData->GetGuardArea();
				CAreaComponent* pursuitArea = sourceGuardAreaData->GetPursuitArea();
				Float pursuitRange = sourceGuardAreaData->GetPursuitRange();
				targetGuardAreaData->SetupImmediateState( guardArea, pursuitArea, pursuitRange );
				success = guardArea != nullptr;
			}
		}
	}
	RETURN_BOOL( success );
}

void CNewNPC::funcIsConsciousAtWork( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool isConscious = IsConsciousAtWork();
	RETURN_BOOL( isConscious );
}

Bool CNewNPC::IsInLeaveAction()
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

	return workData->IsInLeaveAction();
}

void CNewNPC::funcGetCurrentJTType( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Int32 jtType = GetCurrentJTType();
	RETURN_INT( jtType );
}


void CNewNPC::funcIsInLeaveAction( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool isInLeaveAction = IsInLeaveAction();

	RETURN_BOOL( isInLeaveAction );
}

void CNewNPC::funcIsSittingAtWork( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool isSitting = IsSittingAtWork();
	RETURN_BOOL( isSitting );
}

void CNewNPC::funcIsAtWork( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Bool isSitting = IsAtWork();
	RETURN_BOOL( isSitting );
}

void CNewNPC::funcIsPlayingChatScene( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	Bool isInScene = IsPlayingChatScene();

	RETURN_BOOL( isInScene );
}



void CNewNPC::funcCanUseChatInCurrentAP( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;	
	
	Bool ret = CanUseChatInCurrentAP();

	RETURN_BOOL( ret );
}
