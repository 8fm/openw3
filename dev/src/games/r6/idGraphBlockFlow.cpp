/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraphBlockFlow.h"
#include "idTopic.h"

#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CIDGraphBlockFlow )


//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
Color CIDGraphBlockFlow::GetClientColor() const
{
	return Color::LIGHT_RED;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
Color CIDGraphBlockFlow::GetTitleColor() const
{
	return Color( 0xff444400 );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
EGraphBlockShape CIDGraphBlockFlow::GetBlockShape() const
{
	return GBS_Rounded;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CIDGraphBlockFlow::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	data[ i_timePassed ] = 0.0f;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CIDGraphBlockFlow::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timePassed;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void CIDGraphBlockFlow::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );
	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( Out ), LSD_Output ) );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
const CIDGraphBlock* CIDGraphBlockFlow::ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate /*= true */ ) const
{
	InstanceBuffer&	data = topicInstance->GetInstanceData();
	data[ i_timePassed ] = 0.f;
	return TBaseClass::ActivateInput( topicInstance, timeDelta, input, evaluate );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
const CIDGraphBlock* CIDGraphBlockFlow::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	if( m_timeWaiting <= 0.0f )
	{
		 return this;
	}

	InstanceBuffer&	data = topicInstance->GetInstanceData();
	data[ i_timePassed ] +=	timeDelta;

	if ( data[ i_timePassed ] >= m_timeWaiting )
	{
		return ActivateOutput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( FindSocket( CNAME( Out ) ) ) );
	}

	return this;
}

