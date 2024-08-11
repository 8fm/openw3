/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#define RED_ORBIS_SYSTEM_GESTURE

#include <system_gesture.h>

//////////////////////////////////////////////////////////////////////////
// Forward Declarations
//////////////////////////////////////////////////////////////////////////
struct SGestureEvent;

//////////////////////////////////////////////////////////////////////////
// CGestureRecognizerOrbis
//////////////////////////////////////////////////////////////////////////
class CGestureRecognizerOrbis
{
public:
	CGestureRecognizerOrbis();
	~CGestureRecognizerOrbis();

	Bool Init( Int32 padHandle );

public:
	void Update( const ScePadData& padData, TDynArray< SGestureEvent >& outGestureEvents );

private:
	static const Uint32 MAX_TOUCH_EVENTS_PER_RECOGNIZER = 16; // As documented by the library

private:
	SceSystemGestureTouchRecognizer	m_pinchRecognizer;
	SceSystemGestureTouchRecognizer	m_flickRecognizer;
	SceSystemGestureTouchRecognizer	m_dragRecognizer;

private:
	Bool InitPadInfo();
	Bool InitPinchRecognizer();
	Bool InitFlickRecognizer();
	Bool InitDragRecognizer();

private:
	void UpdateRecognizer( SceSystemGestureTouchRecognizer* pRecognizer, TDynArray< SGestureEvent >& outGestureEvents, Bool dropEvents );

private:
	void HandlePinchOutInEvent( const SceSystemGesturePinchOutInEventProperty& eventProperty, const SceSystemGestureTouchState& eventState, TDynArray< SGestureEvent >& outGestureEvents );
	void HandleFlickEvent( const SceSystemGestureFlickEventProperty& eventProperty, const SceSystemGestureTouchState& eventState, TDynArray< SGestureEvent >& outGestureEvents );
	void HandleDragEvent( const SceSystemGestureDragEventProperty& eventProperty, const SceSystemGestureTouchState& eventState, TDynArray< SGestureEvent >& outGestureEvents );

private:
	Int32							m_gestureHandle;
	Int32							m_padHandle;
	Float							m_touchPadWidth;
	Float							m_touchPadHeight;
	Float							m_touchPadNormWidth;
	Float							m_touchPadNormHeight;
};