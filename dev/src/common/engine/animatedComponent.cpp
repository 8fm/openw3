/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "animatedComponent.h"
#include "animConstraintsParam.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphStack.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphStackSnapshot.h"
#include "behaviorGraphContext.h"
#include "../physics/physicsJointedRagdollWrapper.h"
#include "../physics/physicsChainedRagdollWrapper.h"
#include "animBehaviorsAndSetsParam.h"
#include "../physics/physicsSettings.h"
#include "physicsCharacterWrapper.h"
#include "ragdollPhysX.h"

#include "behaviorGraphUtils.inl"
#include "animDangleComponent.h"

#include "renderFragment.h"
#include "skinningAttachment.h"
#include "skeletonUtils.h"
#include "skeletonBoneSlot.h"
#include "immediateJobsCollector.h"
#include "extAnimEvent.h"
#include "skeletalAnimationContainer.h"
#include "animGlobalParam.h"
#include "game.h"

#include "skeleton.h"
#include "slotComponent.h"
#include "componentIterator.h"
#include "persistentEntity.h"
#include "world.h"
#include "layer.h"
#include "tickManager.h"

#include "actorInterface.h"
#include "animatedSkeleton.h"
#include "animatedIterators.h"
#include "speedConfig.h"

#include "../core/taskBatch.h"
#include "../core/gameSave.h"
#include "../core/feedback.h"
#include "../core/dataError.h"
#include "../core/gatheredResource.h"
#include "animationJobs.h"
#include "../core/listener.h"
#include "utils.h"
#include "baseEngine.h"
#include "meshSkinningAttachment.h"

#include "physicsDataProviders.h"

#ifdef DEBUG_AC
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CAnimatedComponent );


RED_DEFINE_NAME(StopAllAnimationsOnSkeleton);
RED_DEFINE_STATIC_NAME(runtimeBehaviorInstanceSlots);

//////////////////////////////////////////////////////////////////////////

CGatheredResource resCutsceneInstanceBehaviorTemplate( TXT("characters\\templates\\behaviors\\cutscene_graph.w2beh"), 0 );

/////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CAnimatedComponentPhysicsRepresentation );

CAnimatedComponentPhysicsRepresentation::CAnimatedComponentPhysicsRepresentation()
{

}

//////////////////////////////////////////////////////////////////////////

#ifndef RED_FINAL_BUILD
Red::Threads::CAtomic< Int32 > CAnimatedComponent::s_fullyProcessedCounter;
Red::Threads::CAtomic< Int32 > CAnimatedComponent::s_skippedCounter;
#endif

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
extern Bool isAnselTurningOn;
extern Bool isAnselTurningOff;
#endif // USE_ANSEL

//////////////////////////////////////////////////////////////////////////

RED_DISABLE_WARNING_MSC( 4355 )

CAnimatedComponent::CAnimatedComponent()
	: m_animations()
	, m_useExtractedMotion( true )
	, m_extractTrajectory( true )
	, m_allowConstraintsUpdate( true )
	, m_behaviorGraphSampleContext( NULL )
	, m_behaviorGraphUpdateContext( NULL )
	, m_trajectoryBone( -1 )
	, m_timeMultiplier( 1.f )
	, m_behaviorGraphStack( NULL )
	, m_includedInAllAppearances( true )
	, m_relativeMoveSpeed( 0.0f )
	, m_moveHeading( CEntity::HEADING_ANY )
	, m_moveDirection( 0.0f )
	, m_moveRotation( 0.f )
	, m_moveRawDesiredRotation( 0.f )
	, m_moveRotationSpeed( 0.f )
	, m_debugDisp( 0 )
	, m_animatedSkeleton( NULL )
	, m_forceUpdateSkinning( false )
	//, m_hasRagdollAttachments( false )
	//, m_allowRagdollInCutscene( true )
	, m_isInCutscene( false )
	, m_isInScene( false )
	, m_animationMultiplier( 1.0f )
	, m_savable( false )
	, m_behaviorGraphStackSnapshot( NULL )
	, m_updateAnimationSyncJob( NULL )
	, m_attachedToParentWithAnimatedAttachment( false )
	, m_attachedToAnimatedComponentParent( false )
	, m_updatedByAnimatedComponent( nullptr )
	//, m_ragdollDeactivated( false )
	, m_asyncPlayedAnimName(CName::NONE)
	, m_ragdollNodeProvidesValidPose( false )
	, m_skeletonPreRagdollWeight( 0.0f )
	, m_ragdollCollisionType( CNAME( Ragdoll ) )
	, m_storedTimeDelta( 0.0f )
	, m_stickRagdollToCapsule( false )
	, m_behaviorLOD( BL_Lod0 )
	, m_rareTickSkip( 0 )
	, m_rareTickSkipMultiplier( 1 )
	, m_rareTickSkipAdd( 0 )
	, m_rareTickSkipLimit( 1 )
	, m_rareTickSkipLimitDueToAI( 0xffff )
	, m_rareTickAccumulatedTimeDelta( 0.0f )
	, m_rareTickPrevToLastTimeDelta( 0.0f )
	, m_rareTickFramesSkipped( 0xffff )
	, m_rareTickDoneForLOD( (EBehaviorLod)0 )
	, m_rareTickInvalidatePrev( true )
	, m_rareTickForceFullProcessing( true )
	, m_disableConstraintsIfPossible( false )
	, m_poseSetManually( false )
	, m_allowPoseActions( true )
	, m_allowScriptNotifications( true )
	, m_defaultSpeedConfigKey()
	, m_speedConfig( nullptr )
	, m_animationSuppressionMask( 0 )
	, m_isRegisteredForComponentLODding( false )
	, m_overrideBudgetedTickDistance( 0.0f )
	, m_overrideDisableTickDistance( 0.0f )
	, m_componentBBoxMS( Box::RESET_STATE )
	, m_hasBehaviorRagdollNode( false )
	, m_createRagdollAsKinematic( false )
	, m_processPostponedEvents( false )
	, m_wetnessSupplier( nullptr )
	, m_teleportDetector( nullptr )
	, m_isNotInCameraView( false )
{
	m_animations = CreateObject< CSkeletalAnimationContainer >( this );
}

void CAnimatedComponent::SetAsCloneOf( const CAnimatedComponent* otherAnimComponent )
{
	m_ragdoll				= otherAnimComponent->m_ragdoll;
	m_skeleton				= otherAnimComponent->m_skeleton;
	m_animationSets			= otherAnimComponent->m_animationSets;
	m_behaviorInstanceSlots = otherAnimComponent->m_behaviorInstanceSlots;
	m_useExtractedMotion	= otherAnimComponent->m_useExtractedMotion;
	m_extractTrajectory		= otherAnimComponent->m_extractTrajectory;
	m_includedInAllAppearances = otherAnimComponent->m_includedInAllAppearances;

#ifndef NO_COMPONENT_GRAPH
	m_graphPositionX = otherAnimComponent->m_graphPositionX;
	m_graphPositionY = otherAnimComponent->m_graphPositionY;
#endif

	SetGUID( otherAnimComponent->GetGUID() );

	OnPostLoad();
}

const CAnimatedComponent* CAnimatedComponent::GetRootAnimatedComponentFromAttachment( const CComponent* component )
{
	const CNode* temp = component;
	const CAnimatedComponent* lastVisitedAnimCmp = nullptr;
	while ( temp )
	{
		if ( temp->IsA<CAnimatedComponent>() )
		{
			lastVisitedAnimCmp = Cast<CAnimatedComponent>( temp );
		}

		const CHardAttachment* att = temp->GetTransformParent();
		temp = att ? att->GetParent() : nullptr;
	}
	return lastVisitedAnimCmp;
}

Int32 CAnimatedComponent::GetLodBoneNum() const
{
	const CSkeleton* s = GetSkeleton();
	if ( s )
	{
		const EBehaviorLod lod = GetLod();

		// We support lod 1 now
		if ( lod == BL_Lod1 && s->HasLod_1() )
		{
			return s->GetLodBoneNum_1();
		}
		else
		{
			return s->GetLodBoneNum_0();
		}
	}

	return 0;
}

void CAnimatedComponent::SetLod( EBehaviorLod lod )
{
	EBehaviorLod prev = m_behaviorLOD;
	m_behaviorLOD = lod;

	OnLodSet( prev, m_behaviorLOD );
}

EBehaviorLod CAnimatedComponent::GetLod() const
{
	//if ( GGame->GetGameplayConfig().m_useBehaviorLod )
	//{
	//	//...
	//}

	if ( GGame->GetGameplayConfig().m_forceBehaviorLod )
	{
		return (EBehaviorLod)GGame->GetGameplayConfig().m_forceBehaviorLodLevel;
	}

	return m_behaviorLOD;
}

void CAnimatedComponent::FreezePoseFadeIn( Float duration )
{
	if ( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->FreezePoseFadeIn( duration );
	}
}

void CAnimatedComponent::UnfreezePoseFadeOut( Float duration )
{
	if ( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->UnfreezePoseFadeOut( duration );
	}
}

void CAnimatedComponent::FreezePoseBlendIn( Float duration )
{
	if ( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->FreezePoseBlendIn( duration );
	}
}

void CAnimatedComponent::UnfreezePoseBlendOut( Float duration )
{
	if ( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->UnfreezePoseBlendOut( duration );
	}
}

Bool CAnimatedComponent::HasFrozenPose() const
{
	return m_behaviorGraphStack ? m_behaviorGraphStack->HasFrozenPose() : false;
}

const ISkeletonDataProvider* CAnimatedComponent::QuerySkeletonDataProvider() const
{
	return static_cast< const ISkeletonDataProvider* >( this );
}

ISlotProvider* CAnimatedComponent::QuerySlotProvider()
{
	return static_cast< ISlotProvider* >( this );
}

IAnimatedObjectInterface* CAnimatedComponent::QueryAnimatedObjectInterface()
{
	return static_cast< IAnimatedObjectInterface* >( this );
}

CAnimatedComponent::~CAnimatedComponent()
{
	if ( m_animatedSkeleton )
	{
		delete m_animatedSkeleton;
		m_animatedSkeleton = NULL;
	}
}

void CAnimatedComponent::OnFinalize()
{
	if ( !GIsCooker && !IsAttached() )
	{
		// destroy context and behavior graphs in case they were not already destroyed
		DestroyBehaviorContext();
		ClearBehaviorGraphs();

		//ASSERT( m_runtimeBehaviorInstanceSlots.Empty() );
		m_runtimeBehaviorInstanceSlots.Clear();
	}

	// Pass to base class
	TBaseClass::OnFinalize();
}

void CAnimatedComponent::OnDestroyed()
{
	DestroyBehaviorContext();

	ClearBehaviorGraphs();

	// Pass to base class
	TBaseClass::OnDestroyed();
}

void CAnimatedComponent::OnSerialize( IFile &file )
{
	// Pass to base class
	TBaseClass::OnSerialize( file );

	// Links to runtime objects
	
	// Temp uncomment ---
		// byDex: not needed due to hierarchial search
		if ( file.IsGarbageCollector() )
		{
			file << m_animations;
			file << m_behaviorGraphStack;
			file << m_cachedAnimatedChildComponents;
			file << m_cachedAnimatedAttachments;
		}
	// ---+
}

void CAnimatedComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	// Change ragdoll
	if (property->GetName() == CNAME( skeleton ) ||
		property->GetName() == CNAME( skeletonMapper ) ||
		property->GetName() == CNAME( animationSets ) )
	{
		Reset();
	}

	// Change behavior context
	if ( property->GetName() == CNAME( skeleton ) ||
		 property->GetName() == CNAME( skeletonMapper ) ||
		 property->GetName() == CNAME( behaviorInstanceSlots ) )
	{
		// Reset
		m_behaviorGraphSampleContext->Deinit();
		m_behaviorGraphSampleContext->Init( this, GetSkeleton(), GetMimicSkeleton() );
		
		m_behaviorGraphStack->ClearAllStack();
		//m_behaviorGraphStack->Init( m_behaviorInstanceSlots, NULL );
	}

	if ( property->GetName() == TXT("isEnabledInAppearance") )
	{
		PerformFullRecreation();
	}

	if ( property->GetName() == TXT("defaultBehaviorAnimationSlotNode") )
	{
		CBehaviorGraphStack* stack = GetBehaviorStack();
		if ( stack && !stack->ActivateBehaviorInstances( m_defaultBehaviorAnimationSlotNode ) )
		{
			GFeedback->ShowError( TXT("There is no animation with this name") );
		}
	}
}

Bool CAnimatedComponent::CanOverridePropertyViaInclusion( const CName& propertyName ) const
{
	return TBaseClass::CanOverridePropertyViaInclusion( propertyName ) &&
		   !( propertyName == CNAME( runtimeBehaviorInstanceSlots ) );
}

Bool CAnimatedComponent::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName == CNAME( behaviorGraphs ) )
	{
		TDynArray< THandle< CBehaviorGraph > > graphs;
		if ( readValue.AsType( graphs ) )
		{
			for ( Uint32 i=0; i<graphs.Size(); ++i )
			{
				CFilePath path( graphs[i].Get()->GetDepotPath() );

				SBehaviorGraphInstanceSlot slot;
				slot.m_graph = graphs[i];
				slot.m_instanceName = CName( path.GetFileName() );

				m_behaviorInstanceSlots.PushBack( slot );
			}

			return true;
		}
	}

	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

void CAnimatedComponent::SetSkeleton( CSkeleton *skeleton )
{
	m_skeleton = skeleton;

	// Create animated skeleton object
	if ( skeleton )
	{
		if ( m_animatedSkeleton && m_animatedSkeleton->GetSkeleton() != skeleton )
		{
			m_animatedSkeleton->SetSkeleton( skeleton );
		}
		else if ( !m_animatedSkeleton )
		{
			m_animatedSkeleton = new CBehaviorAnimatedSkeleton( skeleton );
		}
	}
	else if ( m_animatedSkeleton )
	{
		delete m_animatedSkeleton;
		m_animatedSkeleton  = NULL;
	}

	OnSkeletonChanged();
}

void CAnimatedComponent::SetRagdoll( CRagdoll* ragdoll )
{
	m_ragdoll = ragdoll;
}

void CAnimatedComponent::SetEnabled( Bool enabled )
{
	TBaseClass::SetEnabled( enabled );
	if( !m_ragdollPhysicsWrapper ) return;
	m_ragdollPhysicsWrapper->SwitchToStatic();
}

void CAnimatedComponent::OnSkeletonChanged()
{
}

void CAnimatedComponent::SetUseExtractedMotion( Bool flag )
{
	m_useExtractedMotion = flag;
}

#ifndef NO_EDITOR
void CAnimatedComponent::SetAllowConstraintsUpdate( Bool flag )
{
	m_allowConstraintsUpdate = flag;

	for ( auto iChild = m_cachedAnimatedChildComponents.Begin(), iEnd = m_cachedAnimatedChildComponents.End(); iChild != iEnd; ++iChild )
	{
		CAnimatedComponent* child = *iChild;
		child->SetAllowConstraintsUpdate( flag );
	}

	for ( auto iAC = m_updateAnimatedComponents.Begin(), iEnd = m_updateAnimatedComponents.End(); iAC != iEnd; ++iAC )
	{
		CAnimatedComponent* ac = *iAC;
		ac->SetAllowConstraintsUpdate( flag );
	}
}
#endif

void CAnimatedComponent::SetUseExtractedTrajectory( Bool flag )
{
	m_extractTrajectory = flag;
}

void CAnimatedComponent::UpdateAttachedComponentsTransforms( SUpdateTransformContext& context )
{
	PC_SCOPE( ACUpdateAttachedComps );

	// Clear bbox from previous frame and fill it with now data form current frame
	m_componentBBoxMS.Clear();

	const TList< IAttachment* >& attachments = GetChildAttachments();
	for ( TList< IAttachment* >::const_iterator it = attachments.Begin(); it != attachments.End(); ++it )
	{
		CMeshSkinningAttachment* skinAtt = ( *it )->ToSkinningAttachment();
		if ( skinAtt )
		{
			skinAtt->UpdateTransformAndSkinningData( m_componentBBoxMS, context.m_skinningContext );

			if ( CNode* child = skinAtt->GetChild() )
			{
				child->UpdateTransformChildrenNodes( context, true );
			}
		}
	}

	if ( m_componentBBoxMS.IsEmpty() )
	{
		m_componentBBoxValid.Reset();
	}
	else
	{
		m_componentBBoxValid.Set();
	}

	// Add zero point because sometimes pose are far away from component's localToWorld and it is hard to detect proper visibility
	m_componentBBoxMS.AddPoint( Vector::ZERO_3D_POINT );
}

void CAnimatedComponent::UpdateAttachedComponentsTransformsWithoutSkinning( SUpdateTransformContext& context )
{
	PC_SCOPE( ACUpdateAttachedComps_WithoutSkin );

	RED_FATAL_ASSERT( m_componentBBoxValid, "CAnimatedComponent::UpdateAttachedComponentsTransformsWithoutSkinning" );
	RED_FATAL_ASSERT( !m_componentBBoxMS.IsEmpty(), "CAnimatedComponent::UpdateAttachedComponentsTransformsWithoutSkinning" );

	const TList< IAttachment* >& attachments = GetChildAttachments();
	for ( TList< IAttachment* >::const_iterator it = attachments.Begin(); it != attachments.End(); ++it )
	{
		CMeshSkinningAttachment* skinAtt = ( *it )->ToSkinningAttachment();
		if ( skinAtt )
		{
			skinAtt->UpdateTransformWithoutSkinningData( m_componentBBoxMS, context.m_skinningContext );

			if ( CNode* child = skinAtt->GetChild() )
			{
				child->UpdateTransformChildrenNodes( context, true );
			}
		}
	}
}

void CAnimatedComponent::CalcTransformsWS( const Int32 lodBoneNum )
{
	// Update world space matrices
	ASSERT( m_skeletonModelSpace.Size() == m_skeletonWorldSpace.Size() );
	SkeletonBonesUtils::MulMatrices( m_skeletonModelSpace.TypedData(), m_skeletonWorldSpace.TypedData(), lodBoneNum, &m_localToWorld );
}

void CAnimatedComponent::CalcTransformsInvMS( const Int32 lodBoneNum )
{
#ifdef USE_OPT_SKINNING
	if ( const CSkeleton* skeleton = GetSkeleton() )
	{
		// Update world space matrices
		ASSERT( m_skeletonModelSpace.Size() == m_skeletonInvBindSpace.Size() );
		ASSERT( skeleton->GetBonesNum() == m_skeletonInvBindSpace.SizeInt() );

		SkeletonBonesUtils::MulMatrices( m_skeletonModelSpace.TypedData(), skeleton->GetReferencePoseInvMS(), m_skeletonInvBindSpace.TypedData(), lodBoneNum );
	}
#endif
}

void CAnimatedComponent::CalcTransformsInvMSAndWS()
{
	const Int32 lodBoneNum = Min( GetLodBoneNum(), m_skeletonModelSpace.SizeInt() );

	{
		PC_SCOPE( CalcMatricesForSkinningInvMS );
		CalcTransformsInvMS( lodBoneNum );
	}

	{
		PC_SCOPE( CalcMatricesForSkinningWS );
		CalcTransformsWS( lodBoneNum );
	}
}

void CAnimatedComponent::CalcTransforms()
{
	PC_SCOPE( CalcTransforms );

	const Int32 lodBoneNum = Min( GetLodBoneNum(), m_skeletonModelSpace.SizeInt() );

	const Bool validBehaviorGraphSampleContext = m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid();
	const Bool samplePoseFromRagdoll = m_ragdollPhysicsWrapper && !m_ragdollPhysicsWrapper->IsKinematic() && validBehaviorGraphSampleContext;
	const Bool canCalcMSandWSNow = m_cachedAnimatedAttachments.Empty();

	if ( validBehaviorGraphSampleContext )
	{
		PC_SCOPE( CalcMatricesForSkinningMS );

		// Update model space matrices from provided local space
		SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetSampledPose();
		pose.GetBonesModelSpace( m_skeleton.Get(), m_skeletonModelSpace, lodBoneNum );

		// Calc MS and WS here because of cache
		if ( !samplePoseFromRagdoll && canCalcMSandWSNow )
		{
			CalcTransformsInvMSAndWS();
		}
	}

	{
		PC_SCOPE( RagdollToAnimation );
		Bool inRagdoll = false;
		if ( m_ragdollPhysicsWrapper )
		{
			// get bones from ragdoll
			m_ragdollPhysicsWrapper->SampleBonesModelSpace( m_skeletonModelSpace );

			// if ragdoll is not key framed (ie. this means that ragdoll is active) we should update local bones and process attached animated objects now (properly with stored time delta
			if (! m_ragdollPhysicsWrapper->IsKinematic())
			{
				if( validBehaviorGraphSampleContext )
				{
					SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetSampledPose();

					if ( m_ragdollNodeProvidesValidPose )
					{
						// update pose from input - we will use that for pre-ragdoll blending
						Red::System::MemoryCopy( m_skeletonPreRagdollLocalSpace.Data(), pose.m_outputPose, sizeof( AnimQsTransform ) * m_skeletonPreRagdollLocalSpace.Size() );
					}

					// fill local space basing on model space taken from ragdoll
					pose.FillPoseWithBonesModelSpace( this, m_skeletonModelSpace );

					if ( m_skeletonPreRagdollWeight > 0.0f )
					{
						// apply quick blend
						// blend local space poses
						m_skeletonPreRagdollWeight = Max( 0.0f, m_skeletonPreRagdollWeight - m_storedTimeDelta / 0.2f );
						AnimQsTransform* bone = pose.m_outputPose;
						for ( auto stored = m_skeletonPreRagdollLocalSpace.Begin(), end = m_skeletonPreRagdollLocalSpace.End(); stored != end; ++ stored, ++ bone )
						{
							BlendTwoTransforms( *bone, *bone, *stored, m_skeletonPreRagdollWeight );
						}
						// and get result to model space back
						pose.GetBonesModelSpace( GetSkeleton(), m_skeletonModelSpace, lodBoneNum );
					}

					// update animated objects now
					UpdateAttachedAnimatedObjectsLSConstrainted( m_storedTimeDelta, &pose );
				}

				ASSERT( samplePoseFromRagdoll );

				inRagdoll = true;
			}
		}
		
		if ( !inRagdoll )
		{
			if( validBehaviorGraphSampleContext )
			{
				Red::System::MemoryCopy( m_skeletonPreRagdollLocalSpace.Data(), m_behaviorGraphSampleContext->GetSampledPose().m_outputPose, sizeof( AnimQsTransform ) * m_skeletonPreRagdollLocalSpace.Size() );

				m_skeletonPreRagdollWeight = IsRagdolled( false )? 1.0f : 0.0f; // blend only when from animated ragdoll
			}
			else
			{
				m_skeletonPreRagdollWeight = 0.0f;
			}
		}

		m_ragdollNodeProvidesValidPose = false;
	}

	if ( samplePoseFromRagdoll && canCalcMSandWSNow )
	{
		CalcTransformsInvMSAndWS();
	}
}

void CAnimatedComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );
}

void CAnimatedComponent::OnUpdateTransformNode( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( ACUpdateTransform_npc );

	TBaseClass::OnUpdateTransformNode( context, prevLocalToWorld );

	const Matrix& l2w = GetLocalToWorld();
	SetThisFrameTempLocalToWorld( l2w );

	const Bool isInCinematic = IsInCinematic();

	if ( m_componentBBoxValid && !isInCinematic && context.m_cameraVisibilityData )
	{
		//RED_FATAL_ASSERT( !m_componentBBoxMS.IsEmpty(), "CAnimatedComponent::OnUpdateTransformNode" );
		RED_ASSERT( !m_componentBBoxMS.IsEmpty(), TXT("CAnimatedComponent::OnUpdateTransformNode") );

		// m_componentBBoxMS is BBox of the component in MS from previous frame
		const Box componentBBoxWS = l2w.TransformBox( m_componentBBoxMS );
		m_isNotInCameraView = context.m_cameraVisibilityData->IsBoxOutside( componentBBoxWS );

		if ( m_componentBBoxMS.IsEmpty() )
		{
			m_isNotInCameraView = false;
		}
	}
	else
	{
		m_isNotInCameraView = false;
	}

#ifdef USE_ANSEL
	if( isAnselSessionActive || isAnselTurningOn || isAnselTurningOff )
	{
		m_isNotInCameraView = false;
	}
#endif // USE_ANSEL

	// ideally we could use suppression mask (m_animationSuppressionMask) around here and calculations below to avoid calculating transforms
	// but for that we would need to change how attached objects are updated, what they use etc. which might be risky right now
	// and we still have to update ragdoll to avoid situations in which character moved far away but its ragdoll stayed behind (this of course could be solved by disabling/enabling ragdoll)
	CalcTransforms();

	// check teleport/pose change
	if ( m_teleportDetector && m_behaviorGraphSampleContext )
	{
		const SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetSampledPose();
		const SBehaviorGraphOutput& pose2 = m_behaviorGraphSampleContext->GetMainPose();
		m_teleportDetector->CheckTeleport( pose.m_outputPose, pose.m_numBones, m_skeletonModelSpace, l2w );
	}

	UpdateAttachedAnimatedObjectsWS();

	// If we have children they can modify parent's pose we need to update MS inv and WS later (here)
	if ( m_cachedAnimatedAttachments.Size() > 0 )
	{
		CalcTransformsInvMSAndWS();
	}

	// calculate wetness data. Here we have WS bones from current frame
	if ( m_wetnessSupplier )
	{
		m_wetnessSupplier->CalcWetness( m_skeletonWorldSpace );
	}

	if ( m_ragdollPhysicsWrapper )
	{
		m_ragdollPhysicsWrapper->SyncToAnimation( m_skeletonModelSpace, l2w );
	}

	// ANSEL integration
#ifdef USE_ANSEL
	Bool DO_NOT_USE_SKIN_OPT = isAnselSessionActive || isAnselTurningOn || isAnselTurningOff;
#else
	static Bool DO_NOT_USE_SKIN_OPT = false;
#endif // USE_ANSEL
	if ( DO_NOT_USE_SKIN_OPT )
	{
		// Update child component
		UpdateAttachedComponentsTransforms( context );
	}
	else
	{
		if ( !m_isNotInCameraView )
		{
			UpdateAttachedComponentsTransforms( context );
		}
		else
		{
			RED_FATAL_ASSERT( m_componentBBoxValid, "CAnimatedComponent::OnUpdateTransformNode" );
			UpdateAttachedComponentsTransformsWithoutSkinning( context );
		}
	}
}

namespace
{
	CName GetDefaultAnimationName( const CAnimatedComponent* ac )
	{
		const CEntity* entity = ac->GetEntity();
		const CEntityTemplate* templ = entity->GetEntityTemplate();
		if ( templ )
		{
			CAnimGlobalParam* param = templ->FindParameter< CAnimGlobalParam >();
			if ( param )
			{
				return param->GetDefaultAnimationName();
			}
		}
		return CName::NONE;
	}
}

void CAnimatedComponent::BuildAnimContainer()
{
	PC_SCOPE_PIX(AC_BuildAnimContainer);
	Bool animContBuild = false;

	CEntityTemplate* templ = GetEntity()->GetEntityTemplate();
	if ( templ )
	{
		PC_SCOPE_PIX(AC_BuildAnimContainerTemplate);
		TDynArray< CAnimAnimsetsParam* > params;
		templ->GetAllParameters( params );

		TSkeletalAnimationSetsArray sets;

		const Uint32 animsetSize = params.Size();
		if ( animsetSize > 0 )
		{
			for ( Uint32 i=0; i<animsetSize; ++i )
			{
				const CAnimAnimsetsParam* param = params[ i ];
				if ( param->GetComponentName() == GetName() )
				{
					if ( !animContBuild )
					{
						sets.PushBack( m_animationSets );

						animContBuild = true;
					}

					sets.PushBack( param->GetAnimationSets() );
				}
			}

			if ( animContBuild )
			{
				m_animations->RebuildFromAnimationSets( sets );
			}
		}
	}

	if ( !animContBuild )
	{
		m_animations->RebuildFromAnimationSets( m_animationSets );
	}

#ifndef NO_DEFAULT_ANIM
	m_animations->CreateDebugAnimation( m_skeleton.Get(), GetDefaultAnimationName( this ) );
#endif
}

void CAnimatedComponent::PostInitialization()
{
	DEBUG_ANIM_MATRIX( GetEntity()->GetLocalToWorld() );

	PC_SCOPE_PIX( AC_PostInitialization );
	Reset();

	// Create context for animation
	CreateBehaviorContext();

	ASSERT( m_behaviorGraphUpdateContext );
	ASSERT( m_behaviorGraphSampleContext );

	ForceTPose();

	// Rebuild animation container
	BuildAnimContainer();

	// Load behaviors after animation container creation
	LoadBehaviorGraphs();

	// Cache animation attachments
	CacheAnimatedAttachments();

	// Called event
	OnPostInitializationDone();

	// Set as initialized
	m_postInstanceInitializationDone = true;
}

void CAnimatedComponent::OnPostInitializationDone()
{
	
}

void CAnimatedComponent::OnPostComponentInitializedAsync()
{
	TBaseClass::OnPostComponentInitializedAsync();

	// Initialize
	if ( !m_postInstanceInitializationDone )
	{
		PostInitialization();
	}
}

void CAnimatedComponent::ProcessScriptNotificationsFromStack()
{
	if ( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->ProcessScriptNotifications( this );
	}
}

void CAnimatedComponent::ForcePoseAndStuffDuringCook()
{
	// Create context for animation
	CreateBehaviorContext();

	// PTom: this function can be removed - cache initial component pose ( and force it in async part )
	// Force initial pose
	if ( m_behaviorGraphStack && m_behaviorGraphSampleContext )
	{
		ForceBehaviorPose();
	}

	if ( GIsCooker )
	{
		DestroyBehaviorContext();
	}
}


void CAnimatedComponent::OnInitialized()
{
	TBaseClass::OnInitialized();

	// Initialize
	if ( !m_postInstanceInitializationDone )
	{
		PostInitialization();
	}
}


void CAnimatedComponent::OnUninitialized()
{
	TBaseClass::OnUninitialized();

	// Clear behavior graph instance stack
	ClearBehaviorGraphs();

	// Destroy context
	DestroyBehaviorContext();

	// Clear animations
	ClearAnimContainer();

	// Request full initialization
	m_postInstanceInitializationDone = false;
}

void CAnimatedComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	// Register in debug group
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_AI );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Behavior );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Wetness );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_TeleportDetector );
#ifdef DEBUG_CAM_ASAP
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_CameraVisibility );
#endif

	PC_SCOPE_PIX( CAnimatedComponent_OnAttached );

	// Reset timer multiplier
	m_timeMultiplier = 1.0f;

	// Attach only if used by selected appearance
	if ( m_includedInAllAppearances || IsUsedInAppearance() )
	{
		// PTom: this function can be removed - cache initial component pose ( and force it in async part )
		// Force initial pose
		//ForceBehaviorPose();
		// Register in proper tick groups
		if ( ShouldAddToTickGroups() )
		{
			AddToTickGroups( world );
		}

	}

	if( m_isFrozenOnStart )
	{
		Freeze();
	}

	if ( GetEntity()->IsDependentComponentGameplayLODable( this ) )
	{
		ASSERT( !m_isRegisteredForComponentLODding );
		m_isRegisteredForComponentLODding = true;
		world->GetComponentLODManager().Register( this );
	}

	if ( m_defaultSpeedConfigKey.Empty() == false )
	{
		m_speedConfig = GGame->GetSpeedConfigManager()->GetSpeedConfig( m_defaultSpeedConfigKey );
	}

	const CSkeleton* sk = GetSkeleton();
	if ( const CWetnessComponent* wc = GetEntity()->FindComponent< CWetnessComponent >() )
	{
		if ( sk != nullptr )
		{
			RED_FATAL_ASSERT( m_wetnessSupplier == nullptr, "Something went wrong with attach. Wetness will leak" );
			m_wetnessSupplier = new CWetnessSupplier( wc, sk->GetBonesNum() );
		}
	}

	if ( sk && sk->GetTeleportDetectorData() )
	{
		RED_FATAL_ASSERT( m_teleportDetector == nullptr, "Teleport detector is already created. Something went wrong." );
		m_teleportDetector = new CTeleportDetector( sk, GetLocalToWorld() );
	}

	m_isNotInCameraView = false;
	m_componentBBoxValid.Reset();
}

void CAnimatedComponent::OnDetached( CWorld* world )
{
	if ( m_isRegisteredForComponentLODding )
	{
		world->GetComponentLODManager().Unregister( this );
		m_isRegisteredForComponentLODding = false;
	}

	DontUpdateByOtherAnimatedComponent();
	for ( auto iAC = m_updateAnimatedComponents.Begin(), iEnd = m_updateAnimatedComponents.End(); iAC != iEnd; ++ iAC )
	{
		CAnimatedComponent* ac = *iAC;
		ac->DontUpdateByOtherAnimatedComponent();
	}
	m_updateAnimatedComponents.ClearFast();

	// Remove from all tick groups
	RemoveFromTickGroups( world );

	TBaseClass::OnDetached( world );

	if( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->Reset();
	}

	// Stop all played animations
	if ( m_animatedSkeleton )
	{
		m_animatedSkeleton->StopAllAnimation();
	}

	// Clear event track list
	m_trackedEvents.Clear();

	// Unregister from debug group
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_AI );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Behavior );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Wetness );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_TeleportDetector );
#ifdef DEBUG_CAM_ASAP
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_CameraVisibility );
#endif

	if ( m_ragdollPhysicsWrapper )
	{
		m_ragdollPhysicsWrapper->Release();
		m_ragdollPhysicsWrapper = nullptr;
	}

	if ( m_wetnessSupplier )
	{
		delete m_wetnessSupplier;
		m_wetnessSupplier = nullptr;
	}

	if ( m_teleportDetector )
	{
		delete m_teleportDetector;
		m_teleportDetector = nullptr;
	}

	m_isNotInCameraView = false;
	m_componentBBoxValid.Reset();
}

void CAnimatedComponent::DontUpdateByOtherAnimatedComponent()
{
	if ( m_updatedByAnimatedComponent )
	{
		m_updatedByAnimatedComponent->m_updateAnimatedComponents.RemoveFast( this );
		m_updatedByAnimatedComponent = nullptr;
	}
	UpdateTickGroups();
}

void CAnimatedComponent::UpdateByOtherAnimatedComponent( CAnimatedComponent* ac )
{
	DontUpdateByOtherAnimatedComponent();
	m_updatedByAnimatedComponent = ac;
	if ( m_updatedByAnimatedComponent )
	{
		m_updatedByAnimatedComponent->m_updateAnimatedComponents.PushBackUnique( this );
	}
	UpdateTickGroups();
}

void CAnimatedComponent::SetACMotionEnabled( Bool b )
{

}
Bool CAnimatedComponent::IsACMotionEnabled() const
{
	return true;
}
Bool CAnimatedComponent::CanUseBehavior() const
{
	return	m_behaviorGraphStack && 
			m_behaviorGraphStack->GetNumInstances() > 0 &&
			m_behaviorGraphStack->IsActive() && 
			m_behaviorGraphUpdateContext && 
			m_behaviorGraphSampleContext && 
			m_behaviorGraphSampleContext->IsValid();
}

Bool CAnimatedComponent::CanUseAnimatedSkeleton() const
{
	return	m_behaviorGraphSampleContext && 
			m_behaviorGraphSampleContext->IsValid() && 
			m_animatedSkeleton && 
			m_animatedSkeleton->IsPlayingAnyAnimation();
}

Bool CAnimatedComponent::CanUseBehaviorLod() const
{
	// PTom TODO
	return true;//!GetEntity()->IsA< CCamera >();
}

EBehaviorLod CAnimatedComponent::GetBehaviorLodLevel() const
{
	/*if ( CanUseBehaviorLod() )
	{
		if ( GGame->GetGameplayConfig().m_useBehaviorLod )
		{
			//...
		}

		if ( GGame->GetGameplayConfig().m_forceBehaviorLod )
		{
			return (EBehaviorLod)GGame->GetGameplayConfig().m_forceBehaviorLodLevel;
		}
	}*/

	return BL_Lod0;
}

void CAnimatedComponent::SetupBehaviorContexts( SBehaviorSampleContext* sampleContext, SBehaviorUpdateContext* updateContext /* = NULL  */)
{
	ASSERT( sampleContext );

	EBehaviorLod lod = GetBehaviorLodLevel();

	sampleContext->SetLodLevel( lod );

	if ( updateContext )
	{
		updateContext->SetLodLevel( lod );
	}
}

void CAnimatedComponent::ProcessPoseActionsFromBehaviorContexts( const SBehaviorGraphOutput& output, SBehaviorSampleContext* sampleContext, SBehaviorUpdateContext* updateContext /* = NULL  */)
{
	sampleContext->ProcessPostActions( this, output );

	if ( updateContext )
	{
		updateContext->ProcessPostActions( this, output );
	}
}

void CAnimatedComponent::OnTickPrePhysics( Float timeDelta )
{
	Float orgTimeDelta = timeDelta;

	//FinishAsyncUpdateAndSample();

	m_movementFinalized = false;

	timeDelta *= m_timeMultiplier;

	TBaseClass::OnTickPrePhysics( timeDelta );

	for ( auto iAC = m_updateAnimatedComponents.Begin(), iEnd = m_updateAnimatedComponents.End(); iAC != iEnd; ++ iAC )
	{
		CAnimatedComponent* ac = *iAC;
		ac->OnTickPrePhysics( orgTimeDelta );
	}
}

void CAnimatedComponent::OnTickPrePhysicsPost( Float timeDelta )
{
	Float orgTimeDelta = timeDelta;

	if ( ! GGame->GetGameplayConfig().m_animationMultiUpdate )
	{
		// use Async version to have both working the same way (so we won't have difference between debugger on/off)
		InternalUpdateAndSampleMultiAsyncPart( m_timeMultiplier * timeDelta );
	}

#ifdef PHYSICS_NAN_CHECKS
	if( !GetLocalToWorld().IsOk() )
	{
		RED_FATAL_ASSERT( GetLocalToWorld().IsOk(), "NANS" );
#ifndef RED_PLATFORM_ORBIS
		for( Uint32 i = 0; i != 16; ++i )
		{
			const Float* temp = &GetLocalToWorld().V[ 0 ].X;
			int mask = _fpclass( *( temp + i ) );
			String message = String::Printf( TXT( "%f %x" ), *( temp + i ), *(unsigned int*)( temp + i ) );
			if( mask & _FPCLASS_SNAN ) message += TXT( " _FPCLASS_SNAN Signaling NaN " );
			if( mask & _FPCLASS_QNAN ) message += TXT( " _FPCLASS_QNAN Quiet NaN " );
			if( mask & _FPCLASS_NINF ) message += TXT( " _FPCLASS_NINF Negative infinity ( –INF) " );
			if( mask & _FPCLASS_NN ) message += TXT( " _FPCLASS_NN Negative normalized non-zero " );
			if( mask & _FPCLASS_ND ) message += TXT( " _FPCLASS_ND Negative denormalized " );
			if( mask & _FPCLASS_NZ ) message += TXT( " _FPCLASS_NZ Negative zero ( – 0) " );
			if( mask & _FPCLASS_PZ ) message += TXT( " _FPCLASS_PZ Positive 0 (+0) " );
			if( mask & _FPCLASS_PD ) message += TXT( " _FPCLASS_PD Positive denormalized " );
			if( mask & _FPCLASS_PN ) message += TXT( " _FPCLASS_PN Positive normalized non-zero " );
			if( mask & _FPCLASS_PINF ) message += TXT( " _FPCLASS_PINF Positive infinity (+INF) " );
			RED_LOG( RED_LOG_CHANNEL( NaNCheck ), message.AsChar() );
		}
#endif	
	}
	else
#endif
	{
		PC_SCOPE( AnimatePrePhysicsRagdoll );
		CPhysicsWorld* physicsWorld = nullptr;
		if ( !m_ragdollPhysicsWrapper && m_ragdoll && GetWorld()->GetPhysicsWorld( physicsWorld )
#ifndef RED_FINAL_BUILD
			&& !SPhysicsSettings::m_dontCreateRagdolls 
#endif					
			)
		{
			CSkeleton* skeleton = GetSkeleton();
			::BoneInfo bi;
			if ( !skeleton )
			{
				return;
			}
			const Uint32 numBones = skeleton->GetBonesNum();
			TDynArray<::BoneInfo> bones( numBones );

			for ( Uint32 i = 0; i < numBones; ++i )
			{
				::BoneInfo& bi = bones[i];

				bi.m_boneName = skeleton->GetBoneNameAnsi( i );
				bi.m_parentIndex = (Uint16)skeleton->GetParentBoneIndex( i );

				const AnimQsTransform trans = skeleton->GetBoneLS( i );
#ifdef USE_HAVOK_ANIMATION
				HavokTransformToMatrix( trans, &bi.m_initLocalPose );
#else
				const RedMatrix4x4 matrix = trans.ConvertToMatrix();
				bi.m_initLocalPose = reinterpret_cast<const Matrix&>(matrix);
#endif
			}

#ifdef USE_PHYSX
			CPhysicsEngine::CollisionMask collisionType;
			CPhysicsEngine::CollisionMask collisionGroup;
			m_ragdollCollisionType.RetrieveCollisionMasks( collisionType, collisionGroup );
			CEntity* parentEntity = GetEntity();
			Bool persistentEntity = parentEntity->IsA< CPeristentEntity >();

			if( CWorld* world = GetWorld() )
			{
				ERenderVisibilityResult result = parentEntity->GetLastFrameVisibility();

				TRenderVisibilityQueryID visibiltyId = parentEntity->GetVisibilityQuery();
				if( parentEntity && parentEntity->IsPlayer() )
				{
					//hack becouse what have been seen from tests on player visibility query is always return as unvisible...
					visibiltyId = 0;
				}
				if( m_hasBehaviorRagdollNode )
				{
					collisionType |= GPhysicEngine->GetCollisionTypeBit( CNAME( SoftKinematicContact ) );

#ifdef EP2_SHEEP_RAGDOLL_ANGULAR_DAMPING_HACK
					auto file = m_ragdoll->GetFile();
					String fileName = String::EMPTY;
					if( file )
					{
						fileName = file->GetFileName();
					}
					Float angularDamp = SPhysicsSettings::m_ragdollChainedAngularDamper;
					if( fileName == TXT("sheep_ragdoll.w2ragdoll") )
					{
						angularDamp = 0.5f;
					}

					if( CPhysicsChainedRagdollWrapper* wrapper = physicsWorld->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this )
						, m_ragdoll.Get()->GetState(), m_ragdoll.Get()->GetPhysicsBuffer(), bones, collisionType, collisionGroup, visibiltyId, angularDamp ) )
#else
					if( CPhysicsChainedRagdollWrapper* wrapper = physicsWorld->GetWrappersPool< CPhysicsChainedRagdollWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider() this )
						, m_ragdoll.Get()->GetState(), m_ragdoll.Get()->GetPhysicsBuffer(), bones, collisionType, collisionGroup, visibiltyId ) )
#endif
					{
						if( wrapper->IsOk() )
						{
							m_ragdollPhysicsWrapper = wrapper;
							wrapper->SetCodeCallback( CPhysicsWrapperInterface::EPCCT_OnCollision, GetEntity()->QueryPhysicalCollisionTriggerCallback(), GetEntity() );
							wrapper->SetFlag( PRBW_DetailedConntactInfo, true );
						}
						else
						{
							wrapper->Release();
							m_ragdollPhysicsWrapper = physicsWorld->GetWrappersPool< CPhysicsJointedRagdollWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), m_ragdoll.Get()->GetState(), m_ragdoll.Get()->GetPhysicsBuffer(), bones, collisionType, collisionGroup, visibiltyId );
							m_ragdollPhysicsWrapper->SwitchToKinematic( true );
						}
					}
				}
				else
				{
					m_ragdollPhysicsWrapper = physicsWorld->GetWrappersPool< CPhysicsJointedRagdollWrapper, SWrapperContext >()->Create( CPhysicsWrapperParentComponentProvider( this ), m_ragdoll.Get()->GetState(), m_ragdoll.Get()->GetPhysicsBuffer(), bones, collisionType, collisionGroup, visibiltyId );
				}
				if ( m_ragdollPhysicsWrapper != nullptr && m_createRagdollAsKinematic )
				{
					m_ragdollPhysicsWrapper->SwitchToKinematic( true );
				}
			}
#endif
		}
	}

	timeDelta *= m_animationMultiplier;

	{
		PC_SCOPE( UpdateAndSampleAnimationSyncPart );
		InternalUpdateAndSampleMultiSyncPart( m_timeMultiplier * timeDelta );
	}

	for ( auto iAC = m_updateAnimatedComponents.Begin(), iEnd = m_updateAnimatedComponents.End(); iAC != iEnd; ++ iAC )
	{
		CAnimatedComponent* ac = *iAC;
		ac->OnTickPrePhysicsPost( orgTimeDelta );
	}
}

void CAnimatedComponent::OnTick( Float timeDelta )
{
	Float orgTimeDelta = timeDelta;

	timeDelta *= m_timeMultiplier;

	TBaseClass::OnTick( timeDelta );

	for ( auto iAC = m_updateAnimatedComponents.Begin(), iEnd = m_updateAnimatedComponents.End(); iAC != iEnd; ++ iAC )
	{
		CAnimatedComponent* ac = *iAC;
		ac->OnTick( orgTimeDelta );
	}

	if( !m_processPostponedEvents ) return;
	for ( auto iter = m_lastFrameEvents.Begin(); iter != m_lastFrameEvents.End(); ++iter )
	{
		iter->m_extEvent->ProcessPostponed( *iter, this );
	}
	m_processPostponedEvents = false;
}

void CAnimatedComponent::OnTickPostPhysics( Float timeDelta )
{
	PC_SCOPE_PIX( CAnimatedComponent_OnTickPostPhysics )

	timeDelta *= m_timeMultiplier;

	TBaseClass::OnTickPostPhysics( timeDelta );

	if ( !m_movementFinalized )
	{
		{
			PC_SCOPE( CAnimatedComponent_FinalizeMovement_PreSeparation );
			FinalizeMovement1_PreSeparation(timeDelta);
		}
		{
			PC_SCOPE( CAnimatedComponent_FinalizeMovement_PostSeparation );
			FinalizeMovement2_PostSeparation(timeDelta);
		}
	}

	// store time delta in case we won't be updating constraints now
	m_storedTimeDelta = timeDelta;
}

void CAnimatedComponent::OnTickPostPhysicsPost( Float timeDelta )
{
	Float orgTimeDelta = timeDelta;

	timeDelta *= m_timeMultiplier;

	TBaseClass::OnTickPostPhysicsPost( timeDelta );

	if ( ! ShouldBeUpdatedByAnimatedComponentParent() && ! m_updatedByAnimatedComponent )
	{
		PC_SCOPE( PostPoseConstraintsSyncPart );

		if ( !GGame->GetGameplayConfig().m_animationMultiUpdate )
		{
			UpdatePoseConstraints( timeDelta );
		}

		//PostUpdatePoseConstraints( timeDelta );
	}
}

void CAnimatedComponent::UpdatePoseConstraints( Float timeDelta )
{
	if ( m_animationSuppressionMask != 0 )
	{
		return;
	}

	Float orgTimeDelta = m_timeMultiplier != 0.0f? timeDelta / m_timeMultiplier : timeDelta;

	// apply pose constraints (but only if sample is valid, it may not be valid if animated component is not connected - and maybe due to this fact, it shouldn't be ticked at all?)
	if ( m_behaviorGraphSampleContext != NULL &&
		 m_behaviorGraphSampleContext->IsValid() )
	{
		// by this moment we need to have local to world updated (check CAnimatedComponent::FinalizeMovement)
		SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetSampledPose();
			
		// Apply pose constraints - after ApplyDeltaMovement
		ProcessPoseConstraints( timeDelta, pose );
	}

	// Do pose processing on cached
	for ( auto iChild = m_cachedAnimatedChildComponents.Begin(), iEnd = m_cachedAnimatedChildComponents.End(); iChild != iEnd; ++ iChild )
	{
		CAnimatedComponent* child = *iChild;
		if ( child->ShouldBeUpdatedByAnimatedComponentParent() )
		{
			child->UpdatePoseConstraints( timeDelta );
		}
	}

	for ( auto iAC = m_updateAnimatedComponents.Begin(), iEnd = m_updateAnimatedComponents.End(); iAC != iEnd; ++ iAC )
	{
		CAnimatedComponent* ac = *iAC;
		ac->UpdatePoseConstraints( orgTimeDelta * ac->m_timeMultiplier );
	}

	PostUpdatePoseConstraints( timeDelta );
}

void CAnimatedComponent::PostUpdatePoseConstraints( Float timeDelta )
{
	Float orgTimeDelta = m_timeMultiplier != 0.0f? timeDelta / m_timeMultiplier : timeDelta;

	// sync part
	if ( m_behaviorGraphSampleContext != NULL &&
		 m_behaviorGraphSampleContext->IsValid() )
	{
		PC_SCOPE( PostProcessPoseConstraints_AttachedObjects );
		SBehaviorGraphOutput& constraintedPose = m_behaviorGraphSampleContext->GetSampledPose();
		Float attachedAnimatedObjectsTimeDelta = (! m_ragdollPhysicsWrapper || m_ragdollPhysicsWrapper->IsKinematic())? timeDelta : 0.0f;
		UpdateAttachedAnimatedObjectsLSConstrainted( attachedAnimatedObjectsTimeDelta, &constraintedPose );
	}

	// Update children, so whole update is in proper order - this is reason why CollectImmediateJobs doesn't create jobs for animated components attached to 
	for ( auto iChild = m_cachedAnimatedChildComponents.Begin(), iEnd = m_cachedAnimatedChildComponents.End(); iChild != iEnd; ++ iChild )
	{
		CAnimatedComponent* child = *iChild;
		if ( child->ShouldBeUpdatedByAnimatedComponentParent() )
		{
			child->PostUpdatePoseConstraints( timeDelta );
		}
	}

	for ( auto iAC = m_updateAnimatedComponents.Begin(), iEnd = m_updateAnimatedComponents.End(); iAC != iEnd; ++ iAC )
	{
		CAnimatedComponent* ac = *iAC;
		if ( ac->m_updatedByAnimatedComponent )
		{
			ac->PostUpdatePoseConstraints( orgTimeDelta * ac->m_timeMultiplier );
		}
	}
}
void CAnimatedComponent::OnTickPostUpdateTransform( Float timeDelta )
{
	PC_SCOPE_PIX( CAnimatedComponent_OnTickPostUpdateTransform );

	Float orgTimeDelta = timeDelta;

	timeDelta *= m_timeMultiplier;

	TBaseClass::OnTickPostUpdateTransform( timeDelta );

	/*if ( m_forceUpdateSkinning )
	{
		m_forceUpdateSkinning = false;
		CEntity* ent = GetEntity();
		ent->ForceUpdateTransform();
		ent->ForceUpdateBounds();
	}*/

	//if ( CanUseAsyncUpdateMode() )
	//{
	//	StartAsyncUpdateAndSample( timeDelta );
	//}

	for ( auto iAC = m_updateAnimatedComponents.Begin(), iEnd = m_updateAnimatedComponents.End(); iAC != iEnd; ++ iAC )
	{
		CAnimatedComponent* ac = *iAC;
		ac->OnTickPostUpdateTransform( orgTimeDelta );
	}
}

ISlot* CAnimatedComponent::CreateSlot( const String& slotStr )
{
	//RED_MESSAGE( "CAnimatedComponent::CreateSlot - TODO" )

	// Try bone slot
	TDynArray< BoneInfo > bones;
	GetBones( bones );

	const CName slotName = CName( slotStr );

	// Linear search by name
	for ( Uint32 i=0; i<bones.Size(); i++ )
	{
		const BoneInfo& bone = bones[i];
		if ( bone.m_name == slotName )
		{
			// Create skeleton slot
			CSkeletonBoneSlot *newSlot = new CSkeletonBoneSlot( i );
			newSlot->SetParent( this );
			return newSlot;
		}
	}

	// Slot not found
	return NULL;
}

void CAnimatedComponent::EnumSlotNames( TDynArray< String >& slotNames ) const
{
	TDynArray< BoneInfo > bones;
	GetBones( bones );

	for ( Uint32 i = 0; i < bones.Size(); ++i )
	{
		slotNames.PushBack( bones[i].m_name.AsString() );
	}
}

void CAnimatedComponent::Reset()
{
	// Restore state
	SetSkeleton( GetSkeleton() );

	//dex++: changed to standardized standarized CSkeleton interface
	if ( nullptr != m_skeleton.Get() )
	{
		// Always keep size of bones array synced
		const Uint32 numBones = m_skeleton->GetBonesNum();
		if ( numBones != m_skeletonWorldSpace.Size() )
		{
			m_skeletonModelSpace.Resize( numBones );
			m_skeletonWorldSpace.Resize( numBones );
			m_skeletonPreRagdollLocalSpace.Resize( numBones );
#ifdef USE_OPT_SKINNING
			m_skeletonInvBindSpace.Resize( numBones );
#endif

			// Set T-pose on skeleton
			//ForceTPose();
		}
	}
	//dex--

	// Cache trajectory bone
	CacheTrajectoryBone();
}

void CAnimatedComponent::LoadBehaviorGraphs()
{
	PC_SCOPE_PIX(AC_LoadBehaviorGraphs);
	if ( !m_behaviorGraphStack )
	{
		// Create stack
		m_behaviorGraphStack = ::CreateObject< CBehaviorGraphStack >( this );
		m_behaviorGraphStack->SetFlag( OF_Transient );
	}
	else
	{
		ASSERT( m_behaviorGraphStack );
	}

	// Activate
	ASSERT( !m_behaviorGraphStack->IsActive() );
	m_behaviorGraphStack->Activate();

	CEntityTemplate* templ = GetEntity()->GetEntityTemplate();

	// Find constraint graph
	const CBehaviorGraph* constraintGraph = NULL;
	if ( templ && this == GetEntity()->GetRootAnimatedComponent() )
	{
		const CAnimConstraintsParam* constraintParam = templ->FindParameter< CAnimConstraintsParam >();
		if ( constraintParam )
		{
			constraintGraph = constraintParam->GetConstraintGraph();
		}
	}

	// Load 'alwaysLoaded' behavior graphs
	//for ( Uint32 i=0; i<m_behaviorInstanceSlots.Size(); ++i )
	//{
	//	SBehaviorGraphInstanceSlot& slot = m_behaviorInstanceSlots[ i ];
	//	if ( slot.m_alwaysLoaded )
	//	{
	//		slot.m_graph.Load();
	//	}
	//}

	{
		m_runtimeBehaviorInstanceSlots.Clear();

		CEntityTemplate* templ = GetEntity()->GetEntityTemplate();
		if ( templ )
		{
			TDynArray< CAnimBehaviorsParam* > params;
			templ->GetAllParameters( params );

			const Int32 slotsSize = params.SizeInt();
			if ( slotsSize > 0 )
			{
				for ( Int32 i=slotsSize-1; i>=0; --i )
				{
					CAnimBehaviorsParam* param = params[ i ];
					if ( param->GetComponentName() == GetName() )
					{
						m_runtimeBehaviorInstanceSlots.PushBack( param->GetSlots() );
					}
				}
			}
		}

		{
			m_runtimeBehaviorInstanceSlots.PushBack( m_behaviorInstanceSlots );

			m_behaviorGraphStack->Init( m_runtimeBehaviorInstanceSlots, constraintGraph, m_defaultBehaviorAnimationSlotNode );
		}
	}

	// Load snapshot
	if( m_behaviorGraphStackSnapshot )
	{
		// Restore behavior stack
		VERIFY( m_behaviorGraphStack->RestoreSnapshot( m_behaviorGraphStackSnapshot ) );

		// Reset snapshot
		m_behaviorGraphStackSnapshot = NULL;
	}
}

void CAnimatedComponent::ClearBehaviorGraphs()
{
	if ( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->Deactivate();
		m_behaviorGraphStack->ClearAllStack();
		m_behaviorGraphStack = NULL;
	}

	m_runtimeBehaviorInstanceSlots.Clear();
}

void CAnimatedComponent::CreateBehaviorContext()
{
	PC_SCOPE_PIX(AC_CreateBehaviorContext);
	// Create behavior graph sample context if needed
	if ( !m_behaviorGraphSampleContext )
	{
		m_behaviorGraphSampleContext = new SBehaviorSampleContext;
		m_behaviorGraphSampleContext->Init( this, GetSkeleton(), GetMimicSkeleton() );
	}
	else
	{
		//ASSERT( m_behaviorGraphSampleContext );
	}

	if ( !m_behaviorGraphUpdateContext )
	{
		m_behaviorGraphUpdateContext = new SBehaviorUpdateContext;
	}
	else
	{
		//ASSERT( m_behaviorGraphUpdateContext );
	}
}

void CAnimatedComponent::DestroyBehaviorContext()
{
	if ( m_behaviorGraphUpdateContext )
	{
		delete m_behaviorGraphUpdateContext;
		m_behaviorGraphUpdateContext = NULL;
	}

	if ( m_behaviorGraphSampleContext )
	{
		delete m_behaviorGraphSampleContext;
		m_behaviorGraphSampleContext = NULL;
	}
}

void CAnimatedComponent::ForceTPose( Bool scheduleUpdateTransformAfter )
{
	const CSkeleton* skeleton = GetSkeleton();

	// m_behaviorGraphSampleContext is null - TODO
	if ( skeleton && m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetSampledPose();
		pose.SetPose( m_skeleton.Get() );

		// Reset delta motion and events
#ifdef USE_HAVOK_ANIMATION
		pose.m_deltaReferenceFrameLocal.setIdentity();
#else	
		pose.m_deltaReferenceFrameLocal.SetIdentity();
#endif
		pose.ClearEventsAndUsedAnims();

		ProcessBehaviorOutputAll( 0.f, pose );

		if ( scheduleUpdateTransformAfter )
		{
			ScheduleUpdateTransformNode();
		}
	}
}

void CAnimatedComponent::ForceTPoseAndRefresh()
{
	const CSkeleton* skeleton = GetSkeleton();

	if ( skeleton && m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		SBehaviorGraphOutput& pose = m_behaviorGraphSampleContext->GetSampledPose();
		pose.SetPose( skeleton );

		// Reset delta motion and events
#ifdef USE_HAVOK_ANIMATION
		pose.m_deltaReferenceFrameLocal.setIdentity();
#else	
		pose.m_deltaReferenceFrameLocal.SetIdentity();
#endif
		pose.ClearEventsAndUsedAnims();

		ProcessBehaviorOutputAll( 0.f, pose );

		CalcTransforms();

		for ( ComponentIterator< CAnimDangleComponent > it( GetEntity() ); it; ++it )
		{
			(*it)->ForceReset();
		}

		UpdateAttachedAnimatedObjectsLS( 0.f, &pose );

		UpdateAttachedAnimatedObjectsLSConstrainted( 0.f, &pose );

		UpdateAttachedAnimatedObjectsWS();

		if ( m_ragdollPhysicsWrapper )
		{
			m_ragdollPhysicsWrapper->SyncToAnimation( m_skeletonModelSpace, GetLocalToWorld() );
		}

		ForceUpdateTransformNodeAndCommitChanges();
	}
}

void CAnimatedComponent::ForceBehaviorPose()
{
	ASSERT( m_behaviorGraphStack );
	ASSERT( m_behaviorGraphSampleContext );

	if ( m_behaviorGraphStack && m_behaviorGraphStack->IsActive() && m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		SBehaviorGraphOutput& output = m_behaviorGraphStack->Sample( m_behaviorGraphSampleContext );

		// Reset delta motion and events
#ifdef USE_HAVOK_ANIMATION
		output.m_deltaReferenceFrameLocal.setIdentity();
#else
		output.m_deltaReferenceFrameLocal.SetIdentity();
#endif
		output.ClearEventsAndUsedAnims();
		
		ProcessBehaviorOutputAll( 0.f, output );

		ForceUpdateTransformNodeAndCommitChanges();
		ForceUpdateBoundsNode();
	}
}

void CAnimatedComponent::ForceBehaviorPose( const SBehaviorGraphOutput& pose )
{
	if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() )
	{
		SBehaviorGraphOutput& output = m_behaviorGraphSampleContext->ForcePoseLocalSpace( pose );

		ProcessBehaviorOutputAll( 0.f, output );

		ForceUpdateTransformNodeAndCommitChanges();
		ForceUpdateBoundsNode();
	}
}

Bool CAnimatedComponent::SyncToPoseWS( const CAnimatedComponent* refComponent )
{
	if ( !m_behaviorGraphSampleContext || !m_behaviorGraphSampleContext->IsValid() || 
		!refComponent->m_behaviorGraphSampleContext || !refComponent->m_behaviorGraphSampleContext->IsValid() )
	{
		return false;
	}

	TDynArray< Int32 > mapping;
	Uint32 mappedBonesCount = 0;
	for ( BoneIterator thisIt( this ); thisIt; ++thisIt )
	{
		Int32 index = -1;
		const AnsiChar* myName = thisIt.GetName();
		for ( BoneIterator refIt( refComponent ); refIt; ++refIt )
		{
			if ( Red::System::StringCompare( myName, refIt.GetName() ) == 0 )
			{
				index = refIt.GetIndex();
				mappedBonesCount++;
				break;
			}
		}
		mapping.PushBack( index );
	}

	SBehaviorGraphOutput& thisPose = m_behaviorGraphSampleContext->GetSampledPose();
	const SBehaviorGraphOutput& refPose = refComponent->m_behaviorGraphSampleContext->GetSampledPose();

	RED_ASSERT( static_cast< Uint32 >( GetBonesNum() ) == mapping.Size() );
	RED_ASSERT( static_cast< Uint32 >( GetBonesNum() ) == thisPose.m_numBones );

	const Uint32 size = Min( static_cast< Uint32 >( GetBonesNum() ), thisPose.m_numBones );
	for ( Uint32 i = 0; i < size; ++i )
	{
		const Int32 index = mapping[ i ];
		if ( index != -1 )
		{
			RED_ASSERT( index < static_cast< Int32 >( refPose.m_numBones ) );
			thisPose.m_outputPose[ i ] = refPose.m_outputPose[ index ];
		}
	}

	// mark pose as "manually" set to suppress t-pose forcing
	m_poseSetManually = true;
	// and switch physics wrapper to kinematic state
	CPhysicsWrapperInterface* physicsWrapper = GetPhysicsRigidBodyWrapper();
	if ( physicsWrapper != nullptr )
	{
		physicsWrapper->SwitchToKinematic( true );
	}
	else
	{
		m_createRagdollAsKinematic = true;
	}

	if ( mapping.Size() > 0 && size > 0 && mappedBonesCount > 1 )
	{
		Uint32 firstMappedBoneIndex = 0;
		while ( firstMappedBoneIndex < mapping.Size() && mapping[ firstMappedBoneIndex ] == -1 )
		{
			firstMappedBoneIndex++;
		}
		if ( firstMappedBoneIndex == 0 ) // root bone mapped
		{
			const CEntity* refEntity = refComponent->GetEntity();
			CEntity* myEntity = GetEntity();
			myEntity->SetPosition( refEntity->GetWorldPositionRef() );
			myEntity->SetRotation( refEntity->GetWorldRotation() );
			return true;
		}
		else if ( firstMappedBoneIndex < mapping.Size() )
		{
			const Int32 firstBoneNotRoot = mapping[ firstMappedBoneIndex ];
			RED_ASSERT( firstBoneNotRoot != -1 );
			RED_ASSERT( firstBoneNotRoot < refComponent->m_skeletonWorldSpace.SizeInt() );
			if ( const CSkeleton* refSkeleton = refComponent->GetSkeleton() )
			{
				const Int32 firstBoneNotRootParent = refSkeleton->GetParentBoneIndex( firstBoneNotRoot );
				if ( firstBoneNotRootParent != -1 )
				{
					if ( firstBoneNotRootParent < refComponent->m_skeletonWorldSpace.SizeInt() )
					{
						const Matrix& refBoneWS = refComponent->m_skeletonWorldSpace[ firstBoneNotRootParent ];
						CEntity* myEntity = GetEntity();
						myEntity->SetPosition( refBoneWS.GetTranslation() );
						myEntity->SetRotation( refBoneWS.ToEulerAngles() );
						return true;
					}
				}
			}
		}
	}
	return false;
}

void CAnimatedComponent::ResetAnimationCache()
{
	if ( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->ResetAnimationCache();
	}
}

void CAnimatedComponent::ProcessBehaviorOutputAll( Float timeDelta, SBehaviorGraphOutput& output )
{
	// Pose
	ProcessBehaviorOutputPose( output );

	// Motion
	ProcessBehaviorOutputMotion( output );

	// Events
	ProcessBehaviorOutputEvents( output );

	// Constraints
	ProcessPoseConstraints( timeDelta, output );
}

void CAnimatedComponent::ProcessBehaviorOutputPose( SBehaviorGraphOutput& output )
{
	PC_SCOPE( ProcessBehaviorOutput );

	// Let the entity process the pose
	{
		PC_SCOPE( EntityPosePostProcess );
		GetEntity()->OnProcessBehaviorPose( this, output );
	}

	// Post process the pose in the component
	{
		PC_SCOPE( ComponentPosePostProcess );
		OnProcessBehaviorPose( output );
	}

	// store used anims data
	m_recentlyUsedAnims = output.m_usedAnims;
}

void CAnimatedComponent::ProcessBehaviorOutputMotion( SBehaviorGraphOutput& output )
{
	// Read and transform object translation
#ifdef USE_HAVOK_ANIMATION
	ASSERT( output.m_deltaReferenceFrameLocal.isOk() );
	HavokTransformToMatrix_Renormalize( output.m_deltaReferenceFrameLocal, &m_characterTransformDelta );

#else
	ASSERT( output.m_deltaReferenceFrameLocal.IsOk() );
	RedMatrix4x4 convertedMatrix = output.m_deltaReferenceFrameLocal.ConvertToMatrixNormalized();
	m_characterTransformDelta = reinterpret_cast< const Matrix& >( convertedMatrix );
#endif

	
	// Process extracted motion
	if ( m_useExtractedMotion && IsAttached() )
	{
		m_characterTransformDelta.SetTranslation( GetLocalToWorld().TransformVector( m_characterTransformDelta.GetTranslation() ) );
	}
	else
	{
		m_characterTransformDelta = Matrix::IDENTITY;
	}

	ASSERT( m_characterTransformDelta.IsOk() );

	// Reset
#ifdef USE_HAVOK_ANIMATION
	output.m_deltaReferenceFrameLocal.setIdentity();
#else
	output.m_deltaReferenceFrameLocal.SetIdentity();
#endif
}

void CAnimatedComponent::ProcessBehaviorOutputEvents( SBehaviorGraphOutput& output )
{
	PC_SCOPE( ProcessAnimationEvents );

	if ( output.m_numEventsFired > 0 )
	{
		CEntity *entity = Cast< CEntity >( GetParent() );

		TStaticArray< Uint32, 32 > poEvts;

		for ( Uint32 i=0; i<output.m_numEventsFired; ++i )
		{
			// yes, create copy, we may need to modify it before processing
			CAnimationEventFired currEvent = output.m_eventsFired[i];

			RED_FATAL_ASSERT( currEvent.m_extEvent, "Anim event ext event is nullptr" );

			ASSERT( currEvent.m_extEvent != NULL );

			if ( currEvent.m_type == AET_Duration &&
				 currEvent.m_extEvent )
			{
				Bool wasPresent = false;
				// maybe it was present in this call as start? don' bother with start in the middle then
				CAnimationEventFired* thisFrameFired = output.m_eventsFired;
				for ( Uint32 j=0; j<i; ++j, ++thisFrameFired )
				{
					if ( thisFrameFired->m_extEvent == currEvent.m_extEvent &&
						thisFrameFired->m_type == AET_DurationStart )
					{
						wasPresent = true;
						break;
					}
				}
				if ( ! wasPresent )
				{
					for ( TStaticArray< CAnimationEventFired, 32 >::const_iterator iLastFrameEvent = m_lastFrameEvents.Begin(); iLastFrameEvent != m_lastFrameEvents.End(); ++ iLastFrameEvent )
					{
						if ( iLastFrameEvent->m_extEvent == currEvent.m_extEvent )
						{
							wasPresent = true;
							break;
						}
					}
					if ( ! wasPresent )
					{
						currEvent.m_type = AET_DurationStartInTheMiddle;
					}
				}
			}

			// Special case for duration events
			if ( currEvent.m_type == AET_DurationStart || currEvent.m_type == AET_DurationStartInTheMiddle )
			{
				// Start event
				const CExtAnimDurationEvent* de = static_cast< const CExtAnimDurationEvent* >( currEvent.m_extEvent );
				de->Start( currEvent, this );

				ASSERT( m_trackedEvents.Size() < m_trackedEvents.Capacity() );

				if ( de->AlwaysFiresEnd() && m_trackedEvents.Size() < m_trackedEvents.Capacity() )
				{
					m_trackedEvents.PushBack( currEvent );
					poEvts.PushBack( i );
				}
			}
			else if ( currEvent.m_type == AET_DurationEnd )
			{
				// End event
				const CExtAnimDurationEvent* de = static_cast< const CExtAnimDurationEvent* >( currEvent.m_extEvent );
				de->Stop( currEvent, this );

				if ( de->AlwaysFiresEnd() )
				{
					const Int32 size = (Int32)m_trackedEvents.Size();
					for ( Int32 j=0; j<size; ++j )
					{
						if ( m_trackedEvents[ j ].m_extEvent == currEvent.m_extEvent )
						{
							m_trackedEvents.RemoveAtFast( j );
							break;
						}
					}
				}
			}
			else if ( currEvent.m_type == AET_Duration && static_cast< const CExtAnimDurationEvent* >( currEvent.m_extEvent )->AlwaysFiresEnd() )
			{
				poEvts.PushBack( i );
			}

			// Let the event process itself
			{
				PC_SCOPE( ExtEventProcess );
				currEvent.m_extEvent->Process( currEvent, this );
			}

			// Propagate to entity
			if ( entity )
			{
				PC_SCOPE( EntityEventProcess );

				entity->ProcessAnimationEvent( &currEvent );

				if ( currEvent.m_type == AET_DurationStartInTheMiddle )
				{
					// process it also as normal "duration" event as this was original one
					const EAnimationEventType prevType = currEvent.m_type;
					currEvent.m_type = AET_Duration;
					entity->ProcessAnimationEvent( &currEvent );
					currEvent.m_type = prevType;
				}
			}
		}

		// Tracked events
		{
			for ( Int32 i=(Int32)m_trackedEvents.Size()-1; i>=0; --i )
			{
				CAnimationEventFired& trackedEvt = m_trackedEvents[ i ];
				trackedEvt.m_alpha = 1.f;
				trackedEvt.m_type = AET_DurationEnd;

				Int32 evtFounded = -1;

				const Uint32 poEvtSize = poEvts.Size();
				for ( Uint32 j=0; j<poEvtSize; ++j )
				{
					const CAnimationEventFired& oEvt = output.m_eventsFired[ poEvts [ j ] ];
					if ( trackedEvt.m_extEvent == oEvt.m_extEvent )
					{
						evtFounded = (Int32)j;
					}
				}
				
				if ( evtFounded != -1 )
				{
					poEvts.RemoveAtFast( evtFounded );
				}
				else
				{
					const CExtAnimDurationEvent* de = static_cast< const CExtAnimDurationEvent* >( trackedEvt.m_extEvent );
					de->Stop( trackedEvt, this );

					{
						PC_SCOPE( ExtEventProcess );
						trackedEvt.m_extEvent->Process( trackedEvt, this );
					}

					if ( entity )
					{
						PC_SCOPE( EntityEventProcess );
						entity->ProcessAnimationEvent( &trackedEvt );
					}

					m_trackedEvents.RemoveAtFast( i );
				}
			}
		}

		// Store last frame events
		{
			m_lastFrameEvents.Clear();
			CAnimationEventFired* eventFired = output.m_eventsFired;
			for ( Uint32 i=0; i<output.m_numEventsFired; ++i, ++eventFired )
			{
				m_lastFrameEvents.PushBack( *eventFired );
			}
		}
	}
}

void CAnimatedComponent::ProcessPoseConstraints( Float timeDelta, SBehaviorGraphOutput& output )
{
	PC_SCOPE( ProcessPoseConstraints );

	if ( m_behaviorGraphStack && m_behaviorGraphStack->IsActive() && m_behaviorGraphStack->HasPoseConstraints() &&
		m_behaviorGraphSampleContext && m_behaviorGraphUpdateContext && 
		timeDelta > 0.f )
	{
		SBehaviorGraphOutput& pose = m_behaviorGraphStack->ApplyPoseConstraints( timeDelta, m_behaviorGraphUpdateContext, m_behaviorGraphSampleContext, output );
		PostProcessPoseConstraints( pose );
	}
	else
	{
		PostProcessPoseConstraints( output );
	}
}

namespace
{
	RED_INLINE Bool ShouldUpdateAnimation( ETickGroup tickGroup )
	{
		if ( tickGroup == TICK_PrePhysicsPost )
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	RED_INLINE Bool ShouldUpdatePoseConstraints( ETickGroup tickGroup )
	{
		if ( tickGroup == TICK_PostPhysicsPost )
		{
			return true;
		}
		else
		{
			return false;
		}
	}
};

void CAnimatedComponent::CollectImmediateJobs( STickManagerContext& context, CTaskBatch& taskBatch )
{
	if ( m_animationSuppressionMask != 0 )
	{
		// do not create jobs when we suppressed animations
		return;
	}

	if ( GGame->GetGameplayConfig().m_animationMultiUpdate )
	{
#ifndef RED_FINAL_BUILD
		if ( s_fullyProcessedCounter.GetValue() != 0 || s_skippedCounter.GetValue() != 0 )
		{
			s_fullyProcessedCounter.SetValue( 0 );
			s_skippedCounter.SetValue( 0 );
		}
#endif
		const ETickGroup tickGroup = context.m_group;

		// this looks silly, all this updateAnimations, but it is to make it easier to extend it (to add poses etc)
		const Bool updateAnimations = ShouldUpdateAnimation( tickGroup );
		const Bool updatePoseConstraints = ShouldUpdatePoseConstraints( tickGroup ) && m_allowConstraintsUpdate;

		// don't create jobs for components attached to animated components that are not attached through animated attachment - they will be handled in proper method
		if ( ( updateAnimations || updatePoseConstraints ) && ! ShouldBeUpdatedByAnimatedComponentParent() && ! m_updatedByAnimatedComponent )
		{
			CJobImmediateUpdateAnimationContext jobContext;
			jobContext.m_timeDelta = context.m_timeDelta * m_timeMultiplier;
			jobContext.m_updateAnimations = updateAnimations;
			jobContext.m_updatePoseConstraints = updatePoseConstraints;

			//FIXME: Make recyclable
			CJobImmediateUpdateAnimation* updateAnimationJob = new ( CTask::Root ) CJobImmediateUpdateAnimation( taskBatch.GetSyncToken(), this ); 
			updateAnimationJob->Setup( jobContext );
			
			// Fire and forget. Addref'd as part of batch.
			taskBatch.Add( *updateAnimationJob );
			updateAnimationJob->Release();

			context.m_shouldWait = true;
		}
	}
}

#ifndef NO_EDITOR_FRAGMENTS
// Generate editor rendering fragments
void CAnimatedComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if ( flags == SHOW_Behavior )
	{
		if ( m_behaviorGraphStack )
		{
			m_behaviorGraphStack->GenerateEditorFragments( frame );
		}

		if ( IsRagdolled( false ) )
		{
			Vector currPos = GetEntity()->GetWorldPosition();
			String txt = String::Printf( TXT("RAGDOLL") );
			frame->AddDebugText( currPos, txt, 0, -5, false, IsRagdolled( true )? Color::LIGHT_RED : Color::LIGHT_GREEN );
		}

		DisplaySkeleton( frame );
	}

	if ( m_wetnessSupplier != nullptr && flags == SHOW_Wetness )
	{
		Int32 bonesCount = m_skeletonWorldSpace.Size();
		for( Int32 i=0; i<bonesCount; ++i )
		{
			const Vector& bonePos = m_skeletonWorldSpace[i].GetTranslationRef();
			frame->AddDebugSphere( bonePos, 0.01f, Matrix::IDENTITY, Color( Uint8( m_wetnessSupplier->GetWetnessDataFromBone( i ) * 255.f ), 0, 0 ), true );
			frame->AddDebugText( bonePos, String::Printf( TXT("%.2f"), m_wetnessSupplier->GetWetnessDataFromBone( i ) ), 0, 0, false, Color::GREEN );
		}
	}

	if ( m_teleportDetector != nullptr && flags == SHOW_TeleportDetector )
	{
		m_teleportDetector->OnGenerateEditorFragments( frame, m_skeletonWorldSpace );
	}

#ifdef DEBUG_CAM_ASAP
	if ( flags == SHOW_CameraVisibility )
	{
		const Box componentBBoxWS = GetLocalToWorld().TransformBox( m_componentBBoxMS );
		if ( m_isNotInCameraView )
		{
			frame->AddDebugBox( componentBBoxWS, Matrix::IDENTITY, Color( 255, 255, 255 ), true, true );
		}
		else
		{
			frame->AddDebugBox( componentBBoxWS, Matrix::IDENTITY, Color( 0, 255, 0 ), true, true );
		}
	}
#endif
}
#endif //NO_EDITOR_FRAGMENTS

Bool CAnimatedComponent::IsDispSkeleton( EAnimatedComponentDebugDisp flag ) const
{
	return ( m_debugDisp & flag ) != 0;
}

void CAnimatedComponent::SetDispSkeleton( EAnimatedComponentDebugDisp flag, Bool enabled )
{
	if ( enabled )
	{
		m_debugDisp |= flag;	// Set
	}
	else
	{
		m_debugDisp &= ~flag;	// Clear
	}
}

void CAnimatedComponent::DisplaySkeleton( CRenderFrame *frame, Color color ) const
{
	TDynArray< ISkeletonDataProvider::BoneInfo > bones;
	Uint32 bonesNum = GetBones( bones );

	TDynArray< DebugVertex > skeletonPoints;

	if ( IsDispSkeleton( ACDD_SkeletonBone ) )
	{
		// Draw bones
		for( Uint32 i=0; i<bonesNum; i++ )
		{
			Int32 parentIndex = bones[i].m_parent;
			if ( parentIndex != -1 )
			{
				Matrix start = GetBoneMatrixWorldSpace( parentIndex );
				Matrix end = GetBoneMatrixWorldSpace( i );

				skeletonPoints.PushBack( DebugVertex( start.GetTranslation(), color ) );
				skeletonPoints.PushBack( DebugVertex( end.GetTranslation(), color ) );
			}
		}

		if (skeletonPoints.Size() > 0)
		{
			new (frame) CRenderFragmentDebugLineList( frame, 
				Matrix::IDENTITY,
				&skeletonPoints[0],
				skeletonPoints.Size(),
				RSG_DebugOverlay );
		}
	}

	if ( IsDispSkeleton( ACDD_SkeletonAxis ) )
	{
		for( Uint32 i=0; i<bonesNum; i++ )
		{
			Matrix boneMatrix = GetBoneMatrixWorldSpace( i );
			frame->AddDebugAxis( boneMatrix.GetTranslation(), boneMatrix, 0.1f, true );
		}
	}

	if ( IsDispSkeleton( ACDD_SkeletonName ) )
	{
		for( Uint32 i=0; i<bonesNum; i++ )
		{
			Matrix boneMatrix = GetBoneMatrixWorldSpace( i );
			frame->AddDebugText( boneMatrix.GetTranslation(), bones[i].m_name.AsString(), false, color );
		}
	}
}

void CAnimatedComponent::OnAppearanceChanged( Bool added )
{
	// Reattach
	if ( added && IsAttached() && GetParentAttachments().Empty() )
	{
		CAnimatedComponent* animComponent = GetEntity()->GetRootAnimatedComponent();
		if ( animComponent != nullptr )
		{
			animComponent->Attach( this, ClassID< CAnimatedAttachment >() );
		}

		PerformFullRecreation();
	}
}

void CAnimatedComponent::OnStreamIn()
{
	if ( IsUsedInAppearance() && IsAttached() && GetParentAttachments().Empty() )
	{
		CAnimatedComponent* animComponent = GetEntity()->GetRootAnimatedComponent();
		//RED_ASSERT( animComponent != nullptr, TXT("No root animated component in %s"), GetFriendlyName().AsChar() );
		RED_ASSERT( animComponent != this, TXT("The attachment of parent and child must be different" ) );

		if ( animComponent != nullptr )
		{
			if ( animComponent != this )
			{
				Bool isOk = true;
				{
					const CNode* _parent = animComponent;
					const CNode* _child = this;

					while ( const CHardAttachment* parentAtt = _parent->GetTransformParent() )
					{
						const CNode* parentParentNode = parentAtt->GetParent();

						RED_ASSERT( parentParentNode != _child, TXT("Attachments of parent and child has a cycle") );
						if ( parentParentNode == _child )
						{
							isOk = false;
							break;
						}

						_parent = parentParentNode;
					}
				}
				if ( isOk )
				{
					animComponent->Attach( this, ClassID< CAnimatedAttachment >() );
				}
			}

			PerformFullRecreation();
		}
	}
}

void CAnimatedComponent::OnSaveGameplayState( IGameSaver* saver )
{
	TBaseClass::OnSaveGameplayState( saver );

	if ( m_savable && m_behaviorGraphStack )
	{
		CGameSaverBlock block( saver, CNAME(behaviorStack) );

		CBehaviorGraphStackSnapshot* snapshot = m_behaviorGraphStack->CreateSnapshot( NULL );
		if( snapshot )
		{
			saver->SaveObject( snapshot );
		}
	}
}

Bool CAnimatedComponent::CheckShouldSave() const
{
	return ( m_savable && m_behaviorGraphStack );
}

void CAnimatedComponent::OnLoadGameplayState( IGameLoader* loader )
{
	TBaseClass::OnLoadGameplayState( loader );

	if ( m_savable )
	{
		ASSERT( !m_behaviorGraphStackSnapshot );

		CGameSaverBlock block( loader, CNAME(behaviorStack) );

		m_behaviorGraphStackSnapshot = loader->RestoreObject< CBehaviorGraphStackSnapshot >();
	}
}
								           

void CAnimatedComponent::OnParentAttachmentAdded( IAttachment* attachment )
{
	// Pass to the base class
	TBaseClass::OnParentAttachmentAdded( attachment );

	// update attached to anim parent
	UpdateAttachedToAnimParent();
}

void CAnimatedComponent::OnParentAttachmentBroken( IAttachment* attachment )
{
	// Pass to the base class
	TBaseClass::OnParentAttachmentBroken( attachment );

	// update attached to anim parent
	UpdateAttachedToAnimParent();
}

void CAnimatedComponent::OnChildAttachmentAdded( IAttachment* attachment )
{
	TBaseClass::OnChildAttachmentAdded( attachment );

	if ( attachment->IsA< CAnimatedAttachment >() )
	{
		AddChildAnimatedAttachment( static_cast< CAnimatedAttachment* >( attachment ) );
	}
	else if ( attachment->GetChild()->IsA< CAnimatedComponent >() )
	{
		m_cachedAnimatedChildComponents.PushBack( static_cast< CAnimatedComponent* >( attachment->GetChild() ) );
	}

	UpdateAttachedToAnimParent();
}

void CAnimatedComponent::OnChildAttachmentBroken( IAttachment* attachment )
{
	TBaseClass::OnChildAttachmentBroken( attachment );

	if ( attachment->IsA< CAnimatedAttachment >() )
	{
		RemoveChildAnimatedAttachment( static_cast< CAnimatedAttachment* >( attachment ) );
	}
	else if ( attachment->GetChild()->IsA< CAnimatedComponent >() )
	{
		m_cachedAnimatedChildComponents.Remove( static_cast< CAnimatedComponent* >( attachment->GetChild() ) );
	}

	UpdateAttachedToAnimParent();
}

Bool CAnimatedComponent::IsRagdolled( Bool realRagdoll ) const
{
	return m_ragdollPhysicsWrapper && ! m_ragdollPhysicsWrapper->IsKinematic();
}

Bool CAnimatedComponent::IsStatic( ) const
{
	return m_ragdollPhysicsWrapper && m_ragdollPhysicsWrapper->IsStatic();
}

CPhysicsWrapperInterface* CAnimatedComponent::GetPhysicsRigidBodyWrapper() const
{
	return m_ragdollPhysicsWrapper;
}

ECharacterPhysicsState CAnimatedComponent::GetCurrentPhysicsState() const
{
	return CPS_Animated;
}

Bool CAnimatedComponent::ShouldAddToTickGroups() const
{
	return !m_attachedToParentWithAnimatedAttachment && !m_updatedByAnimatedComponent;
}

void CAnimatedComponent::UpdateTickGroups( CWorld* world )
{
	if ( ! world )
	{
		CLayer* layer = GetLayer();
		if ( layer )
		{
			world = layer->GetWorld();
		}
	}
	if ( world )
	{
		if ( ShouldAddToTickGroups() )
		{
			AddToTickGroups( world );
		}
		else
		{
			RemoveFromTickGroups( world );
		}
	}
}

void CAnimatedComponent::AddToTickGroups( CWorld* world )
{
	if ( GetTickMask() )
	{
		return;
	}

	// Add to all tick groups
	world->GetTickManager()->AddToGroup( this, TICK_Main );
	world->GetTickManager()->AddToGroup( this, TICK_PrePhysics );
	world->GetTickManager()->AddToGroup( this, TICK_PrePhysicsPost );
	world->GetTickManager()->AddToGroup( this, TICK_PostPhysics );
	world->GetTickManager()->AddToGroup( this, TICK_PostPhysicsPost );
	world->GetTickManager()->AddToGroup( this, TICK_PostUpdateTransform );
}

void CAnimatedComponent::RemoveFromTickGroups( CWorld* world )
{
	if ( GetTickMask() )
	{
		// Remove from all tick groups
		world->GetTickManager()->Remove( this );
	}
}

void CAnimatedComponent::CacheTrajectoryBone()
{
	m_trajectoryBone = FindBoneByName( TXT("Trajectory") );
}

void CAnimatedComponent::OnCinematicStorySceneStarted()
{
	if ( m_teleportDetector )
	{
		m_teleportDetector->SetEnabled( CTeleportDetector::EM_SceneMode );
	}

	RED_ASSERT( !m_isInScene );
	m_isInScene = true;
}

void CAnimatedComponent::OnCinematicStorySceneEnded()
{
	RED_ASSERT( m_isInScene );
	m_isInScene = false;

	if ( m_teleportDetector )
	{
		m_teleportDetector->RequestSetDisabled( CTeleportDetector::EM_SceneMode );
	}

	m_componentBBoxValid.Reset();
}

void CAnimatedComponent::OnCutsceneStarted()
{
	RED_ASSERT( !m_isInCutscene );
	m_isInCutscene = true;

	if ( m_teleportDetector )
	{
		m_teleportDetector->SetEnabled( CTeleportDetector::EM_CutsceneMode );
	}
}

void CAnimatedComponent::OnCutsceneEnded()
{
	RED_ASSERT( m_isInCutscene );
	m_isInCutscene = false;

	if ( m_teleportDetector )
	{
		m_teleportDetector->RequestSetDisabled( CTeleportDetector::EM_CutsceneMode );
	}

	m_componentBBoxValid.Reset();
}

void CAnimatedComponent::OnCutsceneDebugCheck()
{
	//RED_ASSERT( m_isInCutscene );

	if ( IsACMotionEnabled() )
	{
		SetACMotionEnabled( false );
	}
	if ( UseExtractedMotion() )
	{
		SetUseExtractedMotion( false );
	}
}

void CAnimatedComponent::BaseOnSlotComponent( const CSlotComponent* slotComponent )
{
#ifndef NO_COMPONENT_GRAPH
	int graphPosX,graphPosY;
	slotComponent->GetGraphPosition( graphPosX, graphPosY );
	SetGraphPosition( graphPosX, graphPosY );
#endif

	m_skeleton = slotComponent->GetSourceSkeleton();
	OnPostLoad();
}

CEventNotifier< CAnimationEventFired >*	CAnimatedComponent::GetAnimationEventNotifier( const CName &eventName )
{
	TEventHandlersMap::iterator it = m_animationEventHandlers.Find( eventName );
	if ( it == m_animationEventHandlers.End() )
	{
		CEventNotifier< CAnimationEventFired > newNotifier;
		m_animationEventHandlers.Insert( eventName, newNotifier );
		it = m_animationEventHandlers.Find( eventName );
	}

	ASSERT( it != m_animationEventHandlers.End() );
	return it != m_animationEventHandlers.End() ? &(it->m_second) : NULL;
}

CEntity* CAnimatedComponent::GetAnimatedObjectParent() const
{
	return GetEntity();
}

Bool CAnimatedComponent::HasSkeleton() const
{
	return m_skeleton.IsValid();
}

Bool CAnimatedComponent::HasTrajectoryBone() const
{
	return m_trajectoryBone != -1;
}

Int32 CAnimatedComponent::GetTrajectoryBone() const
{
	return m_trajectoryBone;
}

Bool CAnimatedComponent::UseExtractedMotion() const
{
	return m_useExtractedMotion;
}

Bool CAnimatedComponent::UseExtractedTrajectory() const
{
	return m_extractTrajectory;
}

Int32 CAnimatedComponent::GetBonesNum() const
{
	return m_skeleton ? m_skeleton->GetBonesNum() : 0;	
}

Int32 CAnimatedComponent::GetTracksNum() const
{
	return m_skeleton ? m_skeleton->GetTracksNum() : 0;
}

Int32 CAnimatedComponent::GetParentBoneIndex( Int32 bone ) const
{
	return m_skeleton ? m_skeleton->GetParentBoneIndex( bone ) : -1;
}

const Int16* CAnimatedComponent::GetParentIndices() const
{
	return m_skeleton ? m_skeleton->GetParentIndices() : NULL;
}

void CAnimatedComponent::PlayEffectForAnimation( const CName& animation, Float time )
{
	GetEntity()->PlayEffectForAnimation( animation, time );
}

Float CAnimatedComponent::GetRadius() const
{
	return 0.f;
}

Bool CAnimatedComponent::IsHandlingTimeSwitch()
{
    return false;
}

#ifndef NO_EDITOR

void CAnimatedComponent::Setup( CAnimatedComponent::SetupData& data )
{
	if ( data.m_set )
	{
		AddAnimationSet( data.m_set );
	}

	if ( data.m_slot )
	{
		m_behaviorInstanceSlots.PushBack( *data.m_slot );
	}

	SetSkeleton( data.m_skeleton );

	if ( m_behaviorGraphSampleContext )
	{
		m_behaviorGraphSampleContext->Deinit();
		m_behaviorGraphSampleContext->Init( this, GetSkeleton(), GetMimicSkeleton() );
	}

	if ( m_behaviorGraphStack )
	{
		m_behaviorGraphStack->ClearAllStack();

		m_runtimeBehaviorInstanceSlots.Clear();

		m_runtimeBehaviorInstanceSlots.PushBack( m_behaviorInstanceSlots );

		m_behaviorGraphStack->Init( m_runtimeBehaviorInstanceSlots, NULL, m_defaultBehaviorAnimationSlotNode );
	}
}

void CAnimatedComponent::DestroyRagdoll()
{
	if( !m_ragdollPhysicsWrapper ) return;

	m_ragdollPhysicsWrapper->Release();
	m_ragdollPhysicsWrapper = 0;
}

void CAnimatedComponent::EditorOnTransformChangeStart()
{
	TBaseClass::EditorOnTransformChangeStart();
	if( !GGame->IsActive() )
	{
		if ( m_ragdollPhysicsWrapper )
		{
			m_ragdollPhysicsWrapper->SwitchToKinematic( true );
		}
	}
}

void CAnimatedComponent::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();

	ForceUpdateTransformNodeAndCommitChanges();
}

void CAnimatedComponent::EditorOnTransformChangeStop()
{
	TBaseClass::EditorOnTransformChangeStop();
	if( !GGame->IsActive() )
	{
		if ( m_ragdollPhysicsWrapper )
		{
			// we have base check between gameplay entity or entity
			Bool isPersistentEntity = GetParent()->IsA< CPeristentEntity >();
			m_ragdollPhysicsWrapper->SwitchToKinematic( isPersistentEntity );
		}
	}
}
#endif

void CAnimatedComponent::GetBoneMatrixMovementModelSpaceInAnimation( Uint32 boneIndex, CName const & animation, Float time, Float deltaTime, Matrix & refAtTime, Matrix & refWithDeltaTime ) const
{
	if ( CSkeletalAnimationSetEntry const * animSetEntry = m_animations->FindAnimation( animation ) )
	{
		if ( CSkeletalAnimation* animation = animSetEntry->GetAnimation() )
		{
			Uint32 bonesNum = animation->GetBonesNum();
			Uint32 tracksNum = animation->GetTracksNum();
			AnimQsTransform* bonesAtTime = (AnimQsTransform*) RED_ALLOCA( sizeof(AnimQsTransform) * bonesNum );
			AnimFloat* tracksAtTime = (AnimFloat*) RED_ALLOCA( sizeof(AnimFloat) * tracksNum );
			AnimQsTransform* bonesWithDeltaTime = (AnimQsTransform*) RED_ALLOCA( sizeof(AnimQsTransform) * bonesNum );
			AnimFloat* tracksWithDeltaTime = (AnimFloat*) RED_ALLOCA( sizeof(AnimFloat) * tracksNum );
			animation->Sample( time, bonesNum, tracksNum, bonesAtTime, tracksAtTime );
			animation->Sample( time + deltaTime, bonesNum, tracksNum, bonesWithDeltaTime, tracksWithDeltaTime );
			AnimQsTransform movement = animation->HasExtractedMotion()? animation->GetMovementBetweenTime( time, time + deltaTime, 0 ) : AnimQsTransform::IDENTITY;
			// TODO there should be no need to do this in near future and when future becomes present or past, remove this
			if ( UseExtractedTrajectory() && HasTrajectoryBone() )
			{
				SBehaviorGraphOutput::ExtractTrajectoryOn( this, bonesAtTime, bonesNum );
				SBehaviorGraphOutput::ExtractTrajectoryOn( this, bonesWithDeltaTime, bonesNum );
			}
			else if ( bonesNum > 0 )
			{
#ifdef USE_HAVOK_ANIMATION // VALID
				bonesAtTime[ 0 ].m_rotation.normalize();
				bonesWithDeltaTime[ 0 ].m_rotation.normalize();
#else
				bonesAtTime[ 0 ].Rotation.Normalize();
				bonesWithDeltaTime[ 0 ].Rotation.Normalize();
#endif
			}
			AnimQsTransform boneAtTimeMS = GetSkeleton()->GetBoneMS( boneIndex, bonesAtTime, bonesNum );
			AnimQsTransform boneWithDeltaTimeMS = GetSkeleton()->GetBoneMS( boneIndex, bonesWithDeltaTime, bonesNum );
			AnimQsTransform boneWithDeltaTimeRMS /* relative model space */;
			boneWithDeltaTimeRMS.SetMul( boneWithDeltaTimeMS, movement );
			{
				RedMatrix4x4 conversionMatrix = boneAtTimeMS.ConvertToMatrixNormalized();
				refAtTime = reinterpret_cast< const Matrix& >( conversionMatrix );
			}
			{
				RedMatrix4x4 conversionMatrix = boneWithDeltaTimeRMS.ConvertToMatrixNormalized();
				refWithDeltaTime = reinterpret_cast< const Matrix& >( conversionMatrix );
			}
			return;
		}
	}

	refAtTime = Matrix::IDENTITY;
	refWithDeltaTime = Matrix::IDENTITY;
}

void CAnimatedComponent::ClearAnimContainer() 
{ 
	m_animations->Clear(); 
}

void CAnimatedComponent::SuppressAnimation( Bool suppress, AnimationSuppressReason reason )
{
	const Uint8 prevMask = m_animationSuppressionMask;
	if ( suppress )
	{
		m_animationSuppressionMask |= reason;
	}
	else
	{
		m_animationSuppressionMask &= ~reason;
	}
}

void CAnimatedComponent::SetSkipUpdateAndSampleFrames( Uint32 skipUpdateAndSampleFrames )
{
	if ( m_rareTickSkip != skipUpdateAndSampleFrames )
	{
		m_rareTickSkip = skipUpdateAndSampleFrames;
		m_rareTickAccumulatedTimeDelta = 0.0f;
		m_rareTickForceFullProcessing = true; // force next full processing
	}
}

void CAnimatedComponent::SetSkipUpdateAndSampleFramesBias( Uint32 multiplier, Uint32 addition )
{
	if ( m_rareTickSkipMultiplier != multiplier ||
		 m_rareTickSkipAdd != addition )
	{
		m_rareTickSkipMultiplier = multiplier;
		m_rareTickSkipAdd = addition;
		m_rareTickAccumulatedTimeDelta = 0.0f;
		m_rareTickForceFullProcessing = true; // force next full processing
	}
}

void CAnimatedComponent::SetSkipUpdateAndSampleFramesLimit( Uint32 limit )
{
	if ( m_rareTickSkipLimit != limit )
	{
		m_rareTickSkipLimit = limit;
		m_rareTickAccumulatedTimeDelta = 0.0f;
		m_rareTickForceFullProcessing = true; // force next full processing
	}
}

void CAnimatedComponent::SetSkipUpdateAndSampleFramesLimitDueToAI( Uint32 limit )
{
	if ( m_rareTickSkipLimitDueToAI != limit )
	{
		m_rareTickSkipLimitDueToAI = limit;
		m_rareTickAccumulatedTimeDelta = 0.0f;
		m_rareTickForceFullProcessing = true; // force next full processing
	}
}

void CAnimatedComponent::CopyRateTickSettingsFrom( const CAnimatedComponent * source )
{
	if ( m_rareTickSkip != source->m_rareTickSkip ||
		 m_rareTickSkipMultiplier != source->m_rareTickSkipMultiplier ||
		 m_rareTickSkipAdd != source->m_rareTickSkipAdd ||
		 m_rareTickSkipLimit != source->m_rareTickSkipLimit ||
		 m_rareTickSkipLimitDueToAI != source->m_rareTickSkipLimitDueToAI )
	{
		m_rareTickSkip = source->m_rareTickSkip;;
		m_rareTickSkipMultiplier = source->m_rareTickSkipMultiplier;
		m_rareTickSkipAdd = source->m_rareTickSkipAdd;
		m_rareTickSkipLimit = source->m_rareTickSkipLimit;
		m_rareTickSkipLimitDueToAI = source->m_rareTickSkipLimitDueToAI;
		m_rareTickForceFullProcessing = true;
	}
}

void CAnimatedComponent::SetTeleportDetectorForceUpdateOneFrame()
{
	if( m_teleportDetector )
	{
		m_teleportDetector->SetForceUpdateOneFrame();
	}
}

Bool CAnimatedComponent::IsGameplayLODable() const
{
	if ( CBehaviorGraphStack* behaviorStack = GetBehaviorStack() )
	{
		return
			!behaviorStack->HasActiveInstance( CNAME( StoryScene ) ) &&
			!behaviorStack->HasActiveInstance( CNAME( Cutscene ) ) &&
			!behaviorStack->HasActiveInstance( CNAME( lever ) );
	}
	return true;
}

void CAnimatedComponent::UpdateLOD( ILODable::LOD newLOD, CLODableManager* manager )
{
	ASSERT( m_currentLOD != newLOD );

	switch ( m_currentLOD )
	{

	// Full tick
	case ILODable::LOD_0:
		if ( newLOD == ILODable::LOD_1 )
		{
			SetTickBudgeted( true, CComponent::BR_Lod );
		}
		else
		{
			SuppressTick( true, SR_Lod );
		}
		break;

	// Budgeted tick
	case ILODable::LOD_1:
		SetTickBudgeted( false, CComponent::BR_Lod );
		if ( newLOD == ILODable::LOD_2 )
		{
			SuppressTick( true, SR_Lod );
		}
		break;

	// Disabled/suppressed tick
	case ILODable::LOD_2:
		SuppressTick( false, SR_Lod );
		if ( newLOD == ILODable::LOD_1 )
		{
			SetTickBudgeted( true, CComponent::BR_Lod );
		}
		break;
	}

	m_currentLOD = newLOD;
}

ILODable::LOD CAnimatedComponent::ComputeLOD( CLODableManager* manager ) const
{
#ifdef USE_ANSEL
	if ( isAnselSessionActive || isAnselTurningOn || isAnselTurningOff )
	{
		return ILODable::LOD_0;
	}
#endif // USE_ANSEL

	if ( !GetEntity()->IsGameplayLODable() )
	{
		return ILODable::LOD_0;
	}

	const Float distSqr = manager->GetPosition().DistanceSquaredTo2D( GetWorldPositionRef() );

	// Shouldn't it be budgeted?

	const Float budgetableDistance = m_overrideBudgetedTickDistance == 0.0f ? manager->GetBudgetableDistanceSqr() : Red::Math::MSqr( m_overrideBudgetedTickDistance );
	if ( distSqr < budgetableDistance )
	{
		return ILODable::LOD_0;	// Don't budget
	}

	// Shouldn't it be disabled?

	const Float disableDistance = m_overrideDisableTickDistance == 0.0f ? manager->GetDisableDistanceSqr() : Red::Math::MSqr( m_overrideDisableTickDistance );
	if ( distSqr < disableDistance )
	{
		return ILODable::LOD_1;	// Budget
	}

	return ILODable::LOD_2;	// Disable
}

void CAnimatedComponent::SetResource( CResource* resource )
{
	if ( CRagdoll* rag = Cast<CRagdoll>(resource) )
	{
		SetRagdoll( rag );
	}
	else if ( CSkeleton* skel = Cast<CSkeleton>(resource) )
	{
		SetSkeleton( skel );
	}
	else if( resource != nullptr )
	{
		RED_HALT( "Cannot set '%ls' to '%ls' component.", resource->GetFile()->GetFileName().AsChar(), m_name.AsChar() );
	}
}

void CAnimatedComponent::GetResource( TDynArray< const CResource* >& resources ) const
{
	if ( m_ragdoll )
	{
		resources.PushBack( m_ragdoll );
	}

	if ( m_skeleton )
	{
		resources.PushBack( m_skeleton );
	}
}

#ifdef DEBUG_AC
#pragma optimize("",on)
#endif
