/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraph.h"
#include "idGraphBlockEvents.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CIDGraphBlockEvents )

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
void CIDGraphBlockEvents::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );
	m_output = Cast< CIDGraphSocket > ( CreateSocket( SIDGraphSocketSpawnInfo( CNAME( Out ), LSD_Output ) ) );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
const CIDGraphBlock* CIDGraphBlockEvents::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	return ActivateOutput( topicInstance, timeDelta, m_output );
}
	
//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Color CIDGraphBlockEvents::GetTitleColor() const
{
	return Color( 0xff444400 );
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
Color CIDGraphBlockEvents::GetClientColor() const
{
	return Color::LIGHT_MAGENTA;
}

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
EGraphBlockShape CIDGraphBlockEvents::GetBlockShape() const
{
	return GBS_LargeCircle;
}
