/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gestureSystem.h"
#include "gestureListener.h"

IMPLEMENT_ENGINE_CLASS( CGestureSystem );

namespace Config
{
	TConfigVar< Bool >	cvFlickForSwipe( "Input/Gestures", "FlickForSwipe", true, eConsoleVarFlag_Developer );
	
	// When assigning dead-zones, take into account that the thumb can swipe farther when moving towards the palm
	// so can swipe further left with the left thumb - further right with the right thumb.
	// Also, if the gesture recognizer doesn't stretch axes, then swiping up and down may result in larger magnitudes than left or right

	TConfigVar< Float > cvSwipeVerticalDeadzoneSq( "Input/Gestures", "SwipeVerticalDeadzoneSq", 0.2f, eConsoleVarFlag_Developer);
	TConfigVar< Float > cvSwipeHorizontalDeadzoneSq( "Input/Gestures", "SwipeHorizontalDeadzoneSq", 0.05f, eConsoleVarFlag_Developer);

#ifdef RED_LOGGING_ENABLED
	static TConfigVar< Bool > cvLogSwipe( "Input/Gestures/Log", "Swipe", true, eConsoleVarFlag_Developer );
	static TConfigVar< Bool > cvLogPan( "Input/Gestures/Log", "Pan", true, eConsoleVarFlag_Developer );
	static TConfigVar< Bool > cvLogPinch( "Input/Gestures/Log", "Pinch", true, eConsoleVarFlag_Developer );
#endif

	TConfigVar< Bool >	cvEnableGestures( "Input/Gestures", "EnableGestures", true, eConsoleVarFlag_Save );
	TConfigVar< Bool >	cvEnableSwipe( "Input/Gestures", "EnableSwipe", true, eConsoleVarFlag_Save );
	TConfigVar< Bool >	cvEnablePan( "Input/Gestures", "EnablePan", true, eConsoleVarFlag_Save );
	TConfigVar< Bool >	cvEnablePinch( "Input/Gestures", "EnablePinch", true, eConsoleVarFlag_Save );
}

CGestureSystem::CGestureSystem()
{
}

CGestureSystem::~CGestureSystem()
{
}

void CGestureSystem::Update( const TDynArray< SGestureEvent >& gestureEvents )
{
	if ( !Config::cvEnableGestures.Get() )
	{
		return;
	}

	for ( const SGestureEvent& event : gestureEvents  )
	{
		switch ( event.m_type )
		{
		case eGestureEventType_Swipe:
			{
				OnSwipe( event.m_swipe );
			}
			break;
		case eGestureEventType_Pan:
			{
				OnPan( event.m_pan );
			}
			break;
		case eGestureEventType_Pinch:
			{
				OnPinch( event.m_pinch );
			}
			break;
		default:
			RED_FATAL( "Unhandled gesture event type %u", event.m_type );
			break;
		}
	}
}

#ifdef RED_LOGGING_ENABLED
static const Char* GetSwipeTxtForLog( IGestureListener::EStandardSwipe swipe )
{
	const Char* txt = TXT("<Unknown>");
	switch ( swipe )
	{
	case IGestureListener::SWIPE_LEFT:
		txt = TXT("SWIPE_LEFT");
		break;
	case IGestureListener::SWIPE_RIGHT:
		txt = TXT("SWIPE_RIGHT");
		break;
	case IGestureListener::SWIPE_UP:
		txt = TXT("SWIPE_UP");
		break;
	case IGestureListener::SWIPE_DOWN:
		txt = TXT("SWIPE_DOWN");
		break;
	default:
		break;
	}

	return txt;
}
#endif // RED_LOGGING_ENABLED

static Bool GetSwipe( const SGestureEvent::Swipe& swipeEvent, IGestureListener::EStandardSwipe& outStandardSwipe )
{
	if ( !swipeEvent.m_isFlick && Config::cvFlickForSwipe.Get() )
	{
		return false;
	}

	const Float x = swipeEvent.m_normalizedTranslation[0];
	const Float y = swipeEvent.m_normalizedTranslation[1];
	const Float sqMag = x * x + y * y;

	Bool retval = false;
	if( Abs( x ) > Abs( y ) )
	{
		if ( sqMag >= Config::cvSwipeHorizontalDeadzoneSq.Get() )
		{
			outStandardSwipe = x > 0.f ? IGestureListener::SWIPE_RIGHT : IGestureListener::SWIPE_LEFT;
			retval = true;
		}
	}
	else
	{
		if ( sqMag >= Config::cvSwipeVerticalDeadzoneSq.Get() )
		{
			outStandardSwipe = y > 0.f ? IGestureListener::SWIPE_UP : IGestureListener::SWIPE_DOWN;
			retval = true;
		}
	}

	return retval;
}

#ifdef RED_LOGGING_ENABLED
static String GetSwipeStringForLog( const SGestureEvent::Swipe& swipeEvent )
{
	const Float magSq = swipeEvent.m_normalizedTranslation[0] * swipeEvent.m_normalizedTranslation[0]
					+ swipeEvent.m_normalizedTranslation[1] * swipeEvent.m_normalizedTranslation[1];

	return String::Printf(TXT("[Swipe: trans={%1.2f,%1.2f}, vel={%1.2f,%1.2f}, magSq=%1.2f, isFlick=%d]"), 
		swipeEvent.m_normalizedTranslation[0],
		swipeEvent.m_normalizedTranslation[1],
		swipeEvent.m_normalizedVelocity[0],
		swipeEvent.m_normalizedVelocity[1],
		magSq,
		(swipeEvent.m_isFlick ? 1 : 0) );
}
#endif

void CGestureSystem::OnSwipe( const SGestureEvent::Swipe& swipeEvent )
{
	if ( !Config::cvEnableSwipe.Get() )
	{
		return;
	}

#ifdef RED_LOGGING_ENABLED
	if ( Config::cvLogSwipe.Get() )
	{
		LOG_ENGINE(TXT("%ls"), GetSwipeStringForLog( swipeEvent ).AsChar());
	}
#endif

	IGestureListener::EStandardSwipe standardSwipe;
	if ( GetSwipe( swipeEvent, standardSwipe ) )
	{
#ifdef RED_LOGGING_ENABLED
		if ( Config::cvLogSwipe.Get() )
		{
			LOG_ENGINE(TXT("%ls --> %ls"), GetSwipeStringForLog( swipeEvent ).AsChar(), GetSwipeTxtForLog( standardSwipe ));
		}
#endif

		for ( IGestureListener* gestureListener : m_gestureListener )
		{
			gestureListener->OnSwipe( standardSwipe );
		}
	}
}

void CGestureSystem::OnPan( const SGestureEvent::Pan& panEvent )
{
	if ( !Config::cvEnablePan.Get() )
	{
		return;
	}

#ifdef RED_LOGGING_ENABLED
	if ( Config::cvLogPan.Get() )
	{
		LOG_ENGINE(TXT("Pan {%1.8f,%1.8f}"), panEvent.m_normalizedTranslation[0], panEvent.m_normalizedTranslation[1]);
	}
#endif

	for ( IGestureListener* gestureListener : m_gestureListener )
	{
		const Float x = panEvent.m_normalizedTranslation[0];
		const Float y = panEvent.m_normalizedTranslation[1];
		gestureListener->OnSwipe( x, y );
	}
}

void CGestureSystem::OnPinch( const SGestureEvent::Pinch& pinchEvent )
{
	if ( !Config::cvEnablePinch.Get() )
	{
		return;
	}

#ifdef RED_LOGGING_ENABLED
	if ( Config::cvLogPinch.Get() )
	{
		LOG_ENGINE(TXT("Pinch %1.2f"), pinchEvent.m_scaleFactor );
	}
#endif

	for ( IGestureListener* gestureListener : m_gestureListener )
	{
		gestureListener->OnPinch( pinchEvent.m_scaleFactor );
	}
}

void CGestureSystem::RegisterListener( IGestureListener* listener )
{
	if( !listener )
	{
		return;
	}

	m_gestureListener.PushBackUnique( listener );
}

void CGestureSystem::UnregisterListener( IGestureListener* listener )
{
	m_gestureListener.Remove( listener );
}

void CGestureSystem::UnregisterAllListeners()
{
	m_gestureListener.Clear();
}
