/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "moveEntities.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/worldIterators.h"

BEGIN_EVENT_TABLE( CEdMoveEntity, wxSmartLayoutPanel )
	EVT_BUTTON( XRCID( "Move" ), CEdMoveEntity::OnMove )
	EVT_BUTTON( XRCID( "Cancel" ), CEdMoveEntity::OnCancel )
END_EVENT_TABLE()

CEdMoveEntity::CEdMoveEntity( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TEXT( "MoveWorldEntities" ), true )
{
}

void CEdMoveEntity::OnMove( wxCommandEvent& event )
{
	// Extract shit
	const Float x = _wtof( XRCCTRL( *this, "OffsetX", wxTextCtrl )->GetValue().wc_str() );
	const Float y = _wtof( XRCCTRL( *this, "OffsetY", wxTextCtrl )->GetValue().wc_str() );
	const Float z = _wtof( XRCCTRL( *this, "OffsetZ", wxTextCtrl )->GetValue().wc_str() );
	const Bool selectedOnly = XRCCTRL( *this, "SelectedOnly", wxCheckBox )->GetValue();
	const Bool terrainChunksOnly = XRCCTRL( *this, "TerrainChunksOnly", wxCheckBox )->GetValue();	

	// Filter entities
	if ( GGame->GetActiveWorld() )
	{
		if ( !terrainChunksOnly )
		{
			// Move only entities that have some drawable components
			TDynArray< CEntity * > allEntitiesToMove;
			for ( WorldAttachedEntitiesIterator it( GGame->GetActiveWorld() ); it; ++it )
			{
				CEntity* entity = *it;

				// Not on any layer
				if ( !entity->GetLayer() || !entity->GetLayer()->GetLayerInfo() )
				{
					continue;
				}

				// Not selected
				if ( selectedOnly && !entity->IsSelected() )
				{
					continue;
				}

				// Check if entity has any drawable component
				Bool hasDrawableComponents = false;
				const TDynArray< CComponent* >& components = entity->GetComponents();
				for ( Uint32 j=0; j<components.Size(); j++ )
				{
					CComponent* comp = components[j];
					if ( comp )
					{
						hasDrawableComponents = true;
						break;
					}
				}

				// Entity has drawable component, we can move it
				if ( hasDrawableComponents )
				{
					allEntitiesToMove.PushBack( entity );
				}
			}

			// Move entities
			GFeedback->BeginTask( TXT("Moving entities..."), false );
			for ( Uint32 i=0; i<allEntitiesToMove.Size(); i++ )
			{
				CEntity* entity = allEntitiesToMove[i];

				// Update progress
				GFeedback->UpdateTaskProgress( i, allEntitiesToMove.Size() );

				// Update position
				Vector pos = entity->GetWorldPosition();
				pos += Vector( x, y, z );
				entity->SetRawPlacement( &pos, NULL, NULL );

				// Transform now
				entity->ForceUpdateTransformNodeAndCommitChanges();
				entity->ForceUpdateBoundsNode();

				// Move
				entity->MarkModified();
			}
			GFeedback->EndTask();
		}
	}
}

void CEdMoveEntity::OnCancel( wxCommandEvent& event )
{
	Hide();
}
