#include "build.h"
#include "questsAnalyzer.h"
#include "questThread.h"
#include "questGraphBlock.h"


CQuestsAnalyzer::CQuestsAnalyzer( CQuestThread& thread )
	: m_thread( &thread )
{
	Update();
}

void CQuestsAnalyzer::Update()
{
	m_blocks.Clear();

	TDynArray< Node > threadsStack;
	threadsStack.PushBack( Node( String( TXT( "/" ) ) + m_thread->GetName() + TXT( "/" ), m_thread ) );

	while ( threadsStack.Empty() == false )
	{
		Node currThread = threadsStack.PopBack();

		TDynArray< const CQuestGraphBlock* > blocks;
		currThread.thread->GetActiveBlocks( blocks );

		Uint32 count = blocks.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			m_blocks.Insert( currThread.path + blocks[ i ]->GetBlockName(), 
				SQuestBlockExecContext( currThread.thread, blocks[ i ] ) );
		}

		const TDynArray< CQuestThread* >& threads = currThread.thread->GetChildrenThreads();
		count = threads.Size();
		for ( Uint32 i = 0; i < count; ++i )
		{
			threadsStack.PushBack( Node( currThread.path + threads[ i ]->GetName() + TXT( "/" ), threads[ i ] ) );
		}
	}
}

SQuestBlockExecContext* CQuestsAnalyzer::FindBlock( const String& blockName, Bool matchPart )
{
	
	ActiveQuestBlocksMap::iterator it;
	
	if ( matchPart )
	{
		for ( it = m_blocks.Begin(); it != m_blocks.End(); ++it )
		{
			if ( it->m_first.ContainsSubstring( blockName ) )
			{
				break;
			}
		}
	}
	else
	{
		it = m_blocks.Find( blockName );
	}

	if ( it == m_blocks.End() )
	{
		return NULL;
	}
	else
	{
		return &it->m_second;
	}
}
