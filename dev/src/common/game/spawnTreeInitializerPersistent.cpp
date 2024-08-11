/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeInitializerPersistent.h"
#include "spawnTreeBaseEntry.h"

#include "../engine/game.h"
#include "../engine/idTagManager.h"

IMPLEMENT_ENGINE_CLASS( CSpawnTreeInitializerPersistent );

///////////////////////////////////////////////////////////////////////////////
// CSpawnTreeInitializerPersistent
///////////////////////////////////////////////////////////////////////////////

Bool CSpawnTreeInitializerPersistent::Accept( CActor* actor ) const
{
	return false;
}

void CSpawnTreeInitializerPersistent::OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{	
	auto encounter = instance->GetEncounter();	
	auto creatureEntry = encounter->FindActiveCreatureEntry( entry->GetUniqueListId( *instance ) );
	entityInfo.m_idTag = GCommonGame->GetIdTagManager()->CreateFromUint64( creatureEntry->m_branchHash );	
	entityInfo.m_entityNotSavable = false;
}

String CSpawnTreeInitializerPersistent::GetEditorFriendlyName() const 
{
	return TXT("Spawn Persistent");
}

///////////////////////////////////////////////////////////////////////////////

