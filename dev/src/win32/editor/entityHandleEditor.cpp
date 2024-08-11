/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "entityHandleEditor.h"
#include "../../common/engine/hitProxyObject.h"


class CEntityHandleSelector : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEntityHandleSelector, IEditorTool, 0 );

public:
	CEntityHandleEditor* m_editor;

public:
	CEntityHandleSelector()
	{
	}

	void SetEditor( CEntityHandleEditor* editor )
	{
		m_editor = editor;
	}

	virtual String GetCaption() const
	{
		return TXT( "Entity handle" );
	}

	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
	{
		return true;
	}

	virtual void End()
	{
	}

	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects )
	{
		if ( objects.Size() )
		{
			CComponent* tc = Cast< CComponent >( objects[0]->GetHitObject() );

			if ( tc )
			{
				CEntity* node = tc->GetEntity();

				if ( node && m_editor )
				{
					m_editor->OnSelect( node );
				}
			}

			return true;
		}

		return false;
	}

	virtual Bool UsableInActiveWorldOnly() const { return false; }

};

BEGIN_CLASS_RTTI( CEntityHandleSelector );
PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CEntityHandleSelector );


CEntityHandleEditor::CEntityHandleEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
	, m_selector( NULL )
{
}

void CEntityHandleEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_propertyItem->AddButton( m_propertyItem->GetPage()->GetStyle().m_iconClear, wxCommandEventHandler( CEntityHandleEditor::OnClearReference ), this );
	m_propertyItem->AddButton( m_propertyItem->GetPage()->GetStyle().m_iconPick, wxCommandEventHandler( CEntityHandleEditor::OnUseSelected ), this );
	m_propertyItem->AddButton( m_propertyItem->GetPage()->GetStyle().m_iconBrowse, wxCommandEventHandler( CEntityHandleEditor::OnSelectReferenced ), this );
}

void CEntityHandleEditor::DestroySelector()
{
	if ( m_selector )
	{
		wxTheFrame->GetWorldEditPanel()->SetTool( NULL );

		m_selector->RemoveFromRootSet();
		m_selector = NULL;
	}
}

void CEntityHandleEditor::CloseControls()
{
	DestroySelector();
}

Bool CEntityHandleEditor::GrabValue( String& displayValue )
{
	// Reset
	displayValue = TXT("None");

	// Read handle
	EntityHandle handle;
	if ( m_propertyItem->Read( &handle ) )
	{
		displayValue = handle.ToString();
	}

	// Valid string
	return true;
}

Bool CEntityHandleEditor::SaveValue()
{
	return true;
}

void CEntityHandleEditor::OnSelectReferenced( wxCommandEvent &event )
{
	// Get world
	CWorld *world = GGame->GetActiveWorld();
	if ( !world )
	{
		wxMessageBox( TXT("No world is loaded now"), TXT("No world!"), wxOK | wxICON_ERROR );
		return;
	}

	// Get entity handle value
	EntityHandle handle;
	if ( m_propertyItem->Read( &handle ) )
	{
		// Get entity
		CEntity* entity = handle.Get();
		if ( entity )
		{
			// Select entity
			world->GetSelectionManager()->DeselectAll();
			world->GetSelectionManager()->Select( entity );
		}
		else
		{
			wxMessageBox( TXT("Invalid or no entity"), TXT("No entity!"), wxOK | wxICON_ERROR );
		}
	}
}

void CEntityHandleEditor::OnUseSelected( wxCommandEvent &event )
{
	// Get world
	CWorld *world = GGame->GetActiveWorld();
	if ( !world )
	{
		wxMessageBox( TXT("No world is loaded now"), TXT("No world!"), wxOK | wxICON_ERROR );
		return;
	}

	if ( m_selector = Cast<CEntityHandleSelector>( CreateObject<CEntityHandleSelector>( NULL, (Uint16)0 ) ) )
	{
		m_selector->AddToRootSet();
		m_selector->SetEditor( this );
		wxTheFrame->GetWorldEditPanel()->SetTool( m_selector );
	}
}

void CEntityHandleEditor::OnSelect( CEntity* entity )
{
	// Get entity handle value
	EntityHandle handle;
	if ( m_propertyItem->Read( &handle ) )
	{
		// Set new value
		handle.Set( entity );

		// Write value
		m_propertyItem->Write( &handle );

		// Refresh display
		m_propertyItem->GrabPropertyValue();
		m_propertyItem->Collapse();
	}
	DestroySelector();
}

void CEntityHandleEditor::OnClearReference( wxCommandEvent &event )
{
	// Get entity handle value
	EntityHandle handle;
	if ( m_propertyItem->Read( &handle ) )
	{
		// Clear value
		handle.Set( NULL );

		// Write value
		m_propertyItem->Write( &handle );

		// Refresh display
		m_propertyItem->GrabPropertyValue();
		m_propertyItem->Collapse();
	}
}
