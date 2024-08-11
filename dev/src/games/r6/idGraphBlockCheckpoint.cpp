/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "idGraphBlockCheckpoint.h"
#include "idTopic.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CIDGraphBlockCheckpoint )

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
Color CIDGraphBlockCheckpoint::GetClientColor() const
{
	return Color( 0x55222200 );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
Color CIDGraphBlockCheckpoint::GetTitleColor() const
{
	return Color::BLACK;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
EGraphBlockShape CIDGraphBlockCheckpoint::GetBlockShape() const
{
	return GBS_Arrow;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CIDGraphBlockCheckpoint::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CIDGraphBlockCheckpoint::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CIDGraphBlockCheckpoint::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );
	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( Out ), LSD_Output ) );
	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( Connector ), LSD_Input ) );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
const CIDGraphBlock* CIDGraphBlockCheckpoint::ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate /*= true */ ) const
{
	//topicInstance->SetCheckpoint( this );
	return TBaseClass::ActivateInput( topicInstance, timeDelta, input, evaluate );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
const CIDGraphBlock* CIDGraphBlockCheckpoint::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	return ActivateOutput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( FindSocket( CNAME( Out ) ) ) );
}

