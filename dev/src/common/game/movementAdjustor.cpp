/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "actor.h"

#include "movementAdjustor.h"
#include "movingAgentComponent.h"
#include "..\engine\behaviorGraphUtils.inl"
#include "..\engine\skeleton.h"

///////////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR
// this should be enabled only when looking for certain bugs. I know that designers wanted it to be always available but as it puts lots of lines and (...), I decided to disable it and help designers to find/solve bugs
//#define DEBUG_MOVEMENT_ADJUSTOR 1
//#define DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME 1
#endif

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( EMovementAdjustmentNotify );
IMPLEMENT_ENGINE_CLASS( CMovementAdjustor );
IMPLEMENT_ENGINE_CLASS( SMovementAdjustmentRequestTicket );

///////////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( MovementAdjustor );
RED_DEFINE_STATIC_NAMED_NAME( syncPointEvent, "AdjustmentSyncPoint" );

CMovementAdjustor::CMovementAdjustor()
:	m_tighteningRatio( 0.0f )
,	m_oneFrameTranslationVelocity( Vector::ZEROS )
,	m_oneFrameRotationVelocity( EulerAngles::ZEROS )
{

}

CMovementAdjustor::~CMovementAdjustor()
{
	CancelAll();
}

CMovingAgentComponent* CMovementAdjustor::GetMAC() const
{
	return Cast<CMovingAgentComponent>( GetParent() );
}

void CMovementAdjustor::Tick( SMovementAdjustorContext& context, Float deltaSeconds )
{
	if ( deltaSeconds == 0.0f )
	{
		return;
	}

	context.ClearOutput();

	bool somethingHasFinished = false;
	for ( TDynArray< SMovementAdjustmentRequest >::iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		SMovementAdjustmentRequest& element = *it;
		element.Tick( context, deltaSeconds );
		if ( element.HasFinished() )
		{
			somethingHasFinished = true;
			element.CallEvent( MAN_AdjustmentEnded );
		}
	}

	if ( somethingHasFinished )
	{
		// do this only if something has finished
		for ( Int32 idx = 0; idx < m_requests.SizeInt(); ++ idx )
		{
			if ( m_requests[idx].HasFinished() )
			{
#if DEBUG_MOVEMENT_ADJUSTOR
				RED_LOG( MovementAdjustor, TXT("%s: Finished %s"), GetMAC()? GetMAC()->GetEntity()->GetName().AsChar() : TXT("--"), m_requests[idx].m_name.AsChar() );
				m_requests[idx].LogReasonOfFinished();
#endif
				m_requests[idx].FinalizeRequest();
				m_requests.RemoveAtFast( idx );
				m_cachedRequest = NULL;
				-- idx;
			}
		}
	}

	context.FinalizeAdjustments();

	// process tightening at the end - after adjustments are finalized
	if ( m_tighteningRatio > 0.0f )
	{
		// make it be the same
		Vector locationAdjustment = context.m_currentDeltaLocation;
		EulerAngles rotationAdjustment = context.m_currentDeltaRotation;

		Vector currentLocation = context.m_currentLocation;
		Float currentYaw = context.m_currentRotation.Yaw;

		// calculate required adjustment for location
		// we want to get as close to follow point on the line as possible
		//
		//	^ tightening dir
		//	|
		//	o follow point
		//	|
		//	|
		//	* target loc
		//	|\
		//	| o current loc
		//	|

		Vector tighteningDir = EulerAngles::YawToVector( m_tighteningDir );
		Vector tighteningPerpendicular( tighteningDir.Y, -tighteningDir.X, 0.0f );
		Vector toFollowPoint = m_tighteningFollowPoint - currentLocation;
		toFollowPoint.Z = 0.0f;
		Float distToTightening = tighteningPerpendicular.Dot2( toFollowPoint );
		Float currDelta = context.m_currentDeltaLocation.Mag2();
		// "pythagoras, my old friend!"
		// if we're close enough (currDelta <= distToTightening) we will end moving on the path further, otherwise we will be moving towards path (perpendicular to it)
		Float onTightening = MSqrt( Max( 0.0f, currDelta * currDelta - distToTightening * distToTightening ) );
		locationAdjustment = -tighteningPerpendicular * Min( currDelta, distToTightening ) + tighteningDir * onTightening;

		// keep vertical movement
		locationAdjustment.Z = context.m_currentDeltaLocation.Z;

		// calculate required adjustment for rotation
		rotationAdjustment.Yaw = EulerAngles::NormalizeAngle180( m_tighteningDir - currentYaw );

		// apply
		Float invTighteningRatio = 1.0f - m_tighteningRatio;
		context.m_outputDeltaLocation = context.m_outputDeltaLocation * invTighteningRatio + locationAdjustment * m_tighteningRatio;
		context.m_outputDeltaRotation = context.m_outputDeltaRotation * invTighteningRatio + rotationAdjustment * m_tighteningRatio;

		// reset tightening
		m_tighteningRatio = 0.0f;
	}

	// add one frame translation/rotation and reset them
	context.m_outputDeltaLocation += m_oneFrameTranslationVelocity * deltaSeconds;
	context.m_outputDeltaRotation += m_oneFrameRotationVelocity * deltaSeconds;
	m_oneFrameTranslationVelocity = Vector::ZEROS;
	m_oneFrameRotationVelocity = EulerAngles::ZEROS;
}

void CMovementAdjustor::TightenMovementTo( const Vector2& followPoint, Float tighteningDir, Float tighteningRatio )
{
	m_tighteningFollowPoint = followPoint;
	m_tighteningDir = tighteningDir;
	m_tighteningRatio = tighteningRatio;
}

void CMovementAdjustor::GenerateDebugFragments( CRenderFrame* frame, SMovementAdjustorContext& context )
{
	Uint32 line = 1;
	for ( TDynArray< SMovementAdjustmentRequest >::iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		it->GenerateDebugFragments( frame, context, line );
	}
}

SMovementAdjustmentRequest* CMovementAdjustor::CreateNewRequest( CName name )
{
#if DEBUG_MOVEMENT_ADJUSTOR
	RED_LOG( MovementAdjustor, TXT("%s: Create new request %s"), GetMAC()? GetMAC()->GetEntity()->GetName().AsChar() : TXT("--"), name.AsChar() );
#endif
	m_requests.PushBack( SMovementAdjustmentRequest( this, name, GetNextID() ) );
	m_cachedRequest = &( m_requests[m_requests.Size() - 1]);
	return m_cachedRequest;
}

void CMovementAdjustor::Cancel( const SMovementAdjustmentRequestTicket& requestTicket )
{
	for ( TDynArray< SMovementAdjustmentRequest >::iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		if ( it->m_id == requestTicket.m_id )
		{
#if DEBUG_MOVEMENT_ADJUSTOR
			RED_LOG( MovementAdjustor, TXT("%s: Cancelled %s"), GetMAC()? GetMAC()->GetEntity()->GetName().AsChar() : TXT("--"), it->m_name.AsChar() );
#endif
			it->CallEvent( MAN_AdjustmentCancelled );
			it->Cancel();
			m_cachedRequest = NULL;
			break;
		}
	}
}

void CMovementAdjustor::CancelByName( const CName& requestName )
{
	for ( TDynArray< SMovementAdjustmentRequest >::iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		if ( it->m_name == requestName )
		{
#if DEBUG_MOVEMENT_ADJUSTOR
			RED_LOG( MovementAdjustor, TXT("%s: Cancelled by name %s"), GetMAC()? GetMAC()->GetEntity()->GetName().AsChar() : TXT("--"), it->m_name.AsChar() );
#endif
			it->CallEvent( MAN_AdjustmentCancelled );
			it->Cancel();
			m_cachedRequest = NULL;
		}
	}
}

void CMovementAdjustor::CancelAll()
{
#if DEBUG_MOVEMENT_ADJUSTOR
	Bool thereWereRequests = ! m_requests.Empty();
#endif
	for ( TDynArray< SMovementAdjustmentRequest >::iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		it->CallEvent( MAN_AdjustmentCancelled );
		it->Cancel();
	}
	m_requests.ClearFast();
	m_cachedRequest = NULL;
	m_oneFrameTranslationVelocity = Vector::ZEROS;
	m_oneFrameRotationVelocity = EulerAngles::ZEROS;
#if DEBUG_MOVEMENT_ADJUSTOR
	if (thereWereRequests)
	{
		RED_LOG( MovementAdjustor, TXT("%s: Cancelled all"), GetMAC() && GetMAC()->GetEntity()? GetMAC()->GetEntity()->GetName().AsChar() : TXT("--") );
	}
#endif
}

MoveAdjustmentID CMovementAdjustor::GetNextID()
{
	while ( IsIDUsed( m_nextID ) )
	{
		++ m_nextID;
	}

	return m_nextID ++; // post increment
}

Bool CMovementAdjustor::IsIDUsed( MoveAdjustmentID id )
{
	for ( TDynArray< SMovementAdjustmentRequest >::const_iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		if ( it->m_id == id )
		{
			return true;
		}
	}
	return id == 0;
}

SMovementAdjustmentRequest* CMovementAdjustor::GetRequest( CName name )
{
	if (m_cachedRequest && m_cachedRequest->m_name == name )
	{
		return m_cachedRequest;
	}
	for ( TDynArray< SMovementAdjustmentRequest >::iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		if ( it->m_name == name )
		{
			m_cachedRequest = &( *it );
			return &(*m_cachedRequest);
		}
	}
	return NULL;
}

SMovementAdjustmentRequest* CMovementAdjustor::GetRequest( const SMovementAdjustmentRequestTicket& requestTicket )
{
	if (m_cachedRequest && m_cachedRequest->m_id == requestTicket.m_id )
	{
		return m_cachedRequest;
	}
	for ( TDynArray< SMovementAdjustmentRequest >::iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		if ( it->m_id == requestTicket.m_id )
		{
			m_cachedRequest = &( *it );
			return &(*m_cachedRequest);
		}
	}
	return NULL;
}

Bool CMovementAdjustor::IsRequestActive( const SMovementAdjustmentRequestTicket& requestTicket ) const
{
	if (m_cachedRequest && m_cachedRequest->m_id == requestTicket.m_id )
	{
		return true;
	}
	for ( TDynArray< SMovementAdjustmentRequest >::const_iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		if ( it->m_id == requestTicket.m_id )
		{
			return true;
		}
	}
	return false;
}

Bool CMovementAdjustor::HasAnyActiveRotationRequests() const
{
	for ( TDynArray< SMovementAdjustmentRequest >::const_iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		if ( it->HasRotationAdjustment() )
		{
			return true;
		}
	}
	return false;
}

Bool CMovementAdjustor::HasAnyActiveTranslationRequests() const
{
	for ( TDynArray< SMovementAdjustmentRequest >::const_iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		if ( it->HasLocationAdjustment() )
		{
			return true;
		}
	}
	return false;
}

void CMovementAdjustor::BindRequestToEvent( SMovementAdjustmentRequest* request, const CName & eventName )
{
	ASSERT( ! request->IsBoundedToEvent( eventName ), TXT("Request should not be bound at this time. Yet.") );
	for ( TDynArray< SMovementAdjustmentRequest >::const_iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		if ( it->IsBoundedToEvent( eventName ) )
		{
			// already bound
			return;
		}
	}

	if ( CMovingAgentComponent* mac = GetMAC() )
	{
		mac->GetAnimationEventNotifier( eventName )->RegisterHandler( this );
	}
}

void CMovementAdjustor::UnbindRequestFromEvent( SMovementAdjustmentRequest* request, const CName & eventName )
{
	ASSERT( ! request->IsBoundedToEvent( eventName ), TXT("Request should not be bound at this time. Already.") );
	for ( TDynArray< SMovementAdjustmentRequest >::const_iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		if ( it->IsBoundedToEvent( eventName ) )
		{
			// something else still bound
			return;
		}
	}

	if ( CMovingAgentComponent* mac = GetMAC() )
	{
		mac->GetAnimationEventNotifier( eventName )->UnregisterHandler( this );
	}
}

void CMovementAdjustor::HandleEvent( const CAnimationEventFired &event )
{
	for ( TDynArray< SMovementAdjustmentRequest >::iterator it = m_requests.Begin(); it != m_requests.End(); ++ it )
	{
		if ( it->IsBoundedToEvent( event.GetEventName() ) )
		{
			it->HandleEvent( event );
		}
	}
}

void CMovementAdjustor::AddOneFrameTranslationVelocity(Vector const & _translation)
{
	m_oneFrameTranslationVelocity += _translation;
}

void CMovementAdjustor::AddOneFrameRotationVelocity(EulerAngles const & _rotation)
{
	m_oneFrameRotationVelocity += _rotation;
}

// -------------------------------------------------------
// ------------------- scripting support -----------------
// -------------------------------------------------------

void CMovementAdjustor::funcIsRequestActive( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsRequestActive( ticket ) );
}

void CMovementAdjustor::funcHasAnyActiveRequest( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( HasAnyActiveRequest() );
}

void CMovementAdjustor::funcHasAnyActiveRotationRequests( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( HasAnyActiveRotationRequests() );
}

void CMovementAdjustor::funcHasAnyActiveTranslationRequests( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( HasAnyActiveTranslationRequests() );
}

void CMovementAdjustor::funcCancel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	FINISH_PARAMETERS;

	Cancel( ticket );
}

void CMovementAdjustor::funcCancelByName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, requestName, CName::NONE );
	FINISH_PARAMETERS;

	CancelByName( requestName );
}

void CMovementAdjustor::funcCancelAll( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	CancelAll();
}

void CMovementAdjustor::funcCreateNewRequest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_STRUCT( SMovementAdjustmentRequestTicket, CreateNewRequest( name )->GetTicket() );
}

void CMovementAdjustor::funcGetRequest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( name ) )
	{
		RETURN_STRUCT( SMovementAdjustmentRequestTicket, request->GetTicket() );
	}
	else
	{
		RETURN_STRUCT( SMovementAdjustmentRequestTicket, SMovementAdjustmentRequestTicket::Invalid() );
	}
}

void CMovementAdjustor::funcBlendIn( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Float, blendInTime, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->BlendIn(blendInTime);
	}
}

void CMovementAdjustor::funcDontEnd( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->DontEnd();
	}
}

void CMovementAdjustor::funcKeepActiveFor( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Float, keepActiveFor, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->KeepActiveFor( keepActiveFor );
	}
}

void CMovementAdjustor::funcAdjustmentDuration( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Float, duration, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->AdjustmentDuration( duration );
	}
}

void CMovementAdjustor::funcContinuous( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->Continuous();
	}
}

void CMovementAdjustor::funcBaseOnNode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( THandle< CNode >, onNode, NULL );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->BaseOnNode( onNode.Get() );
	}
}

void CMovementAdjustor::funcBindToEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER_OPT( Bool, adjustDurationOnNextEvent, false );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->BindToEvent( eventName, adjustDurationOnNextEvent );
	}
}

void CMovementAdjustor::funcBindToEventAnimInfo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( SAnimationEventAnimInfo, animInfo, SAnimationEventAnimInfo() );
	GET_PARAMETER_OPT( Bool, bindOnly, false );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->BindToEventAnimInfo( animInfo, bindOnly );
	}
}

void CMovementAdjustor::funcScaleAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER_OPT( Bool, scaleAnimation, true );
	GET_PARAMETER_OPT( Bool, scaleLocation, true );
	GET_PARAMETER_OPT( Bool, scaleRotation, false );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->ScaleAnimation( scaleAnimation, scaleLocation, scaleRotation );
	}
}

void CMovementAdjustor::funcScaleAnimationLocationVertically( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Bool, scaleAnimationLocationVertically, true );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->ScaleAnimationLocationVertically( scaleAnimationLocationVertically );
	}
}

void CMovementAdjustor::funcDontUseSourceAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Bool, dontUseSourceAnimation, true );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->DontUseSourceAnimation( dontUseSourceAnimation );
	}
}

void CMovementAdjustor::funcUpdateSourceAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( SAnimationEventAnimInfo, animInfo, SAnimationEventAnimInfo() );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->UpdateSourceAnimation( animInfo );
	}
}

void CMovementAdjustor::funcSyncPointInAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER_OPT( Float, syncPointTime, -1.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->SyncPointInAnimation( syncPointTime );
	}
}

void CMovementAdjustor::funcUseBoneForAdjustment( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER_OPT( CName, boneName, CName::NONE );
	GET_PARAMETER_OPT( Bool, useContinuously, false );
	GET_PARAMETER_OPT( Float, useBoneForLocationAdjustmentWeight, 1.0f );
	GET_PARAMETER_OPT( Float, useBoneForRotationAdjustmentWeight, 0.0f );
	GET_PARAMETER_OPT( Float, useBoneToMatchTargetHeadingWeight, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->UseBoneForAdjustment( boneName, useContinuously, useBoneForLocationAdjustmentWeight, useBoneForRotationAdjustmentWeight, useBoneToMatchTargetHeadingWeight );
	}
}

void CMovementAdjustor::funcCancelIfSourceAnimationUpdateIsNotUpdated( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER_OPT( Bool, cancelIfSourceAnimationUpdateIsNotUpdated, true );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->CancelIfSourceAnimationUpdateIsNotUpdated( cancelIfSourceAnimationUpdateIsNotUpdated );
	}
}

void CMovementAdjustor::funcKeepLocationAdjustmentActive( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->KeepLocationAdjustmentActive();
	}
}

void CMovementAdjustor::funcKeepRotationAdjustmentActive( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->KeepRotationAdjustmentActive();
	}
}

void CMovementAdjustor::funcMatchEntitySlot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( THandle< CEntity >, entity, NULL );
	GET_PARAMETER( CName, slotName, CName::NONE );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->MatchEntitySlot( entity.Get(), slotName );
	}
}

void CMovementAdjustor::funcReplaceTranslation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER_OPT( Bool, replaceTranslation, true );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->ReplaceTranslation( replaceTranslation );
	}
}

void CMovementAdjustor::funcReplaceRotation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER_OPT( Bool, replaceRotation, true );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->ReplaceRotation( replaceRotation );
	}
}

void CMovementAdjustor::funcShouldStartAt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Vector, targetLocation, Vector::ZEROS );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->ShouldStartAt( targetLocation );
	}
}

void CMovementAdjustor::funcSlideTo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Vector, targetLocation, Vector::ZEROS );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->SlideTo( targetLocation );
	}
}

void CMovementAdjustor::funcSlideBy( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Vector, slideByDistance, Vector::ZEROS );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->SlideBy( slideByDistance );
	}
}

void CMovementAdjustor::funcSlideTowards( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( THandle< CNode >, node, NULL );
	GET_PARAMETER_OPT( Float, minDistance, 0.0f );
	GET_PARAMETER_OPT( Float, maxDistance, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->SlideTowards( node.Get(), minDistance, maxDistance );
	}
}

void CMovementAdjustor::funcSlideToEntity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( THandle< CEntity >, entity, NULL );
	GET_PARAMETER_OPT( CName, entityBoneName, CName::NONE );
	GET_PARAMETER_OPT( Float, minDistance, 0.0f );
	GET_PARAMETER_OPT( Float, maxDistance, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->SlideToEntity( entity.Get(), entityBoneName, minDistance, maxDistance );
	}
}

void CMovementAdjustor::funcMaxLocationAdjustmentSpeed( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Float, locationAdjustmentMaxSpeed, 0.0f );
	GET_PARAMETER( Float, locationAdjustmentMaxSpeedZ, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->MaxLocationAdjustmentSpeed( locationAdjustmentMaxSpeed, locationAdjustmentMaxSpeedZ );
	}
}

void CMovementAdjustor::funcMaxLocationAdjustmentDistance( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER_OPT( Bool, throughSpeed, false );
	GET_PARAMETER_OPT( Float, locationAdjustmentMaxDistanceXY, -1.0f );
	GET_PARAMETER_OPT( Float, locationAdjustmentMaxDistanceZ, -1.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->MaxLocationAdjustmentDistance( throughSpeed, locationAdjustmentMaxDistanceXY, locationAdjustmentMaxDistanceZ );
	}
}

void CMovementAdjustor::funcAdjustLocationVertically( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Bool, adjustLocationVertically, true );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->AdjustLocationVertically( adjustLocationVertically );
	}
}

void CMovementAdjustor::funcShouldStartFacing( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Float, targetHeading, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->ShouldStartFacing( targetHeading );
	}
}

void CMovementAdjustor::funcRotateTo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Float, targetHeading, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->RotateTo( targetHeading );
	}
}

void CMovementAdjustor::funcRotateBy( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Float, byHeading, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->RotateBy( byHeading );
	}
}

void CMovementAdjustor::funcRotateTowards( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( THandle< CNode >, node, NULL );
	GET_PARAMETER_OPT( Float, offsetHeading, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->RotateTowards( node.Get(), offsetHeading );
	}
}

void CMovementAdjustor::funcMatchMoveRotation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->MatchMoveRotation();
	}
}

void CMovementAdjustor::funcMaxRotationAdjustmentSpeed( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Float, maxSpeed, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->MaxRotationAdjustmentSpeed( maxSpeed );
	}
}

void CMovementAdjustor::funcSteeringMayOverrideMaxRotationAdjustmentSpeed( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER_OPT( Bool, steeringMayOverrideMaxRotationAdjustmentSpeed, true );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->SteeringMayOverrideMaxRotationAdjustmentSpeed( steeringMayOverrideMaxRotationAdjustmentSpeed );
	}
}

void CMovementAdjustor::funcLockMovementInDirection( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( Float, heading, 0.0f );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->LockMovementInDirection( heading );
	}
}

void CMovementAdjustor::funcRotateExistingDeltaLocation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER_OPT( Bool, rotateExistingDeltaLocation, true );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->RotateExistingDeltaLocation( rotateExistingDeltaLocation );
	}
}

void CMovementAdjustor::funcNotifyScript( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( THandle< IScriptable >, notifyObject, NULL );
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER( EMovementAdjustmentNotify, notify, MAN_None );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->NotifyScript( notifyObject, eventName, notify );
	}
}

void CMovementAdjustor::funcDontNotifyScript( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SMovementAdjustmentRequestTicket, ticket, SMovementAdjustmentRequestTicket::Invalid() );
	GET_PARAMETER( THandle< IScriptable >, notifyObject, NULL );
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER( EMovementAdjustmentNotify, notify, MAN_None );
	FINISH_PARAMETERS;

	if ( SMovementAdjustmentRequest* request = GetRequest( ticket ) )
	{
		request->DontNotifyScript( notifyObject, eventName, notify );
	}
}

void CMovementAdjustor::funcAddOneFrameTranslationVelocity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, translationVelocity, Vector::ZEROS );
	FINISH_PARAMETERS;

	AddOneFrameTranslationVelocity( translationVelocity );
}

void CMovementAdjustor::funcAddOneFrameRotationVelocity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EulerAngles, rotationVelocity, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	AddOneFrameRotationVelocity( rotationVelocity );
}

///////////////////////////////////////////////////////////////////////////////

SMovementAdjustmentNotify::SMovementAdjustmentNotify( const THandle< IScriptable >& callee, const CName& eventName, EMovementAdjustmentNotify notify )
	: m_callee( callee )
	, m_eventName( eventName )
	, m_notify( notify )
{
}

void SMovementAdjustmentNotify::Call( const CName& requestName ) const
{
	if ( IScriptable* callee = m_callee.Get() )
	{
		callee->CallEvent( m_eventName, requestName, m_notify );
	}
}

///////////////////////////////////////////////////////////////////////////////

SMovementAdjustmentRequest::SMovementAdjustmentRequest( CMovementAdjustor* movementAdjustor, CName name, const SMovementAdjustmentRequestTicket& ticket)
	:	m_movementAdjustor( movementAdjustor )
	,	m_name( name )
	,	m_id( ticket.m_id )
	,	m_firstUpdate( true )
	,	m_reachedDestinationOff( 0.1f )
	,	m_reachedRotationOff( 0.1f )
{
	Clear();
}

RED_INLINE void SMovementAdjustmentRequest::LogReasonOfFinished() const
{
#if DEBUG_MOVEMENT_ADJUSTOR
	if ( m_forceFinished )
	{
		if (! m_boundToEvent.Empty())
		{
			RED_LOG( MovementAdjustor, TXT("    : forced finished - was bound to event '%ls'"), m_boundToEvent.AsChar() );
		}
		else
		{
			RED_LOG( MovementAdjustor, TXT("    : forced finished - was bound to played animation") );
		}
	}
	else if ( m_cancelled )
	{
		RED_LOG( MovementAdjustor, TXT("    : was cancelled") );
	}
	else if ( m_adjustmentDuration != 0.0f && m_adjustmentTimeLeft <= 0.0f )
	{
		RED_LOG( MovementAdjustor, TXT("    : adjustmentDuration was set (non zero: %.3f) and we're out of time now (%.3f)"), m_adjustmentDuration, m_adjustmentTimeLeft );
	}
	else if ( m_keepForTimeLeft <= 0.0f )
	{
		RED_LOG( MovementAdjustor, TXT("    : we were meant to keep for specific time and we're out of that time (%.3f)"), m_keepForTimeLeft );
	}
#endif
}

void SMovementAdjustmentRequest::Clear()
{
	m_forceFinished = false;
	m_cancelled = false;
	
	m_locationNeedsUpdate = false;
	m_rotationNeedsUpdate = false;

	m_blendInTime = 0.0f;
	m_timeActive = 0.0f;

	m_dontAutoEnd = false;
	m_adjustmentTimeLeft = 0.0f;
	m_adjustmentDuration = 1.0f;
	m_keepForTimeLeft = 0.0f;
	m_basedOnNode = NULL;
	
	m_locationAdjustmentSoFar = Vector::ZEROS;

	m_boundToEvent = CName::NONE;
	m_handledEvent = false;
	m_deltaSecondsFromEvent = 0.0f;
	m_lastEventTime = -1.0f;

	m_scaleAnimation = false;
	m_scaleAnimationLocation = true;
	m_scaleAnimationRotation = false;
	m_scaleAnimationLocationVertically = false;

	m_dontUseSourceAnimation = false;
	m_sourceAnimation = SAnimationEventAnimInfo();
	m_prevSourceAnimLocalTime = -1.0f;
	m_sourceAnimationUpdated = false;
	m_skipSourceAnimationUpdateCheck = false;
	m_adjustDurationOnNextEvent = false;
	m_cancelIfSourceAnimationUpdateIsNotUpdated = false;
	m_syncPointTime = -1.0f;
	m_autoFindSyncPointTime = true;
	m_autoSyncPointTime = -1.0f;
	
	m_storeTotalDelta = true;
	m_totalDeltaLocationMS = Vector::ZEROS;
	m_totalDeltaRotation = EulerAngles::ZEROS;

	m_useBoneName = CName::NONE;
	m_useBoneIdx = -1;
	m_updateBoneTransform = false;
	m_updateBoneTransformContinuously = false;
	m_useBoneForLocationWeight = 1.0f;
	m_useBoneForRotationWeight = 0.0f;
	m_useBoneToMatchTargetHeadingWeight = 1.0f;

	m_adjustLocation = LAT_NONE;
	m_replaceTranslation = false;
	m_targetLocation = Vector::ZEROS;
	m_locationAdjustmentVector = Vector::ZEROS;
	m_adjustLocationVertically = false;
	m_moveToNode = NULL;
	m_moveToEntity = NULL;
	m_entityBoneName = CName::NONE;
	m_entityBoneIdx = -1;
	m_entitySlotName = CName::NONE;
	m_entitySlot = NULL;
	m_entitySlotMatrixValid = false;
	m_locationAdjustmentMinDistanceToTarget = 0.0f;
	m_locationAdjustmentMaxDistanceToTarget = 100.0f;
	m_locationAdjustmentMaxSpeed = 0.0f;
	m_locationAdjustmentMaxSpeedZ = 0.0f;
	m_locationAdjustmentMaxDistanceThroughSpeed = false;
	m_locationAdjustmentMaxDistanceXY = -1.0f;
	m_locationAdjustmentMaxDistanceZ = -1.0f;
	m_reachedDestination = false;

	m_adjustRotation = RAT_NONE;
	m_replaceRotation = false;
	m_targetHeading = 0.0f;
	m_rotationAdjustmentHeading = 0.0f;
	m_rotationAdjustmentMaxSpeed = 0.0f;
	m_steeringMayOverrideMaxRotationAdjustmentSpeed = false;
	m_faceNode = NULL;
	m_reachedRotation = false;

	m_lockMovementInDirection = false;
	m_lockedMovementHeading = 0.0f;
	m_rotateExistingDeltaLocation = false;

	m_notifies.ClearFast();
}

void SMovementAdjustmentRequest::FinalizeRequest()
{
	UnbindFromEvent();
}

void SMovementAdjustmentRequest::UnbindFromEvent()
{
	if ( ! m_boundToEvent.Empty() )
	{
		CName unbindFrom = m_boundToEvent;
		m_boundToEvent = CName::NONE; // clear before unbinding
		m_movementAdjustor->UnbindRequestFromEvent( this, unbindFrom );
	}
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::BlendIn( Float blendInTime )
{
	m_blendInTime = blendInTime;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::DontEnd()
{
	m_dontAutoEnd = true;
	m_keepForTimeLeft = 0.0f;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::KeepActiveFor( Float duration )
{
	m_dontAutoEnd = false;
	m_keepForTimeLeft = duration;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::AdjustmentDuration( Float duration )
{
	m_dontAutoEnd = false;
	m_adjustmentDuration = duration;
	m_adjustmentTimeLeft = duration;
	m_keepForTimeLeft = Max( m_keepForTimeLeft, duration );
	m_firstUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::Continuous()
{
	m_adjustmentDuration = 0.0f;
	m_keepForTimeLeft = 0.0f;
	m_dontAutoEnd = true;
	m_firstUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::BaseOnNode( const CNode* node )
{
	m_basedOnNode = node;
	m_locationNeedsUpdate = true;
	m_rotationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::BindToEvent( CName eventName, Bool adjustDurationOnNextEvent )
{
	UnbindFromEvent();
	if (! eventName.Empty() )
	{
		m_movementAdjustor->BindRequestToEvent( this, eventName );
		m_boundToEvent = eventName; // set after binding
		// we will depend on source animation that comes with handled event
		CancelIfSourceAnimationUpdateIsNotUpdated( true );
		m_skipSourceAnimationUpdateCheck = true; // just in any case, let it go for one frame
		m_adjustDurationOnNextEvent = adjustDurationOnNextEvent;
		if ( m_adjustDurationOnNextEvent )
		{
			DontEnd(); // until next tick
		}
	}
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::BindToEventAnimInfo( const SAnimationEventAnimInfo& animInfo, Bool bindOnly )
{
	BindToEvent( animInfo.m_eventName );
	AdjustmentDuration( ( animInfo.m_eventEndsAtTime - animInfo.m_localTime ) );
	UpdateSourceAnimation( animInfo );
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::ScaleAnimation( Bool scaleAnimation, Bool scaleLocation, Bool scaleRotation )
{
	m_scaleAnimation = scaleLocation;
	m_scaleAnimationLocation = scaleLocation;
	m_scaleAnimationRotation = scaleRotation;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::ScaleAnimationLocationVertically( Bool scaleAnimationLocationVertically )
{
	m_scaleAnimationLocationVertically = scaleAnimationLocationVertically;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::DontUseSourceAnimation( bool dontUseSourceAnimation )
{
	m_dontUseSourceAnimation = dontUseSourceAnimation;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::UpdateSourceAnimation( const SAnimationEventAnimInfo& animInfo )
{
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
	RED_LOG_SPAM( MovementAdjustor, TXT("MovAdj: Update source animation %s @%.3f event %s"), animInfo.m_animation->GetAnimation()->GetName().AsChar(), animInfo.m_localTime, animInfo.m_eventName.AsChar() );
#endif
	if ( m_sourceAnimation.m_animation != animInfo.m_animation )
	{
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
		RED_LOG_SPAM( MovementAdjustor, TXT("changed animation!") );
#endif
		m_autoFindSyncPointTime = true;
		m_autoSyncPointTime = -1.0f;
		m_prevSourceAnimLocalTime = -1.0f;
		m_storeTotalDelta = true;
		if ( m_sourceAnimation.m_animation != NULL )
		{
			// when we switched animation, mark delta seconds not to be used (it will use delta seconds as provided by system - better than nothing, right?)
			m_deltaSecondsFromEvent = -1.0f;
		}
	}
	else
	{
		m_prevSourceAnimLocalTime = m_sourceAnimation.m_localTime;
	}
	m_sourceAnimation = animInfo;
	m_sourceAnimationUpdated = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::SyncPointInAnimation( Float syncPointTime )
{
	m_syncPointTime = syncPointTime;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::UseBoneForAdjustment( CName boneName, Bool useContinuously, Float useBoneForLocationAdjustmentWeight, Float useBoneForRotationAdjustmentWeight, Float useBoneToMatchTargetHeadingWeight )
{
	m_useBoneName = boneName;
	m_useBoneIdx = -1;
	m_updateBoneTransform = true;
	m_updateBoneTransformContinuously = useContinuously;
	m_useBoneForLocationWeight = useBoneForLocationAdjustmentWeight;
	m_useBoneForRotationWeight = useBoneForRotationAdjustmentWeight;
	m_useBoneToMatchTargetHeadingWeight = Clamp(useBoneToMatchTargetHeadingWeight, 0.0f, 1.0f);
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::CancelIfSourceAnimationUpdateIsNotUpdated( Bool cancelIfSourceAnimationUpdateIsNotUpdated )
{
	m_cancelIfSourceAnimationUpdateIsNotUpdated = cancelIfSourceAnimationUpdateIsNotUpdated;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::KeepLocationAdjustmentActive()
{
	m_adjustLocation = LAT_KEEP;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::ReplaceTranslation( Bool replaceTranslation )
{
	m_replaceTranslation = replaceTranslation;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::ShouldStartAt( const Vector& targetLocation )
{
	m_adjustLocation = LAT_SHOULD_START_AT;
	m_targetLocation = targetLocation;
	m_locationAdjustmentVector = Vector::ZEROS;
	m_locationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::SlideTo( const Vector& targetLocation )
{
	m_adjustLocation = LAT_TO_LOCATION;
	m_targetLocation = targetLocation;
	m_locationAdjustmentVector = targetLocation;
	m_locationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::SlideBy( const Vector& slideByDistance )
{
	m_adjustLocation = LAT_BY_VECTOR;
	m_targetLocation = Vector::ZEROS;
	m_locationAdjustmentVector = slideByDistance;
	m_locationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::SlideTowards( const CNode* node, Float minDistance, Float maxDistance )
{
	m_adjustLocation = LAT_AT_DIST_TO_NODE;
	m_targetLocation = Vector::ZEROS;
	m_locationAdjustmentVector = Vector::ZEROS;
	m_moveToNode = node;
	m_locationAdjustmentMinDistanceToTarget = minDistance;
	m_locationAdjustmentMaxDistanceToTarget = Max( m_locationAdjustmentMinDistanceToTarget, maxDistance );
	m_locationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::SlideToEntity( const CEntity* entity, const CName& entityBoneName, Float minDistance, Float maxDistance )
{
	m_adjustLocation = LAT_TO_ENTITY;
	m_targetLocation = Vector::ZEROS;
	m_locationAdjustmentVector = Vector::ZEROS;
	m_moveToEntity = entity;
	m_entityBoneName = entityBoneName;
	m_locationAdjustmentMinDistanceToTarget = minDistance;
	m_locationAdjustmentMaxDistanceToTarget = Max( m_locationAdjustmentMinDistanceToTarget, maxDistance );
	m_locationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::MatchEntitySlot( const CEntity* entity, const CName& slotName )
{
	m_adjustLocation = LAT_MATCH_ENTITY_SLOT;
	m_targetLocation = Vector::ZEROS;
	m_locationAdjustmentVector = Vector::ZEROS;
	m_moveToEntity = entity;
	m_entitySlotName = slotName;
	m_entitySlot = NULL;
	m_locationAdjustmentMinDistanceToTarget = 0.0f;
	m_locationAdjustmentMaxDistanceToTarget = 0.0f;
	m_locationNeedsUpdate = true;
	m_adjustRotation = RAT_MATCH_ENTITY_SLOT;
	m_targetHeading = 0.0f;
	m_rotationAdjustmentHeading = 0.0f;
	m_rotationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::MaxLocationAdjustmentSpeed( Float locationAdjustmentMaxSpeed, Float locationAdjustmentMaxSpeedZ )
{
	m_locationAdjustmentMaxSpeed = locationAdjustmentMaxSpeed;
	m_locationAdjustmentMaxSpeedZ = locationAdjustmentMaxSpeedZ;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::MaxLocationAdjustmentDistance( Bool throughSpeed, Float locationAdjustmentMaxDistanceXY, Float locationAdjustmentMaxDistanceZ )
{
	m_locationAdjustmentMaxDistanceThroughSpeed = throughSpeed;
	m_locationAdjustmentMaxDistanceXY = locationAdjustmentMaxDistanceXY;
	m_locationAdjustmentMaxDistanceZ = locationAdjustmentMaxDistanceZ;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::AdjustLocationVertically( Bool adjustLocationVertically )
{
	m_adjustLocationVertically = adjustLocationVertically;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::KeepRotationAdjustmentActive()
{
	m_adjustRotation = RAT_KEEP;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::ReplaceRotation( Bool replaceRotation )
{
	m_replaceRotation = replaceRotation;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::ShouldStartFacing( Float targetHeading )
{
	m_adjustRotation = RAT_SHOULD_START_FACING;
	m_targetHeading = targetHeading;
	m_rotationAdjustmentHeading = 0.0f;
	m_rotationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::RotateTo( Float targetHeading )
{
	m_adjustRotation = RAT_TO_HEADING;
	m_targetHeading = targetHeading;
	m_rotationAdjustmentHeading = targetHeading;
	m_rotationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::RotateBy( Float byHeading )
{
	m_adjustRotation = RAT_BY_ANGLE;
	m_targetHeading = 0.0f;
	m_rotationAdjustmentHeading = byHeading;
	m_rotationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::RotateTowards( const CNode* node, Float offsetHeading )
{
	m_adjustRotation = RAT_FACE_NODE;
	m_faceNode = node;
	m_targetHeading = 0.0f;
	m_rotationAdjustmentHeading = offsetHeading;
	m_rotationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::MatchMoveRotation()
{
	m_adjustRotation = RAT_MATCH_MOVE_ROTATION;
	m_rotationNeedsUpdate = true;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::MaxRotationAdjustmentSpeed( Float rotationAdjustmentMaxSpeed )
{
	m_rotationAdjustmentMaxSpeed = rotationAdjustmentMaxSpeed;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::SteeringMayOverrideMaxRotationAdjustmentSpeed( Bool steeringMayOverrideMaxRotationAdjustmentSpeed )
{
	m_steeringMayOverrideMaxRotationAdjustmentSpeed = steeringMayOverrideMaxRotationAdjustmentSpeed;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::LockMovementInDirection( Float heading )
{
	m_lockMovementInDirection = true;
	m_lockedMovementHeading = heading;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::RotateExistingDeltaLocation( Bool rotateExistingDeltaLocation )
{
	m_rotateExistingDeltaLocation = rotateExistingDeltaLocation;
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::NotifyScript( const THandle< IScriptable >& notifyObject, const CName& eventName, EMovementAdjustmentNotify MAN )
{
	m_notifies.PushBack( SMovementAdjustmentNotify( notifyObject, eventName, MAN ) );
	return this;
}

SMovementAdjustmentRequest* SMovementAdjustmentRequest::DontNotifyScript( const THandle< IScriptable >& notifyObject, const CName& eventName, EMovementAdjustmentNotify MAN )
{
	for ( Int32 idx = 0; idx < m_notifies.SizeInt(); ++ idx )
	{
		SMovementAdjustmentNotify& notify = m_notifies[idx];
		if ( notify.m_notify == MAN &&
			 notify.m_eventName == eventName &&
			 notify.m_callee == notifyObject )
		{
			m_notifies.RemoveAtFast( idx );
			-- idx;
		}
	}
	return this;
}

void SMovementAdjustmentRequest::HandleEvent( const CAnimationEventFired &event )
{
	m_handledEvent = true;
	// store delta seconds from event to know how much did we actually advanced, handle backward animation
	m_deltaSecondsFromEvent = m_lastEventTime >= 0.0f? Abs( event.m_animInfo.m_localTime - m_lastEventTime ) : 0.0f;
	m_lastEventTime = event.m_animInfo.m_localTime;
	Float timeLeft = Abs( event.m_animInfo.m_eventEndsAtTime - event.m_animInfo.m_localTime );
	// update time left to be in sync with event
	m_adjustmentTimeLeft = timeLeft;
	m_keepForTimeLeft = Max( timeLeft, m_keepForTimeLeft );
	UpdateSourceAnimation( event.m_animInfo );
	if ( m_adjustDurationOnNextEvent )
	{
		m_adjustDurationOnNextEvent = false;
		AdjustmentDuration( timeLeft );
	}
}

Vector SMovementAdjustmentRequest::GetTargetLocation() const
{
	ASSERT( m_adjustLocation == LAT_TO_LOCATION );
	if ( const CNode* node = m_basedOnNode.Get() )
	{
		return node->GetLocalToWorld().TransformPoint( m_targetLocation );
	}
	return m_targetLocation;
}

void SMovementAdjustmentRequest::InvalidateEntitySlotMatrix()
{
	m_entitySlotMatrixValid = false;
}

void SMovementAdjustmentRequest::UpdateEntitySlotMatrix()
{
	if ( m_entitySlotMatrixValid )
	{
		return;
	}
	if ( ! m_entitySlotName.Empty() )
	{
		if ( const CEntity* entity = m_moveToEntity.Get() )
		{
			if ( ! m_entitySlot )
			{
				if ( const CEntityTemplate* templ = entity->GetEntityTemplate() )
				{
					m_entitySlot = templ->FindSlotByName( m_entitySlotName, true );
				}
			}
			if ( m_entitySlot )
			{
				m_entitySlot->CalcMatrix( entity, m_entitySlotMatrixWS, NULL );
				m_entitySlotMatrixValid = true;
			}
		}
	}
}

Bool SMovementAdjustmentRequest::GetNodeEntityLocation( Vector& outLoc )
{
	Bool foundTarget = false;
	ASSERT( m_adjustLocation == LAT_AT_DIST_TO_NODE || m_adjustLocation == LAT_TO_ENTITY || m_adjustLocation == LAT_MATCH_ENTITY_SLOT );

	// check if it makes sense to use predicted location using velocity
	Vector velocity = Vector::ZEROS;
	Bool useVelocity = ! IsContinuous();
	Float timeLeftToSyncPoint = useVelocity? Max( 0.0f, GetSyncPoint() - m_sourceAnimation.m_localTime ) : 0.0f;
	useVelocity = useVelocity && timeLeftToSyncPoint > 0.0f;

	if ( m_adjustLocation == LAT_AT_DIST_TO_NODE )
	{
		if ( const CNode* node = m_moveToNode.Get() )
		{
			outLoc = node->GetWorldPosition();
			foundTarget = true;
			if ( useVelocity )
			{
				if ( const CEntity* entity = Cast< CEntity >( node ) )
				{
					if ( const CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( entity->GetRootAnimatedComponent() ) )
					{
						velocity = mac->GetVelocity();
						ASSERT( velocity.SquareMag3() < 100.0f * 100.0f, TXT("Target velocity is really big, check why it happens before adding checks and zeroing velocity here.") );
					}
				}
			}
		}
	}
	if ( m_adjustLocation == LAT_TO_ENTITY )
	{
		if ( const CEntity* entity = m_moveToEntity.Get() )
		{
			outLoc = entity->GetWorldPosition();
			if ( ! m_entityBoneName.Empty() )
			{
				if ( m_entityBoneIdx == -1 )
				{
					m_entityBoneIdx = entity->GetRootAnimatedComponent()->FindBoneByName( m_entityBoneName );
				}
				if ( m_entityBoneIdx != -1 )
				{
					outLoc = entity->GetRootAnimatedComponent()->GetBoneMatrixWorldSpace( m_entityBoneIdx ).GetTranslation();
				}
			}
			if ( useVelocity )
			{
				if ( const CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( entity->GetRootAnimatedComponent() ) )
				{
					velocity = mac->GetVelocity();
				}
			}
			foundTarget = true;
		}
	}
	if ( m_adjustLocation == LAT_MATCH_ENTITY_SLOT )
	{
		if ( const CEntity* entity = m_moveToEntity.Get() )
		{
			outLoc = entity->GetWorldPosition();
			UpdateEntitySlotMatrix();
			if ( m_entitySlotMatrixValid )
			{
				outLoc = m_entitySlotMatrixWS.GetTranslation();
			}
			if ( useVelocity )
			{
				if ( const CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( entity->GetRootAnimatedComponent() ) )
				{
					velocity = mac->GetVelocity();
				}
			}
			foundTarget = true;
		}
	}
	if ( useVelocity )
	{
		Vector prevLoc = outLoc;
		outLoc += velocity * timeLeftToSyncPoint;
	}
	return foundTarget;
}

Matrix SMovementAdjustmentRequest::GetNodeMatrix() const
{
	if ( const CNode* node = m_basedOnNode.Get() )
	{
		return node->GetLocalToWorld();
	}
	return Matrix::IDENTITY;
}

void SMovementAdjustmentRequest::Tick( SMovementAdjustorContext& context, Float deltaSeconds )
{
	if ( m_adjustLocation == LAT_NONE && m_adjustRotation == RAT_NONE && ! m_lockMovementInDirection )
	{
		// not set up yet
		return;
	}

	if ( IsBoundedToAnyEvent() && m_handledEvent && ! m_dontUseSourceAnimation && m_deltaSecondsFromEvent >= 0.0f ) // handle case when we switched animation - we don't want to have negative values here
	{
		deltaSeconds = m_deltaSecondsFromEvent;
	}

	if ( deltaSeconds == 0.0f )
	{
		// won't update anything now
		return;
	}

	// for match entity slot, invalidate slot matrix to have it updated properly
	ASSERT( ! ( ( m_adjustRotation == RAT_MATCH_ENTITY_SLOT ) ^ ( m_adjustLocation == LAT_MATCH_ENTITY_SLOT ) ), TXT("Match entity slot is exclusive") );
	InvalidateEntitySlotMatrix();

#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
	RED_LOG_SPAM( MovementAdjustor, TXT("%s: tick '%ls'"), context.m_mac->GetEntity()->GetName().AsChar(), m_name.AsChar() );
	RED_LOG_SPAM( MovementAdjustor, TXT("%s: scale anim? %s%s%s%s"), context.m_mac->GetEntity()->GetName().AsChar(), m_scaleAnimation? TXT("scale") : TXT("don't"), m_scaleAnimationLocation? TXT(" +loc") : TXT(""), m_scaleAnimationLocationVertically? TXT(" +vert") : TXT(""), m_scaleAnimationRotation? TXT(" +rot") : TXT("") );
#endif

	m_timeActive += deltaSeconds;
	Float blendInCoef = m_blendInTime != 0.0f? Min( 1.0f, m_timeActive / m_blendInTime ) : 1.0f;

	Float actualDeltaSeconds = Min( deltaSeconds, m_adjustmentTimeLeft );

	Float timeCoef = 0.0f;
	Float timeCoefPercentage = 0.0f;
	Float timeLeftPt = 1.0f;
	Float timeAlongPtCoef = 1.0f;
	Bool timeCoefsValid = false;
	if ( m_adjustmentDuration == 0.0f || m_adjustmentTimeLeft == 0.0f ) // if no time left, request may still be active, due to collision etc.
	{
		// just be there
		timeCoefPercentage = 1.0f;
		timeCoef = 1.0f;
		timeLeftPt = 1.0f;
		timeAlongPtCoef = 1.0f;
	}
	else
	{
		timeCoefsValid = true;
		timeCoefPercentage = (actualDeltaSeconds / m_adjustmentTimeLeft);
		timeCoef = (actualDeltaSeconds / m_adjustmentDuration);
		timeLeftPt = m_adjustmentTimeLeft / m_adjustmentDuration;
		timeAlongPtCoef = timeLeftPt < 1.0f-FLT_EPSILON ? (1.0f / (1.0f - ( timeLeftPt ) )) : 0.0f;
	}

	if ( ! m_sourceAnimationUpdated )
	{
		m_prevSourceAnimLocalTime = m_sourceAnimation.m_localTime;
		// advance time to end at event's end when time for this request/adjustment runs out
		if ( m_cancelIfSourceAnimationUpdateIsNotUpdated && ! m_skipSourceAnimationUpdateCheck )
		{
#if DEBUG_MOVEMENT_ADJUSTOR
			if (! m_boundToEvent.Empty())
			{
				RED_LOG( MovementAdjustor, TXT("%s:%s: No source animation updated and was bound to event '%ls'"), context.m_mac->GetEntity()->GetName().AsChar(), m_name.AsChar(), m_boundToEvent.AsChar() );
			}
			else
			{
				RED_LOG( MovementAdjustor, TXT("%s:%s: No source animation updated"), context.m_mac->GetEntity()->GetName().AsChar(), m_name.AsChar() );
			}
#endif
			m_forceFinished = true;
			return;
		}
		if ( m_sourceAnimation.m_animation != NULL && ! m_dontUseSourceAnimation )
		{
			Float timeToEnd = m_sourceAnimation.m_eventEndsAtTime - m_sourceAnimation.m_localTime;
			Float animAdvTime = timeCoefsValid? timeToEnd * timeCoefPercentage : deltaSeconds;
			if ( timeToEnd > 0.0f )
			{
				m_sourceAnimation.m_localTime = Min(m_sourceAnimation.m_eventEndsAtTime, m_sourceAnimation.m_localTime + animAdvTime);
			}
			else if ( timeToEnd < 0.0f )
			{
				m_sourceAnimation.m_localTime = Max(m_sourceAnimation.m_eventEndsAtTime, m_sourceAnimation.m_localTime + animAdvTime);
			}
		}
	}
	m_skipSourceAnimationUpdateCheck = false;
	
	Vector currentLocation = context.m_currentLocation;
	EulerAngles currentRotation = context.m_currentRotation;
	Vector animDeltaLocationMS = Vector::ZEROS;
	EulerAngles animDeltaRotation = EulerAngles::ZEROS;

	if ( m_autoFindSyncPointTime && ! m_dontUseSourceAnimation )
	{
		// find sync point event and use it
		m_autoSyncPointTime = -1.0f;
		if ( const CSkeletalAnimationSetEntry* animSetEntry = m_sourceAnimation.m_animation )
		{
			TDynArray< CExtAnimEvent* >	syncEvents; // TODO use stack based array?
			animSetEntry->GetAllEvents( syncEvents );
			for ( TDynArray< CExtAnimEvent* >::iterator it = syncEvents.Begin(); it != syncEvents.End(); ++ it )
			{
				const CExtAnimEvent* animEvent = *it;
				if ( animEvent->GetEventName() == CNAME( syncPointEvent ) )
				{
					Float eventTime = animEvent->GetStartTime();
					if ( eventTime >= m_sourceAnimation.m_localTime &&
						 (m_autoSyncPointTime < 0.0f || m_autoSyncPointTime > eventTime ) )
					{
						m_autoSyncPointTime = eventTime;
					}
				}
			}
		}
		// if we've found - good, if not - good either, just don't do it anymore
		m_autoFindSyncPointTime = false;
	}
	if ( CalculateAnimDeltaToSyncPoint( context, animDeltaLocationMS, animDeltaRotation ) )
	{
		if ( m_storeTotalDelta )
		{
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
			RED_LOG_SPAM( MovementAdjustor, TXT("%s: store total delta rot %.3f"), context.m_mac->GetEntity()->GetName().AsChar(), animDeltaRotation );
#endif
			// store them, we will know how much through anims movement we are
			m_totalDeltaLocationMS = animDeltaLocationMS;
			m_totalDeltaRotation = animDeltaRotation;
			m_storeTotalDelta = false;
		}
	}

	// pretend that we're already at the end (if we played animation)
	currentRotation += animDeltaRotation;

	// clear source animation update
	m_sourceAnimationUpdated = false;

	Vector animDeltaLocationCurrentFrameMS = Vector::ZEROS;
	EulerAngles animDeltaRotationCurrentFrame = EulerAngles::ZEROS;
	if ( m_scaleAnimation && m_prevSourceAnimLocalTime >= 0.0f && ! m_dontUseSourceAnimation )
	{
		CalculateAnimDelta( context, animDeltaLocationCurrentFrameMS, animDeltaRotationCurrentFrame, m_prevSourceAnimLocalTime, m_sourceAnimation.m_localTime );
	}

	if ( m_updateBoneTransform || m_updateBoneTransformContinuously )
	{
		UpdateBoneTransformMS( context );
	}

	// additional target heading, so bone rotation will modify target heading to match bone rotation to be placed at target
	Float additionalTargetMatchHeadingWeight = 0.0f;
	Float additionalTargetMatchHeading = 0.0f;
	Float additionalTargetHeadingWeight = 0.0f;
	Float additionalTargetHeading = 0.0f;
	// modify current rotation by bone rotation (to actually change rotation to desired rotation)
	if ( ! m_useBoneName.Empty() && m_useBoneForRotationWeight > 0.0f )
	{
		// ATM there is no nice way to handle values of m_useBoneToMatchTargetHeadingWeight between 0 and 1
		// when trying to match heading, target heading is more important and bone rotation should match it
		additionalTargetMatchHeadingWeight = m_useBoneForRotationWeight * m_useBoneToMatchTargetHeadingWeight;
		additionalTargetMatchHeading -= m_boneRotMS.Yaw;
		// when not trying to match heading, we want to bone to face target heading
		additionalTargetHeadingWeight = m_useBoneForRotationWeight * (1.0f - m_useBoneToMatchTargetHeadingWeight);
		additionalTargetHeading -= EulerAngles::YawFromXY(m_boneLocMS.X, m_boneLocMS.Y);
	}

	Bool matchRequestedTargetRotationYawToCurrent = false;
	Float requestedTargetRotationYaw = context.m_currentRotation.Yaw;
	if ( m_adjustRotation != RAT_NONE )
	{
		context.m_replaceDeltaRotation |= m_replaceRotation;
		if ( ! context.m_replaceDeltaRotation )
		{
			currentRotation += context.m_currentDeltaRotation;
		}

		Float adjustment = 0.0f;

		Bool reachedRotation = false;

		Float totalAdjustmentLeft = 0.0f;
		if ( m_adjustRotation == RAT_MATCH_MOVE_ROTATION )
		{
			Float moveRotation = context.m_mac->GetMoveRotationWorldSpace();
			//adjustment += EulerAngles::NormalizeAngle180( moveRotation - animDeltaRotation.Yaw ) * timeCoef;
			adjustment += moveRotation * timeCoef; // moveRotation comes from steering and is clamped already...
			matchRequestedTargetRotationYawToCurrent = true;
		}
		else if ( m_adjustRotation == RAT_SHOULD_START_FACING )
		{
			if ( const CNode* node = m_basedOnNode.Get() )
			{
				const EulerAngles& nodeAngles = node->GetWorldRotation();
				Float nodeHeading = nodeAngles.Yaw;
				Float currentHeadingInNodeSpace = EulerAngles::NormalizeAngle180( context.m_currentRotation.Yaw - nodeHeading );
				if ( m_firstUpdate || m_rotationNeedsUpdate )
				{
					m_rotationAdjustmentHeading = EulerAngles::NormalizeAngle180( m_targetHeading - currentHeadingInNodeSpace ) * timeAlongPtCoef; // calculate rotation adjustment within node's space
				}
				else
				{
					// calculate additional adjustment that is result of node's movement (this is not too precise and may fail on greater rotation)
					adjustment += nodeHeading - m_prevNodeHeading;
				}
				adjustment += m_rotationAdjustmentHeading * timeCoef;
				m_prevNodeHeading = nodeHeading;
			}
			else
			{
				if ( m_firstUpdate || m_rotationNeedsUpdate )
				{
					m_rotationAdjustmentHeading = ( m_targetHeading - context.m_currentRotation.Yaw ) * timeAlongPtCoef;
				}
				adjustment += m_targetHeading * timeCoef;
			}
			matchRequestedTargetRotationYawToCurrent = true;
		}
		else if ( m_adjustRotation == RAT_TO_HEADING )
		{
			Float additionalHeading = additionalTargetHeading  * additionalTargetHeadingWeight + additionalTargetMatchHeading * additionalTargetMatchHeadingWeight;
			Float targetHeading = m_targetHeading + additionalHeading;
			if ( const CNode* node = m_basedOnNode.Get() )
			{
				const EulerAngles& nodeAngles = node->GetWorldRotation();
				Float nodeHeading = nodeAngles.Yaw;
				targetHeading = nodeHeading + m_targetHeading + additionalHeading;
			}
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
			RED_LOG_SPAM( MovementAdjustor, TXT("%s: rotate to heading : target heading %.3f = %.3f + %.3f"), context.m_mac->GetEntity()->GetName().AsChar(), targetHeading, m_targetHeading, additionalHeading );
#endif
			totalAdjustmentLeft = EulerAngles::NormalizeAngle180( targetHeading - currentRotation.Yaw );
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
			RED_LOG_SPAM( MovementAdjustor, TXT("%s: total adjusment left %.3f"), context.m_mac->GetEntity()->GetName().AsChar(), totalAdjustmentLeft );
#endif
			adjustment += totalAdjustmentLeft * timeCoefPercentage;
			requestedTargetRotationYaw = targetHeading;
			reachedRotation = Abs( EulerAngles::NormalizeAngle180( ( currentRotation.Yaw + adjustment ) - targetHeading ) ) < m_reachedRotationOff;
		}
		else if ( m_adjustRotation == RAT_MATCH_ENTITY_SLOT )
		{
			UpdateEntitySlotMatrix();
			if ( m_entitySlotMatrixValid )
			{
				Float additionalHeading = additionalTargetHeading  * additionalTargetHeadingWeight + additionalTargetMatchHeading * additionalTargetMatchHeadingWeight;
				Float targetHeading = m_entitySlotMatrixWS.GetYaw() + additionalHeading;
				totalAdjustmentLeft = EulerAngles::NormalizeAngle180( targetHeading - currentRotation.Yaw );
				adjustment += totalAdjustmentLeft * timeCoefPercentage;
				requestedTargetRotationYaw = targetHeading;
				reachedRotation = Abs( EulerAngles::NormalizeAngle180( ( currentRotation.Yaw + adjustment ) - targetHeading ) ) < m_reachedRotationOff;
			}
		}
		else if ( m_adjustRotation == RAT_BY_ANGLE )
		{
			if ( m_scaleAnimation && m_scaleAnimationRotation )
			{
				totalAdjustmentLeft = m_totalDeltaRotation.Yaw != 0.0f? m_rotationAdjustmentHeading * animDeltaRotation.Yaw / m_totalDeltaRotation.Yaw : 0.0f;
			}
			else
			{
				totalAdjustmentLeft = m_rotationAdjustmentHeading * timeLeftPt;
			}
			adjustment += m_rotationAdjustmentHeading * timeCoef;
			matchRequestedTargetRotationYawToCurrent = true;
		}
		else if ( m_adjustRotation == RAT_FACE_NODE )
		{
			if ( const CNode* node = m_faceNode.Get() )
			{
				const Vector& nodeLocation = node->GetWorldPosition();
				const Float& nodeRotationYaw = node->GetWorldYaw();
				Vector towardsNode = nodeLocation - ( IsContinuous() ? currentLocation : ( currentLocation + context.m_currentRotation.TransformVector( animDeltaLocationMS ) ) );
				// get heading basing on node location
				Float targetHeading = EulerAngles::YawFromXY(towardsNode.X, towardsNode.Y);
				// if there is request to match target (node) heading, modify target heading by taking node rotation
				targetHeading = targetHeading + additionalTargetMatchHeadingWeight * EulerAngles::NormalizeAngle180( nodeRotationYaw + additionalTargetMatchHeading - targetHeading );
				targetHeading += m_rotationAdjustmentHeading + additionalTargetHeadingWeight * additionalTargetHeading;
				totalAdjustmentLeft = EulerAngles::NormalizeAngle180( targetHeading - currentRotation.Yaw );
				adjustment += totalAdjustmentLeft * timeCoefPercentage;
				requestedTargetRotationYaw = targetHeading;
				reachedRotation = Abs( EulerAngles::NormalizeAngle180( ( currentRotation.Yaw + adjustment ) - targetHeading ) ) < m_reachedRotationOff;
			}
		}

#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
		RED_LOG_SPAM( MovementAdjustor, TXT("%s: Rotation adjustment %s %.3f"), context.m_mac->GetEntity()->GetName().AsChar(), m_name.AsChar(), adjustment );
#endif
		if ( m_rotateExistingDeltaLocation )
		{
			context.m_turnExistingDeltaLocation.Yaw += adjustment;
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
			RED_LOG_SPAM( MovementAdjustor, TXT("%s: Turn existing delta location %s %.3f"), context.m_mac->GetEntity()->GetName().AsChar(), m_name.AsChar(), adjustment );
#endif
		}

		// use scaling of anim (if possible and replace any adjustment so far)
		if ( m_scaleAnimation && m_scaleAnimationRotation )
		{
			// calculate rotation left from point where adjustment brings us (current rotation + anim delta + total adjustment left) minus current rotation (current rotation) which gives us (anim delta + total adjustment left)
			const Float rotationLeft = animDeltaRotation.Yaw + totalAdjustmentLeft;
			const Float rotationToEndPlusCurrentFrame = animDeltaRotationCurrentFrame.Yaw + animDeltaRotation.Yaw;
			adjustment = rotationToEndPlusCurrentFrame != 0.0f? ( animDeltaRotationCurrentFrame.Yaw / rotationToEndPlusCurrentFrame ) * rotationLeft : 0.0f;
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
			RED_LOG_SPAM( MovementAdjustor, TXT("%s: Scaled rotation to %.3f"), context.m_mac->GetEntity()->GetName().AsChar(), adjustment );
#endif
			if ( ! m_replaceRotation )
			{
				adjustment -= animDeltaRotationCurrentFrame.Yaw;
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
				RED_LOG_SPAM( MovementAdjustor, TXT("%s: Scaled, not replacing rotation: modified by %.3f to %.3f"), context.m_mac->GetEntity()->GetName().AsChar(), animDeltaRotationCurrentFrame.Yaw, adjustment );
#endif
			}
		}

		// limit to max speed
		if ( m_rotationAdjustmentMaxSpeed != 0.0f )
		{
			Float actualLimit = m_steeringMayOverrideMaxRotationAdjustmentSpeed? context.m_rotationAdjustmentMaxSpeedFromSteering : m_rotationAdjustmentMaxSpeed;
			Float limit = (actualLimit * deltaSeconds);
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
			RED_LOG_SPAM( MovementAdjustor, TXT("%s: pre max speed %.3f limit is %.3f = %.3f * %.3f"), context.m_mac->GetEntity()->GetName().AsChar(), adjustment, limit, actualLimit, deltaSeconds );
#endif
			adjustment = Clamp(adjustment, -limit, limit);
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
			RED_LOG_SPAM( MovementAdjustor, TXT("%s: post max speed %.3f"), context.m_mac->GetEntity()->GetName().AsChar(), adjustment );
#endif
		}
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
		RED_LOG_SPAM( MovementAdjustor, TXT("%s: Rotation adjustment limited to %s %.3f (max speed %.3f, actual %.3f)"), context.m_mac->GetEntity()->GetName().AsChar(), m_name.AsChar(), adjustment, m_rotationAdjustmentMaxSpeed, m_steeringMayOverrideMaxRotationAdjustmentSpeed? context.m_rotationAdjustmentMaxSpeedFromSteering : m_rotationAdjustmentMaxSpeed );
#endif
		if ( m_replaceRotation )
		{
			context.m_outputDeltaRotation.Yaw += context.m_currentDeltaRotation.Yaw * ( 1.0f - blendInCoef );
		}
		adjustment *= blendInCoef;

#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
		RED_LOG_SPAM( MovementAdjustor, TXT("%s: Final rotation adjustment %s %.3f"), context.m_mac->GetEntity()->GetName().AsChar(), m_name.AsChar(), adjustment );
#endif
		context.m_outputDeltaRotation.Yaw += adjustment;

		// notify script
		if ( m_reachedRotation != reachedRotation )
		{
			m_reachedRotation = reachedRotation;
			if ( m_reachedRotation )
			{
				CallEvent( MAN_RotationAdjustmentReachedDestination );
			}
		}
	}

	if ( m_lockMovementInDirection )
	{
		Vector newDeltaLocation = EulerAngles::YawToVector( m_lockedMovementHeading ) * context.m_currentDeltaLocation.Mag2();
		context.m_currentDeltaLocation.X = newDeltaLocation.X;
		context.m_currentDeltaLocation.Y = newDeltaLocation.Y;
	}

	if ( matchRequestedTargetRotationYawToCurrent )
	{
		requestedTargetRotationYaw = context.m_currentRotation.Yaw;
	}

	// get anim delta location in world space using requested target rotation
	const Matrix requestedTargetRotationMat = EulerAngles( 0.0f, 0.0f, requestedTargetRotationYaw ).ToMatrix();
	Vector animDeltaLocationWS = requestedTargetRotationMat.TransformVector( animDeltaLocationMS );
	Vector animDeltaLocationCurrentFrameWS = requestedTargetRotationMat.TransformVector( animDeltaLocationCurrentFrameMS );

	// pretend that we're already at the end (if we played animation)
	Bool addAnimDeltaToLocation = ! IsContinuous();
	Vector currentLocationWithAnimDelta = currentLocation;
	if ( addAnimDeltaToLocation ) // TODO but maybe it should be explicitly said that it should use or shouldn't use location prediction?
	{
		currentLocationWithAnimDelta += animDeltaLocationWS;
	}

	// modify current location by bone location (to actually move location to desired location)
	if ( ! m_useBoneName.Empty() && m_useBoneForLocationWeight > 0.0f )
	{
		EulerAngles requestedTargetRotation = EulerAngles( 0.0f, 0.0f, requestedTargetRotationYaw );
		currentLocationWithAnimDelta += requestedTargetRotation.TransformVector( m_boneLocMS ) * m_useBoneForLocationWeight;
	}

	if ( m_adjustLocation != LAT_NONE )
	{
		context.m_replaceDeltaLocation |= m_replaceTranslation;
		if ( ! context.m_replaceDeltaLocation )
		{
			currentLocation += context.m_currentDeltaLocation;
			currentLocationWithAnimDelta += context.m_currentDeltaLocation;
		}

		Vector adjustment = Vector::ZEROS;

		Bool reachedDestination = false;

		Vector totalAdjustmentLeft = Vector::ZEROS; // total adjustment that left to target location
		if ( m_adjustLocation == LAT_SHOULD_START_AT )
		{
			if ( const CNode* node = m_basedOnNode.Get() )
			{
				Matrix nodeMatrix = GetNodeMatrix();
				Matrix nodeMatrixInv = nodeMatrix.FullInverted();
				Vector currentLocationWithAnimDeltaInNodeSpace = nodeMatrixInv.TransformPoint( context.m_currentLocation );
				if ( m_firstUpdate || m_locationNeedsUpdate )
				{
					m_locationAdjustmentVector = ( m_targetLocation - currentLocationWithAnimDeltaInNodeSpace ) * timeAlongPtCoef; // calculate location adjustment within node's space
				}
				else
				{
					// calculate additional adjustment that is result of node's movement (this is not too precise and may fail on greater rotation)
					Vector locationPrevFrame = m_prevNodeMatrix.TransformPoint( m_prevLocationInNodeSpace );
					Vector locationThisFrame = nodeMatrix.TransformPoint( m_prevLocationInNodeSpace );
					adjustment += locationThisFrame - locationPrevFrame;
				}
				adjustment += nodeMatrix.TransformVector( m_locationAdjustmentVector ) * timeCoef;
				m_prevNodeMatrix = nodeMatrix;
				m_prevLocationInNodeSpace = currentLocationWithAnimDeltaInNodeSpace;
			}
			else
			{
				if ( m_firstUpdate || m_locationNeedsUpdate )
				{
					m_locationAdjustmentVector = ( m_targetLocation - context.m_currentLocation ) * timeAlongPtCoef;
				}
				adjustment += m_locationAdjustmentVector * timeCoef;
			}
		}
		else if ( m_adjustLocation == LAT_TO_LOCATION )
		{
			const Vector targetLoc = GetTargetLocation();
			totalAdjustmentLeft = targetLoc - currentLocationWithAnimDelta;
			adjustment += totalAdjustmentLeft * timeCoefPercentage;
			reachedDestination = ( ( currentLocationWithAnimDelta + totalAdjustmentLeft ) - targetLoc ).SquareMag2() <= m_reachedDestinationOff * m_reachedDestinationOff;
		}
		else if ( m_adjustLocation == LAT_BY_VECTOR )
		{
			Vector locationAdjustmentVectorTransformed = GetNodeMatrix().TransformVector( m_locationAdjustmentVector );
			if ( m_scaleAnimation && m_scaleAnimationLocation )
			{
				// calculate basing on how far through delta location we are
				Vector totalDeltaLocationWS = requestedTargetRotationMat.TransformVector( m_totalDeltaLocationMS );
				totalAdjustmentLeft.X = totalDeltaLocationWS.X != 0.0f? locationAdjustmentVectorTransformed.X * animDeltaLocationWS.X / totalDeltaLocationWS.X : 0.0f;
				totalAdjustmentLeft.Y = totalDeltaLocationWS.Y != 0.0f? locationAdjustmentVectorTransformed.Y * animDeltaLocationWS.Y / totalDeltaLocationWS.Y : 0.0f;
				totalAdjustmentLeft.Z = totalDeltaLocationWS.Z != 0.0f? locationAdjustmentVectorTransformed.Z * animDeltaLocationWS.Z / totalDeltaLocationWS.Z : 0.0f;
			}
			else
			{
				totalAdjustmentLeft = locationAdjustmentVectorTransformed * timeLeftPt;
			}
			if ( addAnimDeltaToLocation )
			{
				totalAdjustmentLeft -= animDeltaLocationWS; // as this might be taken over by further
			}
			adjustment += locationAdjustmentVectorTransformed * timeCoef;
		}
		else if ( m_adjustLocation == LAT_MATCH_ENTITY_SLOT )
		{
			Vector targetLoc = Vector::ZEROS;
			if ( GetNodeEntityLocation( targetLoc ) )
			{
				totalAdjustmentLeft = targetLoc - currentLocationWithAnimDelta;
				adjustment += totalAdjustmentLeft * timeCoefPercentage;
				reachedDestination = ( ( currentLocationWithAnimDelta + totalAdjustmentLeft ) - targetLoc ).SquareMag2() <= m_reachedDestinationOff * m_reachedDestinationOff;
			}
		}
		else if ( m_adjustLocation == LAT_AT_DIST_TO_NODE ||
				  m_adjustLocation == LAT_TO_ENTITY )
		{
			Vector targetLoc = Vector::ZEROS;
			if ( GetNodeEntityLocation( targetLoc ) )
			{
				// find target location that by default is currentLocationWithAnimDelta, but it has to lay within distance range and is closer to player
				// get origin, ray (from origin to with-anim-delta)
				Vector origin = currentLocation;
				Vector end = Vector( currentLocationWithAnimDelta.X, currentLocationWithAnimDelta.Y, origin.Z );
				Vector ray = currentLocationWithAnimDelta - origin;
				Float rayLength = ray.Mag2();
				Vector rayNormalized = ray.Normalized2();
				Vector targetForAdjustment = currentLocationWithAnimDelta;
				// get target loc at same plane
				Vector targetLoc2D = Vector( targetLoc.X, targetLoc.Y, origin.Z );
				// get both spheres (min and max distance)
				Sphere atTargetMin( targetLoc2D, m_locationAdjustmentMinDistanceToTarget );
				Sphere atTargetMax( targetLoc2D, m_locationAdjustmentMaxDistanceToTarget );
				Vector intersectForMin0, intersectForMin1;
				Vector intersectForMax0, intersectForMax1;
				// calculate where we go through spheres
				Int32 collidesMin = atTargetMin.IntersectEdge( origin, end, intersectForMin0, intersectForMin1 );
				Int32 collidesMax = atTargetMax.IntersectEdge( origin, end, intersectForMax0, intersectForMax1 );
				// get distance along ray for min and for max spheres
				Float alongDirForMin = ( intersectForMin0 - origin ).Dot2( rayNormalized );
				Float alongDirForMax = ( intersectForMax0 - origin ).Dot2( rayNormalized );
				// get valid range (at least two cases: we're moving closer to sphere (forMax < forMin), we're going away (forMin < forMax)
				Float alongDirMin = Min( collidesMin == 0? 0.0f : alongDirForMin, collidesMax == 0? 0.0f : alongDirForMax );
				Float alongDirMax = Min( collidesMin == 0? rayLength : alongDirForMin, collidesMax == 0? rayLength : alongDirForMax );
				// get current along dir value
				Float alongDirNow = ( targetForAdjustment - origin ).Dot2( rayNormalized );
				// calculate valid target for adjustment
				targetForAdjustment = origin + rayNormalized * Clamp( alongDirNow, alongDirMin, alongDirMax );
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
				RED_LOG_SPAM( MovementAdjustor, TXT("%s: Slide to dist"), context.m_mac->GetEntity()->GetName().AsChar() );
				RED_LOG_SPAM( MovementAdjustor, TXT(": curr dist %.3f curr+ad dist %.3f tfa dist %.3f"), ( targetLoc - origin ).Mag2(), ( targetLoc - currentLocationWithAnimDelta ).Mag2(), ( targetLoc - targetForAdjustment ).Mag2() );
				RED_LOG_SPAM( MovementAdjustor, TXT(": along dir for min [%i]:%.3f for max [%i]:%.3f"),
							  collidesMin, collidesMin == 0? 0.0f : alongDirForMin,
							  collidesMax, collidesMax == 0? 0.0f : alongDirForMax );
				RED_LOG_SPAM( MovementAdjustor, TXT(": along dir min %.3f max %.3f -> now %.3f (ray length %.3f)"), alongDirMin, alongDirMax, alongDirNow, rayLength );
#endif
				// check now if at that adjusted position we're not too far away or too close (because until now we were checking situations in which we cross both spheres)
				// and finally, calculate adjustment (from predicted current location with anim delta)
				Vector toTargetFromTFA = targetLoc - targetForAdjustment;
				toTargetFromTFA.Z = 0.0f;
				Float distance = toTargetFromTFA.Mag2();
				if ( distance != 0.0f && ( distance < m_locationAdjustmentMinDistanceToTarget || distance > m_locationAdjustmentMaxDistanceToTarget ) )
				{
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
					RED_LOG_SPAM( MovementAdjustor, TXT(": keep within ranges (dist %.3f)"), distance );
#endif
					const Float requiredDistance = Clamp( distance, m_locationAdjustmentMinDistanceToTarget, m_locationAdjustmentMaxDistanceToTarget );
					targetForAdjustment += toTargetFromTFA * ( ( distance - requiredDistance ) / distance ); // divided by distance to normalize toTargetFromTFA
				}
				if ( distance >= m_locationAdjustmentMinDistanceToTarget - m_reachedDestinationOff && distance <= m_locationAdjustmentMaxDistanceToTarget + m_reachedDestinationOff)
				{
					reachedDestination = true;
				}
				// calculate actual adjustment now
				totalAdjustmentLeft = targetForAdjustment - currentLocationWithAnimDelta;
				totalAdjustmentLeft.Z = 0.0f; // we need now only for XY plane (vertical adjustment will be done below)
				adjustment += totalAdjustmentLeft * timeCoefPercentage;
				if ( m_adjustLocationVertically )
				{
					Vector toTarget = targetLoc - currentLocationWithAnimDelta;
					totalAdjustmentLeft.Z += toTarget.Z;
					adjustment.Z += toTarget.Z * timeCoefPercentage;
#if DEBUG_MOVEMENT_ADJUSTOR && DEBUG_MOVEMENT_ADJUSTOR_EVERY_FRAME
					RED_LOG_SPAM( MovementAdjustor, TXT("%s: Slide vertically %.3f (%.3f) -> %.3f, adj %.3f (%.3f)"), context.m_mac->GetEntity()->GetName().AsChar(), currentLocationWithAnimDelta.Z, currentLocation.Z, toTarget.Z, adjustment.Z, timeCoefPercentage );
#endif
				}
			}
		}

		// use scaling of anim (if possible and replace any adjustment so far)
		if ( m_scaleAnimation && ( m_scaleAnimationLocation || m_scaleAnimationLocationVertically ) )
		{
			// calculate location left from point where adjustment brings us (current location + anim delta + total adjustment left) minus current location (current location) which gives us (anim delta + total adjustment left)
			const Vector locationLeft = addAnimDeltaToLocation? animDeltaLocationWS + totalAdjustmentLeft : totalAdjustmentLeft;
			// this uses current rotation
			const Vector locationToEndPlusCurrentFrame = animDeltaLocationCurrentFrameWS + animDeltaLocationWS;
			const Vector prevAdjustment = adjustment;
			if ( m_scaleAnimationLocation )
			{
				adjustment.X = Abs(locationToEndPlusCurrentFrame.X) > 0.03f? ( animDeltaLocationCurrentFrameWS.X / locationToEndPlusCurrentFrame.X ) * locationLeft.X : 0.0f;
				adjustment.Y = Abs(locationToEndPlusCurrentFrame.Y) > 0.03f? ( animDeltaLocationCurrentFrameWS.Y / locationToEndPlusCurrentFrame.Y ) * locationLeft.Y : 0.0f;
				if ( ! m_replaceTranslation )
				{
					adjustment.X -= animDeltaLocationCurrentFrameWS.X;
					adjustment.Y -= animDeltaLocationCurrentFrameWS.Y;
				}
			}
			if ( m_scaleAnimationLocationVertically )
			{
				adjustment.Z = locationToEndPlusCurrentFrame.Z != 0.0f? ( animDeltaLocationCurrentFrameWS.Z / locationToEndPlusCurrentFrame.Z ) * locationLeft.Z : 0.0f;
				if ( ! m_replaceTranslation )
				{
					adjustment.Z -= animDeltaLocationCurrentFrameWS.Z;
				}
			}
		}

		// adjust vertically or not?
		adjustment *= m_adjustLocationVertically? Vector::ONES : Vector( 1.0f, 1.0f, 0.0f );

		// limit to max speed
		if ( m_locationAdjustmentMaxSpeed != 0.0f )
		{
			const Float deltaThisFrameSq = adjustment.SquareMag2();
			if ( deltaThisFrameSq > 0.0f )
			{
				const Float maxDeltaThisFrame = m_locationAdjustmentMaxSpeed * deltaSeconds;
				const Float coef = Min( 1.0f, maxDeltaThisFrame / MSqrt( deltaThisFrameSq ) );
				adjustment.X *= coef;
				adjustment.Y *= coef;
			}
		}
		if ( m_locationAdjustmentMaxSpeedZ != 0.0f )
		{
			const Float deltaThisFrame = Abs( adjustment.Z );
			if ( deltaThisFrame > 0.0f )
			{
				const Float maxDeltaThisFrame = m_locationAdjustmentMaxSpeedZ * deltaSeconds;
				const Float coef = Min( 1.0f, maxDeltaThisFrame / deltaThisFrame );
				adjustment.Z *= coef;
			}
		}

		// limit to speed through distance
		if ( m_locationAdjustmentMaxDistanceThroughSpeed )
		{
			if ( m_locationAdjustmentMaxDistanceXY >= 0.0f )
			{
				const Float deltaThisFrameSq = adjustment.SquareMag2();
				if ( deltaThisFrameSq > 0.0f )
				{
					const Float maxDeltaThisFrame = m_locationAdjustmentMaxDistanceXY * timeCoef;
					const Float coef = Min( 1.0f, maxDeltaThisFrame / MSqrt( deltaThisFrameSq ) );
					adjustment.X *= coef;
					adjustment.Y *= coef;
				}
			}
			if ( m_locationAdjustmentMaxDistanceZ >= 0.0f)
			{
				const Float deltaThisFrame = Abs( adjustment.Z );
				if ( deltaThisFrame > 0.0f )
				{
					const Float maxDeltaThisFrame = m_locationAdjustmentMaxDistanceZ * timeCoef;
					adjustment.Z *= Min( 1.0f, maxDeltaThisFrame / deltaThisFrame );
				}
			}
		}
		// limit adjustment to distance
		if ( m_locationAdjustmentMaxDistanceXY >= 0.0f )
		{
			const Vector newAdjustmentSoFar = m_locationAdjustmentSoFar + adjustment;
			const Float distanceSq = newAdjustmentSoFar.SquareMag2();
			if ( distanceSq > m_locationAdjustmentMaxDistanceXY * m_locationAdjustmentMaxDistanceXY )
			{
				const Vector newAdjustmentSoFarMax = newAdjustmentSoFar * m_locationAdjustmentMaxDistanceXY / MSqrt( distanceSq );
				adjustment.X = newAdjustmentSoFarMax.X - m_locationAdjustmentSoFar.X;
				adjustment.Y = newAdjustmentSoFarMax.Y - m_locationAdjustmentSoFar.Y;
			}
		}
		if ( m_locationAdjustmentMaxDistanceZ >= 0.0f )
		{
			const Float newAdjustmentSoFarZ = m_locationAdjustmentSoFar.Z + adjustment.Z;
			const Float distance = Abs( newAdjustmentSoFarZ );
			if ( distance > m_locationAdjustmentMaxDistanceZ )
			{
				const Float newAdjustmentSoFarMaxZ = newAdjustmentSoFarZ * m_locationAdjustmentMaxDistanceXY / distance;
				adjustment.Z = newAdjustmentSoFarMaxZ - m_locationAdjustmentSoFar.Z;
			}
		}

		if ( m_replaceTranslation )
		{
			context.m_outputDeltaLocation += context.m_currentDeltaLocation * ( 1.0f - blendInCoef );
		}
		adjustment *= blendInCoef;

		// store adjustment in output and in "so far"
		context.m_outputDeltaLocation += adjustment;
		m_locationAdjustmentSoFar += adjustment;

		// notify script
		if ( m_reachedDestination != reachedDestination )
		{
			m_reachedDestination = reachedDestination;
			if ( m_reachedDestination )
			{
				CallEvent( MAN_LocationAdjustmentReachedDestination );
			}
		}
	}

	// update actual time and other parameters
	m_adjustmentTimeLeft -= actualDeltaSeconds;
	m_keepForTimeLeft -= deltaSeconds;
	m_firstUpdate = false;
	m_locationNeedsUpdate = false;
	m_rotationNeedsUpdate = false;
	m_handledEvent = false;
}

Bool SMovementAdjustmentRequest::CalculateAnimDeltaToSyncPoint( SMovementAdjustorContext& context, Vector& animDeltaLocation, EulerAngles& animDeltaRotation )
{
	return CalculateAnimDelta( context, animDeltaLocation, animDeltaRotation, m_sourceAnimation.m_localTime, GetSyncPoint() );
}

Bool SMovementAdjustmentRequest::CalculateAnimDelta( SMovementAdjustorContext& context, Vector& animDeltaLocation, EulerAngles& animDeltaRotation, Float startTime, Float endTime )
{
	if ( const CSkeletalAnimationSetEntry* animSetEntry = m_sourceAnimation.m_animation )
	{
		if ( CSkeletalAnimation* animation = animSetEntry->GetAnimation() )
		{
			if ( animation->HasExtractedMotion() )
			{
				// calculate end location&rotation of character that will be used in later calculations
				AnimQsTransform movement = animation->GetMovementBetweenTime( startTime, endTime, 0 );
				Matrix movementMatrix = AnimQsTransformToMatrix( movement );
				animDeltaLocation = movementMatrix.GetTranslation();
				animDeltaRotation = movementMatrix.ToEulerAngles();
				return true;
			}
		}
	}
	return false;
}

void SMovementAdjustmentRequest::UpdateBoneTransformMS( SMovementAdjustorContext& context )
{
	m_boneLocMS = Vector::ZEROS;
	m_boneRotMS = EulerAngles::ZEROS;
	if ( ! m_useBoneName.Empty() )
	{
		if ( const CSkeletalAnimationSetEntry* animSetEntry = m_sourceAnimation.m_animation )
		{
			if ( CSkeletalAnimation* animation = animSetEntry->GetAnimation() )
			{
				if ( CSkeleton* skeleton = context.m_mac->GetSkeleton() )
				{
					Uint32 bonesNum = animation->GetBonesNum();
					Uint32 tracksNum = animation->GetTracksNum();
					AnimQsTransform* bones = (AnimQsTransform*) RED_ALLOCA( sizeof(AnimQsTransform) * bonesNum );
					AnimFloat* tracks = (AnimFloat*) RED_ALLOCA( sizeof(AnimFloat) * tracksNum );
					// TODO not sample whole frame?
					animation->Sample( m_updateBoneTransformContinuously? m_sourceAnimation.m_localTime : GetSyncPoint(), bonesNum, tracksNum, bones, tracks );
					// TODO there should be no need to do this in near future and when future becomes present or past, remove this
					if ( context.m_mac->UseExtractedTrajectory() && context.m_mac->HasTrajectoryBone() )
					{
						SBehaviorGraphOutput::ExtractTrajectoryOn( context.m_mac, bones, bonesNum );
					}
					else if ( bonesNum > 0 )
					{
#ifdef USE_HAVOK_ANIMATION // VALID
						bones[ 0 ].m_rotation.normalize();
#else
						bones[ 0 ].Rotation.Normalize();
#endif
					}
					if ( m_useBoneIdx < 0 )
					{
						m_useBoneIdx = skeleton->FindBoneByName(m_useBoneName);
					}
					AnimQsTransform boneTransformMS = skeleton->GetBoneMS( m_useBoneIdx, bones, (Int32)bonesNum );
					Matrix boneMatrixMS = AnimQsTransformToMatrix( boneTransformMS );
					m_boneLocMS = boneMatrixMS.GetTranslation();
					m_boneRotMS = boneMatrixMS.ToEulerAngles();
					m_updateBoneTransform = false; // updated!
				}
			}
		}
	}
}

void SMovementAdjustmentRequest::GenerateDebugFragments( CRenderFrame* frame, SMovementAdjustorContext& context, Uint32& yDispl )
{
	Uint32 xDispl = 0;
	frame->AddDebugText( context.m_currentLocation, m_name.AsChar(), xDispl, yDispl*2, true, Color(255,255,255), Color(0,0,0,198) );
	++ yDispl;
	frame->AddDebugText( context.m_currentLocation, String::Printf( TXT("adjLeft:%.3f"), m_adjustmentTimeLeft ), xDispl, yDispl*2, true, Color(180,180,180), Color(0,0,0,128) );
	++ yDispl;
	frame->AddDebugText( context.m_currentLocation, String::Printf( TXT("keepFor:%.3f"), m_keepForTimeLeft ), xDispl, yDispl*2, true, Color(180,180,180), Color(0,0,0,128) );
	++ yDispl;
	if ( m_adjustLocation != LAT_NONE )
	{
		Vector animDeltaLocation = Vector::ZEROS;
		EulerAngles animDeltaRotation = EulerAngles::ZEROS;
		if ( CalculateAnimDeltaToSyncPoint( context, animDeltaLocation, animDeltaRotation ) )
		{
			animDeltaLocation = context.m_currentRotation.TransformVector( animDeltaLocation );
			if ( ! m_adjustLocationVertically )
			{
				animDeltaLocation.Z = 0.0f;
			}
			Matrix m = Matrix::IDENTITY;
			m.SetTranslation( context.m_currentLocation );
			frame->AddDebugArrow( m, animDeltaLocation, 1.0f, Color(0,0,255), true ); // anim delta location (left)
		}
		if ( ! m_useBoneName.Empty() )
		{
			Vector boneAt = context.m_currentLocation + context.m_currentRotation.TransformVector( m_boneLocMS );
			EulerAngles boneRot = context.m_currentRotation + m_boneRotMS;
			Matrix m = boneRot.ToMatrix();
			m.SetTranslation( boneAt );
			frame->AddDebugLine( context.m_currentLocation, boneAt, Color(255,0,255), true );
			Float x = 0.1f;
			frame->AddDebugLine( boneAt + Vector(-x,0.0f,0.0f), boneAt + Vector(x,0.0f,0.0f), Color(255,0,255), true );
			frame->AddDebugLine( boneAt + Vector(0.0f,-x,0.0f), boneAt + Vector(0.0f,x,0.0f), Color(255,0,255), true );
			frame->AddDebugArrow( m, Vector(0,0.3f,0), 1.0f, Color(255,0,255), true );
		}
		if ( m_adjustLocation == LAT_TO_LOCATION )
		{
			frame->AddDebugText( context.m_currentLocation, TXT("AdjToLoc"), xDispl, yDispl*2, true, Color(125,255,125), Color(0,0,0,128) );
			++ yDispl;
			Matrix m = Matrix::IDENTITY;
			m.SetTranslation( context.m_currentLocation );
			frame->AddDebugArrow( m, GetTargetLocation() - context.m_currentLocation, 1.0f, Color(255,0,0,128), true );
			frame->AddDebugArrow( m, GetTargetLocation() - animDeltaLocation - context.m_currentLocation, 1.0f, Color(255,0,0,255), true );
		}
		else if ( m_adjustLocation == LAT_AT_DIST_TO_NODE || m_adjustLocation == LAT_TO_ENTITY )
		{
			if ( m_adjustLocation == LAT_AT_DIST_TO_NODE )
			{
				frame->AddDebugText( context.m_currentLocation, TXT("DistToNode"), xDispl, yDispl*2, true, Color(125,255,195), Color(0,0,0,128) );
			}
			else
			{
				frame->AddDebugText( context.m_currentLocation, TXT("LocToEntity"), xDispl, yDispl*2, true, Color(125,255,195), Color(0,0,0,128) );
			}
			++ yDispl;
			frame->AddDebugText( context.m_currentLocation, String::Printf( TXT("dist %.3f - %.3f"), m_locationAdjustmentMinDistanceToTarget, m_locationAdjustmentMaxDistanceToTarget ), xDispl, yDispl*2, true, Color(125,255,195), Color(0,0,0,128) );
			++ yDispl;
			Vector targetLoc = Vector::ZEROS;
			if ( GetNodeEntityLocation( targetLoc ) )
			{
				frame->AddDebugText( context.m_currentLocation, String::Printf( TXT("currdist %.3f"), (targetLoc - context.m_currentLocation).Mag2() ), xDispl, yDispl*2, true, Color(125,255,195), Color(0,0,0,128) );
				++ yDispl;
				Matrix m = Matrix::IDENTITY;
				m.SetTranslation( context.m_currentLocation );
				frame->AddDebugArrow( m, targetLoc - context.m_currentLocation, 1.0f, Color(125,255,195,128), true );
			}
		}
		else
		{
			frame->AddDebugText( context.m_currentLocation, TXT("LocAdj"), xDispl, yDispl*2, true, Color(125,255,125), Color(0,0,0,128) );
			++ yDispl;
		}
	}
	if ( m_adjustRotation != RAT_NONE )
	{
		if ( m_adjustRotation == RAT_MATCH_MOVE_ROTATION )
		{
			frame->AddDebugText( context.m_currentLocation, TXT("MatchMovRot"), xDispl, yDispl*2, true, Color(245,185,185), Color(0,0,0,128) );
		}
		else if ( m_adjustRotation == RAT_TO_HEADING )
		{
			frame->AddDebugText( context.m_currentLocation, TXT("Rot2Hdg"), xDispl, yDispl*2, true, Color(255,125,125), Color(0,0,0,128) );
			Matrix m = Matrix::IDENTITY;
			m.SetTranslation( context.m_currentLocation );
			frame->AddDebugArrow( m, EulerAngles::YawToVector(m_targetHeading) * 1.1f, 1.0f, Color(255,0,0), true );
			frame->AddDebugArrow( m, EulerAngles::YawToVector(context.m_currentRotation.Yaw), 1.0f, Color(0,255,0), true );
		}
		else if ( m_adjustRotation == RAT_BY_ANGLE )
		{
			frame->AddDebugText( context.m_currentLocation, TXT("RotBy"), xDispl, yDispl*2, true, Color(255,125,125), Color(0,0,0,128) );
		}
		else if ( m_adjustRotation == RAT_FACE_NODE )
		{
			frame->AddDebugText( context.m_currentLocation, TXT("Face"), xDispl, yDispl*2, true, Color(255,125,125), Color(0,0,0,128) );
		}
		else if ( m_adjustRotation == RAT_KEEP )
		{
			frame->AddDebugText( context.m_currentLocation, TXT("Keep"), xDispl, yDispl*2, true, Color(255,185,185), Color(0,0,0,128) );
		}
		else
		{
			frame->AddDebugText( context.m_currentLocation, TXT("RotAdj"), xDispl, yDispl*2, true, Color(255,125,125), Color(0,0,0,128) );
		}
		++ yDispl;
		frame->AddDebugText( context.m_currentLocation, String::Printf( TXT("MaxSpd:%.3f"), m_rotationAdjustmentMaxSpeed ), xDispl, yDispl*2, true, Color(255,125,125), Color(0,0,0,128) );
		++ yDispl;
		if ( m_adjustRotation == RAT_FACE_NODE )
		{
			if ( const CNode* node = m_faceNode.Get() )
			{
				Matrix m = Matrix::IDENTITY;
				m.SetTranslation( context.m_currentLocation );
				const Vector& nodeLocation = node->GetWorldPosition();
				frame->AddDebugArrow( m, nodeLocation - context.m_currentLocation, 1.0f, Color(255,0,0), true );
				frame->AddDebugArrow( m, EulerAngles::YawToVector(context.m_currentRotation.Yaw), 1.0f, Color(0,255,0), true );
			}
		}
	}
	if ( const CSkeletalAnimationSetEntry* animSetEntry = m_sourceAnimation.m_animation )
	{
		if ( CSkeletalAnimation* animation = animSetEntry->GetAnimation() )
		{
			frame->AddDebugText( context.m_currentLocation, String::Printf( TXT("anim:%s"), animation->GetName().AsChar() ), xDispl, yDispl*2, true, Color(125,255,125), Color(0,0,0,128) );
			++ yDispl;
			frame->AddDebugText( context.m_currentLocation, String::Printf( TXT("anim@:%.3f"), m_sourceAnimation.m_localTime ), xDispl, yDispl*2, true, Color(125,255,125), Color(0,0,0,128) );
			++ yDispl;
			frame->AddDebugText( context.m_currentLocation, String::Printf( TXT("evtE@:%.3f"), m_sourceAnimation.m_eventEndsAtTime ), xDispl, yDispl*2, true, Color(125,255,125), Color(0,0,0,128) );
			++ yDispl;
			frame->AddDebugText( context.m_currentLocation, String::Printf( TXT("sync@:%.3f"), GetSyncPoint() ), xDispl, yDispl*2, true, Color(125,255,125), Color(0,0,0,128) );
			++ yDispl;
		}
	}
	if ( m_lockMovementInDirection )
	{
		frame->AddDebugText( context.m_currentLocation, TXT("LockMiD"), xDispl, yDispl*2, true, Color(185,185,245), Color(0,0,0,128) );
		++ yDispl;
	}
}

void SMovementAdjustmentRequest::CallEvent( EMovementAdjustmentNotify MAN ) const
{
	for ( TDynArray< SMovementAdjustmentNotify >::const_iterator iMAN = m_notifies.Begin(); iMAN != m_notifies.End(); ++ iMAN )
	{
		if ( iMAN->m_notify == MAN )
		{
			iMAN->Call( m_name );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

SMovementAdjustorContext::SMovementAdjustorContext( const CMovingAgentComponent* mac, const Vector& deltaLocation, const EulerAngles& deltaRotation )
	:	m_mac( mac )
{
	m_currentLocation = mac->GetWorldPosition();
	m_currentRotation = mac->GetWorldRotation();
	m_currentDeltaLocation = deltaLocation;
	m_currentDeltaRotation = deltaRotation;
	m_rotationAdjustmentMaxSpeedFromSteering = mac->GetMaxRotation();
	ClearOutput();
}

void SMovementAdjustorContext::ClearOutput()
{
	m_replaceDeltaLocation = false;
	m_replaceDeltaRotation = false;
	m_outputDeltaLocation = Vector::ZEROS;
	m_outputDeltaRotation = EulerAngles::ZEROS;
	m_turnExistingDeltaLocation = EulerAngles::ZEROS;
}

void SMovementAdjustorContext::FinalizeAdjustments()
{
	if ( ! m_replaceDeltaLocation )
	{
		if ( ! m_turnExistingDeltaLocation.AlmostEquals( EulerAngles::ZEROS ) )
		{
			m_outputDeltaLocation += m_turnExistingDeltaLocation.TransformVector( m_currentDeltaLocation );
		}
		else
		{
			m_outputDeltaLocation += m_currentDeltaLocation;
		}
	}
	if ( ! m_replaceDeltaRotation )
	{
		m_outputDeltaRotation += m_currentDeltaRotation;
	}
}

///////////////////////////////////////////////////////////////////////////////

