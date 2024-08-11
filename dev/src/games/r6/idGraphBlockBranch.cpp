/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraph.h"
#include "idBasicBlocks.h"
#include "idGraphBlockBranch.h"
#include "idTopic.h"
#include "idInstance.h"
#include "idThread.h"
#include "idCondition.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CIDGraphBlockBranch )

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
void CIDGraphBlockBranch::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );

	ValidateOutputs();

	for ( Uint32 i = 0; i < m_outputs.Size(); ++i )
	{
		CreateSocket( SIDGraphSocketSpawnInfo( CName( GetOutputNameFor( i ) ), LSD_Output ) );
	}
}

void CIDGraphBlockBranch::OnPropertyPostChange( IProperty* prop )
{
	ValidateOutputs();

	TBaseClass::OnPropertyPostChange( prop );
}

const CIDGraphBlock* CIDGraphBlockBranch::ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate /*= true */ ) const
{
	R6_ASSERT( false, TXT("This method shouldn't be ever called on this block. Please DEBUG.") );
	return NULL;
}

const CIDGraphBlock* CIDGraphBlockBranch::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	R6_ASSERT( false, TXT("This method shouldn't be ever called on this block. Please DEBUG.") )
	return NULL;
}

const CIDGraphBlock* CIDGraphBlockBranch::ActivateOutput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* output, Bool evaluate /*= true */ ) const
{
	InstanceBuffer& data = topicInstance->GetInstanceData();

	return TBaseClass::ActivateOutput( topicInstance, timeDelta, output, evaluate );
}

Color CIDGraphBlockBranch::GetTitleColor() const
{
	return Color::CYAN;
}

EGraphBlockShape CIDGraphBlockBranch::GetBlockShape() const
{
	return GBS_Rounded;
}

void CIDGraphBlockBranch::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );
}

void CIDGraphBlockBranch::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
}

const CIDGraphBlock* CIDGraphBlockBranch::Update( CIDTopicInstance* topicInstance ) const
{
	InstanceBuffer& data = topicInstance->GetInstanceData();

	for ( Int32 i = 0; i < m_outputs.SizeInt(); ++i )
	{
		if ( m_outputs[ i ] )
		{
			if ( m_outputs[ i ]->IsFulfilled( topicInstance->GetDialogInstanceID() ) )
			{
				const CIDGraphSocket* output = FindOutputSocketByIndex( i );
				R6_ASSERT( output );
				Float timeDelta = 0.f;

				return ActivateOutput( topicInstance, timeDelta, output, false );
			}
		}
	}

	return this;
}

void CIDGraphBlockBranch::ValidateOutputs()
{
	if ( m_outputs.Size() < 1 )
	{
		m_outputs.Resize( 1 ); 
	}

	if ( m_outputs.Size() > OUTPUT_Max )
	{
		m_outputs.Resize( OUTPUT_Max ); 
	}
}

String CIDGraphBlockBranch::GetOutputNameFor( Uint32 outputIndex ) const
{
	return String::Printf( TXT("%ld"), outputIndex );
}
