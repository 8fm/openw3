#include "build.h"
#include "questLockNPC.h"
#include "questGraphSocket.h"
#include "communitySystem.h"
#include "gameWorld.h"
#include "../engine/tagManager.h"
#include "../engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CQuestLockNPCBlock )

CQuestLockNPCBlock::CQuestLockNPCBlock()
{
	m_name = TXT("Lock NPC");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestLockNPCBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestLockNPCBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	// Pass to base block
	TBaseClass::OnActivate( data, inputName, parentThread );

	// No NPCs to wait for
	if ( m_npcsTag.Empty() )
	{
		ThrowErrorNonBlocking( data, CNAME( Out ), TXT("No NPCs to wait for") );
		return;
	}
}

void CQuestLockNPCBlock::OnExecute( InstanceBuffer& data ) const
{
	// Pass to base class
	TBaseClass::OnExecute( data );

	// Wait for all NPCs
	TStaticArray< CNewNPC*, 16  > foundEntities;
	for ( Uint32 i=0; i<m_npcsTag.GetTags().Size(); ++i )
	{
		// Find the entity
		const CName& tag = m_npcsTag.GetTags()[i];
		CEntity* entity = GGame->GetActiveWorld()->GetTagManager()->GetTaggedEntity( tag );
		if ( !entity )
		{
			return;
		}

		// Entity was found but is not an NPC
		if ( !entity->IsA< CNewNPC >() )
		{
			ThrowError( data, TXT("Entity tagged '%ls' is not an NPC!"), tag.AsString().AsChar() );
			return;
		}

		CNewNPC* npc = static_cast< CNewNPC* >( entity );

		// Add to list
		foundEntities.PushBackUnique( npc );
	}

	// All NPCs are valid and exist, lock them for quests
	for ( Uint32 i=0; i<foundEntities.Size(); ++i )
	{
		CNewNPC* npc = foundEntities[i];
		npc->QuestLock( FindParent< CQuestPhase >(), this );
	}

	// Progress the quest
	ActivateOutput( data, CNAME( Out ) );
}
