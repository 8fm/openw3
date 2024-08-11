#include "build.h"
#include "questGraphInstance.h"
#include "questPhaseInputBlock.h"


IMPLEMENT_ENGINE_CLASS( CQuestGraphInstance );

CQuestGraphInstance::CQuestGraphInstance()
: m_parentGraph( NULL )
, m_data( NULL )
{
}

CQuestGraphInstance::~CQuestGraphInstance()
{
	RED_ASSERT( m_data == nullptr, TXT("Quest data was not deleted. Was the Unbind() called?") );
	Unbind();
}

void CQuestGraphInstance::Bind( const CQuestGraph& parentGraph, 
										 const TDynArray< CGraphBlock* >& graphBlocks,
										 const InstanceDataLayout& dataLayout )
{
	m_parentGraph = &parentGraph;
	m_data = dataLayout.CreateBuffer( this, TXT( "CQuestGraphInstance" ) );

	Uint32 count = graphBlocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		ASSERT( graphBlocks[ i ], TXT("There is a NULL in the graph block array") );
		CQuestGraphBlock* block = Cast< CQuestGraphBlock >( graphBlocks[ i ] );
		if ( block )
		{
			block->OnInitInstance( *m_data );
			m_graphBlocks.PushBack( block );
		}
	}
}

void CQuestGraphInstance::Unbind()
{
	if ( m_data )
	{
		m_data->Release();
		m_data = NULL;
	}

	m_graphBlocks.Clear();
	m_parentGraph = NULL;
}

const CQuestGraphBlock* CQuestGraphInstance::GetStartBlock( const CName& name ) const
{
	Uint32 count = m_graphBlocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( !m_graphBlocks[ i ]->IsExactlyA< CQuestPhaseInputBlock >() )
		{
			continue;
		}
		
		CQuestPhaseInputBlock* phaseInput = SafeCast< CQuestPhaseInputBlock >( m_graphBlocks[ i ] );
		if ( phaseInput->GetSocketID() == name )
		{
			return phaseInput;
		}
	}
	return NULL;
}

const CQuestGraphBlock* CQuestGraphInstance::FindBlockByGUID( const CGUID& guid ) const
{
	const CQuestGraphBlock* block = NULL;

	// Linear search
	const Uint32 count = m_graphBlocks.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		block = Cast< CQuestGraphBlock >( m_graphBlocks[ i ] );

		// at this point we want to check if the block has the memorized GUID.
		// Since guid matching is block-type dependent, we want to leave it up to 
		// the block to decide ( some blocks can hold multiple GUIDs used only for matching purposes )
		if ( block && block->MatchesGUID( guid ) )
		{
			// Found, use it
			return block;
		}
	}

	// Not found
	return NULL;
}
