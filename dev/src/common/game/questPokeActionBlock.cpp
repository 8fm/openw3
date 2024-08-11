#include "build.h"
#include "questPokeActionBlock.h"
#include "questGraphSocket.h"
#include "../core/feedback.h"
#include "../engine/tagManager.h"
#include "../engine/graphConnectionRebuilder.h"

///////////////////////////////////////////////////////////////////////////////
// CBaseQuestScriptedActionsBlock
IMPLEMENT_ENGINE_CLASS( CQuestPokeScriptedActionsBlock )
CQuestPokeScriptedActionsBlock::CQuestPokeScriptedActionsBlock()
	: m_npcTag()
	, m_pokeEvent( CNAME( poke_default ) )
	, m_onlyOneActor( true )
	, m_handleBehaviorOutcome( false )
{
	m_name	= TXT("Poke Scripted Action");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestPokeScriptedActionsBlock::OnRebuildSockets()
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

EGraphBlockShape CQuestPokeScriptedActionsBlock::GetBlockShape() const
{
	return GBS_Default;
}
Color CQuestPokeScriptedActionsBlock::GetClientColor() const
{
	return Color( 232, 130, 127);
}
String CQuestPokeScriptedActionsBlock::GetBlockCategory() const
{
	static const String STR( TXT( "Scripting" ) );
	return STR;
}
Bool CQuestPokeScriptedActionsBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const
{
	return true;
}

#endif

void CQuestPokeScriptedActionsBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
}

void CQuestPokeScriptedActionsBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );
}
 
void CQuestPokeScriptedActionsBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );
}


void CQuestPokeScriptedActionsBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );
	ActivateOutput( data, CNAME( Out ), true );
}

void CQuestPokeScriptedActionsBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );
}

Bool CQuestPokeScriptedActionsBlock::OnProcessActivation( InstanceBuffer& data ) const
{
	struct Local
	{
		static Bool SignalPokeEvent( CEntity* entity, CName pokeEvent )
		{
			CActor* actor		= Cast< CActor >( entity );
			if ( actor )
			{
				return actor->SignalGameplayEventReturnInt( pokeEvent, false ) == 1;
			}
			return false;
		}
	};

	if ( !CQuestGraphBlock::OnProcessActivation( data ) )
	{
		return false;
	}

	CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
	Bool poked = false;
	
	if ( m_onlyOneActor == false )
	{
		TDynArray< CEntity* > entities;
		tagMgr->CollectTaggedEntities( m_npcTag, entities );	
		for ( Uint32 i = 0; i < entities.Size(); ++i )
		{
			poked = Local::SignalPokeEvent( entities[ i ], m_pokeEvent ) || poked;
		}
	}
	else
	{
		CEntity *const taggedEntity = tagMgr->GetTaggedEntity( m_npcTag );
		if ( taggedEntity )
		{
			poked = Local::SignalPokeEvent( taggedEntity, m_pokeEvent );
		}
	}
	// if actor hasn't spawn yet poked will be false :
	return poked;
}
void CQuestPokeScriptedActionsBlock::OnPropertyPostChange( IProperty* property )
{
	CQuestGraphBlock::OnPropertyPostChange( property );
	if ( property->GetName() == TXT("pokeEvent") )
	{
		if ( property->GetType()->GetName() == CNAME( CName ) )
		{
			CName value;
			property->Get( this, &value );
			if ( value.AsString().BeginsWith( TXT("poke_") ) == false )
			{
				GFeedback->ShowError(TXT("Please format your event like this: \"poke_myEventName\""));
				CName empty = CNAME( poke_default );
				property->Set( this, &empty );
			}
		}
	}
}