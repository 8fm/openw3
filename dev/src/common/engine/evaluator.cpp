/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "evaluator.h"

IMPLEMENT_ENGINE_CLASS( IEvaluator );

CurveParameter* IEvaluator::GetCurves()
{
	return NULL;
}

Bool IEvaluator::IsFunction() const
{
	return false;
}