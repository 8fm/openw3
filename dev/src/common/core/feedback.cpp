/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "feedback.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////

// The feedback system
IFeedbackSystem* GFeedback = &GNullFeedback;

// The splash screen system
ISplashScreen* GSplash = &GNullSplash;

// The on screen log
IOnScreenLog* GScreenLog = &GNullOnScreenLog;
