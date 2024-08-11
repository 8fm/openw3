/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "worldSelectionCustomEditor.h"
#include "../../common/game/questGraph.h"
#include "../../common/game/questScopeBlock.h"
#include "../../common/game/quest.h"
#include "../../common/game/commonGameResource.h"
#include "../../common/core/gatheredResource.h"

CGatheredResource resGameWorlds( TXT("gameplay\\globals\\gameworlds.csv"), RGF_NotCooked );

CEdWorldSelectionEditor::CEdWorldSelectionEditor( CPropertyItem* item )
	: ISelectionEditor( item )
{

}

void CEdWorldSelectionEditor::FillChoices()
{
	CGameResource* gameResource = GGame->GetGameResource();
	if ( gameResource == NULL )
	{
		wxMessageBox( wxT("Connect a game resouce with worlds list defined first!"), wxT("Error") );
		return;
	}

	TDynArray< String > worldPaths;
	gameResource->GetWorldPaths( worldPaths );

	m_ctrlChoice->AppendString( String::EMPTY.AsChar() );
	for ( Uint32 i = 0; i < worldPaths.Size(); ++i )
	{
		m_ctrlChoice->AppendString( worldPaths[ i ].AsChar() );
	}
	
}

//////////////////////////////////////////////////////////////////////////

CEdWorldSelectionQuestBindingEditor::CEdWorldSelectionQuestBindingEditor( CPropertyItem* item )
	: CEdWorldSelectionEditor( item )
{

}

Bool SearchForQuestScopeBlock( CQuestScopeBlock* blockToSearch, CQuestGraph* graph, TList< CQuestScopeBlock* >& scopeBlockChain )
{
	if ( blockToSearch == NULL || graph == NULL )
	{
		return false;
	}

	TDynArray< CGraphBlock* > graphBlocks = graph->GraphGetBlocks();
	for ( TDynArray< CGraphBlock* >::iterator blockIter = graphBlocks.Begin();
		blockIter != graphBlocks.End(); ++blockIter )
	{
		CQuestScopeBlock* scopeBlock = Cast< CQuestScopeBlock >( *blockIter );
		if ( scopeBlock == NULL )
		{
			continue;
		}

		if ( scopeBlock == blockToSearch )
		{
			scopeBlockChain.PushBack( scopeBlock );
			return true;
		}

		CQuestGraph* scopeSubGraph = NULL;
		if ( scopeBlock->GetPhase() != NULL )
		{
			scopeSubGraph = scopeBlock->GetPhase()->GetGraph();
		}
		else
		{
			scopeSubGraph = scopeBlock->GetGraph();
		}

		if ( scopeSubGraph == NULL )
		{
			continue;
		}

		if ( SearchForQuestScopeBlock( blockToSearch, scopeSubGraph, scopeBlockChain ) == true )
		{
			scopeBlockChain.PushBack( scopeBlock );
			return true;
		}
	}
	
	return false;
}

void CEdWorldSelectionQuestBindingEditor::FillChoices()
{
	CCommonGameResource* gameResource = Cast< CCommonGameResource >( GGame->GetGameResource() );
	if ( gameResource == NULL )
	{
		return;
	}

	CQuest* mainQuest = gameResource->GetMainQuest().Get();
	if ( mainQuest == NULL )
	{
		return;
	}

	CQuestScopeBlock* currentQuestScopeBlock = GetPropertyOwnerObject< CQuestScopeBlock >();
	if ( currentQuestScopeBlock == NULL )
	{
		return;
	}

	TList< CQuestScopeBlock* >	scopeBlocksChain;
	SearchForQuestScopeBlock( currentQuestScopeBlock, mainQuest->GetGraph(), scopeBlocksChain );

	Bool canBindWorldToQuest = true;
	for ( TList< CQuestScopeBlock* >::iterator scopeIter = scopeBlocksChain.Begin();
		scopeIter != scopeBlocksChain.End(); ++scopeIter )
	{
		if ( (*scopeIter)->GetRequiredWorld().Empty() == false && *scopeIter != currentQuestScopeBlock )
		{
			canBindWorldToQuest = false;
			break;
		}
	}

	if ( canBindWorldToQuest == true )
	{
		CEdWorldSelectionEditor::FillChoices();
	}
	else
	{
		m_ctrlChoice->AppendString( String::EMPTY.AsChar() );
	}
	
}

//////////////////////////////////////////////////////////////////////////

CEdCSVWorldSelectionEditor::CEdCSVWorldSelectionEditor( CPropertyItem* item )
	: ISelectionEditor( item )
{

}

void CEdCSVWorldSelectionEditor::FillChoices()
{
	C2dArray* res = resGameWorlds.LoadAndGet< C2dArray > ();
	if ( nullptr == res )
	{
		wxMessageBox( wxT("Missing gameplay\\globals\\gameWorlds.csv"), wxT("Error") );	
		return;
	}

	m_ctrlChoice->AppendString( String::EMPTY.AsChar() );
	for ( Uint32 i = 0; i < res->GetNumberOfRows(); ++i )
	{
		m_ctrlChoice->AppendString( res->GetValue( 0, i ).AsChar() );
	}

}
