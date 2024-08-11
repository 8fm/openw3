#include "build.h"
#include "questInterestPointBlock.h"
#include "reactionsManager.h"
#include "questGraphSocket.h"
#include "../engine/tagManager.h"
#include "../engine/graphConnectionRebuilder.h"


IMPLEMENT_ENGINE_CLASS( CQuestInterestPointBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestInterestPointBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestInterestPointBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	if ( !m_interestPoint || m_positionTag.Empty() )
	{
		// nothing to do - move on
		ActivateOutput( data, CNAME( Out ) );
		return;
	}

	// find the broadcast point
	TDynArray< CNode* > nodes;
	TagList tags;
	tags.AddTag( m_positionTag );
	GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( tags, nodes, BCTO_MatchAny );
	
	CNode* node = NULL;
	// find first non-NULL node
	for ( TDynArray< CNode* >::const_iterator it = nodes.Begin(); it != nodes.End(); ++it )
	{
		if ( *it )
		{
			node = *it;
			break;
		}
	}

	// broadcast the interest point
	if ( !node )
	{
		ThrowError( data, TXT( "No node '%ls' found" ), m_positionTag.AsString().AsChar() );
	}
	else
	{
		GCommonGame->GetReactionsManager()->BroadcastInterestPoint( m_interestPoint, THandle< CNode >( node ), m_duration );
		ActivateOutput( data, CNAME( Out ) );
	}
}
