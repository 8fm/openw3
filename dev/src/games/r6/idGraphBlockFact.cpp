/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraph.h"
#include "idGraphBlockFact.h"

#include "../../common/game/factsDB.h"
#include "../../common/engine/gameTimeManager.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CIDGraphBlockFact )

//------------------------------------------------------------------------------------------------------------------
// Graph block
//------------------------------------------------------------------------------------------------------------------
CIDGraphBlockFact::CIDGraphBlockFact()
	: m_value( 1 )
	, m_validFor( CFactsDB::EXP_NEVER_SEC )
{
}

String CIDGraphBlockFact::GetCaption() const
{
	String name;
	if ( m_name )
	{
		name	= m_name.AsString();
	}
	else
	{
		name	= m_factID.AsChar();
	}

	return name;
}

void CIDGraphBlockFact::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );
	m_output = Cast< CIDGraphSocket > ( CreateSocket( SIDGraphSocketSpawnInfo( CNAME( Out ), LSD_Output ) ) );
}

const CIDGraphBlock* CIDGraphBlockFact::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	if ( m_factID.Empty() )
	{
		RED_LOG( Dialog, TXT("Empty m_factID in block %s"), GetFriendlyName().AsChar() );
	}
	else
	{
		const EngineTime& time = GGame->GetEngineTime();
		GCommonGame->GetSystem< CFactsDB >()->AddFact( m_factID, m_value, time, m_validFor );
	}

	return ActivateOutput( topicInstance, timeDelta, m_output );
}
	
Color CIDGraphBlockFact::GetTitleColor() const
{
	return Color( 0xff444400 );
}

EGraphBlockShape CIDGraphBlockFact::GetBlockShape() const
{
	return GBS_Default;
}
