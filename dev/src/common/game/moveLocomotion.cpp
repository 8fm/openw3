#include "build.h"
#include "moveLocomotion.h"

#include "../core/feedback.h"

#include "moveLocomotionSegment.h"
#include "movementTargeter.h"
#include "moveStateModifier.h"
#include "moveSteeringLocomotionSegment.h"


///////////////////////////////////////////////////////////////////////////////

CMoveLocomotion::CMoveLocomotion( CMovingAgentComponent& agent )
	: m_agent( agent )
	, m_controller( NULL )
	, m_idleSegment( new CMoveLSSteering( false ) )
	, m_activeSegment( NULL )
	, m_wasSegmentActivated( false )
	, m_listeners()
{
	m_idleSegment->IncludeStaticTargeters( true );
}

CMoveLocomotion::~CMoveLocomotion()
{
	if ( m_controller )
	{
		m_controller->OnControlDenied( *this );
		m_controller = NULL;
	}

	CancelMove();

	if ( m_idleSegment )
	{
		m_idleSegment->Deactivate( m_agent );
		m_idleSegment->Release();
		m_idleSegment = NULL;
	}
}

void CMoveLocomotion::OnSerialize( IFile& file )
{
	for ( TList< IMoveLocomotionSegment* >::iterator it = m_segsQueue.Begin(); it != m_segsQueue.End(); ++it )
	{
		if ( *it )
		{
			(*it)->OnSerialize( file );
		}
	}
}

void CMoveLocomotion::Tick( Float timeDelta )
{
#ifndef NO_DEBUG_PAGES
	// Measure locomotion update time
	CTimeCounter timer;
#endif

	ProcessLocomotion( timeDelta );

#ifndef NO_DEBUG_PAGES
	// Was it slow ?
	const Float timeElapsed = timer.GetTimePeriod();
	if ( timeElapsed > 0.002f )
	{
		GScreenLog->PerfWarning( timeElapsed, TXT("LOCOMOTION"), TXT("Slow locomotion update for agent '%ls'"), m_agent.GetEntity()->GetFriendlyName().AsChar() );
	}
#endif
}

// ----------------------------------------------------------------------------
// Locomotion commands
// ----------------------------------------------------------------------------

CMoveLSSteering& CMoveLocomotion::PushSteeringSegment( Bool switchToEntityRepresentation )
{
	CMoveLSSteering* segment = new CMoveLSSteering( switchToEntityRepresentation );
	PushSegment( segment );	
	return *segment;
}

void CMoveLocomotion::PushSegment( IMoveLocomotionSegment* segment )
{
	ASSERT( segment );
	if ( !segment )
	{
		return;
	}

	// push it onto the stack
	m_segsQueue.PushBack( segment );
	for ( Uint32 i = 0; i < m_listeners.Size(); ++i )
	{
		m_listeners[ i ]->OnSegmentPushed( segment );
	}
}

void CMoveLocomotion::PushStateModifier( IMoveStateModifier* modifier )
{
	ASSERT( modifier );
	if ( !modifier )
	{
		return;
	}

	// add the modifier to the list of modifiers
	m_modifiers.PushBack( modifier );

	// immediately activate the modifier
	modifier->Activate( m_agent );
}

void CMoveLocomotion::CancelCurrentCommand()
{
	// deactivate the active segment, if there's one
	if ( m_activeSegment && m_activeSegment != m_idleSegment )
	{
		m_activeSegment->Deactivate( m_agent );
		m_activeSegment = nullptr;

		// don't delete the segment here - it will be deleted in a sec when we
		// delete the contents of the segments queue
	}

	// clear locomotion segments
	for ( TList< IMoveLocomotionSegment* >::iterator it = m_segsQueue.Begin(); it != m_segsQueue.End(); ++it )
	{
		if ( *it )
		{
			(*it)->Release();
		}
	}
	m_segsQueue.Clear();

	// deactivate and release the state modifiers
	DeactivateStateModifiers();

	// inform the controller that the command it issued has been canceled
	if ( m_controller )
	{
		m_controller->OnSegmentFinished( MS_Failed );
	}
}

// ----------------------------------------------------------------------------
// Locomotion processing
// ----------------------------------------------------------------------------

IMoveLocomotionSegment* CMoveLocomotion::GetCurrentSegment()
{
	return m_segsQueue.Empty() ? m_idleSegment : m_segsQueue.Front();
}

Bool CMoveLocomotion::PopSegment()
{
	Bool keepProcessing = true;

	if ( !m_segsQueue.Empty() )
	{
		ASSERT( m_activeSegment == m_segsQueue.Front() );
		m_segsQueue.PopFront();

		if ( m_activeSegment && m_activeSegment != m_idleSegment )
		{
			m_activeSegment->Deactivate( m_agent );
			m_activeSegment->Release();
			m_activeSegment = NULL;
		}
	}
	
	if ( m_segsQueue.Empty() )
	{
		DeactivateStateModifiers();

		// notify the controller that the movement command's been processed - maybe we'll get more
		// to chew on
		if ( m_controller )
		{
			m_controller->OnSegmentFinished( MS_Completed );
		}
		keepProcessing = !m_segsQueue.Empty();
	}

	return keepProcessing;
}

void CMoveLocomotion::DeactivateStateModifiers()
{
	// deactivate and release all state modifiers
	for ( TList< IMoveStateModifier* >::iterator it = m_modifiers.Begin(); it != m_modifiers.End(); ++it )
	{
		if ( *it )
		{
			(*it)->Deactivate( m_agent );
			(*it)->Release();
		}
	}
	m_modifiers.Clear();
}

void CMoveLocomotion::OnSteeringBehaviorChanged( CMovingAgentComponent* owner, CMoveSteeringBehavior* prevSteeringGraph, InstanceBuffer* prevRuntimeData, CMoveSteeringBehavior* newSteeringGraph, InstanceBuffer* newRuntimeData )
{
	if( prevSteeringGraph == newSteeringGraph && prevRuntimeData == newRuntimeData )
	{
		return;
	}
	IMoveLocomotionSegment* segment = GetCurrentSegment();
	if( segment )
	{
		segment->OnSteeringBehaviorChanged( owner, prevSteeringGraph, prevRuntimeData, newSteeringGraph, newRuntimeData );
	}

}

void CMoveLocomotion::ProcessLocomotion( Float timeDelta )
{
	static Bool continueProcessing = true;
	continueProcessing = true;

	while ( continueProcessing )
	{
		IMoveLocomotionSegment* segment = GetCurrentSegment();

		if ( segment != m_activeSegment )
		{
			m_activeSegment			= segment;
			m_wasSegmentActivated	= false;
		}

		// activate an inactive segment
		if ( m_activeSegment && !m_wasSegmentActivated )
		{
			m_wasSegmentActivated						= m_activeSegment->Activate( m_agent );
		}

		continueProcessing = false;
		// we can move on only if we have an active segment
		if ( m_activeSegment && m_wasSegmentActivated )
		{
			// tick the active segment
			ELSStatus status = m_activeSegment->Tick( timeDelta, m_agent );
			switch( status ) 
			{
			case LS_InProgress:
				{
					break;
				}
			case LS_Completed:		
				{
					continueProcessing = PopSegment();
					break;
				}
			case LS_Failed:			
				{
					// the segment failed
					CancelMove();
					break;
				}
			}
		}
	}
}

void CMoveLocomotion::CancelMove()
{
	CancelCurrentCommand();
}

Bool CMoveLocomotion::CanCancelMovement() const
{
	if ( !m_segsQueue.Empty() )
	{
		return m_segsQueue.Front()->CanBeCanceled();
	}
	else
	{
		return true;
	}
}

void CMoveLocomotion::AttachController( IController& controller )
{
	if ( m_controller )
	{
		m_controller->OnControlDenied( *this );
	}
	m_controller = &controller;
	CancelMove();
	if ( m_controller )
	{
		m_controller->OnControlGranted( *this );
	}
}

void CMoveLocomotion::DetachController( IController& controller )
{
	if ( m_controller == &controller )
	{
		m_controller = NULL;

		CancelMove();
		controller.OnControlDenied( *this );
	}
}

void CMoveLocomotion::OnControllerDetached()
{
	m_controller = NULL;
	CancelMove();
}
void CMoveLocomotion::AddTargeter_MoveLSSteering(  IMovementTargeter *const targeter )
{
	// Ading the targetter to each segment :
	for( TList< IMoveLocomotionSegment* >::iterator segmentIter = m_segsQueue.Begin();
		segmentIter != m_segsQueue.End(); ++segmentIter )
	{
		IMoveLocomotionSegment* segment = *segmentIter;
		CMoveLSSteering *const moveLSSteering = segment->AsCMoveLSSteering( );
		if ( moveLSSteering )
		{
			moveLSSteering->AddTargeter( targeter );
		}
		
	}
}
void CMoveLocomotion::RemoveTargeter_MoveLSSteering(  IMovementTargeter *const targeter )
{
	// Ading the targetter to each segment :
	for( TList< IMoveLocomotionSegment* >::iterator segmentIter = m_segsQueue.Begin();
		segmentIter != m_segsQueue.End(); ++segmentIter )
	{
		IMoveLocomotionSegment* segment = *segmentIter;
		CMoveLSSteering *const moveLSSteering = segment->AsCMoveLSSteering( );
		if ( moveLSSteering )
		{
			moveLSSteering->RemoveTargeter( targeter );
		}
		
	}
}
void CMoveLocomotion::AddMoveLocomotionListener( IMoveLocomotionListener *const listener )
{
	m_listeners.PushBackUnique( listener );
}
void CMoveLocomotion::RemoveMoveLocomotionListener( IMoveLocomotionListener *const listener )
{
	m_listeners.Remove( listener );
}
// ----------------------------------------------------------------------------
// Debugging
// ----------------------------------------------------------------------------

void CMoveLocomotion::GenerateDebugFragments( CRenderFrame* frame )
{
	// draw the locomotion path
	if ( !m_segsQueue.Empty() )
	{
		m_segsQueue.Front()->GenerateDebugFragments( m_agent, frame );
	}
}

void CMoveLocomotion::GenerateDebugPage( TDynArray< String >& debugLines ) const
{
	for( TList< IMoveLocomotionSegment* >::const_iterator segmentIter = m_segsQueue.Begin();
		segmentIter != m_segsQueue.End(); ++segmentIter )
	{
		const IMoveLocomotionSegment* segment = *segmentIter;
		segment->GenerateDebugPage( debugLines );
	}
}

///////////////////////////////////////////////////////////////////////////////

CMoveLocomotion::IController::IController()
: m_locomotion( NULL )
{
}

CMoveLocomotion::IController::~IController()
{
	if ( m_locomotion )
	{
		m_locomotion->OnControllerDetached();
		m_locomotion = NULL;
	}
}

void CMoveLocomotion::IController::OnControlGranted( CMoveLocomotion& locomotion )
{
	ASSERT( m_locomotion != &locomotion );
	if ( m_locomotion == &locomotion )
	{
		return;
	}

	// we can't get reassigned
	ASSERT( !m_locomotion, TXT( "Can't reassign a CMoveLocomotion controller behind host CMoveLocomotion's back" ) );
	if ( m_locomotion )
	{
		ERR_GAME( TXT( "Can't reassign a CMoveLocomotion controller behind host CMoveLocomotion's back" ) );
		return;
	}
	m_locomotion = &locomotion;
	OnControllerAttached();
}

void CMoveLocomotion::IController::OnControlDenied( CMoveLocomotion& locomotion )
{
	ASSERT( m_locomotion == &locomotion );
	if ( m_locomotion == &locomotion )
	{
		OnControllerDetached();
		m_locomotion = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
