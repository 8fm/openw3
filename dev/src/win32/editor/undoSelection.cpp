#include "build.h"

#include "undoSelection.h"
#include "undoManager.h"

IMPLEMENT_ENGINE_CLASS( CUndoSelection );

void CUndoSelection::CreateStep( CEdUndoManager& undoManager, CSelectionManager &selManager, Uint32 transactionId )
{
    CUndoSelection *lastSelection = Cast< CUndoSelection >( undoManager.GetCurrentStep() );

    if ( lastSelection && lastSelection->m_transactionId == transactionId )
	{
		StoreSelection( selManager, lastSelection->m_selectedNodes, lastSelection->m_selectedLayer );
	}
    else
	{
		TDynArray< CNode* > selectedNodes;
		THandle< ISerializable > selectedLayer;
		StoreSelection( selManager, selectedNodes, selectedLayer );

		if ( !lastSelection || lastSelection->m_selectedNodes != selectedNodes || lastSelection->m_selectedLayer != selectedLayer )
		{
			CUndoSelection* step = new CUndoSelection( undoManager, selManager, selectedNodes, selectedLayer, transactionId );
			step->PushStep();
		}

	}
}

CUndoSelection::CUndoSelection( CEdUndoManager& undoManager, CSelectionManager &selManager, const TDynArray< CNode* >& selectedNodes, THandle< ISerializable > selectedLayer, Uint32 transactionId )
    : IUndoStep         ( undoManager   )
    , m_selectionManager( &selManager   )
	, m_selectedNodes   ( selectedNodes )
	, m_selectedLayer   ( selectedLayer )
    , m_transactionId   ( transactionId )
{
}

void CUndoSelection::StoreSelection( const CSelectionManager& selManager, TDynArray< CNode* >& selectedNodes, THandle< ISerializable >& selectedLayer )
{
	selManager.GetSelectedNodes( selectedNodes );
	Sort( selectedNodes.Begin(), selectedNodes.End() ); // to allow easy comparison
	selectedLayer = selManager.GetSelectedLayer();
}	

void CUndoSelection::DoUndo()
{
    IUndoStep *prevStep = GetPrevStep();

    while ( prevStep )
    {
        CUndoSelection *prevSelection = Cast<CUndoSelection>( prevStep );

        if ( prevSelection )
        {
            prevSelection->DoRedo();
            return;
        }

        prevStep = prevStep->GetPrevStep();
    }

    m_selectionManager->DeselectAll();
}

void CUndoSelection::DoRedo()
{
    CSelectionManager::CSelectionTransaction transaction(*m_selectionManager);
    m_selectionManager->DeselectAll();
    m_selectionManager->SelectLayer( m_selectedLayer.Get() );
    for ( Uint32 i = 0 ; i < m_selectedNodes.Size(); ++i )
        if ( ! m_selectedNodes[i]->IsSelected() )
            m_selectionManager->Select( m_selectedNodes[i] );
}

void CUndoSelection::OnObjectRemoved( CObject *object )
{
    Uint32 size = m_selectedNodes.Size();

    m_selectedNodes.Remove( Cast<CNode>( object ) );
    
    CEntity *entity = Cast<CEntity>( object );
    if ( entity )
    {
        TDynArray<CComponent*> components;
        CollectEntityComponents( entity, components);

        for( Uint32 iComponent = 0; iComponent < components.Size(); ++iComponent )
            m_selectedNodes.Remove( components[iComponent] );
    }

    if ( size != 0 && m_selectedNodes.Size() == 0 )
        PopStep();
}