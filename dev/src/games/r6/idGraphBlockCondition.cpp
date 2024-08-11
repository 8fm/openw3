/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraph.h"
#include "idGraphBlockCondition.h"
#include "idTopic.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CIDGraphBlockCondition )

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
void CIDGraphBlockCondition::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );
	m_trueOutput = Cast< CIDGraphSocket > ( CreateSocket( SIDGraphSocketSpawnInfo( CNAME( True ), LSD_Output ) ) );
	m_falseOutput = Cast< CIDGraphSocket > ( CreateSocket( SIDGraphSocketSpawnInfo( CNAME( False ), LSD_Output ) ) );
}

const CIDGraphBlock* CIDGraphBlockCondition::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	if ( m_conditions ? m_conditions->IsFulfilled( topicInstance->GetDialogInstanceID() ) : true )
	{
		return ActivateOutput( topicInstance, timeDelta, m_trueOutput );
	}
	else
	{
		return ActivateOutput( topicInstance, timeDelta, m_falseOutput );
	}
}
	
Color CIDGraphBlockCondition::GetClientColor() const
{
	return Color( 0, 160, 0 );
}

EGraphBlockShape CIDGraphBlockCondition::GetBlockShape() const
{
	return GBS_TriangleLeft;
}
