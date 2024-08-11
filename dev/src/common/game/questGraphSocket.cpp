#include "build.h"
#include "questGraphSocket.h"

IMPLEMENT_ENGINE_CLASS( CQuestGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CQuestGraphSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	// We can connect any input to any output of any scene graph socket that isn't in the same block
	if ( otherSocket->GetBlock() != GetBlock() )
	{
		Bool inputOutput = (GetDirection() == LSD_Input && otherSocket->GetDirection() == LSD_Output);
		Bool outputInput = (GetDirection() == LSD_Output && otherSocket->GetDirection() == LSD_Input);
		if ( inputOutput || outputInput )
		{
			// Cut control central socket can be connected to any other central socket with opposite direction
			if ( GetPlacement() == LSP_Center )
			{
				return otherSocket->GetPlacement() == LSP_Center && ( otherSocket->IsA< CQuestCutControlGraphSocket >() );
			}
			else
			{
				return true;
			}
		}
	}

	return false;
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestCutControlGraphSocket );

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CQuestCutControlGraphSocket::CanConnectTo( CGraphSocket *otherSocket )
{
	// We can connect any input to any output of any scene graph socket that isn't in the same block
	if ( otherSocket->GetBlock() != GetBlock() )
	{
		Bool inputOutput = (GetDirection() == LSD_Input && otherSocket->GetDirection() == LSD_Output);
		Bool outputInput = (GetDirection() == LSD_Output && otherSocket->GetDirection() == LSD_Input);
		if ( inputOutput || outputInput )
		{
			// Cut control central socket can be connected to any other central socket with opposite direction
			if ( GetPlacement() == LSP_Center )
			{
				return otherSocket->GetPlacement() == LSP_Center && otherSocket->IsA< CQuestGraphSocket >();
			}
			else
			{
				return true;
			}
		}
	}

	return false;
}

#endif

//////////////////////////////////////////////////////////////////////////

