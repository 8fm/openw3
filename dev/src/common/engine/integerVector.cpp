#include "build.h"
#include "integerVector.h"

// This is tricky - we _neeed_ to use as base 2 factor here since
// those are the only ones that have finite expansion as a fraction in binary system
// Using for example 0.001f as a conversion factor is _bad_ idea.
const Float IntegerUnit::TRIGGER_TO_WORLD = 0.00390625f; // 1.0f / 256.0f
const Float IntegerUnit::WORLD_TO_TRIGGER = 256.0f;

// This is not inlined (yet) for debugging purposes
Int32 IntegerUnit::ToTriggerUnits( const Float x )
{
	return (Int32)(x * WORLD_TO_TRIGGER);
}

Float IntegerUnit::FromTriggerUnits( const Int32 x )
{
	return (Float)(x * TRIGGER_TO_WORLD);
}

//---------------------------------------------------------------------------

