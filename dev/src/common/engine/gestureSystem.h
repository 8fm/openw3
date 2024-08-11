/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "inputKeyToDeviceMapping.h"
#include "userProfile.h"
#include "rawInputManager.h"
#include "gestureEvent.h"
#include "gestureListener.h"

class IGestureListener;

class CGestureSystem : public CObject
{
	DECLARE_ENGINE_CLASS( CGestureSystem, CObject, 0 );

	TDynArray< IGestureListener* >		m_gestureListener;

public:

	CGestureSystem();
	virtual ~CGestureSystem();

	void Update( const TDynArray< SGestureEvent >& gestureEvents );

	void RegisterListener( IGestureListener* listener );
	void UnregisterListener( IGestureListener* listener );
	void UnregisterAllListeners();

private:
	void OnSwipe( const SGestureEvent::Swipe& swipeEvent );
	void OnPan( const SGestureEvent::Pan& panEvent );
	void OnPinch( const SGestureEvent::Pinch& pinchEvent );
};


BEGIN_CLASS_RTTI( CGestureSystem );
	PARENT_CLASS( CObject )
END_CLASS_RTTI();

