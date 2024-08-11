#include "build.h"

#include "..\..\common\game\selfUpdatingComponent.h"
#include "followerPOIComponent.h"

IMPLEMENT_ENGINE_CLASS( CFollowerPOIComponent );

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CFollowerPOIComponent::funcGetPriority( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( GetPriority() );
}