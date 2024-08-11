#include "build.h"
#include "questScriptedActionsBlock.h"

#include "../core/feedback.h"
#include "../core/gameSave.h"
#include "../core/instanceDataLayoutCompiler.h"

#include "../engine/tagManager.h"
#include "../engine/graphConnectionRebuilder.h"

#include "actorLatentAction.h"
#include "aiActionParameters.h"
#include "behTreeDynamicNodeEvent.h"
#include "behTreeNodeFinishAnimations.h"
#include "behTreeNodeSequence.h"
#include "questGraphSocket.h"
#include "questScriptedActionsAIParameters.h"




IMPLEMENT_ENGINE_CLASS( SScriptedActionData )
IMPLEMENT_ENGINE_CLASS( SScriptedActionSerializedState )
IMPLEMENT_ENGINE_CLASS( CBaseQuestScriptedActionsBlock )

///////////////////////////////////////////////////////////////////////////////
// SScriptedActionData
///////////////////////////////////////////////////////////////////////////////
SScriptedActionData::~SScriptedActionData()
{
	delete m_listener;
}
///////////////////////////////////////////////////////////////////////////////
// SScriptedActionSerializedState
///////////////////////////////////////////////////////////////////////////////
SScriptedActionSerializedState::SScriptedActionSerializedState()
{

}
SScriptedActionSerializedState::~SScriptedActionSerializedState()
{
	Free();
}
void SScriptedActionSerializedState::Push( const IdTag& guid, IGameDataStorage* aiState )
{
	Val v;

	v.m_guid = guid;
	v.m_aiState = aiState;

	m_list.PushBack( v );
}
void SScriptedActionSerializedState::Free()
{
	for( auto it = m_list.Begin(), end = m_list.End(); it != end; ++it )
	{
		delete it->m_aiState;
	}
	m_list.Clear();
}
///////////////////////////////////////////////////////////////////////////////
// CBaseQuestScriptedActionsBlock
///////////////////////////////////////////////////////////////////////////////
CBaseQuestScriptedActionsBlock::CBaseQuestScriptedActionsBlock()
	: m_actionsPriority( AIP_AboveCombat )
	, m_onlyOneActor( true )
	, m_handleBehaviorOutcome( false )
{
	
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBaseQuestScriptedActionsBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Center ) );
	if ( m_handleBehaviorOutcome )
	{
		CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Success ), LSD_Output, LSP_Right ) );
		CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Failure ), LSD_Output, LSP_Right ) );
	}
	else
	{
		CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
	}
}

EGraphBlockShape CBaseQuestScriptedActionsBlock::GetBlockShape() const
{
	return GBS_Default;
}
Color CBaseQuestScriptedActionsBlock::GetClientColor() const
{
	return Color( 192, 80, 77 );
}
String CBaseQuestScriptedActionsBlock::GetBlockCategory() const
{
	static const String STR( TXT( "Scripting" ) );
	return STR;
}
Bool CBaseQuestScriptedActionsBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const
{
	return true;
}

#endif

void CBaseQuestScriptedActionsBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_scriptedActionDataArray;
	compiler << i_serializedState;
}

void CBaseQuestScriptedActionsBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );
	instanceData[ i_scriptedActionDataArray ]		= TDynArray< SScriptedActionData > ();
}
 
void CBaseQuestScriptedActionsBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );
}


void CBaseQuestScriptedActionsBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );
	TDynArray< SScriptedActionData > & scriptedActionDataArray	= data[ i_scriptedActionDataArray ];
	Bool finished	= true;
	Bool success	= true;
	for ( Uint32 i = 0; i < scriptedActionDataArray.Size(); ++i )
	{
		SScriptedActionData & scriptedActiondata = scriptedActionDataArray[ i ];
		if ( scriptedActiondata.m_listener->m_state != CQuestScriptedActionsBlockAIListener::AS_PROGRESS )
		{
			if( scriptedActiondata.m_listener->m_state != CQuestScriptedActionsBlockAIListener::AS_COMPLETED_SUCCCESS )
			{
				success = false;
			}
		}
		else
		{
			finished = false;
		}
	}
	if ( finished )
	{
		TDynArray< SScriptedActionData > & scriptedActionDataArray	= data[ i_scriptedActionDataArray ];
		scriptedActionDataArray.Clear();

		ActivateOutput( data, CNAME( Out ), true );

		if( success )
		{
			ActivateOutput( data, CNAME( Success ), true );
		}
		else
		{
			ActivateOutput( data, CNAME( Failure ), true );
		}
	}
	
}

void CBaseQuestScriptedActionsBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );

	TDynArray< SScriptedActionData > & scriptedActionDataArray	= data[ i_scriptedActionDataArray ];

	for ( Uint32 i = 0; i < scriptedActionDataArray.Size(); ++i )
	{
		SScriptedActionData & scriptedActiondata = scriptedActionDataArray[ i ];
		if ( scriptedActiondata.m_behaviorId >= 0 )
		{
			CActor *const actor = scriptedActiondata.m_actor.Get();
			if ( actor )	
			{
				if ( actor->CancelAIBehavior( scriptedActiondata.m_behaviorId, GetCancelEventName(), false ) == false )
				{
					GFeedback->ShowError( TXT("CBaseQuestScriptedActionsBlock, could not cancel action on actor %s"), actor->GetName().AsChar() );
				}
			}
		}
	}
	scriptedActionDataArray.Clear();
	data[ i_serializedState ].Free();
	m_forcedAction = nullptr;
}


Bool CBaseQuestScriptedActionsBlock::WaitForActorToAppear( InstanceBuffer& data ) const
{
	CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
	TDynArray< CEntity* > entities;
	tagMgr->CollectTaggedEntities( m_npcTag, entities );		
	
	TDynArray< SScriptedActionData > & scriptedActionDataArray	= data[ i_scriptedActionDataArray ];
	ASSERT( scriptedActionDataArray.Empty() );

	for ( TDynArray< CEntity* >::const_iterator it = entities.Begin(); it != entities.End(); ++it )
	{
		CActor* thisActor = Cast< CActor >( *it );
		if ( thisActor )
		{
			if ( m_onlyOneActor && !scriptedActionDataArray.Empty() )
			{
				RED_LOG_ERROR( CNAME( CBaseQuestScriptedActionsBlock ), TXT( "CBaseQuestScriptedActionsBlock, Too many NPCs with a tag '%ls' were found - can't decide which one to pick" ), m_npcTag.AsString().AsChar() );
				return true;
			}
			scriptedActionDataArray.PushBack( thisActor );
		}
	}
	if ( !scriptedActionDataArray.Empty() )
	{
		return true;
	}
	return false;
	
}


Bool CBaseQuestScriptedActionsBlock::OnProcessActivation( InstanceBuffer& data ) const
{
	if ( !CQuestGraphBlock::OnProcessActivation( data ) )
	{
		return false;
	}

	TDynArray< SScriptedActionData > & scriptedActionDataArray	= data[ i_scriptedActionDataArray ];
	if ( !scriptedActionDataArray.Empty() )
	{
		return true;
	}

	if ( !WaitForActorToAppear( data ) )
	{
		scriptedActionDataArray.Clear();
		return false;
	}

	ASSERT( scriptedActionDataArray.Size() != 0 );

	if ( scriptedActionDataArray.Size() == 0 )
	{
		ThrowError( data, TXT("Invalid data found - call 911") );
		return false;
	}

	CAIQuestScriptedActionsTree* forcedAction = ComputeAIActions();
	if ( forcedAction == nullptr )
	{
		ThrowError( data, TXT("No ai actions spawned - call emergency") );
		scriptedActionDataArray.Clear();
		return false;
	}

	// at this point we have all we need, so we can add a goal to our NPCs
	const SScriptedActionSerializedState& serializedState = data[ i_serializedState ];

	Bool forceAIBehaviorSuccess = false;
	for ( Uint32 i = 0; i < scriptedActionDataArray.Size(); ++i )
	{
		SScriptedActionData & scriptedActiondata = scriptedActionDataArray[ i ];
		CActor *const actor = scriptedActiondata.m_actor.Get();
		if ( actor )
		{
			if ( scriptedActiondata.m_listener == nullptr )
			{
				scriptedActiondata.m_listener = new CQuestScriptedActionsBlockAIListener();
			}
			forcedAction->SetListenerParam( scriptedActiondata.m_listener );

			IGameDataStorage* aiState = nullptr;
			for ( auto it = serializedState.m_list.Begin(), end = serializedState.m_list.End(); it != end; ++it )
			{
				if ( it->m_guid == actor->GetIdTag() )
				{
					aiState = it->m_aiState;
					break;
				}
			}

			forceAIBehaviorSuccess = actor->ForceAIBehavior( forcedAction, IBehTreeNodeDefinition::Priority( m_actionsPriority ), &scriptedActiondata.m_behaviorId, GetEventName(), aiState, EAIForcedBehaviorInterruptionLevel::High );
		}
	}


	forcedAction->ClearListenerParam();

	if ( forceAIBehaviorSuccess == false )
	{
		scriptedActionDataArray.Clear();
	}

	return forceAIBehaviorSuccess;
}

void CBaseQuestScriptedActionsBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{
	SScriptedActionSerializedState serializedState;

	TDynArray< SScriptedActionData > & scriptedActionDataArray	= data[ i_scriptedActionDataArray ];
	// try to serialize current creatures state
	for ( Uint32 i = 0; i < scriptedActionDataArray.Size(); ++i )
	{
		SScriptedActionData & scriptedActiondata = scriptedActionDataArray[ i ];
		if ( scriptedActiondata.m_behaviorId >= 0 )
		{
			{
				CActor *const actor = scriptedActiondata.m_actor.Get();
				if ( actor )
				{
					SDynamicNodeSaveStateRequestEventData saveRequest;
					actor->SignalGameplayEvent( GetEventName(), &saveRequest, SDynamicNodeSaveStateRequestEventData::GetStaticClass() );

					if ( saveRequest.m_dataBuffer )
					{
						if ( saveRequest.m_invalidated )
						{
							delete saveRequest.m_dataBuffer;
						}
						else
						{
							serializedState.Push( actor->GetIdTag(), saveRequest.m_dataBuffer );
						}
						saveRequest.m_dataBuffer = nullptr;
					}
				}
			}
		}
	}

	CGameSaverBlock block( saver, CNAME( treeState ) );
	saver->WriteValue< Uint16 >( CNAME( numSaved ), Uint16( serializedState.m_list.Size() ) );

	for ( Uint32 i = 0, n = serializedState.m_list.Size(); i != n; ++i )
	{
		const auto& e = serializedState.m_list[ i ];
		saver->WriteValue< IdTag >( CNAME( entryGUID ), e.m_guid );
		saver->AddStorageStream( e.m_aiState );
	}

	TBaseClass::SaveGame( data, saver );
}
void CBaseQuestScriptedActionsBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	{
		CGameSaverBlock block( loader, CNAME( treeState ) );
		Uint16 numSavedCreatures = 0;
		loader->ReadValue< Uint16 >( CNAME( numSaved ), numSavedCreatures );

		SScriptedActionSerializedState& serializedState = data[ i_serializedState ];

		serializedState.m_list.Reserve( numSavedCreatures );

		for ( Uint16 i = 0; i != numSavedCreatures; ++i )
		{
			SScriptedActionSerializedState::Val e;
			loader->ReadValue< IdTag >( CNAME( entryGUID ), e.m_guid );
			e.m_aiState = loader->ExtractDataStorage();
			if ( e.m_aiState )
			{
				serializedState.m_list.PushBack( e );
			}
		}

	}
	TBaseClass::LoadGame( data, loader );
}



///////////////////////////////////////////////////////////////////////////////
// CQuestScriptedActionsBlock
IMPLEMENT_ENGINE_CLASS( CQuestScriptedActionsBlock );

CQuestScriptedActionsBlock::CQuestScriptedActionsBlock()
	: CBaseQuestScriptedActionsBlock() 
	, m_ai( NULL )
{
	m_name = TXT("Scripted actions");
}

CAIQuestScriptedActionsTree* CQuestScriptedActionsBlock::ComputeAIActions() const
{
	CAIQuestScriptedActionsTree* tree = m_forcedAction.Get();
	if ( tree == nullptr )
	{
		if ( !m_ai )
		{
			return nullptr;
		}
		tree = CAIQuestScriptedActionsTree::GetStaticClass()->CreateObject< CAIQuestScriptedActionsTree >();
		tree->InitializeTree();
		tree->InitializeData( m_ai.Get() );
		m_forcedAction = tree;

		tree->InitializeAIParameters();
	}
	return tree;
}

void CQuestScriptedActionsBlock::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	if ( !m_ai && !m_actions.Empty() )
	{
		THandle< CAIActionSequence > seq = NULL;
		
		for ( auto it = m_actions.Begin(), end = m_actions.End(); it != end; ++it )
		{
			IActorLatentAction* action = *it;
			if ( !action )
			{
				continue;
			}

			THandle< IAIActionTree > actionTree = action->ConvertToActionTree( this );
			if ( actionTree )
			{
				if ( m_ai )
				{
					if ( !seq )
					{
						seq = CAIActionSequence::GetStaticClass()->CreateObject< CAIActionSequence >();
						seq->EnableReferenceCounting();
						seq->OnCreated();
						seq->AddAction( m_ai.Get() );
						m_ai = seq;
					}
					seq->AddAction( actionTree );
				}
				else
				{
					m_ai = actionTree;
				}
				
			}
		}
	}
	m_actions.ClearFast();
}

CName CQuestScriptedActionsBlock::GetCancelEventName() const
{
	return CNAME( AI_Forced_Cancel );
}
CName CQuestScriptedActionsBlock::GetEventName() const
{
	return SForcedBehaviorEventData::GetEventName();
}
