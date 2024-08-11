/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiPositionPrediction.h"

IMPLEMENT_ENGINE_CLASS( SAIPositionPrediction )


CName SAIPositionPrediction::EventName()
{
	return CNAME( AI_PredictPosition );
}
