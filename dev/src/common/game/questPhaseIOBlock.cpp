#include "build.h"
#include "questPhaseIOBlock.h"
#include "questScopeBlock.h"
#include "questGraph.h"

IMPLEMENT_ENGINE_CLASS( CQuestPhaseIOBlock )

CQuestPhaseIOBlock::CQuestPhaseIOBlock(void)
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CQuestPhaseIOBlock::GetCaption() const
{
	return m_socketID.AsString();
}

void CQuestPhaseIOBlock::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );

	if ( CQuestScopeBlock *scopeBlock = CQuestScopeBlock::GetParentScopeBlock( this ) )
	{
		scopeBlock->OnRebuildSockets();
	}
}

void CQuestPhaseIOBlock::OnDestroyed()
{
	TBaseClass::OnDestroyed( );

	if ( CQuestScopeBlock *scopeBlock = CQuestScopeBlock::GetParentScopeBlock( this ) )
	{
		scopeBlock->OnRebuildSockets();
	}
}

#endif

#ifndef NO_EDITOR
void CQuestPhaseIOBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPreChange( property );

	if ( property->GetName() == TXT("socketID") )
	{
		CObject* parent = GetParent();
		if ( parent->IsA< CQuestGraph >() )
		{
			CQuestGraph* graph = Cast< CQuestGraph >( parent );
			graph->RepairDuplicateInputs( TDynArray< CQuestPhaseIOBlock* >() );
		}
	}
}
#endif