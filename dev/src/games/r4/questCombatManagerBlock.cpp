/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "questCombatManagerBlock.h"

#include "../../common/core/instanceDataLayoutCompiler.h"

#include "../../common/game/aiParameters.h"
#include "../../common/game/behTreeGuardAreaData.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/game/behTreeMachine.h"
#include "../../common/game/questGraphSocket.h"
#include "../../common/engine/tagManager.h"
#include "../../common/engine/areaComponent.h"
#include "../../common/engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CTaggedActorsListener )
IMPLEMENT_ENGINE_CLASS( IQuestCombatManagerBaseBlock )


///////////////////////////////////////////////////////////////////////////////
// CTaggedActorsListener
///////////////////////////////////////////////////////////////////////////////

CTaggedActorsListener::~CTaggedActorsListener()
{
	RED_ASSERT( !m_wasRegistered, TXT( "Destroying unregistered listener. May cause memory leaks and crashes." ) );
}

void CTaggedActorsListener::Register( const TagList& tagList )
{
	if ( !m_wasRegistered )
	{
		GGame->GetGlobalEventsManager()->AddFilteredListener( GEC_Tag, this, tagList.GetTags() );
		m_wasRegistered = true;
	}
}
void CTaggedActorsListener::Unregister( const TagList& tagList )
{
	if ( m_wasRegistered )
	{
		GGame->GetGlobalEventsManager()->RemoveFilteredListener( GEC_Tag, this, tagList.GetTags() );
		m_wasRegistered = false;
	}
}

void CTaggedActorsListener::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	m_isDirty = true;
}

///////////////////////////////////////////////////////////////////////////////
// CQuestCombatManagerBlock
///////////////////////////////////////////////////////////////////////////////
IQuestCombatManagerBaseBlock::IQuestCombatManagerBaseBlock()
	: TBaseClass()
	, m_overrideGuardArea( false )
	, m_pursuitRange( -1.f )
{
	m_name = TXT("Combat manager");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
String IQuestCombatManagerBaseBlock::GetBlockName() const
{
	String name = m_name;
	CObject* context = const_cast< IQuestCombatManagerBaseBlock* >( this );
	CallFunctionRet< String >( context, CNAME( GetBlockName ), name );
	return name;
}
#endif

void IQuestCombatManagerBaseBlock::ApplyToActor( CActor* actor ) const
{
	IAITree* tree = m_cachedTree.Get();
	if ( !tree )
	{
		CObject* context = const_cast< IQuestCombatManagerBaseBlock* >( this );
		CallFunctionRet< THandle< IAITree > >( context, CNAME( GetAITree ), m_cachedTree );
		tree = m_cachedTree.Get();
	}
	if ( tree )
	{
		actor->ForceDynamicBehavior( tree, CNAME( AI_DynamicCombatStyle ), true );
	}
	if ( m_overrideGuardArea )
	{
		CBehTreeMachine* m = actor->GetBehTreeMachine();
		CBehTreeInstance* behTreeInstance = m ? m->GetBehTreeInstance() : nullptr;
		if ( behTreeInstance )
		{
			CBehTreeGuardAreaData* data = CBehTreeGuardAreaData::Find( behTreeInstance );
			if ( data )
			{
				CTagManager* tagManager = GGame->GetActiveWorld()->GetTagManager();

				CAreaComponent* guardArea = nullptr;
				CAreaComponent* pursuitArea = nullptr;

				if ( !m_guardAreaTag.Empty() )
				{
					CEntity* guardAreaEntity = tagManager->GetTaggedEntity( m_guardAreaTag );
					if( guardAreaEntity )
					{
						guardArea = guardAreaEntity->FindComponent< CAreaComponent >();
					}
				}
				if ( !m_pursuitAreaTag.Empty() )
				{
					CEntity* pursuitAreaEntity = tagManager->GetTaggedEntity( m_pursuitAreaTag );
					if( pursuitAreaEntity )
					{
						pursuitArea = pursuitAreaEntity->FindComponent< CAreaComponent >();
					}
				}

				if ( m_pursuitRange >= 0.f )
				{
					data->SetupImmediateState( guardArea, pursuitArea, m_pursuitRange );
				}
				else
				{
					data->SetupImmediateState( guardArea, pursuitArea );
				}
			}
		}
	}
}
void IQuestCombatManagerBaseBlock::ClearForActor( CActor* actor ) const
{
	actor->CancelDynamicBehavior( CNAME( AI_DynamicCombatStyle ), true );

	if ( m_overrideGuardArea )
	{
		CBehTreeMachine* m = actor->GetBehTreeMachine();
		CBehTreeInstance* behTreeInstance = m ? m->GetBehTreeInstance() : nullptr;
		if ( behTreeInstance )
		{
			CBehTreeGuardAreaData* data = CBehTreeGuardAreaData::Find( behTreeInstance );
			if ( data )
			{
				data->ResetImmediateState();
			}
		}
	}
}

void IQuestCombatManagerBaseBlock::EffectActors( InstanceBuffer& data ) const
{
	if ( !GGame )
	{
		return;
	}
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}
	CTagManager* tagsMgr = world->GetTagManager();
	if ( !world )
	{
		return;
	}

	// TODO: Notice O(n^2) cost
	struct Functor : public CTagManager::DefaultNodeIterator, public Red::System::NonCopyable
	{
		Functor( const IQuestCombatManagerBaseBlock* me, InstanceBuffer& data )
			: m_this( me )
			, m_data( data )												{}
		RED_INLINE Bool EarlyTest( CNode* node ) const					{ return node->IsA< CActor >(); }
		RED_INLINE void Process( CNode* node, Bool isInitialList )
		{
			CActor* actor = static_cast< CActor* >( node );

			auto& effectedActors = m_data[ m_this->i_effectedActors ];

			// find if actor is already processed
			for( Int32 i = effectedActors.Size()-1; i >= 0; --i )
			{
				CActor* effActor = effectedActors[ i ].Get();
				if ( actor == effActor )
				{
					return;
				}

				if ( !effActor )
				{
					effectedActors.RemoveAtFast( i );
					continue;
				}
			}

			m_this->ApplyToActor( actor );

			effectedActors.PushBack( actor );
		}

		const IQuestCombatManagerBaseBlock*	m_this;
		InstanceBuffer&					m_data;
	} functor( this, data );

	tagsMgr->IterateTaggedNodes( m_npcTags, functor, BCTO_MatchAny );

}



#ifndef NO_EDITOR_GRAPH_SUPPORT
void IQuestCombatManagerBaseBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Bottom ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

EGraphBlockShape IQuestCombatManagerBaseBlock::GetBlockShape() const
{
	return GBS_Default;
}
Color IQuestCombatManagerBaseBlock::GetClientColor() const
{
	return Color( 0, 122, 0 );
}
String IQuestCombatManagerBaseBlock::GetBlockCategory() const
{
	static const String STR( TXT( "Scripting" ) );
	return STR;
}
Bool IQuestCombatManagerBaseBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const
{
	return GetClass() != GetStaticClass();
}
#endif // NO_EDITOR_GRAPH_SUPPORT

void IQuestCombatManagerBaseBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_effectedActors;
	compiler << i_changesListener;
}
void IQuestCombatManagerBaseBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );
}
void IQuestCombatManagerBaseBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	if ( inputName == CNAME( In ) )
	{
		data[ i_changesListener ].Register( m_npcTags );
		EffectActors( data );
	}
	else if ( inputName == CNAME( Cut ) )
	{
		ActivateOutput( data, CNAME( Out ) );
	}
}
void IQuestCombatManagerBaseBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	if ( data[ i_changesListener ].IsDirty() )
	{
		EffectActors( data );
		data[ i_changesListener ].ClearDirty();
	}
}
void IQuestCombatManagerBaseBlock::OnDeactivate( InstanceBuffer& data ) const
{
	data[ i_changesListener ].Unregister( m_npcTags );

	// turn off for every actor
	auto& effectedActors = data[ i_effectedActors ];
	for ( auto it = effectedActors.Begin(), end = effectedActors.End(); it != end; ++it )
	{
		CActor* actor = it->Get();
		if ( actor )
		{
			ClearForActor( actor );
		}
	}
	effectedActors.Clear();

	TBaseClass::OnDeactivate( data );
}