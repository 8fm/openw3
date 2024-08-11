/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/game/actionAreaVertex.h"
#include "../../common/engine/hitProxyObject.h"

/// Editor tool for editing vertices
class CEdSpriteEdit : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdSpriteEdit, IEditorTool, 0 );

public:
	CWorld*								m_world;						//!< World shortcut
	CEdRenderingPanel*					m_viewport;						//!< Viewport shortcut
	TDynArray< CComponent* >			m_editedComponents;				//!< Selected components that support vertex edit
	TDynArray< CVertexEditorEntity* >	m_spritesToEdit;

public:
	CEdSpriteEdit();
	virtual String GetCaption() const { return TXT("Sprite edit"); }
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End();	
	
	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	virtual Bool HandleContextMenu( Int32 x, Int32 y, const Vector& collision ) { return true; }

	virtual Bool UsableInActiveWorldOnly() const { return false; }

private:
	void DeleteVertices( const TDynArray< CVertexComponent* > vertices );
	void Reset();
};

BEGIN_CLASS_RTTI( CEdSpriteEdit );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();


IMPLEMENT_ENGINE_CLASS( CEdSpriteEdit );

CEdSpriteEdit::CEdSpriteEdit()
	: m_world( NULL )
	, m_viewport( NULL )
{
}

Bool CEdSpriteEdit::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* sizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	// Remember world
	m_world = world;

	// Remember viewport
	m_viewport = viewport;

	viewport->GetSelectionManager()->SetGranularity( CSelectionManager::SG_Entities );

	// Start editing
	m_spritesToEdit.Clear();
	for ( Uint32 i=0; i<selection.Size(); i++ )
	{
		CComponent* component = selection[i];

		TDynArray< CEntity* > sprites;
		if ( component->OnEditorBeginSpriteEdit( sprites ) && ! sprites.Empty() )
		{
			Bool isAllOk = true;
			for ( Uint32 i = 0; i < sprites.Size(); ++i )
			{
				if ( ! sprites[i]->IsA< CVertexEditorEntity >() )
				{
					ASSERT( sprites[i]->IsA< CVertexEditorEntity >() );
					isAllOk = false;
					break;
				}
			}
			if ( isAllOk )
			{
				for ( Uint32 i = 0; i < sprites.Size(); ++i )
				{
					CVertexEditorEntity * vertex = Cast< CVertexEditorEntity >( sprites[i] );
					m_spritesToEdit.PushBack( vertex );
					vertex->m_owner = component;
					vertex->m_index = i;
				}
				m_editedComponents.PushBack( component );
			}
			else
			{
				component->OnEditorEndSpriteEdit();
			}
		}
	}

	// No area components to edit
	if ( !m_editedComponents.Size() )
	{
		WARN_EDITOR( TXT("No area components selected") );
		return false;
	}

	// Initialized
	return true;
}

void CEdSpriteEdit::End()
{
	// Kill vertices
	for ( Uint32 i=0; i<m_editedComponents.Size(); i++ )
	{
		CComponent* ac = m_editedComponents[i];
		ac->OnEditorEndSpriteEdit();
	}
}

Bool CEdSpriteEdit::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	CSelectionManager::CSelectionTransaction transaction(*m_viewport->GetSelectionManager());

	// Deselect all selected object
	if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
	{
		m_viewport->GetSelectionManager()->DeselectAll();
	}

	// Select only sprites
	for ( Uint32 i=0; i<objects.Size(); i++ )
	{
		CVertexEditorEntity *node = objects[i]->GetHitObject()->FindParent<CVertexEditorEntity>();
		if ( node != NULL && m_spritesToEdit.Exist( node ) )
		{
			m_viewport->GetSelectionManager()->Select( node );
			break;
		}
	}

	// Handled
	return true;
}

