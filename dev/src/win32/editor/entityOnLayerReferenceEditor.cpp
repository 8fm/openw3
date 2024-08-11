/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "entityOnLayerReferenceEditor.h"
#include "../../common/engine/tagManager.h"


namespace // anonymous
{
	class CEntitySelectionDialog : public wxDialog
	{
	private:
		CEntity*	m_selectedEntity;
		wxListBox*	m_selectionList;


	public:
		CEntitySelectionDialog( wxWindow* parent, const TDynArray< CEntity* >& entities )
			: wxDialog( parent, wxID_ANY, wxT( "Select entity" ), wxDefaultPosition, wxDefaultSize )
			, m_selectedEntity( NULL )
		{
			SetSizeHints( wxDefaultSize, wxDefaultSize );

			wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

			m_selectionList = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_HSCROLL ); 
			sizer->Add( m_selectionList, 1, wxALL|wxEXPAND, 5 );

			wxButton* okButton = new wxButton( this, wxID_ANY, wxT("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
			sizer->Add( okButton, 0, wxALL, 5 );

			SetSizer( sizer );
			Layout();

			// Connect Events
			m_selectionList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CEntitySelectionDialog::OnItemSelected ), NULL, this );
			m_selectionList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CEntitySelectionDialog::OnItemSelectedAndConfirmed ), NULL, this );
			okButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEntitySelectionDialog::OnOK ), NULL, this );

			// fill the list with data
			for ( TDynArray< CEntity* >::const_iterator it = entities.Begin(); it != entities.End(); ++it )
			{
				if ( !*it )
				{
					continue;
				}
				m_selectionList->Append( (*it)->GetFriendlyName().AsChar(), *it );
			}
		}

		void OnItemSelected( wxCommandEvent& event )
		{
			Int32 selectedItemIdx = m_selectionList->GetSelection();
			if ( selectedItemIdx < 0 )
			{
				m_selectedEntity = NULL;
			}
			else
			{
				m_selectedEntity = static_cast< CEntity* >( m_selectionList->GetClientData( selectedItemIdx ) );
			}
		}
		
		void OnItemSelectedAndConfirmed( wxCommandEvent& event )
		{ 
			OnItemSelected( event );
			EndDialog(0);
		}
		
		void OnOK( wxCommandEvent& event )
		{
			EndDialog(0);
		}

		RED_INLINE CEntity* GetSelectedEntity() const { return m_selectedEntity; }
	};

	// -------------------------------------------------------------------------

	class CTagsSelectionDialog : public wxDialog
	{
	private:
		CName			m_selectedTag;
		wxListBox*	m_selectionList;


	public:
		CTagsSelectionDialog( wxWindow* parent, const TDynArray< CName >& tags )
			: wxDialog( parent, wxID_ANY, wxT( "Select a tag" ), wxDefaultPosition, wxDefaultSize )
		{
			SetSizeHints( wxDefaultSize, wxDefaultSize );

			wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

			m_selectionList = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
			sizer->Add( m_selectionList, 1, wxALL|wxEXPAND, 5 );

			wxButton* okButton = new wxButton( this, wxID_ANY, wxT("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
			sizer->Add( okButton, 0, wxALL, 5 );

			SetSizer( sizer );
			Layout();

			// Connect Events
			m_selectionList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CTagsSelectionDialog::OnItemSelected ), NULL, this );
			m_selectionList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( CTagsSelectionDialog::OnItemSelectedAndConfirmed ), NULL, this );
			okButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CTagsSelectionDialog::OnOK ), NULL, this );

			// fill the list with data
			for ( TDynArray< CName >::const_iterator it = tags.Begin(); it != tags.End(); ++it )
			{
				if ( it->Empty() )
				{
					continue;
				}
				m_selectionList->Append( it->AsString().AsChar() );
			}
		}

		void OnItemSelected( wxCommandEvent& event )
		{
			Int32 selectedItemIdx = m_selectionList->GetSelection();
			if ( selectedItemIdx < 0 )
			{
				m_selectedTag = CName::NONE;
			}
			else
			{
				m_selectedTag = CName( m_selectionList->GetString( selectedItemIdx ).wc_str() );
			}
		}

		void OnItemSelectedAndConfirmed( wxCommandEvent& event )
		{ 
			OnItemSelected( event );
			EndDialog(0);
		}

		void OnOK( wxCommandEvent& event )
		{
			EndDialog(0);
		}

		RED_INLINE CName GetSelectedTag() const { return m_selectedTag; }
	};

} // anonymous

///////////////////////////////////////////////////////////////////////////////

CEntityOnLayerReferenceEditor::CEntityOnLayerReferenceEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
{
}

void CEntityOnLayerReferenceEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	wxRect valueRect;
	valueRect.y = propertyRect.y + 3;
	valueRect.height = propertyRect.height - 3;
	valueRect.x = propertyRect.x + 2;
	valueRect.width = propertyRect.width - propertyRect.height * 3 - 2;

	// Create text editor
	m_ctrlText = new wxTextCtrlEx
	(
		m_propertyItem->GetPage(),
		wxID_ANY,
		wxEmptyString,
		valueRect.GetTopLeft(),
		valueRect.GetSize(),
		(
			wxNO_BORDER |
			wxTE_PROCESS_ENTER |
			wxTE_READONLY
		)
	);

	m_ctrlText->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_ctrlText->SetFont( m_propertyItem->GetPage()->GetStyle().m_drawFont );
	m_ctrlText->SetSelection( -1, -1 );
	m_ctrlText->SetFocus();

	outSpawnedControls.PushBack( m_ctrlText );

	// Add button to clear reference to entity
	m_propertyItem->AddButton( m_propertyItem->GetPage()->GetStyle().m_iconClear, wxCommandEventHandler( CEntityOnLayerReferenceEditor::OnClearReference ), this );
	// Add button to make reference to selected entity
	m_propertyItem->AddButton( m_propertyItem->GetPage()->GetStyle().m_iconUse, wxCommandEventHandler( CEntityOnLayerReferenceEditor::OnUseSelected ), this );
	// Add button to make select the referenced entity
	m_propertyItem->AddButton( m_propertyItem->GetPage()->GetStyle().m_iconBrowse, wxCommandEventHandler( CEntityOnLayerReferenceEditor::OnSelectReferenced ), this );
}

void CEntityOnLayerReferenceEditor::CloseControls()
{
	if ( m_ctrlText )
	{
		m_ctrlText->Destroy();
		m_ctrlText = nullptr;
	}
}

Bool CEntityOnLayerReferenceEditor::GrabValue( String& displayValue )
{
	// Read value
	displayValue = TXT("None");
	CName entityRef;
	if ( m_propertyItem->Read( &entityRef ) )
	{
		displayValue = entityRef.AsString();
	}

	if ( m_ctrlText )
	{
		m_ctrlText->ChangeValue( displayValue.AsChar() );
		m_ctrlText->SetInsertionPoint( 0 );
	}


	return true;
}

Bool CEntityOnLayerReferenceEditor::SaveValue()
{
	return true;
}

void CEntityOnLayerReferenceEditor::OnSelectReferenced( wxCommandEvent &event )
{
	CWorld *world = GGame->GetActiveWorld();

	if ( !world )
	{
		wxMessageBox( TXT("No world is loaded now"), TXT("No world!"), wxOK | wxICON_ERROR );
		return;
	}

	CName entityRef;
	if ( !m_propertyItem->Read( &entityRef ) )
	{
		return;
	}

	TagList tags; tags.AddTag( entityRef );
	TDynArray< CEntity* > entities;
	world->GetTagManager()->CollectTaggedEntities( entityRef, entities );
	if ( entities.Empty() )
	{
		wxMessageBox( TXT("No entity matching this tag found"), TXT("No entity!"), wxOK | wxICON_ERROR );
		return;
	}

	CEntity* selectedEntity = NULL;
	if ( entities.Size() == 1 )
	{
		selectedEntity = entities[0];
	}
	else
	{
		CEntitySelectionDialog dlg( NULL, entities );
		dlg.ShowModal();
		selectedEntity = dlg.GetSelectedEntity();
	}

	if ( selectedEntity )
	{
		world->GetSelectionManager()->DeselectAll();
		world->GetSelectionManager()->Select( selectedEntity );
	}
	else
	{
		wxMessageBox( TXT("Invalid entity"), TXT("No entity!"), wxOK | wxICON_ERROR );
		return;

	}
}

void CEntityOnLayerReferenceEditor::OnUseSelected( wxCommandEvent &event )
{
	CWorld *world = GGame->GetActiveWorld();
	
	if ( !world )
	{
		wxMessageBox( TXT("No world is loaded now"), TXT("No world!"), wxOK | wxICON_ERROR );
		return;
	}

	TDynArray<CEntity*> entities;
	world->GetSelectionManager()->GetSelectedEntities( entities );

	if ( entities.Size() != 1 )
	{
		wxMessageBox( TXT("Select exactly one entity"), TXT("Wrong selection!"), wxOK | wxICON_ERROR );
		return;
	}

	CEntity *entity = entities[0];
	if ( entity->GetLayer()->IsTransient() )
	{
		wxMessageBox( TXT("Cannot referenced entity that is dynamic"), TXT("Wrong selection!"), wxOK | wxICON_ERROR );
		return;
	}

	// select a tag of th eentity
	CName selectedTag;
	const TDynArray< CName >& tags = entity->GetTags().GetTags();
	if ( tags.Size() == 0 )
	{
		wxMessageBox( TXT("The entity doesn't have any tags"), TXT("No tags on entity!"), wxOK | wxICON_ERROR );
		return;
	}
	else if ( tags.Size() == 1 )
	{
		selectedTag = tags[0];
	}
	else
	{
		CTagsSelectionDialog dlg( NULL, tags );
		dlg.ShowModal();
		selectedTag = dlg.GetSelectedTag();
	}

	// verify that the tag is valid
	if ( selectedTag.Empty() )
	{
		wxMessageBox( TXT("You need to select a tag of the entity"), TXT("No tag specified!"), wxOK | wxICON_ERROR );
		return;
	}

	CName entityRef;

	if ( m_propertyItem->Read( &entityRef ) )
	{
		entityRef = selectedTag;

		// Write value
		m_propertyItem->Write( &entityRef );
		m_propertyItem->GrabPropertyValue();
		m_propertyItem->Collapse();
	}
}

void CEntityOnLayerReferenceEditor::OnClearReference( wxCommandEvent &event )
{
	CName entityRef;
	if ( m_propertyItem->Read( &entityRef ) )
	{
		entityRef = CName::NONE;

		// Write value
		m_propertyItem->Write( &entityRef );
		m_propertyItem->GrabPropertyValue();
		m_propertyItem->Collapse();
	}
}
