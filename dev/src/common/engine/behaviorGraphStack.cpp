/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphStack.h"
#include "behaviorGraphNode.h"
#include "behaviorGraphContext.h"
#include "animatedComponent.h"
#include "../engine/animatedComponentScripts.h"
#include "../engine/entity.h"
#include "../core/feedback.h"
#include "component.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

RED_DEFINE_STATIC_NAME( OnBehaviorGraphNotification );

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStack );

class ISlotAnimationListener;

CBehaviorGraphStack::CBehaviorGraphStack()
	: m_active( false )
	, m_lock( 0 )
	, m_constraintInstance( NULL )
	, m_internalState( SIS_Waiting )
{

}

void CBehaviorGraphStack::Init( const TDynArray< SBehaviorGraphInstanceSlot >& instanceSlots, const CBehaviorGraph* constraintGraph, CName defaultBahaviorGraphAnimationSlotNode )
{
	PC_SCOPE_PIX( CBehaviorGraphStack_Init );
	ASSERT( m_internalState == SIS_Waiting );

	m_slots = &instanceSlots;

	CAnimatedComponent* ac = GetAnimatedComponent();

	ASSERT( m_instances.Size() == 0 );

	// Create default instance
	if ( ( defaultBahaviorGraphAnimationSlotNode == CName::NONE || !ActivateBehaviorInstances( defaultBahaviorGraphAnimationSlotNode ) ) && !m_slots->Empty() )
	{
		ActivateBehaviorInstances( (*m_slots)[0].m_instanceName );
	}

	if ( constraintGraph )
	{
		m_constraintInstance = constraintGraph->CreateInstance( ac, CNAME( Constraint ) );
		if ( m_constraintInstance )
		{
			m_constraintInstance->Activate();
		}
	}
}

void CBehaviorGraphStack::Reset()
{
	ASSERT( m_internalState == SIS_Waiting );

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		CBehaviorGraphInstance* instance = m_instances[ i ];
		instance->Reset();
	}

	if ( m_constraintInstance )
	{
		m_constraintInstance->Reset();
	}
}

void CBehaviorGraphStack::OnFinalize()
{
	/*if ( m_instances.Size() > 0 )
	{
		String str = TXT("ERROR Behavior stack is not empty - ");

		for ( Uint32 i=0; i<m_instances.Size(); i++ )
		{
			const CName& instName = m_instances[i]->GetInstanceName();
			str += String::Printf( TXT(", %d '%ls'"), i, instName.AsChar() );
		}

		ERR_ENGINE( TXT("%s"), str.AsChar() );

		ASSERT( m_instances.Size() == 0 );
	}*/

	ClearAllStack();

	TBaseClass::OnFinalize();
}

void CBehaviorGraphStack::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	// Links to runtime objects
	if ( file.IsGarbageCollector() )
	{
		file << m_instances;
		file << m_constraintInstance;
	}
}

void CBehaviorGraphStack::GenerateEditorFragments( CRenderFrame* frame )
{
	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		m_instances[i]->GenerateEditorFragments( frame );
	}

	if ( m_constraintInstance )
	{
		m_constraintInstance->GenerateEditorFragments( frame );
	}
}

Bool CBehaviorGraphStack::IsActive() const
{
	return m_active;
}

void CBehaviorGraphStack::Activate()
{
	ASSERT( m_internalState == SIS_Waiting );
	m_active = true;
}

void CBehaviorGraphStack::Deactivate()
{
	ASSERT( m_internalState == SIS_Waiting );
	m_active = false;
}

void CBehaviorGraphStack::Update( SBehaviorUpdateContext* context, Float timeDelta )
{
	PC_SCOPE( BehaviorGraphStackUpdate );

#ifndef RED_FINAL_BUILD
	CTimeCounter timer;
#endif

	ASSERT( context );
	ASSERT( m_active );
	ASSERT( m_internalState == SIS_Waiting );

	if ( m_internalState != SIS_Waiting )
	{
		Int32 i = 0;
		while ( m_internalState != SIS_Waiting && i < 1000 )
		{
			// Temp for wb demo
			// Crash fix
			Red::Threads::SleepOnCurrentThread( 1 );
			i++;
		}

		if ( m_internalState != SIS_Waiting )
		{
			HALT( "Critical error in animation system" );
		}
	}

	if ( m_freezer.IsBusy() )
	{
		m_freezer.Update( timeDelta, this );
	}

	if ( HasFrozenPose() )
	{
		return;
	}

	m_internalState = SIS_Updating;

	context->PrepareForUpdate();

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		CBehaviorGraphInstance* inst = m_instances[ i ];
		if ( inst->IsActive() )
		{
			inst->Update( *context, timeDelta );
		}
	}

	m_internalState = SIS_Waiting;

#ifndef RED_FINAL_BUILD
	const Float timeElapsed = timer.GetTimePeriod();
	if ( timeElapsed > 0.002f )
	{
		GScreenLog->PerfWarning( timeElapsed, TXT("ANIMATION"), TXT("Slow behavior update for agent '%ls'"), GetAnimatedComponent()->GetEntity()->GetFriendlyName().AsChar() );
	}
#endif
}

SBehaviorGraphOutput& CBehaviorGraphStack::Sample( SBehaviorSampleContext* context )
{
	PC_SCOPE( BehaviorGraphStackSample );

#ifndef RED_FINAL_BUILD
	CTimeCounter timer;
#endif

	ASSERT( context );
	ASSERT( m_active );
	ASSERT( m_internalState == SIS_Waiting );

	m_internalState = SIS_Sampling;

	// Prepare for sample and get main pose
	SBehaviorGraphOutput& pose = context->PrepareForSample( !HasFrozenPose() );

	DEBUG_ANIM_POSES( pose )

	if ( m_instances.Size() == 0 || HasFrozenPose() )
	{
		m_internalState = SIS_Waiting;

		// Returns last pose
		return pose;
	}

	// Sample all instances in stack
	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		CBehaviorGraphInstance* inst = m_instances[ i ];
		if ( inst->IsActive() )
		{
			context->CachePoseFromPrevSampling();
			inst->Sample( *context, pose );

#ifdef TPOSE_DETECTOR
			if ( GetAnimatedComponent()->GetEntity()->GetRootAnimatedComponent()  == GetAnimatedComponent() && GetAnimatedComponent()->GetEntity()->QueryActorInterface() )
			{
				ASSERT( !pose.IsTPose() );
			}
#endif
		}
	}

	// Remember last pose
	//context->CachePoseFromPrevSampling();

	DEBUG_ANIM_POSES( pose )

	// Finish pose sampling
	context->CompleteSample();

	if ( m_freezer.IsBlending() )
	{
		SBehaviorGraphOutput* frozenPose = context->GetFrozenPose();
		ASSERT( frozenPose );
		if ( frozenPose )
		{
			m_freezer.Blend( pose, *frozenPose );
		}
	}

	m_internalState = SIS_Waiting;

#ifndef RED_FINAL_BUILD
	const Float timeElapsed = timer.GetTimePeriod();
	if ( timeElapsed > 0.002f )
	{
		GScreenLog->PerfWarning( timeElapsed, TXT("ANIMATION"), TXT("Slow behavior sample for agent '%ls'"), GetAnimatedComponent()->GetEntity()->GetFriendlyName().AsChar() );
	}
#endif

	// Return final pose
	return pose;
}

Bool CBehaviorGraphStack::HasPoseConstraints() const
{
	return m_constraintInstance != NULL && m_constraintInstance->IsActive();
}

void CBehaviorGraphStack::SetPoseConstraintsEnable( Bool flag )
{
	ASSERT( m_internalState == SIS_Waiting );

	if ( m_constraintInstance )
	{
		if ( flag )
		{
			m_constraintInstance->Activate();
		}
		else
		{
			m_constraintInstance->Deactivate();
		}
	}
}

SBehaviorGraphOutput& CBehaviorGraphStack::ApplyPoseConstraints(	Float timeDelta, 
																	SBehaviorUpdateContext* updateContext, 
																	SBehaviorSampleContext* sampleContext,
																	SBehaviorGraphOutput& output )
{
	ASSERT( m_active );
	ASSERT( m_constraintInstance );
	ASSERT( updateContext );
	ASSERT( sampleContext );
	ASSERT( m_internalState == SIS_Waiting );

	m_internalState = SIS_Updating;

	// Update
	m_constraintInstance->Update( *updateContext, timeDelta );

	m_internalState = SIS_Sampling;

	// Prepare context for sample constraints
	SBehaviorGraphOutput& pose = sampleContext->PrepareForSampleConstraints( output );

	// Cache prev pose (pose without constraints)
	sampleContext->CachePoseFromPrevSampling();

	DEBUG_ANIM_POSES( pose )

	// Sample
	m_constraintInstance->Sample( *sampleContext, pose );

	DEBUG_ANIM_POSES( pose )

	// Finish pose sampling
	sampleContext->CompleteSampleConstraints();

	m_internalState = SIS_Waiting;

	return pose;
}

Bool CBehaviorGraphStack::GetSyncInfo( CSyncInfo& sync ) const
{
	ASSERT( m_internalState == SIS_Waiting );

	for ( Int32 i=(Int32)m_instances.Size()-1; i>=0; --i )
	{
		CBehaviorGraphInstance* inst = m_instances[i];
		if ( inst->IsActive() )
		{
			if ( inst->GetSyncInfo( sync ) )
			{
				return true;
			}
		}
	}
	return false;
}

Bool CBehaviorGraphStack::SynchronizeTo( const CSyncInfo& sync, const SAnimatedComponentSyncSettings& ass )
{
	ASSERT( m_internalState == SIS_Waiting );

	Bool ret = false;

	for ( Int32 i=(Int32)m_instances.Size()-1; i>=0; --i )
	{
		CBehaviorGraphInstance* inst = m_instances[i];
		if ( (!ass.m_instanceName || inst->GetInstanceName() == ass.m_instanceName) && inst->IsActive() && inst->SynchronizeTo( sync ) )
		{
			ret |= true;

			if ( !ass.m_syncAllInstances )
			{
				break;
			}
		}
	}
	return ret;
}

Bool CBehaviorGraphStack::SynchronizeTo( SBehaviorSyncTags& syncTags )
{
	Bool ret = false;

	for ( Int32 i=(Int32)m_instances.Size()-1; i>=0; --i )
	{
		CBehaviorGraphInstance* inst = m_instances[i];
		if ( inst->IsActive() )
		{
			ret |= inst->ApplyInboundSyncTags( syncTags );
		}
	}

	return ret;
}

Bool CBehaviorGraphStack::HasInstance( const CName& name ) const
{
	for ( Uint32 i=0; i<m_instances.Size(); i++ )
	{
		if ( m_instances[i]->GetInstanceName() == name )
		{
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphStack::HasActiveInstance( const CName& name ) const
{
	for ( Uint32 i=0; i<m_instances.Size(); i++ )
	{
		if ( m_instances[i]->GetInstanceName() == name )
		{
			return m_instances[i]->IsActive();
		}
	}

	return false;
}

CName CBehaviorGraphStack::GetActiveTopInstance() const
{
	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->IsActive() )
		{
			return m_instances[i]->GetInstanceName();
		}
	}

	return CName::NONE;
}

CName CBehaviorGraphStack::GetActiveBottomInstance() const
{
	Uint32 size = m_instances.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_instances[i]->IsActive() )
		{
			return m_instances[i]->GetInstanceName();
		}
	}

	return CName::NONE;
}

void CBehaviorGraphStack::GetInstances( TDynArray< CName >& instances ) const
{
	for ( Uint32 i=0; i<m_instances.Size(); i++ )
	{
		instances.PushBack( m_instances[i]->GetInstanceName() );
	}
}

CName const & CBehaviorGraphStack::GetInstanceName( Uint32 index ) const
{
	return index < m_instances.Size()? m_instances[index]->GetInstanceName() : CName::NONE;
}

String CBehaviorGraphStack::GetInstancesAsString() const
{
	if ( m_instances.Size() == 0 )
	{
		return TXT("<empty>");
	}

	String str;

	for ( Uint32 i=0; i<m_instances.Size(); i++ )
	{
		str += m_instances[i]->GetInstanceName().AsString() + TXT("; ");
	}

	return str;
}

CBehaviorGraphInstance* CBehaviorGraphStack::GetBehaviorGraphInstance( const CName& name ) const
{
	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		if ( m_instances[i]->GetInstanceName() == name )
		{
			return m_instances[i];
		}
	}

	BEH_WARN
	(
		TXT( "Couldn't find behavior instance '%ls' for %s %s ( current instances : %s )" ),
		name.AsString().AsChar(),
		GetAnimatedComponent()->GetName().AsChar(),
		GetAnimatedComponent()->GetEntity()->GetName().AsChar(),
		GetInstancesAsString().AsChar()
	);

	return NULL;
}

void CBehaviorGraphStack::ResetBehaviorInstances()
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		m_instances[i]->Reset();
	}
}

Bool CBehaviorGraphStack::ActivateAllInstances()
{
	ASSERT( m_internalState == SIS_Waiting );

	Bool ret = true;

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		CBehaviorGraphInstance* inst = m_instances[ i ];

		ret &= !inst->IsActive();

		inst->Activate();
	}

	return ret;
}

Bool CBehaviorGraphStack::DeactivateAllInstances()
{
	ASSERT( m_internalState == SIS_Waiting );

	Bool ret = true;

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		CBehaviorGraphInstance* inst = m_instances[ i ];

		ret &= inst->IsActive();

		inst->Deactivate();
	}

	return ret;
}

Bool CBehaviorGraphStack::DeactivateAndResetAllInstances()
{
	ASSERT( m_internalState == SIS_Waiting );

	Bool ret = true;

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		CBehaviorGraphInstance* inst = m_instances[ i ];

		ret &= inst->IsActive();

		inst->DeactivateAndReset();
	}

	return ret;
}

Bool CBehaviorGraphStack::IsEventProcessed( const CName& event ) const
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->IsEventProcessed( event ) )
		{
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphStack::ActivationNotificationReceived( const CName& notification ) const
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->ActivationNotificationReceived( notification ) )
		{
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphStack::DeactivationNotificationReceived( const CName& notification ) const
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->DeactivationNotificationReceived( notification ) )
		{
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphStack::ActivationNotificationReceived( const CName& instance, const CName& notification ) const
{
	ASSERT( m_internalState == SIS_Waiting );
	CBehaviorGraphInstance* inst = GetBehaviorGraphInstance( instance );
	return inst ? inst->ActivationNotificationReceived( notification ) : false;
}

Bool CBehaviorGraphStack::DeactivationNotificationReceived( const CName& instance, const CName& notification ) const
{
	ASSERT( m_internalState == SIS_Waiting );
	CBehaviorGraphInstance* inst = GetBehaviorGraphInstance( instance );
	return inst ? inst->DeactivationNotificationReceived( notification ) : false;
}

Bool CBehaviorGraphStack::AnyActivationNotificationsReceived() const
{
	ASSERT( m_internalState == SIS_Waiting );

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		if ( m_instances[ i ]->GetActivationNotificationNum() > 0 )
		{
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphStack::AnyDeactivationNotificationsReceived() const
{
	ASSERT( m_internalState == SIS_Waiting );

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		if ( m_instances[ i ]->GetDeactivationNotificationNum() > 0 )
		{
			return true;
		}
	}

	return false;
}

void CBehaviorGraphStack::ProcessScriptNotifications( CEntity* entity, SBehaviorGraphScriptNotification const * firstNotification, Uint32 notificationsCount )
{
	for ( SBehaviorGraphScriptNotification const * iEvent = firstNotification; notificationsCount > 0; ++ iEvent, -- notificationsCount )
	{
		// for notification "#Notification#"
		// call On#Notification#();
		entity->CallEvent( iEvent->m_notification );
		// call OnBehaviorGraphNotification( #Notification#, #sourceState# );
		entity->CallEvent( CNAME( OnBehaviorGraphNotification ), iEvent->m_notification, iEvent->m_sourceState );
	}
}

Bool CBehaviorGraphStack::HasAnyScriptNotifications() const
{
	if ( ! m_pendingScriptNotifications.Empty() )
	{
		return true;
	}

	const Uint32 size = m_instances.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CBehaviorGraphInstance* instance = m_instances[ i ];
		const TDynArray< SBehaviorGraphScriptNotification >& nots = instance->GetScriptedNodesNotifications();
		if ( ! nots.Empty() )
		{
			return true;
		}
	}

	return false;
}

void CBehaviorGraphStack::ProcessScriptNotifications( CAnimatedComponent* componenet )
{
	ASSERT( m_internalState == SIS_Waiting );

	CEntity* entity = componenet->GetEntity();

	{
		ProcessScriptNotifications( entity, m_pendingScriptNotifications.TypedData(), m_pendingScriptNotifications.Size() );
		m_pendingScriptNotifications.ClearFast();
	}

	const Uint32 size = m_instances.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		CBehaviorGraphInstance* instance = m_instances[ i ];

		CBehaviorGraphEventHandler* eventHandler = instance->GetEventHandler();
		if ( eventHandler )
		{
			eventHandler->HandleDelayedEvents( instance );
		}

		// create copy of notifications on stack - we need this, as some events may request switching behavior graph and then we will end up with invalid pointer to notifies to be called
		// TODO maybe due to that reason we should instead gather all events from all instances first and then go through that array? for now we only replace instances, so it should be fine
		const TDynArray< SBehaviorGraphScriptNotification >& nots = instance->GetScriptedNodesNotifications();
		Uint32 notsNum = nots.Size();
		Uint32 notsSize = sizeof( SBehaviorGraphScriptNotification ) * notsNum;
		SBehaviorGraphScriptNotification* notsCopy = (SBehaviorGraphScriptNotification*) RED_ALLOCA( notsSize );
		Red::System::MemoryCopy( notsCopy, nots.Data(), notsSize );
		SBehaviorGraphScriptNotification* notToCall = notsCopy;
		ProcessScriptNotifications( entity, notToCall, notsNum );
	}
}

void CBehaviorGraphStack::ClearAllStack()
{
	ASSERT( m_internalState == SIS_Waiting );

	Clear();

	if ( m_constraintInstance )
	{
		m_constraintInstance->Deactivate();
		m_constraintInstance->Unbind();
		m_constraintInstance = NULL;
	}
}

void CBehaviorGraphStack::Clear()
{
	ASSERT( m_internalState == SIS_Waiting );

	// Delete all behavior graph instances
	for (Uint32 i=0; i<m_instances.Size(); i++)
	{
		m_instances[i]->Deactivate();
		m_instances[i]->Unbind();
	}

	// Reset array
	m_instances.Clear();
}

void CBehaviorGraphStack::Lock( Bool flag )
{
	ASSERT( m_internalState == SIS_Waiting );

	if ( flag )
	{
		m_lock++;
	}
	else if ( m_lock > 0 )
	{
		m_lock--;
	}
}

Bool CBehaviorGraphStack::IsLocked() const
{
	return m_lock > 0;
}

void CBehaviorGraphStack::FreezePose()
{
	ASSERT( m_internalState == SIS_Waiting );
	ASSERT( !m_freezer.IsBusy() );

	m_frozen = true;
}

void CBehaviorGraphStack::UnfreezePose()
{
	ASSERT( m_internalState == SIS_Waiting );
	ASSERT( !m_freezer.IsBusy() );

	m_frozen = false;
}

void CBehaviorGraphStack::FreezePoseFadeIn( Float fadeInTime )
{
	m_freezer.Start( Freezer::FFD_Freezing, fadeInTime, this );
}

void CBehaviorGraphStack::UnfreezePoseFadeOut( Float fadeOutTime )
{
	m_freezer.Start( Freezer::FFD_Unfreezing, fadeOutTime, this );
}

void CBehaviorGraphStack::FreezePoseBlendIn( Float blendInTime )
{
	m_freezer.Start( Freezer::FFD_BlendingIn, blendInTime, this );
}

void CBehaviorGraphStack::UnfreezePoseBlendOut( Float blendOutTime )
{
	m_freezer.Start( Freezer::FFD_BlendingOut, blendOutTime, this );
}

Bool CBehaviorGraphStack::HasFrozenPose() const
{
	return m_frozen;
}

Bool CBehaviorGraphStack::IsFrozenPoseFading() const
{
	return m_freezer.IsFading();
}

Bool CBehaviorGraphStack::IsFrozenPoseBlending() const
{
	return m_freezer.IsBlending();
}

Bool CBehaviorGraphStack::HasCachedFrozenPose() const
{
	if ( CAnimatedComponent* ac = FindParent< CAnimatedComponent >() )
	{
		const SBehaviorSampleContext* sc = ac->GetBehaviorGraphSampleContext();
		return sc && sc->HasFrozenPose();
	}

	return false;
}

void CBehaviorGraphStack::CreateaAndCacheFrozenPose()
{
	if ( CAnimatedComponent* ac = FindParent< CAnimatedComponent >() )
	{
		if ( SBehaviorSampleContext* sc = ac->GetBehaviorGraphSampleContext() )
		{
			sc->CreateAndCacheFrozenPose();
		}
	}
}

void CBehaviorGraphStack::ReleaseCacheFrozenPose()
{
	if ( CAnimatedComponent* ac = FindParent< CAnimatedComponent >() )
	{
		if ( SBehaviorSampleContext* sc = ac->GetBehaviorGraphSampleContext() )
		{
			sc->ReleaseFrozenPose();
		}
	}
}

Bool CBehaviorGraphStack::InternalActivateBehaviorInstance( CBehaviorGraph* graph, const CName& name, Bool clearStack, const TDynArray< CName >* newStack )
{
	ASSERT( m_internalState == SIS_Waiting );

	CAnimatedComponent* ac = GetAnimatedComponent();

	if ( HasInstance( name ) && newStack == NULL )
	{
		return false;
	}

	if ( IsLocked() )
	{
		BEH_WARN( TXT("CBehaviorGraphStack - Couldn't activate instance because stack is locked, '%ls' '%ls'"), 
			ac->GetName().AsChar(), 
			ac->GetEntity()->GetName().AsChar() );
		return false;
	}

	if ( IsSynchronizing() )
	{
		BEH_WARN( TXT("Behavior stack is synchronizing and new activate is called - Actor '%ls' - New instance '%ls' - Curr stack:\n%s"),
			ac->GetEntity()->GetName().AsChar(),
			name.AsString().AsChar(),
			DumpStackSyncInfo().AsChar() );

		ASSERT( !TXT("Beh sync - Check log") );
	}

	if ( HasFrozenPose() )
	{
		UnfreezePose();
	}

	if ( clearStack )
	{
		Clear();
	}

	// Create instance
	CBehaviorGraphInstance* newInstance = graph->CreateInstance( ac, name );
	if ( !newInstance )
	{
		return false;
	}

	// Activate
	newInstance->Activate();

	if ( newStack )
	{
		for ( Uint32 i=0; i<newStack->Size(); ++i )
		{
			const CName& instName = (*newStack)[i];

			if ( instName == name )
			{
				// Add to list
				m_instances.PushBack( newInstance );
			}
			else
			{
				// Attach
				Bool ret = AttachBehaviorInstance( instName );
				ASSERT( ret );
			}
		}
	}
	else
	{
		// Add to list
		m_instances.PushBack( newInstance );
	}

	return true;
}

Bool CBehaviorGraphStack::AttachBehaviorInstance( CBehaviorGraph* graph, const CName& name )
{
	ASSERT( m_internalState == SIS_Waiting );
	return InternalActivateBehaviorInstance( graph, name, false, NULL );
}

Bool CBehaviorGraphStack::ActivateBehaviorInstance( CBehaviorGraph* graph, const CName& name, const TDynArray< CName >* newStack /* = NULL  */)
{
	ASSERT( m_internalState == SIS_Waiting );
	return InternalActivateBehaviorInstance( graph, name, true, newStack );
}

Bool CBehaviorGraphStack::InternalActivateBehaviorInstances( const TDynArray< CName >& names, Bool sync )
{
	ASSERT( m_internalState == SIS_Waiting );

	CAnimatedComponent* ac = GetAnimatedComponent();

	if ( IsLocked() )
	{
		BEH_WARN( TXT("CBehaviorGraphStack - Couldn't activate instance because stack is locked, '%ls' '%ls'"), 
			ac->GetName().AsChar(), 
			ac->GetEntity()->GetName().AsChar() );
		return false;
	}

	if ( sync )
	{
		if ( IsSynchronizing() )
		{
			BEH_WARN( TXT("Behavior stack is synchronizing and new activate is called - Actor '%ls' - New stack:\n%s Curr stack:\n%s"),
				ac->GetEntity()->GetName().AsChar(),
				DumpStackSyncInfo( names ).AsChar(),
				DumpStackSyncInfo().AsChar() );

			ASSERT( !TXT("Beh sync - Check log") );
		}
	}

	Bool ret = true;

	// Stack's spacial instances
	TDynArray< CName > topInstances;
	{
		TDynArray< SBehaviorGraphInstanceSlot >::const_iterator it;
		TDynArray< SBehaviorGraphInstanceSlot >::const_iterator end = m_slots->End();

		for ( it = m_slots->Begin(); it != end; ++it )
		{
			const SBehaviorGraphInstanceSlot& s = *it;

			if ( s.m_alwaysOnTopOfStack )
			{
				topInstances.PushBack( s.m_instanceName );
			}
		}
	}

	// Pop
	TDynArray< CName > instToPop;
	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		const CName& cuttInstName = m_instances[i]->GetInstanceName();

		Bool found = false;

		// New instances
		for ( Uint32 j=0; j<names.Size(); ++j )
		{
			const CName& instToPush = names[j];

			if ( instToPush == cuttInstName )
			{
				found = true;
				break;
			}
		}

		// Top instances
		for ( Uint32 j=0; j<topInstances.Size(); ++j )
		{
			const CName& topInst = topInstances[j];

			if ( topInst == cuttInstName )
			{
				found = true;
				break;
			}
		}

		if ( !found )
		{
			instToPop.PushBack( cuttInstName );
		}
	}

	// Synchronization - get sync infos and sync tags
	TDynArray< CBehaviorSyncInfo > syncInfos;
	SBehaviorSyncTags syncTags;
	SBehaviorGraphInstanceStoredVariables syncVariables;

#ifndef NO_EDITOR
	BehaviorListenersSwitcher switcher;
#endif

	// Pop all instances from table
	for ( Uint32 i=0; i<instToPop.Size(); ++i )
	{
		const CName& instToPopName = instToPop[i];

		if ( sync )
		{
			if ( CBehaviorGraphInstance* instance = GetBehaviorGraphInstance( instToPopName ) )
			{
				CBehaviorSyncInfo info;
				instance->GetSyncInfo( info );
				if ( info.IsOk() )
				{
					syncInfos.PushBack( info );
				}
				instance->GetOutboundSyncTags( syncTags );
				instance->StoreInstanceVariables( syncVariables );
			}
		}

#ifndef NO_EDITOR
		switcher.OnInstancePop( GetBehaviorGraphInstance( instToPopName ) );
#endif

		Bool temp = DetachBehaviorInstance( instToPopName );
		ASSERT( temp );
	}	

	// Push
	for ( Uint32 i=0; i<names.Size(); ++i )
	{
		const CName& instName = names[i];

		if ( !AttachBehaviorInstance( instName, &syncVariables ) )
		{
			ret = false;
		}

#ifndef NO_EDITOR
		switcher.OnInstancePush( GetBehaviorGraphInstance( instName ) );
#endif
	}

	// Move top instances on the top of stack
	for ( Uint32 i=0; i<topInstances.Size(); ++i )
	{
		const CName& topInst = topInstances[i];

		Bool found = false;

		for ( Uint32 j=0; j<m_instances.Size(); ++j )
		{
			CBehaviorGraphInstance* inst = m_instances[j];

			if ( inst->GetInstanceName() == topInst )
			{
				m_instances.Erase( m_instances.Begin() + j );
				m_instances.PushBack( inst );
				found = true;
				break;
			}
		}

		if ( found == false && !AttachBehaviorInstance( topInst ) )
		{
			ret = false;
		}
	}

	if ( sync )
	{
		Bool syncRet = false;

		// Synchronization - first try to synchronize to tags
		if ( syncTags.DoesContainAnyTags() )
		{
			for ( Uint32 j=0; j<m_instances.Size(); ++j )
			{
				syncRet = m_instances[j]->ApplyInboundSyncTags( syncTags ) || syncRet;
			}
		}
		
		// now infos
		for ( Uint32 i=0; i<syncInfos.Size(); ++i )
		{
			const CBehaviorSyncInfo& info = syncInfos[i];

			for ( Uint32 j=0; j<m_instances.Size(); ++j )
			{
				syncRet |= m_instances[j]->SynchronizeTo( info );
			}
		}
	}

#ifndef NO_EDITOR
	switcher.Process( m_instances );
#endif

	// sort instances to correspond to the list of names
	// (This is needed since activating new behaviors always adds them to the end of the m_instances)
	for( Uint32 i=0; i<names.Size(); ++i )
	{
		// already in the correct place?
		if( i < m_instances.Size() && m_instances[ i ]->GetInstanceName() == names[ i ] )
		{
			continue;
		}

		// find correct instance index 
		for( Uint32 j=i+1; j<m_instances.Size(); ++j )
		{
			if( m_instances[ j ]->GetInstanceName() == names[ i ] )
			{
				// ... and swap the instances
				CBehaviorGraphInstance* tmp = m_instances[ j ];
				m_instances[ j ] = m_instances[ i ];
				m_instances[ i ] = tmp;
				break;
			}
		}
	}

	return ret;
}

Bool CBehaviorGraphStack::ActivateBehaviorInstances( const CName& name )
{
	ASSERT( m_internalState == SIS_Waiting );
	TDynArray< CName > temp;
	temp.PushBack( name );
	return ActivateBehaviorInstances( temp );
}

Bool CBehaviorGraphStack::ActivateBehaviorInstances( const TDynArray< CName >& names )
{
	ASSERT( m_internalState == SIS_Waiting );
	return InternalActivateBehaviorInstances( names, false );
}

Bool CBehaviorGraphStack::ActivateAndSyncBehaviorInstances( const CName& name )
{
	ASSERT( m_internalState == SIS_Waiting );
	TDynArray< CName > temp;
	temp.PushBack( name );
	return ActivateAndSyncBehaviorInstances( temp );
}

Bool CBehaviorGraphStack::ActivateAndSyncBehaviorInstances( const TDynArray< CName >& names )
{
	ASSERT( m_internalState == SIS_Waiting );
	return InternalActivateBehaviorInstances( names, true );
}

Bool CBehaviorGraphStack::IsSynchronizing() const
{
	ASSERT( m_internalState == SIS_Waiting );

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		if ( m_instances[ i ]->IsSynchronizing() )
		{
			return true;
		}
	}

	return false;
}

String CBehaviorGraphStack::DumpStackSyncInfo( const TDynArray< CName >& names ) const
{
	String str;
	for ( Uint32 i=0; i<names.Size(); ++i )
	{
		str += String::Printf( TXT(" >%s\n"), names[ i ].AsString().AsChar() );
	}
	return str;
}

String CBehaviorGraphStack::DumpStackSyncInfo() const
{
	String str;
	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		str += String::Printf( TXT(" >%s\n"), m_instances[ i ]->GetInstanceName().AsString().AsChar() );
	}
	return str;
}

CAnimatedComponent* CBehaviorGraphStack::GetAnimatedComponent() const
{
	return SafeCast< CAnimatedComponent >( GetParent() );
}

void CBehaviorGraphStack::ResetAnimationCache()
{
	ASSERT( m_internalState == SIS_Waiting );

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		m_instances[i]->UpdateCachedAnimationPointers();
	}
}

void CBehaviorGraphStack::GetBehaviorInstancesBasedOn( const CBehaviorGraph* graph, TDynArray< CBehaviorGraphInstance* >& instances ) const
{
	if ( m_constraintInstance && m_constraintInstance->IsActive() && m_constraintInstance->GetGraph() == graph )
	{
		instances.PushBack( m_constraintInstance );
	}

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		CBehaviorGraphInstance* instance = m_instances[i];

		if ( instance->GetGraph() == graph )
		{
			instances.PushBack( instance );
		}
	}
}

CBehaviorGraphInstance* CBehaviorGraphStack::RecreateBehaviorInstance( const CBehaviorGraph* graph, CBehaviorGraphInstance* instance,  const CName& name )
{
	ASSERT( m_internalState == SIS_Waiting );

	CAnimatedComponent* ac = GetAnimatedComponent();

	if ( m_constraintInstance == instance )
	{
		ASSERT( !m_constraintInstance->IsActive() );
		ASSERT( !m_constraintInstance->IsBinded() );

		//m_constraintInstance->Discard();

		m_constraintInstance = graph->CreateInstance( ac, CNAME( Constraint ) );
		m_constraintInstance->Activate();

		return m_constraintInstance;
	}

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		if ( m_instances[i] == instance )
		{
			const CName& instName = name == CName::NONE ? m_instances[i]->GetInstanceName() : name;

			ASSERT( !m_instances[i]->IsActive() );
			ASSERT( !m_instances[i]->IsBinded() );

			//m_instances[i]->Discard();

			m_instances[i] = graph->CreateInstance( ac, instName );
			m_instances[i]->Activate();

			return m_instances[i];
		}
	}

	ASSERT( 0 );
	return NULL;
}

Bool CBehaviorGraphStack::HasInstanceSlot( const CName& name ) const
{
	return FindSlot( name ) != NULL;
}

Bool CBehaviorGraphStack::HasDefaultStateMachine( const CName& instance ) const
{
	CBehaviorGraphInstance* inst = GetBehaviorGraphInstance( instance );
	return inst ? inst->HasDefaultStateMachine() : false;
}

String CBehaviorGraphStack::GetStateInDefaultStateMachine( const CName& instance ) const
{
	CBehaviorGraphInstance* inst = GetBehaviorGraphInstance( instance );
	return inst ? inst->GetCurrentStateInDefaultStateMachine() : String::EMPTY;
}

const SBehaviorGraphInstanceSlot* CBehaviorGraphStack::FindSlot( const CName& name ) const
{
	ASSERT( m_internalState == SIS_Waiting );

	ASSERT( m_slots, TXT("Should always have slots! Check if you try to access slots when graph is already destroyed maybe?") );
	if ( m_slots ) // this should be not needed
	{
		for ( Uint32 i=0; i<m_slots->Size(); ++i )
		{
			if ( (*m_slots)[i].m_instanceName == name )
			{
				return &((*m_slots)[i]);
			}
		}
	}

	return NULL;
}

Bool CBehaviorGraphStack::AttachBehaviorInstance( const CName& name, const SBehaviorGraphInstanceStoredVariables * copyVariablesValues )
{
	ASSERT( m_internalState == SIS_Waiting );

	CAnimatedComponent* ac = GetAnimatedComponent();

	if ( IsLocked() )
	{
		BEH_WARN( TXT("CBehaviorGraphStack - Couldn't attach instance because stack is locked, '%ls' '%ls'"), 
			ac->GetName().AsChar(), 
			ac->GetEntity()->GetName().AsChar() );
		return false;
	}

	if ( HasInstance( name ) )
	{
		// Nothing to do
		return true;
	}

	if ( HasFrozenPose() )
	{
		UnfreezePose();
	}

	// Get graph resource
	THandle< CBehaviorGraph > graph = LoadBehaviorGraph( name, ac );
	if ( !graph )
	{
		return false;
	}

	// Create instance
	CBehaviorGraphInstance* newInstance = graph->CreateInstance( ac, name );
	if ( !newInstance )
	{
		BEH_ERROR( TXT("CBehaviorGraphStack::AttachBehaviorInstance - Couldn't create instance. Entity %s, component %s, instance name %s"),
			ac->GetEntity()->GetName().AsChar(), 
			ac->GetName().AsChar(), 
			name.AsString().AsChar() );
		return false;
	}

	// Restore variables before graph activation, so any node that latches values will have restored values
	if ( copyVariablesValues )
	{
		newInstance->RestoreInstanceVariables( *copyVariablesValues );
	}

	// Activate
	newInstance->Activate();

	// Add to list
	m_instances.PushBack( newInstance );

	return true;
}

Bool CBehaviorGraphStack::DetachBehaviorInstance( const CName& name )
{
	ASSERT( m_internalState == SIS_Waiting );

	if ( IsLocked() )
	{
        CAnimatedComponent* ac = GetAnimatedComponent();
		BEH_WARN( TXT("CBehaviorGraphStack - Couldn't detach instance because stack is locked, '%ls' '%ls'"), 
			ac->GetName().AsChar(), 
			ac->GetEntity()->GetName().AsChar() );
		return false;
	}

	for ( Uint32 i=0; i<m_instances.Size(); ++i )
	{
		if ( m_instances[i]->GetInstanceName() == name )
		{
			m_instances[i]->Deactivate();
			// gather events that came with deactivation - they would be gone and forgotten when instance is unbinded
			m_pendingScriptNotifications.PushBack( m_instances[i]->GetScriptedNodesNotifications() );

			m_instances[i]->Unbind();

			m_instances.Erase( m_instances.Begin() + i );

			return true;
		}
	}

	return false;
}

THandle< CBehaviorGraph > CBehaviorGraphStack::LoadBehaviorGraph( const CName& instanceName, const CAnimatedComponent* ac )
{
	const SBehaviorGraphInstanceSlot* slot = FindSlot( instanceName );
	if ( !slot )
	{
		BEH_WARN( TXT("CBehaviorGraphStack::LoadBehaviorGraph - Couldn't find graph instance slot %s in %s %s"), 
			instanceName.AsString().AsChar(),
			ac->GetName().AsChar(), 
			ac->GetEntity()->GetName().AsChar() );
		return nullptr;
	}

	// Get graph resource
	THandle< CBehaviorGraph > graph = slot->m_graph.Get();
	if ( !graph )
	{
		BEH_ERROR( TXT("CBehaviorGraphStack::LoadBehaviorGraph - Behavior graph slot resource or file is empty. Entity %s, component %s, instance name %s"),
			ac->GetEntity()->GetName().AsChar(), 
			ac->GetName().AsChar(), 
			slot->m_instanceName.AsString().AsChar() );
	}

	return graph;
}

//////////////////////////////////////////////////////////////////////////
Bool CBehaviorGraphStack::PauseSlotAnimation( const CName& slot, Bool pause )
{
	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->PauseSlotAnimation( slot, pause ) )
		{
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphStack::PlaySlotAnimation( const CName& slot, const CName& animation, const SBehaviorSlotSetup* slotSetup )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->PlaySlotAnimation( slot, animation, slotSetup ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->PlaySlotAnimation( slot, animation, slotSetup ) )
	{
		return true;
	}

	return false;
}

Bool CBehaviorGraphStack::PlaySlotAnimation( const CName& slot, CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup, Bool onlyActive )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->PlaySlotAnimation( slot, skeletalAnimation, slotSetup, onlyActive ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->PlaySlotAnimation( slot, skeletalAnimation, slotSetup, onlyActive ) )
	{
		return true;
	}

	return false;
}

Bool CBehaviorGraphStack::StopSlotAnimation( const CName& slot, Float blendOutTime, Bool onlyActive )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->StopSlotAnimation( slot, blendOutTime, onlyActive ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->StopSlotAnimation( slot, blendOutTime, onlyActive ) )
	{
		return true;
	}

	return false;
}

Bool CBehaviorGraphStack::StopAllSlotAnimation( const CName& slot, Float blendOutTime, Bool onlyActive )
{
    ASSERT( m_internalState == SIS_Waiting );

    Bool ret = false;

    Int32 size = (Int32)m_instances.Size();
    for ( Int32 i=size-1; i>=0; --i )
    {
        ret |= m_instances[i]->StopSlotAnimation( slot, blendOutTime, onlyActive );
    }

    if ( m_constraintInstance && m_constraintInstance->StopSlotAnimation( slot, blendOutTime, onlyActive ) )
    {
        return true;
    }

    return ret;
}

Bool CBehaviorGraphStack::HasSlotAnimation( const CName& slot, Bool onlyActive ) const
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->HasSlotAnimation( slot, onlyActive ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->HasSlotAnimation( slot, onlyActive ) )
	{
		return true;
	}

	return false;
}

Bool CBehaviorGraphStack::GetLookAt( const CName& nodeId, CBehaviorPointCloudLookAtInterface& nodeInterface )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->GetLookAt( nodeId, nodeInterface ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->GetLookAt( nodeId, nodeInterface ) )
	{
		return true;
	}

	return NULL;
}

Bool CBehaviorGraphStack::GetSlot( const CName& slot, IBehaviorGraphSlotInterface& slotInterface )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->GetSlot( slot, slotInterface ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->GetSlot( slot, slotInterface ) )
	{
		return true;
	}

	return NULL;
}

Bool CBehaviorGraphStack::GetSlot( const CName& slot, CBehaviorManualSlotInterface& slotInterface, Bool onlyActive )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->GetSlot( slot, slotInterface, onlyActive ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->GetSlot( slot, slotInterface, onlyActive ) )
	{
		return true;
	}

	return NULL;
}

Bool CBehaviorGraphStack::GetSlot( const CName& slot, CBehaviorMixerSlotInterface& slotInterface, Bool onlyActive, Bool searchFromTheTop )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	if ( searchFromTheTop )
	{
		for ( Int32 i=size-1; i>=0; --i )
		{
			if ( m_instances[i]->GetSlot( slot, slotInterface, onlyActive ) )
			{
				return true;
			}
		}
	}
	else
	{
		for ( Int32 i=0; i<size; ++i )
		{
			if ( m_instances[i]->GetSlot( slot, slotInterface, onlyActive ) )
			{
				return true;
			}
		}
	}

	if ( m_constraintInstance && m_constraintInstance->GetSlot( slot, slotInterface, onlyActive ) )
	{
		return true;
	}

	return NULL;
}

Bool CBehaviorGraphStack::SetNeedRefreshSyncTokensOnSlot( const CName& slot, Bool value )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->SetNeedRefreshSyncTokensOnSlot( slot, value ) )
		{
			return true;
		}
	}

	return false;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
Bool CBehaviorGraphStack::AppendSyncTokenForEntityOnSlot( const CName& slot, const CEntity* entity )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->AppendSyncTokenForEntityOnSlot( slot, entity ) )
		{
			return true;
		}
	}

	return false;
}
#endif

Bool CBehaviorGraphStack::SetSlotPose( const CName& slot, const CAnimatedComponent* componentWithPoseLS, Float blendTime, EBlendType type, IPoseSlotListener* l )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->SetSlotPose( slot, componentWithPoseLS, blendTime, type, l ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance &&  m_constraintInstance->SetSlotPose( slot, componentWithPoseLS, blendTime, type, l ) )
	{
		return true;
	}

	return false;
}

Bool CBehaviorGraphStack::SetSlotPose( const CName& slot, const TDynArray< AnimQsTransform >&poseLS, const TDynArray< Float >& floatTracks, Float blendTime, EBlendType type, IPoseSlotListener* l, const Matrix& localToWorld )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->SetSlotPose( slot, poseLS, floatTracks, blendTime, type, l, localToWorld ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->SetSlotPose( slot, poseLS, floatTracks, blendTime, type, l, localToWorld ) )
	{
		return true;
	}

	return false;
}


Bool CBehaviorGraphStack::IsPoseSlotActive( const CName& slot ) const
{
	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->IsPoseSlotActive( slot ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->IsPoseSlotActive( slot ) )
	{
		return true;
	}

	return false;
}

Bool CBehaviorGraphStack::ResetSlotPose( const CName& slot )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->ResetSlotPose( slot ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->ResetSlotPose( slot ) )
	{
		return true;
	}

	return false;
}

Bool CBehaviorGraphStack::DetachSlotListener( const CName& slot, ISlotAnimationListener* listener )
{
	ASSERT( m_internalState == SIS_Waiting );

	Bool ret = false;

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		ret |= m_instances[i]->DetachSlotListener( slot, listener );
	}

	if ( m_constraintInstance )
	{
		ret |= m_constraintInstance->DetachSlotListener( slot, listener );
	}

	return ret;
}

Bool CBehaviorGraphStack::HasSlotListener( ISlotAnimationListener* listener ) const
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->HasSlotListener( listener ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->HasSlotListener( listener ) )
	{
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

Bool CBehaviorGraphStack::SetBehaviorVariable( const CName varName, Float value, Bool inAllInstances )
{
	ASSERT( m_internalState == SIS_Waiting );
	Bool result = false;

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->SetFloatValue( varName, value ) )
		{
			result = true;
			if ( ! inAllInstances )
			{
				return true;
			}
		}
	}

	if ( m_constraintInstance )
	{
		if ( m_constraintInstance->SetFloatValue( varName, value ) )
		{
			result = true;
			if ( ! inAllInstances )
			{
				return true;
			}
		}
	}

	// if ( ! result )
	// {
	//BEH_WARN_ONCE( this, varName.AsChar(), TXT("Set behavior float - couldn't find float variable '%ls' - %s %s"),
	//	varName.AsChar(), GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
	// }

	return result;
}

Bool CBehaviorGraphStack::SetBehaviorVariable( const CName& instance, const CName varName, Float value )
{
	ASSERT( m_internalState == SIS_Waiting );

	CBehaviorGraphInstance* inst = GetBehaviorGraphInstance( instance );

	if ( !inst )
	{
		BEH_WARN( TXT("Set behavior float - couldn't find instance '%ls' - %s %s"),
			instance.AsString().AsChar(), GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
		return false;
	}

	if ( inst->SetFloatValue( varName, value ) )
	{
		return true;
	}

	//BEH_WARN_ONCE( this, varName.AsChar(), TXT("Set behavior float - couldn't find float variable '%ls' in instance %s - %s %s"),
	//	varName.AsChar(), instance.AsString().AsChar(), GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );

	return false;
}

Bool CBehaviorGraphStack::SetBehaviorVariable( const CName varName, const Vector& value, Bool inAllInstances )
{
	ASSERT( m_internalState == SIS_Waiting );
	Bool result = false;

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->SetVectorValue( varName, value ) )
		{
			result = true;
			if ( ! inAllInstances )
			{
				return true;
			}
		}
	}

	if ( m_constraintInstance )
	{
		if ( m_constraintInstance->SetVectorValue( varName, value ) )
		{
			result = true;
			if ( ! inAllInstances )
			{
				return true;
			}
		}
	}

	// if ( ! result )
	// {
	//BEH_WARN_ONCE( this, varName.AsChar(), TXT("Set behavior float - couldn't find float variable '%ls' - %s %s"),
	//	varName.AsChar(), GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
	// }

	return false;
}

Bool CBehaviorGraphStack::SetBehaviorVariable( const CName& instance, const CName varName, const Vector& value )
{
	ASSERT( m_internalState == SIS_Waiting );

	CBehaviorGraphInstance* inst = GetBehaviorGraphInstance( instance );

	if ( !inst )
	{
		BEH_WARN_ONCE( this, varName.AsChar(), TXT("Set behavior Vector - couldn't find instance '%ls' - %s %s"),
			instance.AsString().AsChar(), GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
		return false;
	}

	if ( inst->SetVectorValue( varName, value ) )
	{
		return true;
	}

	//BEH_WARN( TXT("Set behavior Vector - couldn't find Vector variable '%ls' in instance %s - %s %s"),
	//	varName.AsChar(), instance.AsString().AsChar(), GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );

	return false;
}

//////////////////////////////////////////////////////////////////////////

Bool CBehaviorGraphStack::HasBehaviorFloatVariable( const CName name ) const
{
	for ( auto it = m_instances.Begin(), end = m_instances.End(); it != end; ++it )
	{
		if ( ( *it )->HasFloatValue( name ) )
		{
			return true;
		}
	}

	if ( m_constraintInstance && m_constraintInstance->HasFloatValue( name ) )
	{
		return true;
	}

	return false;
}

Float CBehaviorGraphStack::GetBehaviorFloatVariable( const CName varName, Float defValue ) const
{
	if ( const Float* value = GetBehaviorFloatVariablePtr( varName ) )
	{
		return *value;
	}

	return defValue;
}

Float CBehaviorGraphStack::GetBehaviorInternalFloatVariable( const CName varName, Float defValue ) const
{
	if ( const Float* value = GetBehaviorInternalFloatVariablePtr( varName ) )
	{
		return *value;
	}

	return defValue;
}

const Float* CBehaviorGraphStack::GetBehaviorFloatVariablePtr( const CName varName ) const
{
	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( const Float* value = m_instances[i]->GetFloatValuePtr( varName ) )
		{
			return value;
		}
	}

	if ( m_constraintInstance )
	{
		if ( const Float* value = m_constraintInstance->GetFloatValuePtr( varName ) )
		{
			return value;
		}
	}
	return nullptr;
}

const Float* CBehaviorGraphStack::GetBehaviorInternalFloatVariablePtr( const CName varName ) const
{
	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( const Float* value = m_instances[i]->GetInternalFloatValuePtr( varName ) )
		{
			return value;
		}
	}

	if ( m_constraintInstance )
	{
		if ( const Float* value = m_constraintInstance->GetInternalFloatValuePtr( varName ) )
		{
			return value;
		}
	}
	return nullptr;
}

const Vector* CBehaviorGraphStack::GetBehaviorVectorVariablePtr( const CName varName ) const
{
	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( const Vector* value = m_instances[i]->GetVectorValuePtr( varName ) )
		{
			return value;
		}
	}

	if ( m_constraintInstance )
	{
		if ( const Vector* value = m_constraintInstance->GetVectorValuePtr( varName ) )
		{
			return value;
		}
	}
	return nullptr;
}

Vector CBehaviorGraphStack::GetBehaviorVectorVariable( const CName varName ) const
{
	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( const Vector* value = m_instances[i]->GetVectorValuePtr( varName ) )
		{
			return *value;
		}
	}

	//BEH_WARN( TXT("Get behavior vector - couldn't find vector variable '%ls' - %s %s"),
	//	varName, GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );

	return Vector::ZERO_3D_POINT;
}

Float CBehaviorGraphStack::GetBehaviorFloatVariable( const CName& instance, const CName varName, Float defValue ) const
{
	CBehaviorGraphInstance* inst = GetBehaviorGraphInstance( instance );

	if ( !inst )
	{
		BEH_WARN( TXT("Get behavior float variable - couldn't find instance '%ls' - %s %s"),
			instance.AsString().AsChar(), GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
		return defValue;
	}

	if ( const Float* value = inst->GetFloatValuePtr( varName ) )
	{
		return *value;
	}

	/*BEH_WARN( TXT("Get behavior float - couldn't find float variable '%ls' in instance %s - %s %s"),
		varName, instance.AsString().AsChar(), GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );*/

	return defValue;
}

Vector CBehaviorGraphStack::GetBehaviorVectorVariable( const CName& instance, const CName varName ) const
{
	CBehaviorGraphInstance* inst = GetBehaviorGraphInstance( instance );

	if ( !inst )
	{
		BEH_WARN( TXT("Get behavior vector - couldn't find instance '%ls' - %s %s"),
			instance.AsString().AsChar(), GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
		return Vector::ZERO_3D_POINT;
	}

	if ( const Vector* value = inst->GetVectorValuePtr( varName ) )
	{
		return *value;
	}

	//BEH_WARN( TXT("Get behavior vector - couldn't find vector variable '%ls' in instance %s - %s %s"),
	//	varName, instance.AsString().AsChar(), GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );

	return Vector::ZERO_3D_POINT;
}

//////////////////////////////////////////////////////////////////////////

Bool CBehaviorGraphStack::GenerateBehaviorEvent( const CName& event )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->GenerateEvent( event ) )
		{
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphStack::GenerateBehaviorEvent( const CName& instance, const CName& event )
{
	ASSERT( m_internalState == SIS_Waiting );
	CBehaviorGraphInstance* inst = GetBehaviorGraphInstance( instance );
	return inst ? inst->GenerateEvent( event ) : false;
}

Bool CBehaviorGraphStack::GenerateBehaviorForceEvent( const CName& event )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->GenerateForceEvent( event ) )
		{
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphStack::GenerateBehaviorStackEvent( const CName& event )
{
	ASSERT( m_internalState == SIS_Waiting );

	Bool ret = false;
	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		ret |= m_instances[i]->GenerateEvent( event );
	}

	return ret;
}

Bool CBehaviorGraphStack::GenerateBehaviorStackForceEvent( const CName& event )
{
	ASSERT( m_internalState == SIS_Waiting );

	Bool ret = false;
	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		ret |= m_instances[i]->GenerateForceEvent( event );
	}

	return ret;
}

Bool CBehaviorGraphStack::GenerateBehaviorForceEvent( const CName& instance, const CName& event )
{
	ASSERT( m_internalState == SIS_Waiting );
	CBehaviorGraphInstance* inst = GetBehaviorGraphInstance( instance );
	return inst ? inst->GenerateForceEvent( event ) : false;
}

//////////////////////////////////////////////////////////////////////////

Bool CBehaviorGraphStack::ActivateConstraint( const Vector &target,
											 const CName activationVariableName,
											  const CName variableToControlName, 
											  Float timeout )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->ActivateConstraint( target, activationVariableName, variableToControlName, timeout ) )
		{
			return true;
		}
	}

	BEH_WARN( TXT("Activating static constraint '%ls' '%ls' failed for %s %s"), activationVariableName.AsChar(), variableToControlName.AsChar(),
		GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );

	return false;
}

Bool CBehaviorGraphStack::ActivateConstraint( const CNode* target, 
											  const CName activationVariableName, 
											  const CName variableToControlName,
											  Float timeout )
{
	ASSERT( m_internalState == SIS_Waiting );

	if ( !target )
	{
		BEH_WARN( TXT("ActivateConstraint : Target is NULL !!! - %s %s"), 
			GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
		return false;
	}

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->ActivateConstraint( target, activationVariableName, variableToControlName, timeout ) )
		{
			return true;
		}
	}


	BEH_WARN( TXT("Activating dynamic constraint '%ls' '%ls' failed for %s %s"), activationVariableName.AsChar(), variableToControlName.AsChar(),
		GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );

	return false;
}

Bool CBehaviorGraphStack::ActivateConstraint( const CAnimatedComponent* target, 
											  const String& boneName,
											  const CName activationVariableName, 
											  const CName variableToControlName,
											  Bool useOffset,
											  const Matrix& offsetMatrix,
											  Float timeout )
{
	ASSERT( m_internalState == SIS_Waiting );

	if ( !target )
	{
		BEH_WARN( TXT("ActivateConstraint : Target is NULL !!! - %s %s"), 
			GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
		return false;
	}

	Int32 boneIndex = target->FindBoneByName( boneName.AsChar() );

	if ( boneIndex == -1 )
	{
		BEH_WARN( TXT("ActivateConstraint : Target don't have bone '%ls' - %s %s"), boneName.AsChar(),
			GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );
		return false;
	}

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->ActivateConstraint( target, boneIndex, activationVariableName, variableToControlName, useOffset, offsetMatrix, timeout ) )
		{
			return true;
		}
	}

	BEH_WARN( TXT("Activating bone constraint '%ls' '%ls' '%ls' failed for %s %s"), activationVariableName.AsChar(), variableToControlName.AsChar(), boneName.AsChar(),
		GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );

	return false;
}

Bool CBehaviorGraphStack::ChangeConstraintTarget( const CNode* target, const CName activationVariableName, const CName variableToControlName, Float timeout )
{
	ASSERT( m_internalState == SIS_Waiting );

	if ( target == NULL )
	{
		ASSERT( target );
		return false;
	}

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->ChangeConstraintTarget( target, activationVariableName, variableToControlName, timeout ) )
		{
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphStack::ChangeConstraintTarget( const Vector &target, const CName activationVariableName, const CName variableToControlName, Float timeout )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->ChangeConstraintTarget( target, activationVariableName, variableToControlName, timeout ) )
		{
			return true;
		}
	}

	return false;
}

Bool CBehaviorGraphStack::ChangeConstraintTarget( const CAnimatedComponent* target, const String& boneName, const CName activationVariableName, const CName variableToControlName, Bool useOffset, const Matrix& offsetMatrix, Float timeout )
{
	ASSERT( m_internalState == SIS_Waiting );

	if ( target == NULL )
	{
		ASSERT( target );
		return false;
	}
	
	Int32 boneIndex = target->FindBoneByName( boneName.AsChar() );
	if ( boneIndex < 0 )
	{
		return false;
	}

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->ChangeConstraintTarget( target, boneIndex, activationVariableName, variableToControlName, useOffset, offsetMatrix, timeout ) )
		{
			return true;
		}
	}

	return false;
}

Vector CBehaviorGraphStack::GetConstraintTarget( const CName activationVariableName )
{
	ASSERT( m_internalState == SIS_Waiting );

	Vector target( Vector::ZERO_3D_POINT );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->GetConstraintTarget( activationVariableName, target ) )
		{
			return target;
		}
	}

	BEH_LOG( TXT("Stack hasn't got instance with constraint '%ls' - %s %s"), activationVariableName.AsChar(),
		GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );

	return target;
}

Bool CBehaviorGraphStack::DeactivateConstraint( const CName activationVariableName )
{
	ASSERT( m_internalState == SIS_Waiting );

	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->DeactivateConstraint( activationVariableName ) )
		{
			return true;
		}
	}

	BEH_WARN( TXT("Deactivating constrint failed - stack hasn't got instnace with constraint '%ls' - %s %s"), activationVariableName.AsChar(),
		GetAnimatedComponent()->GetName().AsChar(), GetAnimatedComponent()->GetEntity()->GetName().AsChar() );

	return false;
}

Bool CBehaviorGraphStack::HasConstraint( const CName activationVariableName ) const
{
	Int32 size = (Int32)m_instances.Size();
	for ( Int32 i=size-1; i>=0; --i )
	{
		if ( m_instances[i]->HasConstraint( activationVariableName ) )
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphStack::Freezer::Freezer()
{
	Reset();
}

void CBehaviorGraphStack::Freezer::Reset()
{
	m_state = FFD_None;
	m_speed = 0.f;
	m_progress = 0.f;
}

void CBehaviorGraphStack::Freezer::SetState( EFreezeState s, CBehaviorGraphStack* stack )
{
	Reset();

	//m_state = s; Don't change state because set state is immediate action so it will be finished now and return to idle state
	ASSERT( m_state == FFD_None );
	
	// Set new state
	if ( s == FFD_Freezing )
	{
		if ( !stack->HasFrozenPose() )
		{
			stack->FreezePose();
		}
	}
	else if ( s == FFD_Unfreezing )
	{
		if ( stack->HasFrozenPose() )
		{
			stack->UnfreezePose();
		}
	}
	else if ( s == FFD_BlendingIn )
	{
		if ( !stack->HasCachedFrozenPose() )
		{
			stack->CreateaAndCacheFrozenPose();
		}

		if ( !stack->HasFrozenPose() )
		{
			stack->FreezePose();
		}
	}
	else if ( s == FFD_BlendingOut )
	{
		stack->ReleaseCacheFrozenPose();

		if ( stack->HasFrozenPose() )
		{
			stack->UnfreezePose();
		}
	}
	else
	{
		ASSERT( 0 );
	}
}

void CBehaviorGraphStack::Freezer::Start( EFreezeState s, Float duration, CBehaviorGraphStack* stack )
{
	if ( duration > 0.f )
	{
		m_state = s;
		m_speed = 1.f / duration;
		m_progress = 0.f;

		if ( m_state == FFD_Freezing )
		{
			// Nothing
		}
		if ( m_state == FFD_Unfreezing )
		{
			if ( stack->HasFrozenPose() )
			{
				stack->UnfreezePose();
			}
		}
		else if ( m_state == FFD_BlendingIn )
		{
			if ( !stack->HasCachedFrozenPose() )
			{
				stack->CreateaAndCacheFrozenPose();
			}
		}
		else if ( m_state == FFD_BlendingOut )
		{
			if ( !stack->HasCachedFrozenPose() )
			{
				stack->CreateaAndCacheFrozenPose();
			}

			if ( stack->HasFrozenPose() )
			{
				stack->UnfreezePose();
			}
		}
		else
		{
			ASSERT( 0 );
		}
	}
	else
	{
		// TODO - we need to resolve conflicts like freezer is currently in freezing mode and we are switching to blend in mode
		// Clean up
		/*if ( m_state == FFD_Freezing )
		{

		}*/

		SetState( s, stack );
	}
}

void CBehaviorGraphStack::Freezer::Update( Float& inOutTimeDelta, CBehaviorGraphStack* stack )
{
	const Float dt = inOutTimeDelta; // Don't read ref variables many times

	if ( m_state == FFD_None )
	{
		return;
	}
	else if( m_state == FFD_Freezing )
	{
		RED_ASSERT( m_speed > 0.0f );

		m_progress += m_speed * dt;

		if ( m_progress > 1.0f )
		{
			SetState( FFD_Freezing, stack );
		}
		else
		{
			inOutTimeDelta *= ( 1.f - m_progress );
		}
	}
	else if ( m_state == FFD_Unfreezing )
	{
		RED_ASSERT( m_speed > 0.0f );

		m_progress += m_speed * dt;

		if ( m_progress > 1.0f )
		{
			SetState( FFD_Unfreezing, stack );
		}
		else
		{
			inOutTimeDelta *= m_progress;
		}
	}
	else if ( m_state == FFD_BlendingIn )
	{
		RED_ASSERT( m_speed > 0.0f );

		m_progress += m_speed * dt;

		if ( m_progress > 1.0f )
		{
			SetState( FFD_BlendingIn, stack );
		}
	}
	else if ( m_state == FFD_BlendingOut )
	{
		RED_ASSERT( m_speed > 0.0f );

		m_progress += m_speed * dt;

		if ( m_progress > 1.0f )
		{
			SetState( FFD_BlendingOut, stack );
		}
	}
	else
	{
		ASSERT( 0 );
	}
}

void CBehaviorGraphStack::Freezer::Blend( SBehaviorGraphOutput& pose, SBehaviorGraphOutput& frozenPose ) const
{
	ASSERT( IsBlending() );
	ASSERT( m_progress >= 0.f && m_progress <= 1.f );

	const Float w = m_state == FFD_BlendingIn ? Clamp( m_progress, 0.f, 1.f ) : 1.f - Clamp( m_progress, 0.f, 1.f );

	pose.SetInterpolate( pose, frozenPose, w );
}

Bool CBehaviorGraphStack::Freezer::IsBusy() const
{
	return m_state != FFD_None;
}

Bool CBehaviorGraphStack::Freezer::IsFading() const
{
	return m_state == FFD_Freezing || m_state == FFD_Unfreezing;
}

Bool CBehaviorGraphStack::Freezer::IsBlending() const
{
	return m_state == FFD_BlendingIn || m_state == FFD_BlendingOut;
}

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR

void BehaviorListenersSwitcher::OnInstancePush( CBehaviorGraphInstance* instance )
{
	if ( instance )
	{
		m_instancesName.PushBack( instance->GetInstanceName() );
	}
}

void BehaviorListenersSwitcher::OnInstancePop( CBehaviorGraphInstance* instance )
{
	if ( instance && instance->HasEditorListener() )
	{
		IBehaviorGraphInstanceEditorListener* listener = instance->GetEditorListener();
		if ( listener->CanBeRelinked() )
		{
			listener->RequestToRelink();

			Int32 index = static_cast< Int32 >( m_elems.Grow( 1 ) );
			SwitcherElem& elem = m_elems[ index ];

			elem.m_name = instance->GetInstanceName();
			elem.m_listener = listener;
		}
	}
}

void BehaviorListenersSwitcher::Process( TDynArray< CBehaviorGraphInstance* >& instances )
{
	if ( m_instancesName.Size() == 1 && m_elems.Size() == 1 )
	{
		CBehaviorGraphInstance* inst = NULL;

		for ( Uint32 i=0; i<instances.Size(); ++i )
		{
			if ( instances[ i ]->GetInstanceName() == m_instancesName[ 0 ] )
			{
				inst = instances[ i ];
				break;
			}
		}

		if ( inst )
		{
			m_elems[ 0 ].m_listener->Relink( inst );
			return;
		}
	}
	
	for ( Uint32 i=0; i<m_elems.Size(); ++i )
	{
		m_elems[ i ].m_listener->Relink( NULL );
	}
}

#endif

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
