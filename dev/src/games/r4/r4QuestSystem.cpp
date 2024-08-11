/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "r4QuestSystem.h"
#include "questControlledNPC.h"
#include "questUsedFastTravelCondition.h"
#include "mapFastTravel.h"

IMPLEMENT_ENGINE_CLASS( CR4QuestSystem );

CR4QuestSystem::CR4QuestSystem()
	: CQuestsSystem()
	, m_npcsManager( new CQuestControlledNPCsManager() )
{

}

CR4QuestSystem::~CR4QuestSystem()
{
	delete m_npcsManager;
	m_npcsManager = NULL;
}

void CR4QuestSystem::Deactivate()
{
	TBaseClass::Deactivate();

	m_fastTravelListeners.Clear();
	
	m_npcsManager->Reset();
}

Bool CR4QuestSystem::IsNPCInQuestScene( const CNewNPC* npc ) const
{
	return m_npcsManager->IsNPCInQuestScene( npc );
}

void CR4QuestSystem::AttachFastTravelListener( CQuestUsedFastTravelCondition& listener )
{
	m_fastTravelListeners.PushBackUnique( &listener );
}

void CR4QuestSystem::DetachFastTravelListener( CQuestUsedFastTravelCondition& listener )
{
	m_fastTravelListeners.Remove( &listener );
}

Bool CR4QuestSystem::OnUsedFastTravelEvent( const SUsedFastTravelEvent& event )
{
	Uint32 count = m_fastTravelListeners.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_fastTravelListeners[ i ]->OnEvent( event );
	}

	return true;
}
