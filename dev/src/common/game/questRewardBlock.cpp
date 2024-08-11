/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "questRewardBlock.h"
#include "questGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestRewardBlock )

CQuestRewardBlock::CQuestRewardBlock()
	: m_rewardName( CName::NONE ) , m_targetEntityTag( CNAME( PLAYER ) )
{
	m_name = TXT("Reward");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestRewardBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("rewardName") )
	{
		UpdateCaption();
	}
}

void CQuestRewardBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );

	UpdateCaption();
}

void CQuestRewardBlock::UpdateCaption()
{
	if ( m_rewardName != CName::NONE )
	{
		m_caption = String::Printf( TXT("Reward [%s]"), m_rewardName.AsString().AsChar() );
	}
	else
	{
		m_caption = TXT( "Reward" );
	}
}

#endif

void CQuestRewardBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	if ( m_rewardName == CName::NONE )
	{
		ActivateOutput( data, CNAME( Out ) );
	}
}

void CQuestRewardBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	if( m_targetEntityTag == CNAME( PLAYER) )
	{
		GCommonGame->GiveRewardTo( m_rewardName, GCommonGame->GetPlayer() );
	}
	else
	{
		GCommonGame->GiveRewardTo( m_rewardName, m_targetEntityTag );
	}	
	ActivateOutput( data, CNAME( Out ) );
}
