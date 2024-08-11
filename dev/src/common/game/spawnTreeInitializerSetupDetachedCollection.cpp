/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeInitializerSetupDetachedCollection.h"

#include "spawnTreeBaseEntry.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerCollectDetachedSetup )

///////////////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerCollectDetachedSetup
///////////////////////////////////////////////////////////////////////////////
String CSpawnTreeInitializerCollectDetachedSetup::GetBlockCaption() const
{
	return TXT("SetupDetachedCollection");
}
String CSpawnTreeInitializerCollectDetachedSetup::GetEditorFriendlyName() const
{
	return TXT("SetupDetachedCollection");
}
void CSpawnTreeInitializerCollectDetachedSetup::UpdateEntrySetup( const CBaseCreatureEntry* const  entry, CSpawnTreeInstance& instance, SSpawnTreeEntrySetup& setup ) const
{
	setup.m_stealingDelay = m_maximumCollectionDelay;
	setup.m_greedyStealing = m_greedyCollection;
}

Bool CSpawnTreeInitializerCollectDetachedSetup::IsSpawnableOnPartyMembers() const
{
	return false;
}