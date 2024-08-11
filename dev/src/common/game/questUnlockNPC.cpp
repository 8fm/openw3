#include "build.h"
#include "questUnlockNPC.h"
#include "questGraphSocket.h"
#include "../engine/tagManager.h"
#include "../engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CQuestUnlockNPCBlock )

CQuestUnlockNPCBlock::CQuestUnlockNPCBlock()
{
	m_name = TXT("Unlock NPC");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestUnlockNPCBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestUnlockNPCBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	// Pass to base block
	TBaseClass::OnActivate( data, inputName, parentThread );

	// Wait for all NPCs
	for ( Uint32 i=0; i<m_npcsTag.GetTags().Size(); ++i )
	{
		// Find the entity
		const CName& tag = m_npcsTag.GetTags()[i];
		CEntity* entity = GGame->GetActiveWorld()->GetTagManager()->GetTaggedEntity( tag );
		if ( !entity )
		{
			ThrowError( data, TXT("NPC '%ls' is not spawned!"), tag.AsString().AsChar() );
			return;
		}

		// Entity was found but is not an NPC
		if ( !entity->IsA< CNewNPC >() )
		{
			ThrowError( data, TXT("Entity tagged '%ls' is not an NPC!"), tag.AsString().AsChar() );
			return;
		}

		// Add to list
		CNewNPC* npc = static_cast< CNewNPC* >( entity );
		if ( !npc->IsLockedByQuests() )
		{
			ThrowError( data, TXT("Entity tagged '%ls' was not locked by quests before!"), tag.AsString().AsChar() );
			return;
		}

		// Unlock
		npc->QuestUnlock( FindParent< CQuestPhase >(), this );
	}

	// Progress the quest
	ActivateOutput( data, CNAME( Out ) );
}
