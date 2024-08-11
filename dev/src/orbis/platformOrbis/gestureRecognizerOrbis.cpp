/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/engine/gestureEvent.h"

#include "gestureRecognizerOrbis.h"

namespace Config
{
	// Stretch touchpad axis when normalizing. E.g., both vertical and horizontal map from [0, 1] despite dimension differences
	// If not stretching, then you may have to account for the size ratios in anything that uses the results
	// You'll still have to adjust different deadzones, but at least the range of input will be known (i.e., [0,1])
	TConfigVar< Bool > cvInitStretchAxis( "Input/Gestures/PS4/Init", "StretchAxis", true, eConsoleVarFlag_Developer );
};

CGestureRecognizerOrbis::CGestureRecognizerOrbis()
	: m_gestureHandle( -1 )
	, m_touchPadWidth( 0.f )
	, m_touchPadHeight( 0.f )
	, m_touchPadNormWidth( 0.f )
	, m_touchPadNormHeight( 0.f )
{
}

CGestureRecognizerOrbis::~CGestureRecognizerOrbis()
{
	if ( m_gestureHandle >= 0 )
	{
		::sceSystemGestureClose( m_gestureHandle );
	}
}

Bool CGestureRecognizerOrbis::Init( Int32 padHandle )
{
#ifdef RED_ORBIS_SYSTEM_GESTURE
	if ( padHandle < 0 )
	{
		ERR_ENGINE(TXT("CGestureRecognizerOrbis: invalid padHandle %d"), padHandle );
		return false;
	}
	m_padHandle = padHandle;

	m_gestureHandle = ::sceSystemGestureOpen( SCE_SYSTEM_GESTURE_INPUT_TOUCH_PAD, nullptr );
	if ( m_gestureHandle < 0 )
	{
		ERR_ENGINE(TXT("Failed to call sceSystemGestureOpen. Internal error code 0x%08X"), m_gestureHandle );
		return false;
	}

	if ( !InitPadInfo() )
	{
		return false;
	}

	if ( !InitFlickRecognizer() )
	{
		return false;
	}

	if ( !InitDragRecognizer() )
	{
		return false;
	}

	if ( !InitPinchRecognizer() )
	{
		return false;
	}
#endif // RED_ORBIS_SYSTEM_GESTURE

	return true;
}

Bool CGestureRecognizerOrbis::InitPadInfo()
{
	ScePadControllerInformation padInfo;
	Red::System::MemoryZero( &padInfo, sizeof(ScePadControllerInformation) );

	const Int32 sceErr = ::scePadGetControllerInformation( m_padHandle, &padInfo );
	if ( sceErr != SCE_OK )
	{
		ERR_ENGINE(TXT("scePadGetControllerInformation failed. Internal error code 0x%08X"), sceErr );
		return false;
	}

	m_touchPadWidth = Max< Float >( 1.f, static_cast< Float >( padInfo.touchPadInfo.resolution.x ) );
	m_touchPadHeight = Max< Float >( 1.f, static_cast< Float >( padInfo.touchPadInfo.resolution.y ) );
	
	if ( Config::cvInitStretchAxis.Get() )
	{
		m_touchPadNormHeight = m_touchPadHeight;
		m_touchPadNormWidth = m_touchPadWidth;
	}
	else
	{
		// Normalize against the biggest dimension to avoid "stretching" an axis
		const Float maxSize = Max< Float >( m_touchPadWidth, m_touchPadHeight );
		m_touchPadNormHeight = m_touchPadNormWidth = maxSize;
	}

	LOG_ENGINE(TXT("Touchpad dimension: %1.2fx%1.2f"), m_touchPadWidth, m_touchPadHeight );

	return true;
}

Bool CGestureRecognizerOrbis::InitPinchRecognizer()
{
	const Int32 sceErr = ::sceSystemGestureCreateTouchRecognizer( m_gestureHandle, &m_pinchRecognizer, SCE_SYSTEM_GESTURE_TYPE_PINCH_OUT_IN, nullptr, nullptr );
	if ( sceErr != SCE_OK )
	{
		ERR_ENGINE(TXT("sceSystemGestureCreateTouchRecognizer failed for SCE_SYSTEM_GESTURE_TYPE_PINCH_OUT_IN. Internal error code 0x%08X"), sceErr );
		return false;
	}

	return true;
}

Bool CGestureRecognizerOrbis::InitFlickRecognizer()
{
	const Int32 sceErr = ::sceSystemGestureCreateTouchRecognizer( m_gestureHandle, &m_flickRecognizer, SCE_SYSTEM_GESTURE_TYPE_FLICK, nullptr, nullptr );
	if ( sceErr != SCE_OK )
	{
		ERR_ENGINE(TXT("sceSystemGestureCreateTouchRecognizer failed for SCE_SYSTEM_GESTURE_TYPE_FLICK. Internal error code 0x%08X"), sceErr );
		return false;
	}

	return true;
}

Bool CGestureRecognizerOrbis::InitDragRecognizer()
{
	const Int32 sceErr = ::sceSystemGestureCreateTouchRecognizer( m_gestureHandle, &m_dragRecognizer, SCE_SYSTEM_GESTURE_TYPE_DRAG, nullptr, nullptr );
	if ( sceErr != SCE_OK )
	{
		ERR_ENGINE(TXT("sceSystemGestureCreateTouchRecognizer failed for SCE_SYSTEM_GESTURE_TYPE_DRAG. Internal error code 0x%08X"), sceErr );
		return false;
	}

	return true;
}

void CGestureRecognizerOrbis::Update( const ScePadData& padData, TDynArray< SGestureEvent >& outGestureEvents )
{
#ifdef RED_ORBIS_SYSTEM_GESTURE
	Int32 sceErr = SCE_OK;

	ScePadData mutablePadData = padData;
	SceSystemGestureTouchPadData touchPadData;
	touchPadData.padHandle = m_padHandle;
	touchPadData.reportNumber = 1; 
	touchPadData.padDataBuffer = &mutablePadData;

	sceErr = ::sceSystemGestureUpdatePrimitiveTouchRecognizer(m_gestureHandle, &touchPadData);
	if ( sceErr != SCE_OK )
	{
		ERR_ENGINE(TXT("sceSystemGestureUpdatePrimitiveTouchRecognizer failed. Internal error code 0x%08X"), sceErr );
		return;
	}

	const Uint32 oldSize = outGestureEvents.Size();
	Bool dropEvents = false;
	UpdateRecognizer( &m_pinchRecognizer, outGestureEvents, dropEvents );
	dropEvents = oldSize < outGestureEvents.Size();

	UpdateRecognizer( &m_flickRecognizer, outGestureEvents, dropEvents );
	dropEvents = oldSize < outGestureEvents.Size();

	UpdateRecognizer( &m_dragRecognizer, outGestureEvents, dropEvents );

#endif // RED_ORBIS_SYSTEM_GESTURE
}

void CGestureRecognizerOrbis::UpdateRecognizer( SceSystemGestureTouchRecognizer* pRecognizer, TDynArray< SGestureEvent >& outGestureEvents, Bool dropEvents )
{
	RED_FATAL_ASSERT( pRecognizer, "Null recognizer!" );

	Int32 sceErr = SCE_OK;
	
	sceErr = ::sceSystemGestureUpdateTouchRecognizer( m_gestureHandle, pRecognizer );
	if ( sceErr != SCE_OK )
	{
		ERR_ENGINE(TXT("Failed to call sceSystemGestureUpdateTouchRecognizer. Internal error code 0x%08X"), sceErr );
		return;
	}

	SceSystemGestureTouchEvent touchEvents[ MAX_TOUCH_EVENTS_PER_RECOGNIZER ];
	Uint32 numTouchEvents = 0;
	sceErr = ::sceSystemGestureGetTouchEvents( m_gestureHandle, pRecognizer, &touchEvents[0], MAX_TOUCH_EVENTS_PER_RECOGNIZER, &numTouchEvents );
	if ( sceErr != SCE_OK )
	{
		ERR_ENGINE(TXT("Failed to call sceSystemGestureGetTouchEvents. Internal error code 0x%08X"), sceErr );
		return;
	}

	if ( dropEvents )
	{
		return;
	}

	for ( Uint32 i = 0; i < numTouchEvents; ++i )
	{
		const SceSystemGestureTouchEvent& event = touchEvents[ i ];

		switch ( event.gestureType )
		{
		case SCE_SYSTEM_GESTURE_TYPE_PINCH_OUT_IN:	
			{
				HandlePinchOutInEvent( event.property.pinchOutIn, event.eventState, outGestureEvents );
			}
			break;
		case SCE_SYSTEM_GESTURE_TYPE_FLICK:
			{
				HandleFlickEvent( event.property.flick, event.eventState, outGestureEvents );
			}
			break;
		case SCE_SYSTEM_GESTURE_TYPE_DRAG:
			{
				HandleDragEvent( event.property.drag, event.eventState, outGestureEvents );
			}
		default:
			break;
		}
	}
}

void CGestureRecognizerOrbis::HandlePinchOutInEvent( const SceSystemGesturePinchOutInEventProperty& eventProperty, const SceSystemGestureTouchState& eventState, TDynArray< SGestureEvent >& outGestureEvents )
{
	if ( eventState != SCE_SYSTEM_GESTURE_TOUCH_STATE_ACTIVE )
	{
		return;
	}

	SGestureEvent gestureEvent;
	gestureEvent.m_type = eGestureEventType_Pinch;
	gestureEvent.m_pinch.m_scaleFactor = eventProperty.scale;

	outGestureEvents.PushBack( gestureEvent );
}

static void InitSwipeEvent( SGestureEvent& outGestureEvent, const Float* trans, const Float* vel, Bool isFlick )
{
	outGestureEvent.m_type = eGestureEventType_Swipe;
	SGestureEvent::Swipe& swipe = outGestureEvent.m_swipe;
	swipe.m_normalizedTranslation[ 0 ] = trans[0];
	swipe.m_normalizedTranslation[ 1 ] = trans[1];
	swipe.m_normalizedVelocity[ 0 ] = vel[0];
	swipe.m_normalizedVelocity[ 1 ] = vel[1];
	swipe.m_isFlick = isFlick;
}

static void InitPanEvent( SGestureEvent& outGestureEvent, const Float* trans )
{
	outGestureEvent.m_type = eGestureEventType_Pan;
	SGestureEvent::Pan& pan = outGestureEvent.m_pan;
	pan.m_normalizedTranslation[ 0 ] = trans[0];
	pan.m_normalizedTranslation[ 1 ] = trans[1];
}

void CGestureRecognizerOrbis::HandleFlickEvent( const SceSystemGestureFlickEventProperty& eventProperty, const SceSystemGestureTouchState& eventState, TDynArray< SGestureEvent >& outGestureEvents )
{
	// Flip Y-axis so up is positive

	if ( eventState == SCE_SYSTEM_GESTURE_TOUCH_STATE_ACTIVE )
	{
		const Float trans[] = { 
			(eventProperty.releasedPosition.x - eventProperty.pressedPosition.x ) / m_touchPadNormWidth,
			-(eventProperty.releasedPosition.y - eventProperty.pressedPosition.y ) / m_touchPadNormHeight,
		};

		const Float vel[] = {
			eventProperty.deltaVector.x / m_touchPadNormWidth,
			-eventProperty.deltaVector.y / m_touchPadNormHeight,
		};

		SGestureEvent swipeEvent;
		InitSwipeEvent( swipeEvent, trans, vel, true );
		outGestureEvents.PushBack( swipeEvent );
	}
}

void CGestureRecognizerOrbis::HandleDragEvent( const SceSystemGestureDragEventProperty& eventProperty, const SceSystemGestureTouchState& eventState, TDynArray< SGestureEvent >& outGestureEvents )
{
	// Flip Y-axis so up is positive

	if ( eventState == SCE_SYSTEM_GESTURE_TOUCH_STATE_ACTIVE )
	{
		// Don't spam events when hovering
		if ( eventProperty.deltaVector.x != 0.f || eventProperty.deltaVector.y != 0.f )
		{
			const Float trans[] = {
				eventProperty.deltaVector.x / m_touchPadNormWidth,
				-eventProperty.deltaVector.y / m_touchPadNormHeight,
			};

			SGestureEvent panEvent;
			InitPanEvent( panEvent, trans );
			outGestureEvents.PushBack( panEvent );
		}
	}
	else if ( eventState == SCE_SYSTEM_GESTURE_TOUCH_STATE_END )
	{
		const Float vel[] = {
			eventProperty.deltaVector.x / m_touchPadNormWidth,
			-eventProperty.deltaVector.y / m_touchPadNormHeight,
		};

		const Float trans[] = { 
			(eventProperty.currentPosition.x - eventProperty.pressedPosition.x ) / m_touchPadNormWidth,
			-(eventProperty.currentPosition.y - eventProperty.pressedPosition.y ) / m_touchPadNormHeight,
		};

		SGestureEvent swipeEvent;
		InitSwipeEvent( swipeEvent, trans, vel, false );
		outGestureEvents.PushBack( swipeEvent );
	}
}
