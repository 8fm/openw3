/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnTreeRandomNode.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/gameTimeManager.h"
IMPLEMENT_ENGINE_CLASS( CSpawnTreeSelectRandomNode )



//////////////////////////////////////////////////////////////
// CSpawnTreeSelectRandomNode
//////////////////////////////////////////////////////////////

void CSpawnTreeSelectRandomNode::Activate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = true;

	Bool reroll = instance[ i_currentActiveChild ] == INVALID_CHILD;
	
	if ( !reroll )
	{
		// check if we should change encounter state upon reactivation
		if ( GGame->GetTimeManager()->GetTime() > instance[ i_rerollTime ] )
		{
			// only reroll if encounter is desolated
			if ( instance.GetEncounter()->GetCreaturePool().GetCreaturesCount() == 0 )
			{
				reroll = true;
			}
		}
	}

	if ( reroll && !m_childNodes.Empty() )
	{
		instance[ i_rerollTime ] = GGame->GetTimeManager()->GetTime() + m_rerollDelay;
		instance[ i_currentActiveChild ] = GEngine->GetRandomNumberGenerator().Get< Uint16 >( 0, Uint16( m_childNodes.Size() ) );
	}

}
void CSpawnTreeSelectRandomNode::Deactivate( CSpawnTreeInstance& instance )
{
	instance[ i_active ] = false;
	Uint16 currentActiveChild = instance[ i_currentActiveChild ];
	if ( currentActiveChild != INVALID_CHILD )
	{
		if ( m_childNodes[ currentActiveChild ]->IsActive( instance ) )
		{
			m_childNodes[ currentActiveChild ]->Deactivate( instance );
		}

		instance[ i_currentActiveChild ] = INVALID_CHILD;
	}
}
void CSpawnTreeSelectRandomNode::UpdateLogic( CSpawnTreeInstance& instance ) 
{
	TBaseClass::UpdateLogic( instance );

	if ( instance[ i_active ] )
	{
		Uint16 currentActiveChild = instance[ i_currentActiveChild ];
		if ( currentActiveChild != INVALID_CHILD )
		{
			m_childNodes[ currentActiveChild ]->UpdateLogic( instance );
		}
	}
}

String CSpawnTreeSelectRandomNode::GetEditorFriendlyName() const
{
	return TXT("Random");
}

void CSpawnTreeSelectRandomNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_rerollTime;
	compiler << i_currentActiveChild;
}
void CSpawnTreeSelectRandomNode::OnInitData( CSpawnTreeInstance& instance, CSpawnTreeInitializationContext& context )
{
	TBaseClass::OnInitData( instance, context );

	instance[ i_rerollTime ] = GameTime();
	instance[ i_currentActiveChild ] = INVALID_CHILD;
}

Bool CSpawnTreeSelectRandomNode::IsNodeStateSaving( CSpawnTreeInstance& instance ) const
{
	return GGame->GetTimeManager()->GetTime() < instance[ i_rerollTime ];
}

void CSpawnTreeSelectRandomNode::SaveNodeState( CSpawnTreeInstance& instance, IGameSaver* writer ) const
{
	writer->WriteValue<GameTime>( CNAME( rerollTime ), instance[ i_rerollTime ] );
	writer->WriteValue<Uint16>( CNAME( currentActiveChild ), instance[ i_currentActiveChild ] );

	TBaseClass::SaveNodeState( instance, writer );
}

Bool CSpawnTreeSelectRandomNode::LoadNodeState( CSpawnTreeInstance& instance, IGameLoader* reader ) const
{
	reader->ReadValue<GameTime>( CNAME( rerollTime ), instance[ i_rerollTime ] );
	reader->ReadValue<Uint16>( CNAME( currentActiveChild ), instance[ i_currentActiveChild ] );

	return TBaseClass::LoadNodeState( instance, reader );
}

//////////////////////////////////////////////////////////////
//             _________.__              .__               
//            /   _____/|  |   _______  _|__|____    ____  
//            \_____  \ |  |  /  _ \  \/ /  \__  \  /    \
//            /        \|  |_(  <_> )   /|  |/ __ \|   |  \
//           /_______  /|____/\____/ \_/ |__(____  /___|  /
//                   \/                          \/     \/ 
//////////////////////////////////////////////////////////////
