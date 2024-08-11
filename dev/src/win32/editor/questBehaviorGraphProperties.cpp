#include "build.h"
#include "questBehaviorGraphProperties.h"
#include "../../games/r4/questBehaviorCtrlBlock.h"


namespace // anonymous
{
	class CBehGraphDataItem : public wxClientData
	{
	public:
		CBehaviorGraph*	m_graph;

		CBehGraphDataItem( CBehaviorGraph* graph ) : m_graph( graph ) {}
	};
}

///////////////////////////////////////////////////////////////////////////////

void CQuestBehaviorGraphSelection::FillChoices()
{
	// grab an instance responsible for providing a list of configured
	// behaviors configured in a parent quest behavior control block
	IQuestBehaviorCtrlScopedBlock* block = m_propertyItem->FindPropertyParentWithInterface< IQuestBehaviorCtrlScopedBlock >( 0 );
	ASSERT( block );

	const CQuestBehaviorCtrlBlock *behCtrlBlock = block->GetParentBehaviorBlock();
	ASSERT( behCtrlBlock );

	TDynArray< TSoftHandle< CBehaviorGraph > > graphs;
	behCtrlBlock->GetBehaviorGraphs( graphs );
	Uint32 count = graphs.Size();
	for( Uint32 i = 0; i < count; ++i )
	{
		CBehaviorGraph* graph = graphs[ i ].Get();
		if ( graph )
		{
			m_ctrlChoice->AppendString( graph->GetFriendlyName().AsChar() );
			m_ctrlChoice->SetClientObject( i, new CBehGraphDataItem( graph ) );
		}
	}

	m_ctrlChoice->AppendString( TXT("" ) );
	m_ctrlChoice->SetClientObject( graphs.Size(), new CBehGraphDataItem( NULL ) );
}

void CQuestBehaviorGraphSelection::OnChoiceChanged( wxCommandEvent &event )
{
	Int32 selected = m_ctrlChoice->GetSelection();
	m_propertyItem->SavePropertyValue();
}

Bool CQuestBehaviorGraphSelection::GrabValue( String& displayValue )
{
	CBehaviorGraph* graph;
	m_propertyItem->Read( &graph );

	if ( graph )
	{
		displayValue = graph->GetFriendlyName();
	}
	else
	{
		displayValue = TXT( "" );
	}

	return true;
}

Bool CQuestBehaviorGraphSelection::SaveValue()
{
	CBehGraphDataItem* selectedItem = dynamic_cast< CBehGraphDataItem* >( m_ctrlChoice->GetClientObject( m_ctrlChoice->GetSelection() ) );
	if ( !selectedItem )
	{
		return false;
	}
	CBehaviorGraph* selGraph = selectedItem->m_graph;

	// find graph with a matching name
	Uint32 count = m_ctrlChoice->GetCount();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CBehGraphDataItem* item = dynamic_cast< CBehGraphDataItem* >( m_ctrlChoice->GetClientObject( i ) );
		if ( !item )
		{
			continue;
		}
		
		if ( item->m_graph == selGraph )
		{
			m_propertyItem->Write( &item->m_graph );
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

void CQuestBehaviorTagsSelection::FillChoices()
{
	// grab an instance responsible for providing a list of configured
	// behaviors configured in a parent quest behavior control block
	IQuestBehaviorCtrlScopedBlock*	block = m_propertyItem->FindPropertyParentWithInterface< IQuestBehaviorCtrlScopedBlock >( 0 );
	ASSERT( block );

	const CQuestBehaviorCtrlBlock *behCtrlBlock = block->GetParentBehaviorBlock();
	ASSERT( behCtrlBlock );

	TagList tags;
	behCtrlBlock->GetAllTags( tags );

	const TDynArray< CName >& tagsArr = tags.GetTags();
	Uint32 count = tagsArr.Size();
	for( Uint32 i = 0; i < count; ++i )
	{
		wxString tag( tagsArr[ i ].AsString().AsChar() );
		m_ctrlChoice->AppendString( tag );
	}

	m_ctrlChoice->AppendString( TXT("") );
}

void CQuestBehaviorTagsSelection::OnChoiceChanged( wxCommandEvent &event )
{
	Int32 selected = m_ctrlChoice->GetSelection();

	m_propertyItem->SavePropertyValue();
}
