/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "componentListSelection.h"
#include "effectProperties.h"
#include "entityEditor.h"
#include "../../common/core/depot.h"
#include "../../games/r4/r4DLCEntityTemplateSlotMounter.h"

CComponentListSelectionBase::CComponentListSelectionBase( CPropertyItem* item, Int32 flags )
	: CListSelection( item )
	, m_flags( flags )
{
	FindEntity();
}

Bool CComponentListSelectionBase::FindEntity()
{
	// Find Entity owner
	m_entity = m_propertyItem->FindPropertyParentOfType< CEntity >( 0 );

	// Find from the property page
	if ( !m_entity.IsValid() )
	{
		if ( CEdEffectEditorProperties* page = m_propertyItem->GetPage()->QueryEffectEditorProperties() )
		{
			// Try entity template
			m_entity = page->GetEntity();
		}
		else if ( CEdEntitySlotProperties* page = m_propertyItem->GetPage()->QueryEntitySlotProperties() )
		{
			// Try slot editor
			if ( CEdEntityEditor* editor = page->GetEntityEditor() )
			{
				m_entity = editor->GetPreviewPanel()->GetEntity();
			}
		}
	}

	return m_entity.IsValid();
}

class wxComponentListData : public wxClientData
{
public:
	String m_name;

public:
	wxComponentListData( const String& name )
		: m_name( name )
	{};
};

void CComponentListSelectionBase::AddEntityComponents( CEntity* entity )
{
	Bool appendType  = ( m_flags & CLF_Components ) != 0 && ( m_flags & CLF_Slots ) != 0;

	if ( ( m_flags & CLF_Components ) != 0 )
	{
		// Get components
		TDynArray< CComponent* > entityComponents;
		CollectEntityComponents( entity, entityComponents );

		// Fill choice control with values
		for ( auto entityComponent = entityComponents.Begin(); entityComponent != entityComponents.End(); ++entityComponent )
		{
			if ( IsComponentApplicable( *entityComponent ) )
			{
				String componentName;

				componentName += (*entityComponent)->GetName().AsChar();

				if ( appendType )
				{
					componentName += TXT(" (Component)");
				}

				m_ctrlChoice->Append( componentName.AsChar(), new wxComponentListData( (*entityComponent)->GetName() ) );
			}
		}
	}

	// Fill the list with slot names
	if ( ( m_flags & CLF_Slots ) != 0 && entity->GetEntityTemplate() )
	{
		// Get all slots
		TDynArray< const EntitySlot* > slots;
		entity->GetEntityTemplate()->CollectSlots( slots, true );

		// Fill choices
		for ( auto it=slots.Begin(); it!=slots.End(); ++it )
		{
			const EntitySlot* slot = *it;

			String slotName;

			slotName += slot->GetName().AsString();
			
			if ( appendType )
			{
				slotName + TXT(" (Slot)");
			}

			m_ctrlChoice->Append( slotName.AsChar(), new wxComponentListData( slot->GetName().AsString().AsChar() ) );
		}
	}
}

void CComponentListSelectionBase::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Calculate editor size
	wxSize size = propRect.GetSize();
	if ( !( m_flags & CLF_NoClear ) )
	{
		size.SetWidth( size.GetWidth() - size.GetHeight() );
	}

	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), size );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER | wxCB_SORT );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();
	m_ctrlChoice->Freeze();

	if ( m_entity.IsValid() || FindEntity() )
	{
		AddEntityComponents( m_entity );
	}
	else
	{
		IGameplayDLCMounter* gameplayDLCMounter = m_propertyItem->FindPropertyParentOfType< IGameplayDLCMounter >( 0 );
		if( gameplayDLCMounter )
		{
			CR4EntityTemplateSlotDLCMounter* entityTemplateSlotDLCMounter = Cast<CR4EntityTemplateSlotDLCMounter>( gameplayDLCMounter );
			if( entityTemplateSlotDLCMounter != nullptr )
			{
				CEntity* entity = nullptr;
				CDiskFile* entityTemplateFile = GDepot->FindFile( entityTemplateSlotDLCMounter->GetBaseEntityTemplatePath() );
				if( entityTemplateFile )
				{
					CEntityTemplate* entityTemplate = Cast<CEntityTemplate>( entityTemplateFile->Load() );
					entityTemplate->CreateFullDataBuffer( nullptr, EntityTemplateInstancingInfo(), nullptr );

					// Create temporary entity for capturing
					entity = entityTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
						
				}					
				if ( entity != nullptr ) 
				{
					AddEntityComponents( entity );
					entity->Discard();
				}
			}
		}
	}

	// End edit
	m_ctrlChoice->Thaw();
	m_ctrlChoice->Refresh();

	// No components
	if ( !m_ctrlChoice->GetCount() )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->AppendString( TXT("( no components available )") );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		// Get edit value
		String str;
		GrabValue( str );

		// Find current value on list and select it
		if ( m_ctrlChoice->HasClientObjectData() )
		{
			const size_t count = m_ctrlChoice->GetCount();
			for ( size_t i=0; i<count; ++i )
			{
				wxComponentListData* data = static_cast< wxComponentListData* >( m_ctrlChoice->GetClientObject( i ) );
				if ( data && data->m_name == str )
				{
					m_ctrlChoice->SetSelection( i );
					break;
				}
			}
		}

		// Notify of selection changes
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CComponentListSelectionEffectParameters::OnChoiceChanged ), NULL, this );

		// Create clear button
		if ( ! ( m_flags & CLF_NoClear ) )
		{
			wxPoint btnPos( propRect.GetTopLeft().x + size.GetWidth(), propRect.GetTopLeft().y );
			wxSize btnSize( size.GetHeight(), size.GetHeight() );
			m_clearButton = new wxButton( m_propertyItem->GetPage(),
				wxID_ANY, wxEmptyString, btnPos, btnSize, wxNO_BORDER|wxBU_EXACTFIT|wxBU_NOTEXT );
			m_clearButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CListSelection::OnClearClicked ), NULL, this );
			m_clearButton->SetBitmap( m_propertyItem->GetPage()->GetStyle().m_iconClear );
			m_clearButton->SetToolTip( wxT("Clear Selection") );
		}
	}
}

Bool CComponentListSelectionBase::SaveValue()
{
	// If the last action was to press the clear button, clear the value
	if ( m_isCleared )
	{
		ClearValue();
		return true;
	}

	// No clear
	if ( m_ctrlChoice && m_ctrlChoice->HasClientObjectData() )
	{
		Int32 selection = m_ctrlChoice->GetSelection();
		if ( selection != -1 )
		{
			wxComponentListData* data = static_cast< wxComponentListData* >( m_ctrlChoice->GetClientObject( selection ) );
			if ( data )
			{
				String valueToSave = data->m_name;

				// Write result
				if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
				{
					// Write as CName
					CName name( valueToSave );
					m_propertyItem->Write( &name );
				}
				else
				{
					// Write as string
					m_propertyItem->Write( &valueToSave );
				}
			}
		}
	}

	// Not saved
	return false;
}

//////////////////////////////////////////////////////////////////////////

Bool CComponentListSelectionEffectParameters::IsComponentApplicable( CComponent* component )
{
	// MGol: commented out due to need of attaching sound events to various components
	//CFXParameters effectParams;
	//component->EnumEffectParameters( effectParams );
	return true;//!effectParams.Empty();
}

//////////////////////////////////////////////////////////////////////////

Bool CComponentListSelectionAll::IsComponentApplicable( CComponent* component )
{
	return true;
}
