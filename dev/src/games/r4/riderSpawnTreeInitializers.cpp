#include "build.h"
#include "riderSpawnTreeInitializers.h"
#include "behTreeNodeRiderIdleRoot.h"

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerRiderIdleAI
////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerRiderIdleAI );
CName CSpawnTreeInitializerRiderIdleAI::GetDynamicNodeEventName() const
{
	return CNAME( AI_Load_RiderIdleRoot );
}

String CSpawnTreeInitializerRiderIdleAI::GetEditorFriendlyName() const
{
	static String STR( TXT("Rider Idle AI") );
	return STR;
}

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerRiderStartingBehavior
////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerRiderStartingBehavior );
CName CSpawnTreeInitializerRiderStartingBehavior::GetDynamicNodeEventName() const
{
	return CNAME( AI_Rider_Load_Forced );
}

String CSpawnTreeInitializerRiderStartingBehavior::GetEditorFriendlyName() const
{
	static String STR( TXT("Rider starting behavior") );
	return STR;
}
