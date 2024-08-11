/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeInitializerStartingBehavior.h"

#include "aiParameters.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerBaseStartingBehavior );
IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerStartingBehavior );

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerBaseStartingBehavior
////////////////////////////////////////////////////////////////////
Bool CSpawnTreeInitializerBaseStartingBehavior::CallActivateOnRestore() const
{
	return m_runBehaviorOnLoading;
}
Bool CSpawnTreeInitializerBaseStartingBehavior::CallActivateOnSpawn() const
{
	return true;
}
Bool CSpawnTreeInitializerBaseStartingBehavior::CallActivateOnPoolSpawn() const
{
	return true;
}

Bool CSpawnTreeInitializerBaseStartingBehavior::CallActivateWhenStealing() const
{
	return m_runBehaviorOnActivation;
}

CSpawnTreeInitializerBaseStartingBehavior::EOutput CSpawnTreeInitializerBaseStartingBehavior::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	CacheStuff();
	IAITree* tree = m_lazyTree.Get();
	if ( !tree )
	{
		return OUTPUT_FAILED;
	}
	if ( !actor->ForceAIBehavior( tree, Int8( m_actionPriority ), nullptr, GetDynamicNodeEventName() ) )
	{
		return OUTPUT_POSTPONED;
	}
	return OUTPUT_SUCCESS;
}
void CSpawnTreeInitializerBaseStartingBehavior::OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
	
}

////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerStartingBehavior
////////////////////////////////////////////////////////////////////
CName CSpawnTreeInitializerStartingBehavior::GetDynamicNodeEventName() const
{
	return CNAME( AI_Load_Forced );
}

String CSpawnTreeInitializerStartingBehavior::GetEditorFriendlyName() const
{
	static String STR( TXT("Starting behavior") );
	return STR;
}
