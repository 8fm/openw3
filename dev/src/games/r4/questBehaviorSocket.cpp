/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "questBehaviorSocket.h"
#include "questBehaviorNotificationBlock.h"
#include "questBehaviorEventBlock.h"

IMPLEMENT_ENGINE_CLASS( CQuestBehaviorSyncGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CQuestBehaviorSyncGraphSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	if ( otherSocket->GetBlock() == GetBlock() )
	{
		return false;
	}

	Bool inputOutput = (GetDirection() == LSD_Input && otherSocket->GetDirection() == LSD_Output);
	Bool outputInput = (GetDirection() == LSD_Output && otherSocket->GetDirection() == LSD_Input);
	if ( !inputOutput && !outputInput )
	{
		return false;
	}

	if ( GetPlacement() != LSP_Center || otherSocket->GetPlacement() != LSP_Center )
	{
		return false;
	}

	CQuestBehaviorNotificationBlock* destBlock = Cast< CQuestBehaviorNotificationBlock >( otherSocket->GetBlock() );
	if ( !destBlock )
	{
		return false;
	}

	CQuestBehaviorEventBlock* sourceBlock = Cast< CQuestBehaviorEventBlock >( GetBlock() );
	if ( !sourceBlock )
	{
		return false;
	}

	if ( sourceBlock->DoesContainTags( destBlock->GetNPCTags() ) == false )
	{
		return false;
	}

	return true;
}

#endif