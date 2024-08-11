/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/engine/behaviorGraphStack.h"

#include "animationManualSlotSync.h"
#include "movingPhysicalAgentComponent.h"
#include "../engine/skeletalAnimationEntry.h"
#include "../engine/skeletalAnimationContainer.h"

IMPLEMENT_RTTI_ENUM( EAnimationManualSyncType );
IMPLEMENT_RTTI_ENUM( ESyncRotationUsingRefBoneType );
IMPLEMENT_ENGINE_CLASS( SAnimationSequencePartDefinition );
IMPLEMENT_ENGINE_CLASS( SAnimationSequenceDefinition );
IMPLEMENT_ENGINE_CLASS( CAnimationManualSlotSyncInstance );

RED_DEFINE_STATIC_NAME(AllowSlide);
RED_DEFINE_STATIC_NAME(SlideSyncAdjustment);

CAnimationManualSlotSyncInstance::CAnimationManualSlotSyncInstance()
	: m_currentTime( 0.f )
{
}

void CAnimationManualSlotSyncInstance::Update( Float deltaTime )
{
	TDynArray< SAnimationSequenceInstance >::iterator masterInstance = m_sequenceInstances.Begin();
	if ( masterInstance != m_sequenceInstances.End() )
	{
		SAnimationSequenceInstance& masterSequence = *masterInstance;
		// apply time mutliplier to play animation with same speed as animations from behavior graph
		if ( CEntity* masterEntity = masterSequence.m_entity.Get() )
		{
			if ( CAnimatedComponent* ac = masterEntity->GetRootAnimatedComponent() )
			{
				deltaTime *= ac->GetTimeMultiplier();
			}
		}
	}

	m_currentTime += deltaTime;

	Bool master = true;
	const TDynArray< SAnimationSequenceInstance >::iterator end = m_sequenceInstances.End();
	for( TDynArray< SAnimationSequenceInstance >::iterator it = m_sequenceInstances.Begin(); it != end; ++it )
	{
		it->Update( m_currentTime, deltaTime, master, *this );
		master = false;
	}
}

Bool CAnimationManualSlotSyncInstance::InitNewSequenceInstance( const SAnimationSequenceDefinition& definition )
{
	const CEntity* entity = definition.m_entity.Get();
	if( !entity )
	{
		SET_ERROR_STATE( entity, TXT("CAnimationManualSlotSync: entity is NULL") );
		return false;
	}

	const CAnimatedComponent* animated = entity->GetRootAnimatedComponent();
	if( !animated )
	{
		SET_ERROR_STATE( entity, TXT("CAnimationManualSlotSync: no animated component") );
		return false;
	}

	if ( !animated->GetBehaviorStack() )
	{
		SET_ERROR_STATE( entity, TXT("CAnimationManualSlotSync: no behavior stack") );
		return false;
	}

	CBehaviorManualSlotInterface slotInterface;
	if ( definition.m_startForceEvent.Empty() )
	{
		// find slot - it has to be active!
		animated->GetBehaviorStack()->GetSlot( definition.m_manualSlotName, slotInterface );
	}
	else
	{
		// find any slot, we hope that it will get activated with raised force event
		animated->GetBehaviorStack()->GetSlot( definition.m_manualSlotName, slotInterface, false );
	}
	if( !slotInterface.IsValid() )
	{
		SET_ERROR_STATE( entity, TXT("CAnimationManualSlotSync: manual animation slot is not valid") );
		return false;
	}

	SAnimationSequenceInstance newInstance;
	newInstance.m_entity					= definition.m_entity;
	newInstance.m_brokeOut					= false;
	newInstance.m_hasFinished				= false;
	newInstance.m_slot						= slotInterface;
	newInstance.m_freezeAtEnd				= definition.m_freezeAtEnd;
	newInstance.m_startForceEvent			= definition.m_startForceEvent;
	newInstance.m_startForceEventSent		= newInstance.m_startForceEvent.Empty();
	newInstance.m_raiseEventOnEnd			= definition.m_raiseEventOnEnd;
	newInstance.m_raiseForceEventOnEnd		= definition.m_raiseEventOnEnd;
	newInstance.m_endEventRaised			= newInstance.m_raiseEventOnEnd.Empty();
	newInstance.m_endForceEventRaised		= newInstance.m_raiseForceEventOnEnd.Empty();
	newInstance.m_disabledProxyCollisions	= false;
	newInstance.m_movementAdjustTicket		= SMovementAdjustmentRequestTicket::Invalid();

	const TDynArray<SAnimationSequencePartDefinition>::const_iterator partsEnd = definition.m_parts.End();
	for( TDynArray<SAnimationSequencePartDefinition>::const_iterator partsIt = definition.m_parts.Begin(); partsIt != partsEnd; ++partsIt )
	{
		const SAnimationSequencePartDefinition& part = *partsIt;

		SAnimationSequencePartInstance newPart;
		newPart.m_animState.m_animation		= part.m_animation;
		newPart.m_animState.m_currTime		= 0.f;
		newPart.m_animState.m_prevTime		= 0.f;
		newPart.m_blendTransitionTime		= part.m_blendTransitionTime;
		newPart.m_blendInTime				= part.m_blendInTime;
		newPart.m_blendOutTime				= part.m_blendOutTime;
		newPart.m_allowBreakAtStart			= part.m_allowBreakAtStart;
		newPart.m_allowBreakBeforeEnd		= part.m_allowBreakBeforeEnd;
		newPart.m_useRefBone				= part.m_useRefBone;
		newPart.m_shouldSlide				= part.m_shouldSlide;
		newPart.m_shouldRotate				= part.m_shouldRotate;
		newPart.m_rotationTypeUsingRefBone	= part.m_rotationTypeUsingRefBone;
		newPart.m_finalHeading				= part.m_finalHeading;
		newPart.m_finalPosition				= part.m_finalPosition;
		newPart.m_sequenceIndex				= (Uint32)part.m_sequenceIndex;
		newPart.m_disableProxyCollisions	= part.m_disableProxyCollisions;
		newPart.m_startTime					= 0.f;
		newPart.m_endTime					= 0.f;
		newPart.m_movementDuration			= 0.f;
		newPart.m_movementStartTime			= 0.f;

		// use event for break start/end
		if ( ( ! part.m_allowBreakAtStartBeforeEventsEnd.Empty() || ! part.m_allowBreakBeforeAtAfterEventsStart.Empty() ) &&
			 definition.m_entity.Get() &&
			 definition.m_entity.Get()->GetRootAnimatedComponent() &&
			 definition.m_entity.Get()->GetRootAnimatedComponent()->GetAnimationContainer() )
		{
			if ( const CSkeletalAnimationSetEntry* anim = definition.m_entity.Get()->GetRootAnimatedComponent()->GetAnimationContainer()->FindAnimation( part.m_animation ) )
			{
				// go through anims events
				TDynArray<CExtAnimEvent*> events;
				anim->GetAllEvents( events );
				Float earliestAllowBreakAtStartBeforeEventsEndFound = -1.0f;
				Float latestAllowBreakBeforeAtAfterEventsStart = -1.0f;
				const TDynArray< CExtAnimEvent* >::const_iterator eventsEnd = events.End();
				for( TDynArray< CExtAnimEvent* >::const_iterator eventIt = events.Begin(); eventIt != eventsEnd; ++eventIt )
				{
					// if event is as told in definition, get earliest/latest one
					if( ! part.m_allowBreakAtStartBeforeEventsEnd.Empty() && (*eventIt)->GetEventName() == part.m_allowBreakAtStartBeforeEventsEnd )
					{
						if ( earliestAllowBreakAtStartBeforeEventsEndFound < 0.0f ||
							(*eventIt)->GetEndTimeWithoutClamp() < earliestAllowBreakAtStartBeforeEventsEndFound )
						{
							earliestAllowBreakAtStartBeforeEventsEndFound = (*eventIt)->GetEndTimeWithoutClamp();
						}
					}
					if( ! part.m_allowBreakBeforeAtAfterEventsStart.Empty() && (*eventIt)->GetEventName() == part.m_allowBreakBeforeAtAfterEventsStart )
					{
						if ( latestAllowBreakBeforeAtAfterEventsStart < 0.0f ||
							(*eventIt)->GetStartTime() > latestAllowBreakBeforeAtAfterEventsStart )
						{
							latestAllowBreakBeforeAtAfterEventsStart = (*eventIt)->GetStartTime();
						}
					}
					
				}
				if( earliestAllowBreakAtStartBeforeEventsEndFound >= 0.0f )
				{
					newPart.m_allowBreakAtStart = earliestAllowBreakAtStartBeforeEventsEndFound;
				}
				if( latestAllowBreakBeforeAtAfterEventsStart >= 0.0f )
				{
					newPart.m_allowBreakBeforeEnd = anim->GetDuration() - latestAllowBreakBeforeAtAfterEventsStart;
				}
			}
		}

		newInstance.m_parts.PushBack( newPart );
	}

	ASSERT( newInstance.m_parts.Size() );

	m_sequenceInstances.PushBack( newInstance );

	return true;
}

Bool CAnimationManualSlotSyncInstance::RegisterMaster( const SAnimationSequenceDefinition& masterDefinition )
{
	ASSERT( m_sequenceInstances.Empty() );

	m_sequenceInstances.ClearFast();

	m_currentTime = 0.f;
	m_endTime = 0.f;

	if( !InitNewSequenceInstance( masterDefinition ) )
	{
		return false;
	}

	SAnimationSequenceInstance& masterSequenceInst = m_sequenceInstances[0];
	masterSequenceInst.m_currentPartIndex = 0;
	const Int32 totalPartsAmount = (Int32)masterDefinition.m_parts.Size();
	m_cachedMasterEvents.ClearFast();
	m_cachedMasterEvents.ResizeFast( totalPartsAmount );

	// Master animations are crucial so check if they exists and prepare sequence instances
	Float nextStartTime = 0.f;
	for( Int32 i = 0; i < totalPartsAmount; ++i )
	{
		const SAnimationSequencePartDefinition& partDef = masterDefinition.m_parts[i];
		SAnimationSequencePartInstance& partInst = masterSequenceInst.m_parts[i];

		// Sequence index for master must always be sorted and has to contain all the parts
		ASSERT( partDef.m_sequenceIndex == i );

		const CSkeletalAnimationSetEntry* anim = masterDefinition.m_entity.Get()->GetRootAnimatedComponent()->GetAnimationContainer()->FindAnimation( partDef.m_animation );
		if( !anim )
		{
			SET_ERROR_STATE( masterDefinition.m_entity.Get(), TXT("CAnimationManualSlotSync: master animation doesn't exist") );
			m_sequenceInstances.ClearFast();
			return false;
		}

		// Set animation end time for master and whole synchronization
		partInst.m_startTime = nextStartTime;
		partInst.m_endTime = nextStartTime + anim->GetDuration();

		if( partInst.m_endTime > m_endTime )
		{
			m_endTime = partInst.m_endTime;
		}

		// Calculate when the next animation will start
		nextStartTime = partInst.m_endTime - partInst.m_blendTransitionTime;

		anim->GetAllEvents( m_cachedMasterEvents[i] );

		// Get only movement info for master if he should slide
		if( partDef.m_shouldSlide || partDef.m_shouldRotate )
		{
			const TDynArray< CExtAnimEvent* >::const_iterator masterEventsEnd = m_cachedMasterEvents[i].End();
			for( TDynArray< CExtAnimEvent* >::const_iterator masterEventIt = m_cachedMasterEvents[i].Begin(); masterEventIt != masterEventsEnd; ++masterEventIt )
			{
				if( IsType<CExtAnimDurationEvent>(*masterEventIt) )
				{
					const CExtAnimDurationEvent* durEvent = static_cast<CExtAnimDurationEvent*>( *masterEventIt );
					if( durEvent->GetEventName() == CNAME(AllowSlide) )
					{
						partInst.m_movementStartTime = durEvent->GetStartTime();
						partInst.m_movementDuration = durEvent->GetDuration();
					}
				}
			}
		}
	}

	return true;
}

Bool CAnimationManualSlotSyncInstance::RegisterSlave( const SAnimationSequenceDefinition& slaveDefinition )
{
	ASSERT( !m_sequenceInstances.Empty() );

	if( m_sequenceInstances.Empty() || !InitNewSequenceInstance( slaveDefinition ) )
	{
		return false;
	}

	const SAnimationSequenceInstance& masterSequenceInst = m_sequenceInstances[0];
	SAnimationSequenceInstance& slaveSequenceInst = m_sequenceInstances.Back();

	slaveSequenceInst.m_currentPartIndex = 0;

	// Now we need to calculate the start of animations for slaves and get the movement info
	const Uint32 slavePartsAmount = slaveDefinition.m_parts.Size();
	for( Uint32 j = 0; j < slavePartsAmount; ++j )
	{
		const SAnimationSequencePartDefinition& partDef = slaveDefinition.m_parts[j];
		SAnimationSequencePartInstance& partInst = slaveSequenceInst.m_parts[j];

		ASSERT( (Uint32)partDef.m_sequenceIndex < masterSequenceInst.m_parts.Size() );

		const CSkeletalAnimationSetEntry* slaveAnimation = slaveDefinition.m_entity.Get()->GetRootAnimatedComponent()->GetAnimationContainer()->FindAnimation( partDef.m_animation );
		if( !slaveAnimation )
		{
			SET_ERROR_STATE( slaveDefinition.m_entity.Get(), TXT("CAnimationManualSlotSync: animation doesn't exist") );
			continue;
		}

		// Calculate the start time depending on type of synchronization
		const TDynArray< CExtAnimEvent* >::const_iterator masterEventsEnd = m_cachedMasterEvents[partDef.m_sequenceIndex].End();
		for( TDynArray< CExtAnimEvent* >::const_iterator masterEventIt = m_cachedMasterEvents[partDef.m_sequenceIndex].Begin(); masterEventIt != masterEventsEnd; ++masterEventIt )
		{
			const CExtAnimEvent* event = *masterEventIt;
			if( ! partDef.m_syncEventName.Empty() &&
				event->GetEventName() == partDef.m_syncEventName )
			{
				const Float eventTime = event->GetStartTime();
				if( partDef.m_syncType == AMST_SyncEnd )
				{
					partInst.m_startTime = masterSequenceInst.m_parts[partDef.m_sequenceIndex].m_startTime + eventTime - slaveAnimation->GetDuration();
				}
				else if( partDef.m_syncType == AMST_SyncBeginning )
				{
					partInst.m_startTime = masterSequenceInst.m_parts[partDef.m_sequenceIndex].m_startTime + eventTime;
				}
				else
				{
					ASSERT( partDef.m_syncType == AMST_SyncMatchEvents );
					TDynArray< CExtAnimEvent* > slaveEvents;
					slaveAnimation->GetAllEvents( slaveEvents );
					const TDynArray< CExtAnimEvent* >::const_iterator slaveEventEnd = slaveEvents.End();
					for( TDynArray< CExtAnimEvent* >::const_iterator slaveEventIt = slaveEvents.Begin(); slaveEventIt != slaveEventEnd; ++slaveEventIt )
					{
						const CExtAnimEvent* slaveEvent = *slaveEventIt;
						if( slaveEvent->GetEventName() == partDef.m_syncEventName )
						{
							partInst.m_startTime = masterSequenceInst.m_parts[partDef.m_sequenceIndex].m_startTime + eventTime - slaveEvent->GetStartTime();
						}
					}
				}
				break;
			}
		}

		// Calculate the movement info if should slide
		if( partDef.m_shouldSlide || partDef.m_shouldRotate )
		{
			TDynArray< CExtAnimDurationEvent* > slaveEvents;
			slaveAnimation->GetEventsOfType( slaveEvents );
			const TDynArray< CExtAnimDurationEvent* >::const_iterator slaveEventEnd = slaveEvents.End();
			for( TDynArray< CExtAnimDurationEvent* >::const_iterator slaveEventIt = slaveEvents.Begin(); slaveEventIt != slaveEventEnd; ++slaveEventIt )
			{
				const CExtAnimDurationEvent* slaveEvent = *slaveEventIt;
				if( slaveEvent->GetEventName() == CNAME(AllowSlide) )
				{
					partInst.m_movementStartTime = partInst.m_startTime + slaveEvent->GetStartTime();
					partInst.m_movementDuration = slaveEvent->GetDuration();
				}
			}
		}

		partInst.m_endTime = partInst.m_startTime + slaveAnimation->GetDuration();

		// If the slave animation should be played before the master animation in order to be synced properly,
		// set the current time to the start time of that animation (which will be a negative number).
		// Update will start the animation at the right time, and master animation will always start at 0.0f.
		if( partInst.m_startTime < m_currentTime )
		{
			m_currentTime = partInst.m_startTime;
		}
	}

	// Don't change the last one, but change the end times for overlapping animations
	Float nextTransition = slaveSequenceInst.m_parts[ slavePartsAmount - 1 ].m_startTime;
	for( Int32 j = slavePartsAmount - 2; j >= 0; --j )
	{
		SAnimationSequencePartInstance& partInst = slaveSequenceInst.m_parts[j];

		const Float maxEndTime = nextTransition + partInst.m_blendTransitionTime;
		if( maxEndTime < partInst.m_endTime )
		{
			partInst.m_endTime = maxEndTime;
		}

		nextTransition = partInst.m_startTime;
	}

	if( slaveSequenceInst.m_parts[ slavePartsAmount - 1 ].m_endTime > m_endTime )
	{
		m_endTime = slaveSequenceInst.m_parts[ slavePartsAmount - 1 ].m_endTime;
	}

	return true;
}

void CAnimationManualSlotSyncInstance::RemoveSlavesIfNotStarted()
{
	// skip first - master
	for ( Int32 Idx = 1; Idx < m_sequenceInstances.SizeInt(); ++ Idx )
	{
		if ( ! m_sequenceInstances[ Idx ].HasStarted( m_currentTime ) )
		{
			m_sequenceInstances.RemoveAtFast( Idx );
			-- Idx;
		}
	}
}

void CAnimationManualSlotSyncInstance::BreakSlavesIfPossible()
{
	// skip first - master
	for ( Int32 Idx = 1; Idx < m_sequenceInstances.SizeInt(); ++ Idx )
	{
		m_sequenceInstances[ Idx ].BreakIfPossible( false, *this );
	}
}

void CAnimationManualSlotSyncInstance::BreakMasterIfPossible()
{
	if ( ! m_sequenceInstances.Empty() )
	{
		m_sequenceInstances[ 0 ].BreakIfPossible( true, *this );
	}
}

Bool CAnimationManualSlotSyncInstance::HasEnded() const
{
	Bool allHasEnded = false;
	const TDynArray< SAnimationSequenceInstance >::const_iterator end = m_sequenceInstances.End();
	for( TDynArray< SAnimationSequenceInstance >::const_iterator it = m_sequenceInstances.Begin(); it != end; ++it )
	{
		if (! it->HasEnded())
		{
			allHasEnded = false;
			break;
		}
	}
	return allHasEnded || ( m_endTime > 0.f && m_currentTime >= m_endTime );
}

Bool CAnimationManualSlotSyncInstance::BreakIfPossible( CEntity const * entity )
{
	Bool anyBrokeOut = false;
	Bool master = true;
	const TDynArray< SAnimationSequenceInstance >::iterator end = m_sequenceInstances.End();
	for( TDynArray< SAnimationSequenceInstance >::iterator it = m_sequenceInstances.Begin(); it != end; ++it )
	{
		if (it->m_entity.Get() == entity)
		{
			anyBrokeOut |= it->BreakIfPossible(master, *this);
		}
		master = false;
	}
	return anyBrokeOut;
}

CEntity const * CAnimationManualSlotSyncInstance::GetOtherEntity( Bool isMaster ) const
{
	return m_sequenceInstances[ isMaster? 1 : 0 ].m_entity.Get();
}

//////////////////////////////////////////////////////////////////////////

void CAnimationManualSlotSyncInstance::funcRegisterMaster( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimationSequenceDefinition, def, SAnimationSequenceDefinition() );
	FINISH_PARAMETERS;

	if( RegisterMaster( def ) )
	{
		// Master is always first
		RETURN_INT( 0 );
	}
	else
	{
		RETURN_INT( -1 );
	}
}

void CAnimationManualSlotSyncInstance::funcRegisterSlave( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimationSequenceDefinition, def, SAnimationSequenceDefinition() );
	FINISH_PARAMETERS;

	if( RegisterSlave( def ) )
	{
		RETURN_INT( m_sequenceInstances.Size() - 1 );
	}
	else
	{
		RETURN_INT( -1 );
	}
}

void CAnimationManualSlotSyncInstance::funcStopSequence( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, index, (Uint32)-1 );
	FINISH_PARAMETERS;

	if ( index < m_sequenceInstances.Size() )
	{
		m_sequenceInstances[index].Stop();
	}
}

void CAnimationManualSlotSyncInstance::funcIsSequenceFinished( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, index, (Uint32)-1 );
	FINISH_PARAMETERS;

	if ( index < m_sequenceInstances.Size() )
	{
		const SAnimationSequenceInstance& instance = m_sequenceInstances[index];

		RETURN_BOOL( ( instance.m_currentPartIndex + 1 ) > (Int32)instance.m_parts.Size() );
	}
	else
	{
		RETURN_BOOL( true );
	}
}

void CAnimationManualSlotSyncInstance::funcHasEnded( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( HasEnded() );
}

void CAnimationManualSlotSyncInstance::funcUpdate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, time, 0.f );
	FINISH_PARAMETERS;

	Update( time );
}

void CAnimationManualSlotSyncInstance::funcBreakIfPossible( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, entity, NULL );
	FINISH_PARAMETERS;

	RETURN_BOOL( BreakIfPossible( entity.Get() ) );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 


void CAnimationManualSlotSyncInstance::SAnimationSequenceInstance::Stop()
{
	m_slot.Stop();
	m_slot.Clear();
	OnSwitchPart( NULL );
}

Bool CAnimationManualSlotSyncInstance::SAnimationSequenceInstance::HasEnded() const
{
	return !m_slot.IsValid() || ( m_currentPartIndex+1 > (Int32)m_parts.Size() ) || m_brokeOut;
}

Bool CAnimationManualSlotSyncInstance::SAnimationSequenceInstance::BreakIfPossible( Bool isMaster, CAnimationManualSlotSyncInstance& syncInstance )
{
	if ( HasEnded() )
	{
		return true;
	}
	SAnimationSequencePartInstance& part = m_parts[ m_currentPartIndex ];
	if ( HasStarted( syncInstance.GetCurrentTime() ) && ! part.CanBreakOut() ) // allow breaking out when hasn't started
	{
		return false;
	}
	m_brokeOut = true;
	if ( ! RaiseEndEvent() )
	{
		if ( m_slot.IsValid() && m_slot.IsActive() )
		{
			// blend out from what we are doing now and continue playing
			m_slot.BlendOut( 0.3f, true );
		}
	}
	// this may end up with calling master->slave->master but it will stop there as we will break out or won't be able to
	if ( isMaster )
	{
		syncInstance.BreakSlavesIfPossible();
	}
	else
	{
		syncInstance.BreakMasterIfPossible();
	}
	return true;
}

void CAnimationManualSlotSyncInstance::SAnimationSequenceInstance::Update( Float currentTime, Float deltaTime, Bool isMaster, CAnimationManualSlotSyncInstance& syncInstance )
{
	if( HasEnded() )
	{
		return;
	}

	SAnimationSequencePartInstance& part = m_parts[ m_currentPartIndex ];
	SAnimationSequencePartInstance* nextPart = NULL;
	Int32 nextPartIndex = m_currentPartIndex + 1;
	if( nextPartIndex < (Int32)m_parts.Size() && ( m_parts[ nextPartIndex ].m_sequenceIndex - part.m_sequenceIndex ) == 1 )
	{
		nextPart = &( m_parts[ nextPartIndex ] );
	}

	part.Update( currentTime, deltaTime, isMaster, syncInstance, *this, nextPart );
}

Bool CAnimationManualSlotSyncInstance::SAnimationSequenceInstance::RaiseEndEvent()
{
	Bool result = false;
	if( ! m_endEventRaised )
	{
		if ( CEntity* entity = m_entity.Get() )
		{
			entity->RaiseBehaviorEvent( m_raiseEventOnEnd );
			result = true;
		}
		m_endEventRaised = true;
	}
	if( ! m_endForceEventRaised )
	{
		if ( CEntity* entity = m_entity.Get() )
		{
			entity->RaiseBehaviorForceEvent( m_raiseForceEventOnEnd );
			result = true;
		}
		m_endForceEventRaised = true;
	}
	return result;
}

Bool CAnimationManualSlotSyncInstance::SAnimationSequenceInstance::HasStarted( Float currentTime ) const
{
	return ! m_parts.Empty() && currentTime >= m_parts[0].m_startTime;
}

void CAnimationManualSlotSyncInstance::SAnimationSequenceInstance::OnSwitchPart( SAnimationSequencePartInstance* toPart )
{
	Bool shouldDisableProxyCollisions = toPart? toPart->m_disableProxyCollisions : false;

	if ( m_disabledProxyCollisions != shouldDisableProxyCollisions )
	{
		if ( CActor* actor = Cast< CActor >( m_entity.Get() ) )
		{
			if ( CMovingPhysicalAgentComponent* movPhysAC = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() ) )
			{
				movPhysAC->EnableCharacterCollisions( !shouldDisableProxyCollisions );
			}
		}

		m_disabledProxyCollisions = shouldDisableProxyCollisions;
	}

	// stop any movement adjustment
	if ( m_movementAdjustTicket.IsValid() )
	{
		CActor* actor = Cast<CActor>( m_entity.Get() );
		if( actor )
		{
			if( CMovingAgentComponent* mac = actor->GetMovingAgentComponent() )
			{
				CMovementAdjustor* ma = mac->GetMovementAdjustor();
				ma->Cancel( m_movementAdjustTicket );
				m_movementAdjustTicket = SMovementAdjustmentRequestTicket::Invalid();
			}
		}
	}
}

Bool CAnimationManualSlotSyncInstance::SAnimationSequencePartInstance::CanBreakOut() const
{
	return m_animState.m_currTime < m_allowBreakAtStart ||
		   m_animState.m_currTime > m_endTime - m_startTime - m_allowBreakBeforeEnd;
}

Float CAnimationManualSlotSyncInstance::SAnimationSequencePartInstance::CalculateAnimationBlend()
{
	if( m_animState.m_currTime < m_blendInTime )
	{
		return m_animState.m_currTime / m_blendInTime;
	}
	else if( m_blendOutTime > 0.f )
	{
		const Float blendOutStart = m_endTime - m_startTime - m_blendOutTime;
		if( m_animState.m_currTime > blendOutStart )
		{
			return 1.f - ( (m_animState.m_currTime - blendOutStart) / m_blendOutTime );
		}
	}

	return 1.f;
}

void CAnimationManualSlotSyncInstance::SAnimationSequencePartInstance::Update( Float currentTime, Float deltaTime, Bool isMaster, CAnimationManualSlotSyncInstance& syncInstance, SAnimationSequenceInstance& sequence, SAnimationSequencePartInstance* nextPart )
{
	if ( sequence.m_hasFinished )
	{
		return;
	}

	CEntity* entity = sequence.m_entity.Get();
	if (!entity)
	{
		return;
	}

	const Float prevTime = currentTime - deltaTime;

	if( m_animState.m_currTime == 0.f )
	{
		if( currentTime >= m_startTime )
		{
			m_animState.m_currTime += deltaTime;

			if( ! sequence.m_startForceEventSent )
			{
				ASSERT( ! sequence.m_startForceEvent.Empty(), TXT("We shouldn't be here if there is no force event to raise") );
				if ( entity )
				{
					entity->RaiseBehaviorForceEvent( sequence.m_startForceEvent );
				}
				sequence.m_startForceEventSent = true;
			}

			if( nextPart )
			{
				sequence.OnSwitchPart( nextPart );
				Float blendWeight = 0.f;
				const Float timeToEnd = m_endTime - m_startTime - m_animState.m_currTime;
				if( m_blendTransitionTime > 0.f && timeToEnd < m_blendTransitionTime )
				{
					blendWeight = 1.f - (timeToEnd / m_blendTransitionTime);

					nextPart->m_animState.m_prevTime = nextPart->m_animState.m_currTime;
					nextPart->m_animState.m_currTime += deltaTime;
				}

				sequence.m_slot.PlayAnimations( m_animState, nextPart->m_animState, blendWeight, CalculateAnimationBlend() );
			}
			else
			{
				sequence.OnSwitchPart( &( sequence.m_parts[ sequence.m_currentPartIndex ] ) );
				sequence.m_slot.PlayAnimation( m_animState, CalculateAnimationBlend() );
			}
		}
	}
	else
	{
		if( ! sequence.m_slot.IsActive() )
		{
			// this should bring us to last part and will mark us as inactive stopping any playback
			currentTime = m_endTime;
			nextPart = NULL;
			// clear slaves if they haven't started
			if ( isMaster )
			{
				syncInstance.RemoveSlavesIfNotStarted();
				syncInstance.BreakSlavesIfPossible(); // break if it is possible
			}
		}
		if( currentTime >= m_endTime )
		{
			// Stop or freeze only if next part doesn't exist
			if( !nextPart )
			{
				if( !sequence.m_freezeAtEnd )
				{
					sequence.m_slot.Stop();
				}
				else
				{
					// Make sure that the last frame's dt is 0
					m_animState.m_prevTime = m_animState.m_currTime;

					// Assuming that the only proper way of freezing last frame is with weight = 1
					sequence.m_slot.PlayAnimation( m_animState );
				}

				// we reached end - send end event
				sequence.RaiseEndEvent();

				sequence.m_hasFinished = true;

				sequence.OnSwitchPart( NULL );
			}
			else
			{
				sequence.OnSwitchPart( nextPart );
			}

			++sequence.m_currentPartIndex;
		}
		else
		{
			m_animState.m_prevTime = m_animState.m_currTime;
			m_animState.m_currTime += deltaTime;

			if( nextPart )
			{
				Float blendWeight = 0.f;
				const Float timeToEnd = m_endTime - m_startTime - m_animState.m_currTime;
				if( m_blendTransitionTime > 0.f && timeToEnd < m_blendTransitionTime )
				{
					blendWeight = 1.f - (timeToEnd / m_blendTransitionTime);

					nextPart->m_animState.m_prevTime = nextPart->m_animState.m_currTime;
					nextPart->m_animState.m_currTime += deltaTime;
				}

				sequence.m_slot.PlayAnimations( m_animState, nextPart->m_animState, blendWeight, CalculateAnimationBlend() );
			}
			else
			{
				sequence.m_slot.PlayAnimation( m_animState, CalculateAnimationBlend() );
			}
		}
	}

	if( currentTime > m_movementStartTime && prevTime <= m_movementStartTime && m_movementDuration > 0.f )
	{
		CActor* actor = Cast<CActor>( entity );
		if( actor )
		{
			if( CMovingAgentComponent* mac = actor->GetMovingAgentComponent() )
			{
				CMovementAdjustor* ma = mac->GetMovementAdjustor();
				SMovementAdjustmentRequest* request = ma->CreateNewRequest( CNAME( SlideSyncAdjustment ) );
				// store ticket for future reference
				sequence.m_movementAdjustTicket = request->GetTicket();
				// as simple as that - bind to event and setup slide/rotation
				request->BindToEvent( CNAME( AllowSlide ), true );
				if ( ! m_useRefBone.Empty() )
				{
					if ( CEntity const * otherEntity = syncInstance.GetOtherEntity( isMaster) )
					{
						// use ref bone
						request->UseBoneForAdjustment(m_useRefBone, false, 1.0f, 1.0f, m_rotationTypeUsingRefBone == SRT_TowardsOtherEntity? 0.0f : 1.0f );
						if ( m_shouldSlide )
						{
							request->SlideTowards( otherEntity );
						}
						if ( m_shouldRotate )
						{
							request->RotateTowards( otherEntity );
						}
					}
				}
				else
				{
					if ( m_shouldSlide )
					{
						request->SlideTo( m_finalPosition );
					}
					if ( m_shouldRotate )
					{
						request->RotateTo( m_finalHeading );
					}
				}
			}
			else
			{
				actor->ActionSlideTo( m_finalPosition, m_finalHeading, m_movementDuration, SR_Nearest );
			}
		}
	}
}
