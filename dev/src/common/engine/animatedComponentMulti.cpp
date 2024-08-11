
#include "build.h"
#include "animatedComponent.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphContext.h"
#include "behaviorGraphStack.h"
#include "../physics/PhysicsRagdollWrapper.h"
#include "../core/taskManager.h"
#include "skeletalAnimationContainer.h"
#include "game.h"
#include "materialInstance.h"
#include "animationJobs.h"
#include "animatedSkeleton.h"
#include "entity.h"
#include "camera.h"

#ifdef DEBUG_AC
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////
// Async

Bool CAnimatedComponent::CanUseAsyncUpdateMode() const
{
	return GGame->GetGameplayConfig().m_animationAsyncUpdate && !GetParent()->IsA< CCamera >();
}

void CAnimatedComponent::StartAsyncUpdateAndSample( Float timeDelta )
{
	if ( m_updateAnimationSyncJob )
	{
		ASSERT( !m_updateAnimationSyncJob );
		return;
	}

	CJobImmediateUpdateAnimationContext context;
	context.m_timeDelta = timeDelta;
	context.m_updateAnimations = true;

	m_updateAnimationSyncJob = new ( CTask::Root ) CJobAsyncUpdateAnimation( this, context );
	GTaskManager->Issue( *m_updateAnimationSyncJob, TSP_VeryHigh );
}

void CAnimatedComponent::FinishAsyncUpdateAndSample()
{
	if ( m_updateAnimationSyncJob )
	{
		if ( !m_updateAnimationSyncJob->IsFinished() )
		{
			while( !m_updateAnimationSyncJob->IsFinished() )
			{
				WARN_ENGINE( TXT("UpdateAnimationSyncJob is not ready in pre tick") );
				Red::Threads::SleepOnCurrentThread( 10 );
			}
		}

		VERIFY( !m_updateAnimationSyncJob->IsCancelled() );

		m_updateAnimationSyncJob->Release();
		m_updateAnimationSyncJob = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
// InternalUpdateAndSample
void CAnimatedComponent::InternalUpdateAndSample( Float timeDelta )
{
	Uint32 numBones = 0;

	if (m_asyncPlayedAnimName != CName::NONE && m_animatedSkeleton != NULL)
	{
		if (m_asyncPlayedAnimName == CNAME(StopAllAnimationsOnSkeleton))
		{
			m_animatedSkeleton->StopAllAnimation();

			if ( m_behaviorGraphStack && !m_behaviorGraphStack->IsActive() )
			{
				m_behaviorGraphStack->Activate();
			}
			m_asyncPlayedAnimName = CName::NONE;
		}
	}

	if (m_asyncPlayedAnimName != CName::NONE && m_animatedSkeleton != NULL)
	{
		CPlayedSkeletalAnimation* anim = m_animatedSkeleton->GetPlayedAnimation( m_asyncPlayedAnimName );
		if ( !anim )
		{
			CSkeletalAnimationSetEntry* animEntry = GetAnimationContainer()->FindAnimationRestricted( m_asyncPlayedAnimName );
			anim = m_animatedSkeleton->PlayAnimation( animEntry );
		}

		if ( anim )
		{
			anim->SynchronizeTo( m_asyncPlayedAnimSyncInfo );
			anim->Pause();
			if ( m_behaviorGraphStack && m_behaviorGraphStack->IsActive() )
			{
				m_behaviorGraphStack->Deactivate();
			}
		}

		m_asyncPlayedAnimName = CName::NONE;
	}

	SBehaviorGraphOutput* pose = NULL;

	// Update behavior graph
	if ( CanUseBehavior() )
	{
		PC_SCOPE( ACUpdateAndSampleAnimAll );

		

		{
			PC_SCOPE( ACUpdateAndSampleAnim );

			// Setup contexts
			SetupBehaviorContexts( m_behaviorGraphSampleContext, m_behaviorGraphUpdateContext );

			// Update using graph
			m_behaviorGraphStack->Update( m_behaviorGraphUpdateContext, timeDelta );

			// Sample last behavior instance
			SBehaviorGraphOutput& output = m_behaviorGraphStack->Sample( m_behaviorGraphSampleContext );
			pose = &output;
			numBones = output.m_numBones;

			ProcessBehaviorOutputPose( output );
			ProcessBehaviorOutputMotion( output );
			if ( m_allowPoseActions )
			{
				ProcessPoseActionsFromBehaviorContexts( output, m_behaviorGraphSampleContext, m_behaviorGraphUpdateContext );
			}
			else
			{
				ASSERT( m_behaviorGraphSampleContext->GetPostActions().Empty(), TXT("It should have no post actions!") );
			}
			if ( m_allowScriptNotifications )
			{
				ProcessScriptNotificationsFromStack();
			}
			else
			{
#ifndef RED_FINAL_BUILD
				ASSERT( ! m_behaviorGraphStack->HasAnyScriptNotifications(), TXT("It should have no post actions!") );
#endif
			}
		}
		
		ProcessBehaviorOutputEvents( *pose );
	}
	else if ( CanUseAnimatedSkeleton() )
	{
		PC_SCOPE( ACUpdateAndSampleAnimAll );

		{
			PC_SCOPE( ACUpdateAndSampleAnim );

			// Setup contexts
			SetupBehaviorContexts( m_behaviorGraphSampleContext );

			// Update animated skeleton			
			m_animatedSkeleton->Update( this, timeDelta );

			// Sample animated skeleton
			SBehaviorGraphOutput& output = m_animatedSkeleton->Sample( this, m_behaviorGraphSampleContext );
			pose = &output;
			numBones = output.m_numBones;

			ProcessBehaviorOutputPose( output );
			ProcessBehaviorOutputMotion( output );
			ProcessPoseActionsFromBehaviorContexts( output, m_behaviorGraphSampleContext, m_behaviorGraphUpdateContext );
			ProcessScriptNotificationsFromStack();
		}

		ProcessBehaviorOutputEvents( *pose );
	}

#ifdef USE_HAVOK
	if ( m_ragdollInstance )
	{
		PC_SCOPE( ACUpdateAndSampleSkeletonRagdoll );
		m_ragdollInstance->Tick( timeDelta, outputPoses, numBones );
	}
#endif

	{
		PC_SCOPE( ACUpdateAttachedComp );
		
		if ( pose )
		{
			UpdateAttachedAnimatedObjectsLS( timeDelta, pose );
		}
		else if ( m_behaviorGraphSampleContext )
		{
			UpdateAttachedAnimatedObjectsLS( timeDelta, m_behaviorGraphSampleContext->IsValid() ? &(m_behaviorGraphSampleContext->GetMainPose()) : NULL );
		}
	}
}

void CAnimatedComponent::InternalUpdateAndSampleMultiAsyncPart( Float timeDelta )
{
	Float orgTimeDelta = m_timeMultiplier != 0.0f? timeDelta / m_timeMultiplier : timeDelta;

	PC_SCOPE( ACUpdateAndSampleBehaviorMAP );

	if (m_asyncPlayedAnimName != CName::NONE && m_animatedSkeleton != NULL)
	{
		if (m_asyncPlayedAnimName == CNAME(StopAllAnimationsOnSkeleton))
		{
			m_animatedSkeleton->StopAllAnimation();

			if ( m_behaviorGraphStack && !m_behaviorGraphStack->IsActive() )
			{
				m_behaviorGraphStack->Activate();
			}
			m_asyncPlayedAnimName = CName::NONE;
		}
	}

	if (m_asyncPlayedAnimName != CName::NONE && m_animatedSkeleton != NULL)
	{
		CPlayedSkeletalAnimation* anim = m_animatedSkeleton->GetPlayedAnimation( m_asyncPlayedAnimName );
		if ( !anim )
		{
			// restricted, otherwise it will get DebugAnimation
			if ( CSkeletalAnimationSetEntry* animEntry = GetAnimationContainer()->FindAnimationRestricted( m_asyncPlayedAnimName ) )
			{
				anim = m_animatedSkeleton->PlayAnimation( animEntry );
			}
		}

		if ( anim )
		{
			anim->SynchronizeTo( m_asyncPlayedAnimSyncInfo );
			anim->Pause();
			if ( m_behaviorGraphStack && m_behaviorGraphStack->IsActive() )
			{
				m_behaviorGraphStack->Deactivate();
			}
		}

		m_asyncPlayedAnimName = CName::NONE;
	}

	SBehaviorGraphOutput* pose = NULL;

	// Copy rare tick setting to child components
	for ( auto iChild = m_cachedAnimatedChildComponents.Begin(), iEnd = m_cachedAnimatedChildComponents.End(); iChild != iEnd; ++ iChild )
	{
		CAnimatedComponent* child = *iChild;
		if ( child->ShouldBeUpdatedByAnimatedComponentParent() )
		{
			child->CopyRateTickSettingsFrom( this );
		}
	}

	for ( auto iAC = m_updateAnimatedComponents.Begin(), iEnd = m_updateAnimatedComponents.End(); iAC != iEnd; ++ iAC )
	{
		CAnimatedComponent* ac = *iAC;
		if ( ac->m_updatedByAnimatedComponent )
		{
			ac->CopyRateTickSettingsFrom( this );
		}
	}

	// Update behavior graph
	if ( CanUseBehavior() )
	{
		// Setup contexts
		SetupBehaviorContexts( m_behaviorGraphSampleContext, m_behaviorGraphUpdateContext );

		Uint32 frameSkip = Min( m_rareTickSkipLimitDueToAI, Min( m_rareTickSkipLimit, m_rareTickSkip * m_rareTickSkipMultiplier + m_rareTickSkipAdd ) );

		Bool doFullProcessing = m_rareTickFramesSkipped >= frameSkip || m_rareTickInvalidatePrev || m_rareTickForceFullProcessing;
		
		EBehaviorLod willDoForLOD = m_behaviorGraphSampleContext->GetLodLevel();
		if ( m_rareTickDoneForLOD != willDoForLOD )
		{
			m_rareTickDoneForLOD = willDoForLOD;
			doFullProcessing = true;
			m_rareTickInvalidatePrev = true;
		}

		m_rareTickAccumulatedTimeDelta += timeDelta;
		if ( doFullProcessing )
		{
			// Update using graph
			m_behaviorGraphStack->Update( m_behaviorGraphUpdateContext, m_rareTickAccumulatedTimeDelta );

			// Sample last behavior instance
			SBehaviorGraphOutput& output = m_behaviorGraphStack->Sample( m_behaviorGraphSampleContext );

			pose = &output;

			m_behaviorGraphSampleContext->CachePoseForLastFullUpdateAndSample();
			if ( m_rareTickInvalidatePrev )
			{
				m_behaviorGraphSampleContext->ForcePrevToBeLastFullUpdateAndSample();
			}
			m_rareTickPrevToLastTimeDelta = m_rareTickAccumulatedTimeDelta;
			m_rareTickAccumulatedTimeDelta = 0.0f;
			m_rareTickFramesSkipped = 0;
			if ( m_rareTickForceFullProcessing && frameSkip > 0 )
			{
				// when forced full processing, start at different frames skipped to desynchronize situation in which we have 100 characters skipping every second frame but it is same frame
				static Uint32 frameSkipDesync = 0;
				++ frameSkipDesync;
				m_rareTickFramesSkipped = frameSkipDesync % ( frameSkip + 1 );
			}
			m_rareTickInvalidatePrev = false;
			m_rareTickForceFullProcessing = false;
#ifndef RED_FINAL_BUILD
			s_fullyProcessedCounter.Increment();
#endif
		}
		else
		{
			pose = &m_behaviorGraphSampleContext->RestorePoseFromLastFullUpdateAndSample();
			pose->AdvanceEvents();
			++ m_rareTickFramesSkipped;
#ifndef RED_FINAL_BUILD
			s_skippedCounter.Increment();
#endif
		}

		if ( frameSkip > 0 )
		{
			Float prevToLast = Clamp( ((Float)m_rareTickFramesSkipped) / ((Float)(frameSkip + 1)), 0.0f, 1.0f );
			pose->SetInterpolateWithoutME( m_behaviorGraphSampleContext->GetPoseFromPrevFullUpdateAndSample(), m_behaviorGraphSampleContext->GetPoseFromLastFullUpdateAndSample(), prevToLast );
		}

		ProcessBehaviorOutputPose( *pose );
		ProcessBehaviorOutputMotion( *pose );

		Float useMotion = m_rareTickPrevToLastTimeDelta != 0.0f? Clamp( timeDelta / m_rareTickPrevToLastTimeDelta, 0.0f, 1.0f ) : 1.0f;
		if ( useMotion != 1.0f )
		{
			Vector translation = m_characterTransformDelta.GetTranslation() * useMotion;
			EulerAngles rotation = m_characterTransformDelta.ToEulerAngles() * useMotion;
			rotation.ToMatrix( m_characterTransformDelta );
			m_characterTransformDelta.SetTranslation( translation );
		}
	}
	else if ( CanUseAnimatedSkeleton() )
	{
		// Setup contexts
		SetupBehaviorContexts( m_behaviorGraphSampleContext );

		// Update animated skeleton			
		m_animatedSkeleton->Update( this, timeDelta );

		// Sample animated skeleton
		SBehaviorGraphOutput& output = m_animatedSkeleton->Sample( this, m_behaviorGraphSampleContext );

		ProcessBehaviorOutputPose( output );
		ProcessBehaviorOutputMotion( output );

		pose = &output;
	}
	else if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() && !m_poseSetManually )
	{
		m_behaviorGraphSampleContext->ForceTPose();
	}

	if ( pose )
	{
		UpdateAttachedAnimatedObjectsLSAsync( timeDelta, pose );
	}
	else if ( m_behaviorGraphSampleContext )
	{
		UpdateAttachedAnimatedObjectsLSAsync( timeDelta, m_behaviorGraphSampleContext->IsValid() ? &(m_behaviorGraphSampleContext->GetMainPose()) : NULL );
	}

	// Update children, so whole update is in proper order - this is reason why CollectImmediateJobs doesn't create jobs for animated components attached to 
	for ( auto iChild = m_cachedAnimatedChildComponents.Begin(), iEnd = m_cachedAnimatedChildComponents.End(); iChild != iEnd; ++ iChild )
	{
		CAnimatedComponent* child = *iChild;
		if ( child->ShouldBeUpdatedByAnimatedComponentParent() )
		{
			child->InternalUpdateAndSampleMultiAsyncPart( timeDelta );
		}
	}

	for ( auto iAC = m_updateAnimatedComponents.Begin(), iEnd = m_updateAnimatedComponents.End(); iAC != iEnd; ++ iAC )
	{
		CAnimatedComponent* ac = *iAC;
		if ( ac->m_updatedByAnimatedComponent )
		{
			ac->InternalUpdateAndSampleMultiAsyncPart( orgTimeDelta * ac->m_timeMultiplier );
		}
	}
}

void CAnimatedComponent::InternalUpdateAndSampleMultiSyncPart( Float timeDelta )
{
	Uint32 numBones = 0;

	SBehaviorGraphOutput* pose = NULL;

	if ( CanUseAnimatedSkeleton() || CanUseBehavior() )
	{
		PC_SCOPE( ACUpdateAndSampleBehaviorMSPAll );

		{
			PC_SCOPE( ACUpdateAndSampleBehaviorMSP );

			SBehaviorGraphOutput& output = m_behaviorGraphSampleContext->GetSampledPose();
			pose = &output;
			numBones = output.m_numBones;

			ProcessPoseActionsFromBehaviorContexts( output, m_behaviorGraphSampleContext, m_behaviorGraphUpdateContext );
		}

		{
			PC_SCOPE( ACProcessScriptNotifications );
			ProcessScriptNotificationsFromStack();
		}
		
		ProcessBehaviorOutputEvents( *pose );
	}

#ifdef USE_HAVOK
	if ( m_ragdollInstance )
	{
		PC_SCOPE( ACUpdateAndSampleSkeletonRagdoll2 );

		m_ragdollInstance->Tick( timeDelta, outputPoses, numBones );
	}
#endif

	{
		PC_SCOPE( ACUpdateAttachedComp2 );
		
		if ( pose )
		{
			UpdateAttachedAnimatedObjectsLSSync( timeDelta, pose );
		}
		else if ( m_behaviorGraphSampleContext )
		{
			UpdateAttachedAnimatedObjectsLSSync( timeDelta, m_behaviorGraphSampleContext->IsValid() ? &(m_behaviorGraphSampleContext->GetMainPose()) : NULL );
		}
	}
}

Bool CAnimatedComponent::ExtractMotion( Float timeDelta, Vector& outDeltaPosition, EulerAngles& outDeltaRotation )
{
	if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() && 
		( ( m_behaviorGraphStack && m_behaviorGraphStack->IsActive() ) 
		|| m_animatedSkeleton ) )
	{
		// Use motion extracted from animation to move this component
		outDeltaPosition = m_characterTransformDelta.GetTranslation();
		outDeltaPosition.W = 0.f; // vectors should have W == 0 :)
		outDeltaRotation = m_characterTransformDelta.ToEulerAngles();
		return true;
	}

	return false;
}
//////////////////////////////////////////////////////////////////////////
// Part of finalize movement code
Bool CAnimatedComponent::ProcessMovement( Float timeDelta )
{
	if ( m_useExtractedMotion )
	{
		Vector deltaPos;
		EulerAngles deltaRot;
		if ( ExtractMotion( timeDelta, deltaPos, deltaRot ) )
		{
			PC_SCOPE( ApplyDeltaMovement );

			if ( Vector::Equal3( deltaPos, Vector::ZERO_3D_POINT ) && deltaRot.AlmostEquals( EulerAngles::ZEROS ) )
			{
				return false;
			}
		
			// Move with the whole amount of delta
			CEntity* thisEntity = GetEntity();
			EulerAngles rot = deltaRot;
			rot.Normalize();
			rot = thisEntity->GetRotation() +  rot;
			rot.Normalize();

			// Update entity position, raw way
			const Vector newPosition = thisEntity->GetPosition() + deltaPos;
			thisEntity->SetRawPlacement( &newPosition, &rot, NULL );

			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
// FinalizeMovement
Bool CAnimatedComponent::FinalizeMovement1_PreSeparation( Float timeDelta )
{
	if ( m_movementFinalized )
		return false;

    ASSERT( m_behaviorGraphSampleContext );

    // can't move frozen animated component
    if ( IsTickSuppressed() )
        return false;

	if ( ProcessMovement( timeDelta ) )
	{
		UpdateMovement1_PreSeperation( timeDelta );
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

Bool CAnimatedComponent::UpdateMovement2_PostSeperation( Float timeDelta )
{
	if ( m_useExtractedMotion && !Matrix::Equal( m_characterTransformDelta, Matrix::IDENTITY ) )
	{
		return true;
	}

	return false;
}

void CAnimatedComponent::FinalizeMovement2_PostSeparation( Float timeDelta )
{
	if ( m_movementFinalized )
		return;

    ASSERT( m_behaviorGraphSampleContext );

    m_movementFinalized = true;

    if ( IsTickSuppressed() )
        return;

    // Extract position from behavior graph
    if ( m_behaviorGraphSampleContext && m_behaviorGraphSampleContext->IsValid() && 
        ( ( m_behaviorGraphStack && m_behaviorGraphStack->IsActive() ) 
        || m_animatedSkeleton ) )
    {
        const Bool hasMoved = UpdateMovement2_PostSeperation( timeDelta );
		if ( hasMoved )
		{
			PC_SCOPE( FinalizeMovement2_PostSeparation_UpdateLocalToWorld );

			// we are going to schedule update transform, but we may still need localToWorld before full update transform kicks in
			// why? for example constraints that require localToWorld when calculating bones
			// consider game world's finalize note
			// note: now this is done only for characters that have actually moved
			GetEntity()->HACK_UpdateLocalToWorld();
			HACK_UpdateLocalToWorld();
			//SetThisFrameTempLocalToWorld( GetLocalToWorld() );

			GetEntity()->ScheduleUpdateTransformNode();
		}
		else
		{
			// Update attached objects
			ScheduleUpdateTransformNode();
		}

		//GetEntity()->ScheduleUpdateTransformNode();
	}
}

//////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_AC
#pragma optimize("",on)
#endif
