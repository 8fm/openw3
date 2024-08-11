/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/engine/inputUtils.inl"

#include "inputDeviceMouseDurangoDebug.h"

#if !defined( RED_FINAL_BUILD )

CInputDeviceMouseDurangoDebug::CInputDeviceMouseDurangoDebug()
{
}

CInputDeviceMouseDurangoDebug::~CInputDeviceMouseDurangoDebug()
{
}

void CInputDeviceMouseDurangoDebug::Reset()
{
}

void CInputDeviceMouseDurangoDebug::Update( BufferedInput& outBufferedInput )
{
	// Save previous state

	static SRawMouseReading previousState;

	extern SRawMouseReading DurangoHackGetMouseReading();
	previousState = DurangoHackGetMouseReading();

	// Update mouse state

	extern void DurangoHackUpdateMouseReading();
	DurangoHackUpdateMouseReading();

	// Get latest state

	const SRawMouseReading currentState = DurangoHackGetMouseReading();

	// Create input events

	if ( currentState.dx != 0 )
	{
		outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapMouseAxis( EAxis::DeltaX ), IACT_Axis, Float(currentState.dx) ) );
	}
	if ( currentState.dy != 0 )
	{
		outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapMouseAxis( EAxis::DeltaY ), IACT_Axis, Float(currentState.dy) ) );
	}

	if ( currentState.leftButtonDown != previousState.leftButtonDown )
	{
		const Bool isDown = currentState.leftButtonDown;
		outBufferedInput.PushBack( SBufferedInputEvent( IK_LeftMouse, isDown ? IACT_Press : IACT_Release, isDown ? 1.0f : 0.f ) );
	}
	if ( currentState.middleButtonDown != previousState.middleButtonDown )
	{
		const Bool isDown = currentState.middleButtonDown;
		outBufferedInput.PushBack( SBufferedInputEvent( IK_LeftMouse, isDown ? IACT_Press : IACT_Release, isDown ? 1.0f : 0.f ) );
	}
	if ( currentState.rightButtonDown != previousState.rightButtonDown )
	{
		const Bool isDown = currentState.rightButtonDown;
		outBufferedInput.PushBack( SBufferedInputEvent( IK_LeftMouse, isDown ? IACT_Press : IACT_Release, isDown ? 1.0f : 0.f ) );
	}
}

#endif // !defined( RED_FINAL_BUILD ) || defined( DEMO_BUILD )