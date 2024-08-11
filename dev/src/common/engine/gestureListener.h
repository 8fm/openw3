/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "inputManager.h"


class IGestureListener
{
public:
	enum EStandardSwipe
	{
		SWIPE_LEFT,
		SWIPE_RIGHT,
		SWIPE_DOWN,
		SWIPE_UP
	};

	virtual ~IGestureListener() {}

	virtual void OnSwipe( EStandardSwipe swipe ) {}
	virtual void OnSwipe( Float x, Float y ) {}

	virtual void OnPinch( Float pinch ) {}

// 	virtual void OnTwoFingerSwipe( EStandardSwipe swipe ) {}
// 	virtual void OnTwoFingerSwipe( Float x, Float y ) {}
};
