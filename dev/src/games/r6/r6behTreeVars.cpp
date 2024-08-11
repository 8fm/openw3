/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "r6behTreeVars.h"
#include "aiAction.h"

////////////////////////////////////////////////////////////////////////
// TBehTreeValue
////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CBehTreeValAIAction );

CAIAction* CBehTreeValAIAction::GetVal( const CBehTreeSpawnContext& context ) const
{
	return TGetVal( context );
}