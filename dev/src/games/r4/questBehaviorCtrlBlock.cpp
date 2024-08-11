#include "build.h"

#include "../../common/game/questsSystem.h"
#include "../../common/game/questThread.h"
#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/engine/tagManager.h"

#include "questBehaviorCtrlBlock.h"
#include "questControlledNPC.h"
#include "r4QuestSystem.h"

IMPLEMENT_ENGINE_CLASS( SBehaviorGroup );
IMPLEMENT_ENGINE_CLASS( CQuestBehaviorCtrlBlock );
IMPLEMENT_RTTI_ENUM( EQuestBehaviorSceneSaveMode );

CQuestBehaviorCtrlBlock::CQuestBehaviorCtrlBlock()
	: m_activationTimeout( 1000.f )
{
	m_name = TXT( "Behavior Controller" );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestBehaviorCtrlBlock::OnPropertyPreChange( IProperty* property )
{
	TBaseClass::OnPropertyPreChange( property );

	if ( property->GetName() != TXT("groups") )
	{
		return;
	}

	ASSERT( m_undoGroups.Empty(), TXT("Undo list should be empty at this point") );
	m_undoGroups.Clear();

	for ( TDynArray< SBehaviorGroup* >::const_iterator it = m_groups.Begin();
		it != m_groups.End(); ++it )
	{
		SBehaviorGroup* tmpGroup = NULL;
		if ( *it )
		{
			tmpGroup = CreateObject< SBehaviorGroup >( this );
			tmpGroup->Copy( **it );
		}
		m_undoGroups.PushBack( tmpGroup );
	}
}

void CQuestBehaviorCtrlBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() != TXT("groups") )
	{
		return;
	}

	// verify that the groups don't share tags
	Bool restorePrevList = false;
	TagList allTags;
	for ( TDynArray< SBehaviorGroup* >::const_iterator it = m_groups.Begin();
		it != m_groups.End(); ++it )
	{
		if ( *it == NULL )
		{
			continue;
		}
		if ( !(*it)->m_affectedNPCs.Empty() && TagList::MatchAny( (*it)->m_affectedNPCs, allTags ) )
		{
			// we found duplicated tags
			restorePrevList = true;
			break;
		}
		allTags.AddTags( (*it)->m_affectedNPCs );
	}

	// restore the previous settings, if duplicated tags were found
	if ( restorePrevList )
	{
		for ( TDynArray< SBehaviorGroup* >::const_iterator it = m_groups.Begin();
			it != m_groups.End(); ++it )
		{
			if ( *it )
			{
				(*it)->Discard();
			}
		}
		m_groups.Clear();

		for ( TDynArray< SBehaviorGroup* >::const_iterator it = m_undoGroups.Begin();
			it != m_undoGroups.End(); ++it )
		{
			SBehaviorGroup* tmpGroup = NULL;
			if ( *it )
			{
				tmpGroup = CreateObject< SBehaviorGroup >( this );
				tmpGroup->Copy( **it );
			}
			m_groups.PushBack( tmpGroup );
		}
	}

	// cleanup
	for ( TDynArray< SBehaviorGroup* >::const_iterator it = m_undoGroups.Begin();
		it != m_undoGroups.End(); ++it )
	{
		if ( *it )
		{
			(*it)->Discard();
		}
	}
	m_undoGroups.Clear();
}

#endif

TSoftHandle< CBehaviorGraph > CQuestBehaviorCtrlBlock::GetBehaviorFor( const CName& tag ) const
{
	TSoftHandle< CBehaviorGraph > graph;

	for ( TDynArray< SBehaviorGroup* >::const_iterator it = m_groups.Begin(); 
		it != m_groups.End(); ++it )
	{
		SBehaviorGroup* group = *it;
		if ( group->m_affectedNPCs.HasTag( tag ) )
		{
			graph = group->m_behavior;
			break;
		}
	}

	return graph;
}

void CQuestBehaviorCtrlBlock::GetBehaviorGraphs( TDynArray< TSoftHandle< CBehaviorGraph > >& graphs ) const
{
	for ( TDynArray< SBehaviorGroup* >::const_iterator it = m_groups.Begin(); 
		it != m_groups.End(); ++it )
	{
		SBehaviorGroup* group = *it;

		graphs.PushBackUnique( group->m_behavior );
	}
}

void CQuestBehaviorCtrlBlock::GetAllTags( TagList& tags ) const
{
	for ( TDynArray< SBehaviorGroup* >::const_iterator it = m_groups.Begin(); 
		it != m_groups.End(); ++it )
	{
		SBehaviorGroup* group = *it;
		tags.AddTags( group->m_affectedNPCs );
	}
}

void CQuestBehaviorCtrlBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_activatedNPCs;
	compiler << i_activationStartTime;
	compiler << i_saveLock;
}

void CQuestBehaviorCtrlBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	instanceData[ i_activatedNPCs ] = 0;
	instanceData[ i_activationStartTime ] = 0;
	instanceData[ i_saveLock ] = CGameSessionManager::GAMESAVELOCK_INVALID;
}

void CQuestBehaviorCtrlBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	// load the behaviors
	LoadBehaviors( data );

	TDynArray< CQuestControlledNPC* >* npcsArr = new TDynArray< CQuestControlledNPC* >();
	data[ i_activatedNPCs ] = reinterpret_cast< TGenericPtr >( npcsArr );

	// No saves during behavior scenes
	if ( parentThread->CanBlockSaves() )
	{
		if ( m_saveMode == QBDSM_SaveBlocker  )
		{
		#ifdef NO_EDITOR_GRAPH_SUPPORT
			String lockReason = TXT("BehScene");
		#else
			String lockReason = String::Printf( TXT("BehScene '%ls'"), GetCaption().AsChar() );
		#endif
			SGameSessionManager::GetInstance().CreateNoSaveLock( lockReason, data[ i_saveLock ] );
		}
	}
}

Bool CQuestBehaviorCtrlBlock::OnProcessActivation( InstanceBuffer& data ) const
{
	if ( TBaseClass::OnProcessActivation( data ) == false )
	{
		return false;
	}

	Float currTime = (Float)GGame->GetEngineTime();
	if ( currTime - data[ i_activationStartTime ] > m_activationTimeout )
	{
		ThrowError( data, TXT("Behavior controller timedout when trying to load behaviors") );
		return false;
	}

	if ( AreBehaviorsLoaded( data ) == false )
	{
		return false;
	}

	if ( WaitForNPCsToAppear( data ) == false )
	{
		return false;
	}

	if ( ActivateNPCs( data ) == false )
	{
		ThrowError( data, TXT("Taking control over NPCs failed") );
		return false;
	}

	return true;
}

void CQuestBehaviorCtrlBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );

	// Release save lock
	if ( data[ i_saveLock ] >= 0 )
	{
		SGameSessionManager::GetInstance().ReleaseNoSaveLock( data[ i_saveLock ] );
		data[ i_saveLock ] = CGameSessionManager::GAMESAVELOCK_INVALID;
	}

	DeactivateNPCs( data );
	UnloadBehaviors( data );
	data[ i_activationStartTime ] = 0;
	data[ i_inputNames ].Clear();
}

void CQuestBehaviorCtrlBlock::LoadBehaviors( InstanceBuffer& data ) const
{
	// start loading the behaviors
	for ( TDynArray< SBehaviorGroup* >::const_iterator it = m_groups.Begin(); 
		it != m_groups.End(); ++it )
	{
		SBehaviorGroup* group = *it;
		group->m_behavior.GetAsync();
	}

	// memorize the activation time
	data[ i_activationStartTime ] = (Float)GGame->GetEngineTime();
}

void CQuestBehaviorCtrlBlock::UnloadBehaviors( InstanceBuffer& data ) const
{
	// unloading the behaviors
	for ( TDynArray< SBehaviorGroup* >::const_iterator it = m_groups.Begin(); 
		it != m_groups.End(); ++it )
	{
		SBehaviorGroup* group = *it;
		group->m_behavior.Release();
	}
}

Bool CQuestBehaviorCtrlBlock::AreBehaviorsLoaded( InstanceBuffer& data ) const
{
	for ( TDynArray< SBehaviorGroup* >::const_iterator it = m_groups.Begin(); 
		it != m_groups.End(); ++it )
	{
		SBehaviorGroup* group = *it;
	
		BaseSoftHandle::EAsyncLoadingResult loadingResult = group->m_behavior.GetAsync();

		if ( loadingResult != BaseSoftHandle::ALR_Loaded )
		{
			return false;
		}
	}

	return true;
}

Bool CQuestBehaviorCtrlBlock::WaitForNPCsToAppear( InstanceBuffer& data ) const
{
	TDynArray< CQuestControlledNPC* >* npcsArr = reinterpret_cast< TDynArray< CQuestControlledNPC* >* >( data[ i_activatedNPCs ] );
	if ( !npcsArr )
	{
		ASSERT( npcsArr, TXT( "NPC table not defined" ) );
		return false;
	}

	Bool result = true;

	// try activating the agents
	// set the behaviors on the actors
	for ( TDynArray< SBehaviorGroup* >::const_iterator groupIt = m_groups.Begin(); 
		groupIt != m_groups.End(); ++groupIt )
	{
		SBehaviorGroup* group = *groupIt;
		if ( !group || group->m_behavior == NULL )
		{
			continue;
		}

		const TDynArray< CName >& npcTags = group->m_affectedNPCs.GetTags();

		Uint32 expectedCount = ( group->m_expectedCount == 0 ) ? npcTags.Size() : group->m_expectedCount;
		Uint32 insertedCount = 0;

		// for each tag
		for ( TDynArray< CName >::const_iterator tagIt = npcTags.Begin(); 
			tagIt != npcTags.End(); ++tagIt )
		{
			// collect all nodes with the specified tag
			TDynArray< CNode* > nodes;
			GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( *tagIt, nodes );

			// filter those that are entities with an animated component, that has a valid behavior stack created
			for ( Uint32 i = 0; i < nodes.Size(); ++i )
			{
				CEntity* entity = Cast< CEntity >( nodes[ i ] );
				if ( !entity )
				{
					continue;
				}

				CAnimatedComponent* animComp = entity->GetRootAnimatedComponent();
				if ( !animComp || !animComp->GetBehaviorStack() )
				{
					continue;
				}

				ASSERT( group->m_behavior.GetAsync() == BaseSoftHandle::ALR_Loaded );

				CQuestControlledNPC* npc = new CQuestControlledNPC( *tagIt, group->m_behavior, entity, animComp->GetBehaviorStack() );
				npcsArr->PushBack( npc );
				++insertedCount;

				if ( expectedCount == insertedCount )
				{
					break;
				}
			}

			// stop adding if we've reached the expected number
			if ( expectedCount == insertedCount )
			{
				break;
			}
		}

		// verify that we've encountered the assumed number of entities
		result = expectedCount == insertedCount;
		if ( !result )
		{
			break;
		}
	}

	if ( !result)
	{
		// cleanup
		for ( TDynArray< CQuestControlledNPC* >::const_iterator it = npcsArr->Begin(); 
			it != npcsArr->End(); ++it )
		{
			delete *it;
		}
		npcsArr->Clear();
	}

	return result;
}


Bool CQuestBehaviorCtrlBlock::ActivateNPCs( InstanceBuffer& data ) const
{
	// try activating the agents
	TDynArray< CQuestControlledNPC* >* npcsArr = reinterpret_cast< TDynArray< CQuestControlledNPC* >* >( data[ i_activatedNPCs ] );
	if ( !npcsArr )
	{
		ASSERT( npcsArr, TXT( "No NPCs defined" ) );
		return false;
	}
	CQuestControlledNPCsManager* mgr = GCommonGame->GetSystem< CR4QuestSystem >()->GetNPCsManager();
	Bool result = mgr->Activate( *npcsArr );

	return result;
}

void CQuestBehaviorCtrlBlock::DeactivateNPCs( InstanceBuffer& data ) const
{
	TDynArray< CQuestControlledNPC* >* npcsArr = reinterpret_cast< TDynArray< CQuestControlledNPC* >* >( data[ i_activatedNPCs ] );
	CQuestControlledNPCsManager* mgr = GCommonGame->GetSystem< CR4QuestSystem >()->GetNPCsManager();
	mgr->Deactivate( *npcsArr );

	for ( TDynArray< CQuestControlledNPC* >::iterator npcIt = npcsArr->Begin(); 
		npcIt != npcsArr->End(); ++npcIt )
	{
		delete *npcIt;
	}

	// cleanup 
	delete npcsArr;
	data[ i_activatedNPCs ] = 0;

}
