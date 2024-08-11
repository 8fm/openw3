/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//////////////////////////////////////////////////////////////////////////
// EGestureEventType
//////////////////////////////////////////////////////////////////////////
enum EGestureEventType
{
	eGestureEventType_Invalid,
	eGestureEventType_Swipe,
	eGestureEventType_Pan,
	eGestureEventType_Pinch,
};

//////////////////////////////////////////////////////////////////////////
// SGestureEvent
//////////////////////////////////////////////////////////////////////////
struct SGestureEvent
{
	struct Swipe
	{
		Float	m_normalizedTranslation[2];
		Float	m_normalizedVelocity[2];
		Bool	m_isFlick;
	};

	struct Pan
	{
		Float	m_normalizedTranslation[2];
	};

	struct Pinch
	{
		Float	m_scaleFactor;
	};

	union
	{
		Swipe	m_swipe;
		Pan		m_pan;
		Pinch	m_pinch;
	};

	EGestureEventType m_type;

	SGestureEvent()
		: m_type( eGestureEventType_Invalid )
	{}
};
