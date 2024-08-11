// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "../core/bezier2d.h"
#include "../core/bezier2dHandle.h"
#include "../core/bezierUtils.h"

#include "interpolationBasics.h"
#include "storySceneEventInterpolation.h"
#include "channelStorage.h"
#include "storySceneSection.h"

// =================================================================================================
namespace StoryScene {
// =================================================================================================

/*
Implements most functionality of interpolation events.

\param InterpolationEvent Type of interpolation event that interpolation driver operates on.
*/
template < typename InterpolationEvent >
class InterpolationDriver
{
public:
	static void OnStart( const InterpolationEvent& interpolationEvent, CSceneEventFunctionArgs& args );
	static void OnProcess( const InterpolationEvent& interpolationEvent, CSceneEventFunctionArgs& args );

	static void OnKeyChanged( InterpolationEvent& interpolationEvent, CGUID keyGuid );
	static void AttachKey( InterpolationEvent& interpolationEvent, typename InterpolationEvent::KeyEvent* keyEvent );
	static void DetachKey( InterpolationEvent& interpolationEvent, typename InterpolationEvent::KeyEvent* keyEvent );

	static void Interpolate( const InterpolationEvent& interpolationEvent, Float time, typename InterpolationEvent::Traits::ChannelArray& outInterpolationResult, const typename InterpolationEvent::KeyEvent*& outKeyEventA, const typename InterpolationEvent::KeyEvent*& outKeyEventB, CSceneEventFunctionSimpleArgs& args );
	static void SampleData( const InterpolationEvent& interpolationEvent, Float timeStep, TDynArray< TDynArray< TPair< Vector2, Vector > > >& outData, CSceneEventFunctionSimpleArgs& args );

private:
	static Uint32 GetInsertionIndex( const InterpolationEvent& interpolationEvent, CGUID eventGuid );
};

template < typename InterpolationEvent, typename ChannelRange >
void PreprocessChannels( const InterpolationEvent& interpolationEvent, TDynArray< Float >& outChannelCacheStorage, ChannelRange );

template < typename InterpolationEvent, typename ChannelRange >
void PreprocessChannels( const InterpolationEvent& interpolationEvent, TDynArray< Float >& outChannelCacheStorage, InterpolationChannelRangeNone );

template < typename InterpolationEvent, typename ChannelRange >
void GetChannelsForAllKeys( const InterpolationEvent& interpolationEvent, TDynArray< Float >& outChannelCacheStorage, ChannelRange );

template < typename InterpolationEvent >
void GetChannelsForAllKeys( const InterpolationEvent& interpolationEvent, TDynArray< Float >& outChannelCacheStorage, InterpolationChannelRangeNone );

template < typename InterpolationEvent, typename ChannelRange >
void GetChannelsForVolatileKeys( const InterpolationEvent& interpolationEvent, TDynArray< Float >& outChannelCacheStorage, ChannelRange, CSceneEventFunctionSimpleArgs& args );

template < typename InterpolationEvent >
void GetChannelsForVolatileKeys( const InterpolationEvent& interpolationEvent, TDynArray< Float >& outChannelCacheStorage, InterpolationChannelRangeNone, CSceneEventFunctionSimpleArgs& args );

template < typename InterpolationEvent, typename ChannelRange >
void RecalculateBezierHandles( InterpolationEvent& interpolationEvent, ChannelRange, CSceneEventFunctionSimpleArgs& args );

template < typename InterpolationEvent >
void RecalculateBezierHandles( InterpolationEvent& interpolationEvent, InterpolationChannelRangeNone, CSceneEventFunctionSimpleArgs& args );

// =================================================================================================
// implementation
// =================================================================================================

template < typename InterpolationEvent >
void InterpolationDriver< InterpolationEvent >::OnStart( const InterpolationEvent& interpolationEvent, CSceneEventFunctionArgs& args )
{
	CSceneEventFunctionSimpleArgs simpleArgs( args );

	CStorySceneSection* section = interpolationEvent.GetSceneElement()->GetSection();

	TDynArray< Float >& cachedKeyStartTimes = args.m_data[ interpolationEvent.i_cachedKeyStartTimes ];
	cachedKeyStartTimes.Resize( interpolationEvent.GetNumKeys() );

	// Get key event start times.
	for( Uint32 iKey = 0, numKeys = interpolationEvent.GetNumKeys(); iKey < numKeys; ++iKey )
	{
		Float startTime = 0.0f;
		Float endTime = 0.0f;
		Float duration = 0.0f;

		const CStorySceneEvent* keyEvent = section->GetEvent( interpolationEvent.m_keyGuids[ iKey ] );

		keyEvent->GetEventInstanceData( args.m_data, startTime, endTime, duration );
		cachedKeyStartTimes[ iKey ] = startTime;
	}

	// Resize channel cache so it can hold values of all channels for all keys.
	TDynArray< Float >& channelCacheStorage = args.m_data[ interpolationEvent.i_channelStorage ];
	channelCacheStorage.Resize( interpolationEvent.GetNumKeys() * InterpolationEvent::Traits::numChannels );
	
	// Get values of all channels for all keys (those values are already preprocessed if they are angle values).
	GetChannelsForAllKeys( interpolationEvent, channelCacheStorage, typename InterpolationEvent::Traits::VolatileChannels(), simpleArgs );
	GetChannelsForAllKeys( interpolationEvent, channelCacheStorage, typename InterpolationEvent::Traits::StaticChannels(), simpleArgs );

	// TODO: Bezier handles should not be computed here. We should just use them.
	InterpolationEvent& interpolationEventNonConst = const_cast< InterpolationEvent& >( interpolationEvent );
	RecalculateBezierHandles( interpolationEventNonConst, typename InterpolationEvent::Traits::VolatileChannels(), simpleArgs );
	RecalculateBezierHandles( interpolationEventNonConst, typename InterpolationEvent::Traits::StaticChannels(), simpleArgs );
}

/*

\param currTime Abs time at which to compute interpolation result.
*/
template < typename InterpolationEvent >
void InterpolationDriver< InterpolationEvent >::Interpolate( const InterpolationEvent& interpolationEvent, Float currTime, typename  InterpolationEvent::Traits::ChannelArray& outInterpolationResult, const typename InterpolationEvent::KeyEvent*& outKeyEventA, const typename InterpolationEvent::KeyEvent*& outKeyEventB, CSceneEventFunctionSimpleArgs& args )
{
	const CStorySceneSection* section = interpolationEvent.GetSceneElement()->GetSection();

	TDynArray< Float >& cachedKeyStartTimes = args.m_data[ interpolationEvent.i_cachedKeyStartTimes ];

	// Find two key events that we should consider. At first assume we should consider last two keys of interpolation event
	// which is the case when currTime is between last two keys and also when currTime is greater than last key start time.
	//
	// How can this be that currTime is greater than last key start time? It's because CStoryScenePlayer::InternalFireEvent()
	// takes into account interpolation event end time which, due to the nature of floating point operations, may be slightly
	// greater than start time of last key. Consider:
	//   interpolationEventStartTime = firstKeyStartTime                                      // stored in event instance data
	//   interpolationEventDuration = lastKeyStartTime - firstKeyStartTime                    // stored in event instance data
	//   interpolationEventEndTime = interpolationEventStartTime + interpolationEventDuration // calculated when needed
	// Note that it may happen that interpolationEventEndTime > lastKeyStartTime.
	Uint32 keyAIndex = interpolationEvent.GetNumKeys() - 2;
	Uint32 keyBIndex = interpolationEvent.GetNumKeys() - 1;
	for( Uint32 iKey = 1, numKeys = interpolationEvent.GetNumKeys(); iKey < numKeys; ++iKey )
	{
		if( cachedKeyStartTimes[ iKey ] >= currTime )
		{
			keyAIndex = iKey - 1;
			keyBIndex = iKey;

			break;
		}
	}

	const typename InterpolationEvent::Key* keyA = &interpolationEvent.m_keys[ keyAIndex ];
	const typename InterpolationEvent::Key* keyB = &interpolationEvent.m_keys[ keyBIndex ];

	outKeyEventA = static_cast< const typename InterpolationEvent::KeyEvent* >( section->GetEvent( interpolationEvent.m_keyGuids[ keyAIndex ] ) );
	outKeyEventB = static_cast< const typename InterpolationEvent::KeyEvent* >( section->GetEvent( interpolationEvent.m_keyGuids[ keyBIndex ] ) );

	// Note that we already have values for all keys and for all channels - we got those in OnStart().
	// Keys don't change after event is started so we don't have to get channel values each time in OnProcess().

	const Float keyAStartTime = cachedKeyStartTimes[ keyAIndex ];
	const Float keyBStartTime = cachedKeyStartTimes[ keyBIndex ];
	RED_FATAL_ASSERT( keyBStartTime >= keyAStartTime, "InterpolationDriver< InterpolationEvent >::Interpolate(): keyBStartTime must be >= keyAStartTime." );

	const Float currTimeScaled = Clamp( ( currTime - keyAStartTime ) / ( keyBStartTime - keyAStartTime ), 0.0f, 1.0f );

	// Get channel cache.
	TDynArray< Float >& channelCacheStorage = args.m_data[ interpolationEvent.i_channelStorage ];
	ChannelStorage< InterpolationEvent::Traits::numChannels > channelStorage( channelCacheStorage.TypedData() );

	if( interpolationEvent.CountVolatileKeys() > 0 )
	{
		// Get values of volatile channels for volatile keys.
		GetChannelsForVolatileKeys( interpolationEvent, channelCacheStorage, typename InterpolationEvent::Traits::VolatileChannels(), args );

		// Recalculate bezier handles for volatile channels.
		// TODO: Should we store recalculated bezier handles in interpolation event or should we store them aside?
		// TODO: remove const_cast.
		InterpolationEvent& interpolationEventNonConst = const_cast< InterpolationEvent& >( interpolationEvent );
		RecalculateBezierHandles( interpolationEventNonConst, typename InterpolationEvent::Traits::VolatileChannels(), args );
	}

	typename InterpolationEvent::Traits::ChannelArray& keyAChannels = channelStorage.GetChannels( keyAIndex );
	typename InterpolationEvent::Traits::ChannelArray& keyBChannels = channelStorage.GetChannels( keyBIndex );

	// interpolate each enabled channel
	// note, we're not using IsChannelEnabled( iChannel ) - we interpolate all channels but
	// some of them use "constant" method so actually there's no interpolation
	for( Uint32 iChannel = 0; iChannel < InterpolationEvent::Traits::numChannels; ++iChannel )
	{
		Float valA = keyAChannels[ iChannel ];
		Float valB = keyBChannels[ iChannel ];

		// Result value is initialized to value appropriate for "constant" interpolation method.
		// It will be overwritten if other interpolation method is used for current channel.
		Float resultValue = valA;

		// Get interpolation method to use.
		// TODO: Allow the user to specify different interpolation method for each channel and key.
		EInterpolationMethod interpolationMethod = interpolationEvent.GetInterpolationMethod();

		if( interpolationMethod == EInterpolationMethod::IM_Linear )
		{
			// linear interpolation
			resultValue = ( 1.0f - currTimeScaled ) * valA + currTimeScaled * valB;
		}
		else if( interpolationMethod == EInterpolationMethod::IM_Bezier )
		{
			Vector2 p0( keyAStartTime, valA );
			Vector2 p3( keyBStartTime, valB );

			Vector2 p1 = GetOutgoingTangentControlPoint( keyA->m_bezierHandles[ iChannel ], p0 );
			Vector2 p2 = GetIncomingTangentControlPoint( keyB->m_bezierHandles[ iChannel ], p3 );

			Bezier2D bezier2d( p0, p1, p2, p3 ) ;

			resultValue = bezier2d.Get( currTimeScaled ); // Bezier2D::Get() finds t parameter for given x value for us
		}
		// else "constant" method and resultValue == valA

		outInterpolationResult [ iChannel ] = resultValue ;
	}
}

template < typename InterpolationEvent >
void InterpolationDriver< InterpolationEvent >::OnProcess( const InterpolationEvent& interpolationEvent, CSceneEventFunctionArgs& args )
{
	const Float currTime = args.m_timeInfo.m_timeAbs;
	typename InterpolationEvent::Traits::ChannelArray interpolationResult;
	const typename InterpolationEvent::KeyEvent* keyEventA = nullptr;
	const typename InterpolationEvent::KeyEvent* keyEventB = nullptr;

	CSceneEventFunctionSimpleArgs simpleArgs( args );
	Interpolate( interpolationEvent, currTime, interpolationResult, keyEventA, keyEventB, simpleArgs );
	args.m_destEvent = keyEventB;

	// Pass results for further handling.
	InterpolationEvent::Traits::OnProcess( &interpolationEvent, *keyEventA, interpolationResult, args );
}

/*
Updates interpolation event after key has changed.

\param interpolationEvent Interpolation event to update.
\param keyGuid Key that has changed. Must be belong to interpolationEvent.
*/
template < typename InterpolationEvent >
void InterpolationDriver< InterpolationEvent >::OnKeyChanged( InterpolationEvent& interpolationEvent, CGUID keyGuid )
{
	Uint32 oldIndex = static_cast< Uint32 >( interpolationEvent.m_keyGuids.GetIndex( keyGuid ) );
	ASSERT( oldIndex != -1 ); // OnKeyChanged() called for a key that doesn't belong to interpolationEvent.

	// Store key data so we can later restore it.
	typename InterpolationEvent::Key key( interpolationEvent.m_keys[ oldIndex ] );

	// Remove key from old position.
	interpolationEvent.m_keys.RemoveAt( oldIndex );
	interpolationEvent.m_keyGuids.RemoveAt( oldIndex );

	// Reinsert key at new position.
	Uint32 newIndex = GetInsertionIndex( interpolationEvent, keyGuid );
	interpolationEvent.m_keyGuids.Insert( newIndex, keyGuid );
	interpolationEvent.m_keys.Insert( newIndex, key );

	// If first key has changed then we need to update interpolation event start position and scene element.
	if( newIndex == 0 || oldIndex == 0 )
	{
		CStorySceneSection* section = interpolationEvent.GetSceneElement()->GetSection();

		// Interpolation event is always associated with the same scene element with which first key event is associated.
		// Likewise, interpolation event start position is always equal to first key event start position.
		const CStorySceneEvent* firstKeyEvent = section->GetEvent( interpolationEvent.m_keyGuids[ 0 ] );
		interpolationEvent.SetSceneElement( firstKeyEvent->GetSceneElement() );
		interpolationEvent.SetStartPosition( firstKeyEvent->GetStartPosition() );
	}
}

template < typename InterpolationEvent >
void InterpolationDriver< InterpolationEvent >::AttachKey( InterpolationEvent& interpolationEvent, typename InterpolationEvent::KeyEvent* keyEvent )
{
	ASSERT( keyEvent );

	// assert key event is not already used as a key
	ASSERT( !keyEvent->IsInterpolationEventKey() );

	typename InterpolationEvent::Key key;

	// TODO: key.m_bezierHandles - they all should have defaults? Or we should default to "constant" mode or sth?
	// TODO: m_channels - all to 0

	// Set default interpolation method for all channels of this key.
	for( Uint32 iChannel = 0; iChannel < InterpolationEvent::Traits::numChannels; ++iChannel )
	{
		key.m_interpolationTypes[ iChannel ] = static_cast< Uint32 >( InterpolationEvent::Traits::GetDefaultInterpolationMethod( iChannel ) );
	}

	key.m_volatile = InterpolationEvent::Traits::GetDefaultVolatilityState( *keyEvent );

	Uint32 insertionIndex = GetInsertionIndex( interpolationEvent, keyEvent->GetGUID() );
	interpolationEvent.m_keyGuids.Insert( insertionIndex, keyEvent->GetGUID() );
	interpolationEvent.m_keys.Insert( insertionIndex, key );

	// Make key aware of interpolation event.
	keyEvent->SetInterpolationEventGUID( interpolationEvent.GetGUID() );

	CStorySceneSection* section = interpolationEvent.GetSceneElement()->GetSection();

	// Interpolation event is always associated with the same scene element with which first key event is associated.
	// Likewise, interpolation event start position is always equal to first key event start position.
	const CStorySceneEvent* firstKeyEvent = section->GetEvent( interpolationEvent.m_keyGuids[ 0 ] );
	interpolationEvent.SetSceneElement( firstKeyEvent->GetSceneElement() );
	interpolationEvent.SetStartPosition( firstKeyEvent->GetStartPosition() );
}

/*
Detaches specified key.

\param interpolationEvent Interpolation event from which to detach specified key event. Must have
more than two keys (otherwise it would be left in invalid state after detaching specified key event).
\param keyEvent Key event to detach from interpolation event. Must belong to interpolationEvent.
*/
template < typename InterpolationEvent >
void InterpolationDriver< InterpolationEvent >::DetachKey( InterpolationEvent& interpolationEvent, typename InterpolationEvent::KeyEvent* keyEvent )
{
	// Interpolation event must have more than two keys (otherwise it would be left in invalid state after detaching key event).
	ASSERT( interpolationEvent.m_keys.Size() > 2 );

	Bool keyFound = false;
	for( Uint32 iKey = 0, numKeys = interpolationEvent.m_keys.Size(); iKey < numKeys; ++iKey )
	{
		if( interpolationEvent.m_keyGuids[ iKey ] == keyEvent->GetGUID() )
		{
			keyFound = true;

			interpolationEvent.m_keys.RemoveAt( iKey );
			interpolationEvent.m_keyGuids.RemoveAt( iKey );

			break;
		}
	}

	// Assert we didn't try to detach key that was not attached to specified interpolation event.
	ASSERT( keyFound );

	// Make key aware that it's no longer attached.
	keyEvent->SetInterpolationEventGUID( CGUID::ZERO );

	CStorySceneSection* section = interpolationEvent.GetSceneElement()->GetSection();

	// Interpolation event is always associated with the same scene element with which first key event is associated.
	// Likewise, interpolation event start position is always equal to first key event start position.
	const CStorySceneEvent* firstKeyEvent = section->GetEvent( interpolationEvent.m_keyGuids[ 0 ] );
	interpolationEvent.SetSceneElement( firstKeyEvent->GetSceneElement() );
	interpolationEvent.SetStartPosition( firstKeyEvent->GetStartPosition() );
}

template < typename InterpolationEvent >
void InterpolationDriver< InterpolationEvent>::SampleData( const InterpolationEvent& interpolationEvent, Float timeStep, TDynArray< TDynArray< TPair< Vector2, Vector > > >& outData, CSceneEventFunctionSimpleArgs& args )
{
	// get start time, end time and duration of interpolation event
	Float s, e, d;
	interpolationEvent.GetEventInstanceData( args.m_data, s, e, d );
	const Float interpolationEventStartTime = s;
	const Float interpolationEventEndTime = e;
	const Float interpolationEventDuration = d;

	Uint32 numSteps = static_cast< Uint32 >( interpolationEventDuration / timeStep ) + 1;
	Uint32 stepCounter = 0;

	// prepare storage in outData
	outData.Resize( InterpolationEvent::Traits::numChannels );
	for ( Uint32 iChannel = 0; iChannel < InterpolationEvent::Traits::numChannels; ++iChannel )
	{
		outData[ iChannel ].Reserve( numSteps ); // TODO: should be numSteps + numKeys
	}

	// get points on curve, space by timeStep
	Float time = interpolationEventStartTime + timeStep; // no need to compute for time == interpolationEventStartTime as we've got key there so it will be computed anyway
	while ( time < interpolationEventEndTime )			 // no need to compute for time == interpolationEventEndTime as we've got key there so it will be comptued anyway
	{
		typename InterpolationEvent::Traits::ChannelArray interpolationResult;
		const typename InterpolationEvent::KeyEvent* keyEventA = nullptr;
		const typename InterpolationEvent::KeyEvent* keyEventB = nullptr;
		Interpolate( interpolationEvent, time, interpolationResult, keyEventA, keyEventB, args );

		// get values of all channels at specified time
		for( Uint32 iChannel = 0; iChannel < InterpolationEvent::Traits::numChannels; ++iChannel )
		{
			Vector2 pointOnCurve( time, interpolationResult[ iChannel ] );
			outData[ iChannel ].PushBack( TPair< Vector2, Vector >( pointOnCurve, Vector::ZEROS ) ); // Vector::ZEROS as no Bezier handles are associated
		}

		time += timeStep;
	}

	// Get channel cache.
	TDynArray< Float >& channelCacheStorage = args.m_data[ interpolationEvent.i_channelStorage ];
	ChannelStorage< InterpolationEvent::Traits::numChannels > channelStorage( channelCacheStorage.TypedData() );

	TDynArray< Float >& cachedKeyStartTimes = args.m_data[ interpolationEvent.i_cachedKeyStartTimes ];

	// get key points
	for ( Uint32 iKey = 0, numKeys = interpolationEvent.m_keys.Size(); iKey < numKeys; ++iKey )
	{
		const typename InterpolationEvent::Key& key = interpolationEvent.m_keys[ iKey ];
		const Float keyStartTime = cachedKeyStartTimes[ iKey ];

		typename InterpolationEvent::Traits::ChannelArray& keyChannels = channelStorage.GetChannels( iKey );

		for( Uint32 iChannel = 0; iChannel < InterpolationEvent::Traits::numChannels; ++iChannel )
		{
			Vector2 pointOnCurve( keyStartTime, keyChannels[ iChannel ] );

			Vector2 incomingTangent = key.m_bezierHandles[ iChannel ].GetIncomingTangent();
			Vector2 outgoingTangent = key.m_bezierHandles[ iChannel ].GetOutgoingTangent();

			Vector hanleControlPoints;
			hanleControlPoints.X = -incomingTangent.X; // "-" because we have a different convention for incoming tangents
			hanleControlPoints.Y = -incomingTangent.Y; // "-" because we have a different convention for incoming tangents
			hanleControlPoints.Z = outgoingTangent.X;
			hanleControlPoints.W = outgoingTangent.Y;

			outData[ iChannel ].PushBack( TPair< Vector2, Vector>( pointOnCurve, hanleControlPoints ) );
		}
	}
}

/*
Recalculates bezier handles for specified range of channels.

\tparam InterpolationEvent Interpolation event type.
\tparam ChannelRange Range of channels for which to recalculate bezier handles.

// TODO: we should take locale as arg... key start times depend on locale..
*/
template < typename InterpolationEvent, typename ChannelRange >
void RecalculateBezierHandles( InterpolationEvent& interpolationEvent, ChannelRange, CSceneEventFunctionSimpleArgs& args )
{
	CStorySceneSection* section = interpolationEvent.GetSceneElement()->GetSection();

	// Get key event start times (we're not using interpolationEvent.i_cachedKeyStartTimes as
	// we don't want to depend on OnStart() being called before RecalculateBezierHandles()).
	TDynArray< Float > keyStartTimes;
	keyStartTimes.Resize( interpolationEvent.GetNumKeys() );
	for( Uint32 iKey = 0, numKeys = interpolationEvent.GetNumKeys(); iKey < numKeys; ++iKey )
	{
		const CStorySceneEvent* keyEvent = section->GetEvent( interpolationEvent.GetKeyGuid( iKey ) );
		keyStartTimes[ iKey ] = keyEvent->GetInstanceStartTime( args.m_data );
	}

	// First we need to get values of all channels of all keys.

	// Prepare channel cache storage. Note that we don't use interpolationEvent.i_channelStorage here.
	// It's not that altering its values here would be bad. It's just that we don't want to depend on
	// its existence (and hence - on having access to instance buffer). We want to be able to recompute
	// handles whenever we want.
	TDynArray< Float > channelCacheStorage;
	channelCacheStorage.Resize( interpolationEvent.GetNumKeys() * InterpolationEvent::Traits::numChannels );

	// Get channels values for all keys (those values are already preprocessed if they are angle values).
	GetChannelsForAllKeys( interpolationEvent, channelCacheStorage, ChannelRange(), args );

	// This will make working with channel cache simpler.
	ChannelStorage< InterpolationEvent::Traits::numChannels > channelStorage( channelCacheStorage.TypedData() );

	// here goes the code for computing Bezier handles (form OnStart())

	// compute bezier handles (todo: note that sometimes some keys are bezier, some are linear etc)
	// for now assume all are bezier
	for( Uint32 iChannel = ChannelRange::first; iChannel <= ChannelRange::last; ++iChannel )
	{
		for( Uint32 iKey = 0, numKeys = interpolationEvent.GetNumKeys(); iKey < numKeys; ++iKey )
		{
			typename InterpolationEvent::Key& key = interpolationEvent.GetKey( iKey );

			Vector2 incomingTangent( 1.0f, 0.0f );
			Vector2 outgoingTangent( 1.0f, 0.0f );

			key.m_bezierHandles[ iChannel ].SetIncomingTangent( incomingTangent );
			key.m_bezierHandles[ iChannel ].SetOutgoingTangent( outgoingTangent );
		}
	}

	for( Uint32 iChannel = ChannelRange::first; iChannel <= ChannelRange::last; ++iChannel )
	{
		TDynArray< Vector2 > knots;
		knots.Resize( interpolationEvent.GetNumKeys() );

		TDynArray< Vector2 > p1ControlPoints;
		p1ControlPoints.Resize( interpolationEvent.GetNumKeys() - 1 );

		TDynArray< Vector2 > p2ControlPoints;
		p2ControlPoints.Resize( interpolationEvent.GetNumKeys() - 1);

		// get knot values
		for( Uint32 iControlPoint = 0, numControlPoints = interpolationEvent.GetNumKeys(); iControlPoint < numControlPoints; ++iControlPoint )
		{
			knots[ iControlPoint ].X = keyStartTimes[ iControlPoint ];
			knots[ iControlPoint ].Y = channelStorage.GetChannels( iControlPoint )[ iChannel ];
		}

		ComputeBezierSpline( knots, p1ControlPoints, p2ControlPoints );

		// set bezier handles
		for( Uint32 iControlPoint = 0, numControlPoints = p1ControlPoints.Size(); iControlPoint < numControlPoints; ++iControlPoint )
		{
			Float dx = p1ControlPoints[ iControlPoint ].X - keyStartTimes[ iControlPoint ];
			Float dy = p1ControlPoints[ iControlPoint ].Y - channelStorage.GetChannels( iControlPoint )[ iChannel ];

			Vector2 outgoingTangent( dx, dy );
			interpolationEvent.GetKey( iControlPoint ).m_bezierHandles[ iChannel ].SetOutgoingTangent( outgoingTangent );
		}

		for( Uint32 iControlPoint = 0, numControlPoints = p2ControlPoints.Size(); iControlPoint < numControlPoints; ++iControlPoint )
		{
			Float dx = p2ControlPoints[ iControlPoint ].X - keyStartTimes[ iControlPoint + 1 ];
			Float dy = p2ControlPoints[ iControlPoint ].Y - channelStorage.GetChannels( iControlPoint + 1 )[ iChannel ];

			Vector2 incomingTangent( -dx, -dy );
			interpolationEvent.GetKey( iControlPoint + 1 ).m_bezierHandles[ iChannel ].SetIncomingTangent( incomingTangent );
		}

		// ease in (outgoing tangent of first key is overridden here)
		{
			Float dx = keyStartTimes[ 1 ] - keyStartTimes[ 0 ];

			// this is appropriate for EInterpolationEasingStyle::IES_Smooth
			Vector2 firstKeyOutgoingTangent( interpolationEvent.GetEaseInParameter() * dx, 0.0f );

			// for EInterpolationEasingStyle::IES_Rapid we need to change it a little
			if( interpolationEvent.GetEaseInStyle() == EInterpolationEasingStyle::IES_Rapid )
			{
				Float dy = channelStorage.GetChannels( 1 )[ iChannel ] - channelStorage.GetChannels( 0 )[ iChannel ];
				firstKeyOutgoingTangent.Y = interpolationEvent.GetEaseInParameter() * dy;
			}

			interpolationEvent.GetKey( 0 ).m_bezierHandles[ iChannel ].SetOutgoingTangent( firstKeyOutgoingTangent );
		}

		// ease out (incoming tangent of last key is overridden here)
		{
			const Uint32 lastKeyIndex = interpolationEvent.GetNumKeys() - 1;
			Float dx = keyStartTimes[ lastKeyIndex ] - keyStartTimes[ lastKeyIndex - 1 ];

			// this is appropriate for EInterpolationEasingStyle::IES_Smooth
			Vector2 lastKeyIncomingTangent( interpolationEvent.GetEaseOutParameter() * dx, 0 );

			// for EInterpolationEasingStyle::IES_Rapid we need to change it a little
			if( interpolationEvent.GetEaseOutStyle() == EInterpolationEasingStyle::IES_Rapid )
			{
				Float dy = channelStorage.GetChannels( lastKeyIndex )[ iChannel ] - channelStorage.GetChannels( lastKeyIndex - 1 )[ iChannel ];
				lastKeyIncomingTangent.Y = interpolationEvent.GetEaseOutParameter() * dy;
			}

			interpolationEvent.GetKey( lastKeyIndex ).m_bezierHandles[ iChannel ].SetIncomingTangent( lastKeyIncomingTangent );
		}
	}
}

/*
Overloaded version of RecalculateBezierHandles() for empty channel ranges.
*/
template < typename InterpolationEvent >
RED_INLINE void RecalculateBezierHandles( InterpolationEvent& interpolationEvent, InterpolationChannelRangeNone, CSceneEventFunctionSimpleArgs& args )
{
	// nothing to do
}

/*
Preprocesses channel values.

\tparam InterpolationEvent Interpolation event type.
\tparam ChannelRange Range of channels whose values to preprocess.

\param interpolationEvent Interpolation event whose channel values to preprocess.
\param outChannelCacheStorage (out) Storage for preprocessed channel values.
*/
template < typename InterpolationEvent, typename ChannelRange >
void PreprocessChannels( const InterpolationEvent& interpolationEvent, TDynArray< Float >& outChannelCacheStorage, ChannelRange )
{
	// This will make working with channel cache simpler.
	ChannelStorage< InterpolationEvent::Traits::numChannels > channelStorage( outChannelCacheStorage.TypedData() );

	// any special handling of channel values goes here

	// handle angles
	for( Uint32 iChannel = ChannelRange::first; iChannel <= ChannelRange::last; ++iChannel )
	{
		if( InterpolationEvent::Traits::GetChannelType( iChannel ) == EInterpolationChannelType::ICT_AngleInDegrees )
		{
			Float referenceAngle = 0.0f;
			for( Uint32 iKey = 0, numKeys = interpolationEvent.GetNumKeys(); iKey < numKeys; ++iKey )
			{
				Float nearestAngle = EulerAngles::ToNearestAngle( channelStorage.GetChannels( iKey )[ iChannel ], referenceAngle );
				channelStorage.GetChannels( iKey )[ iChannel ] = nearestAngle;
				referenceAngle = nearestAngle;
			}
		}
	}
}

/*
Overloaded version of PreprocessChannels() for empty channel ranges.
*/
template < typename InterpolationEvent, typename ChannelRange >
void PreprocessChannels( const InterpolationEvent& interpolationEvent, TDynArray< Float >& outChannelCacheStorage, InterpolationChannelRangeNone, CSceneEventFunctionArgs& args )
{
	// nothing to do
}

/*
Gets channel values.

\tparam InterpolationEvent Interpolation event type.
\tparam ChannelRange Range of channels whose values to get.

\param interpolationEvent Interpolation event whose channel values to get.
\param outChannelCacheStorage (out) Storage for channel values of all keys. Channel values are already preprocessed.
*/
template < typename InterpolationEvent, typename ChannelRange >
void GetChannelsForAllKeys( const InterpolationEvent& interpolationEvent, TDynArray< Float >& outChannelCacheStorage, ChannelRange, CSceneEventFunctionSimpleArgs& args )
{
	const CStorySceneSection* section = interpolationEvent.GetSceneElement()->GetSection();

	// This will make working with channel cache simpler.
	ChannelStorage< InterpolationEvent::Traits::numChannels > channelStorage( outChannelCacheStorage.TypedData() );

	// For all keys, get value of all channels from given channel range.
	for( Uint32 iKey = 0, numKeys = interpolationEvent.GetNumKeys(); iKey < numKeys; ++iKey )
	{
		const typename InterpolationEvent::KeyEvent* keyEvent = static_cast< const typename InterpolationEvent::KeyEvent* >( section->GetEvent( interpolationEvent.GetKeyGuid( iKey ) ) );
		typename InterpolationEvent::Traits::ChannelArray& channels = channelStorage.GetChannels( iKey );
		InterpolationEvent::Traits::GetChannels( *keyEvent, channels, ChannelRange(), args );
	}

	PreprocessChannels( interpolationEvent, outChannelCacheStorage, ChannelRange() );
}

/*
Oveloaded version of GetChannels() for empty channel ranges.
*/
template < typename InterpolationEvent >
RED_INLINE void GetChannelsForAllKeys( const InterpolationEvent& /* interpolationEvent */, TDynArray< Float >& /* outChannelCacheStorage */, InterpolationChannelRangeNone, CSceneEventFunctionSimpleArgs& args )
{
	// nothing to do
}

/*
Gets channel values.

\tparam InterpolationEvent Interpolation event type.
\tparam ChannelRange Range of channels whose values to get.

\param interpolationEvent Interpolation event whose channel values to get.
\param outChannelCacheStorage (out) Storage for channel values of all keys. Channel values are already preprocessed.
*/
template < typename InterpolationEvent, typename ChannelRange >
void GetChannelsForVolatileKeys( const InterpolationEvent& interpolationEvent, TDynArray< Float >& outChannelCacheStorage, ChannelRange, CSceneEventFunctionSimpleArgs& args )
{
	const CStorySceneSection* section = interpolationEvent.GetSceneElement()->GetSection();

	// This will make working with channel cache simpler.
	ChannelStorage< InterpolationEvent::Traits::numChannels > channelStorage( outChannelCacheStorage.TypedData() );

	// For all keys, get value of all channels from given channel range.
	for( Uint32 iKey = 0, numKeys = interpolationEvent.GetNumKeys(); iKey < numKeys; ++iKey )
	{
		if( interpolationEvent.GetKey( iKey ).m_volatile )
		{
			const typename InterpolationEvent::KeyEvent* keyEvent = static_cast< const typename InterpolationEvent::KeyEvent* >( section->GetEvent( interpolationEvent.GetKeyGuid( iKey ) ) );
			typename InterpolationEvent::Traits::ChannelArray& channels = channelStorage.GetChannels( iKey );
			InterpolationEvent::Traits::GetChannels( *keyEvent, channels, ChannelRange(), args );
		}
	}

	PreprocessChannels( interpolationEvent, outChannelCacheStorage, ChannelRange() );
}

/*
Oveloaded version of GetChannels() for empty channel ranges.
*/
template < typename InterpolationEvent >
RED_INLINE void GetChannelsForVolatileKeys( const InterpolationEvent& /* interpolationEvent */, TDynArray< Float >& /* outChannelCacheStorage */, InterpolationChannelRangeNone, CSceneEventFunctionSimpleArgs& args )
{
	// nothing to do
}

/*
Returns index at which specified event should be inserted into interpolation event.

\param keyEventGuid GUID of key event to check. Must belong to the same section as interpolation event.
\return Index at which specified event should be inserted into interpolation event.
*/
template < typename InterpolationEvent >
Uint32 InterpolationDriver< InterpolationEvent>::GetInsertionIndex( const InterpolationEvent& interpolationEvent, CGUID keyEventGuid )
{
	const CStorySceneSection* section = interpolationEvent.GetSceneElement()->GetSection();

	const Float keyEventNormalizedStartTime = section->GetEventNormalizedStartTime( keyEventGuid );

	Uint32 insertionIndex = interpolationEvent.GetNumKeys();
	for( Uint32 iKey = 0, numKeys = interpolationEvent.GetNumKeys(); iKey < numKeys; ++iKey )
	{
		const Float ithKeyEventNormalizedStartTime = section->GetEventNormalizedStartTime( interpolationEvent.m_keyGuids[ iKey ] );
		if( keyEventNormalizedStartTime <= ithKeyEventNormalizedStartTime )
		{
			insertionIndex = iKey;
			break;
		}
	}

	return insertionIndex;
}

// =================================================================================================
} // namespace StoryScene
// =================================================================================================
