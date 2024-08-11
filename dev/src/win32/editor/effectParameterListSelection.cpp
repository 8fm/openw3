/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "effectParameterListSelection.h"
#include "effectProperties.h"
#include "../../common/engine/fxTrackGroup.h"
#include "../../common/engine/fxDefinition.h"

CEffectParameterListSelection::CEffectParameterListSelection( CPropertyItem* item )
	: CListSelection( item )
{
	// Try entity template
	CEdEffectEditorProperties* page = item->GetPage()->QueryEffectEditorProperties();
	if ( page )
	{
		// Get entity from page
		m_entity = page->GetEntity();
		ASSERT( m_entity );

		// Get track group
 		CObject *parentObject = m_propertyItem->GetParentObject(0).AsObject();
		m_fxTrackGroup = parentObject->FindParent< CFXTrackGroup >();
		ASSERT( m_fxTrackGroup );

		// Get component name
		m_componentName = m_fxTrackGroup ? m_fxTrackGroup->GetComponentName() : CName::NONE;
	}
}

Bool CEffectParameterListSelection::CanSupportType( const CName &typeName )
{
	return typeName == GetTypeName< Float >();
}

void CEffectParameterListSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	// Get components
	TDynArray< CComponent* > entityComponents;
	if ( m_entity )
	{
		entityComponents = m_entity->GetComponents();
	}

	// Fill choice control with values
	CFXParameters effectParams;
	for ( TDynArray< CComponent* >::iterator entityComponent = entityComponents.Begin(); entityComponent != entityComponents.End(); ++entityComponent )
	{
		if ( (*entityComponent)->GetName() == m_componentName.AsString() )
		{
			(*entityComponent)->EnumEffectParameters( effectParams );
			break;
		}
	}

	if ( m_componentName == CName::NONE )
	{
		// It is a bit of hack - if no component is selected, assume that user wants to select all components, so put all parameters
		effectParams.AddParameter< Float >( CNAME( MeshEffectScalar0 ) );
		effectParams.AddParameter< Float >( CNAME( MeshEffectScalar1 ) );
		effectParams.AddParameter< Float >( CNAME( MeshEffectScalar2 ) );
		effectParams.AddParameter< Float >( CNAME( MeshEffectScalar3 ) );
	}

	// Show list
	if ( effectParams.Empty() )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->AppendString( TXT("( no parameters available )") );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		// Add classes
		TDynArray< String > componentsNames;
		for ( Uint32 i=0; i < effectParams.Size(); i++ )
		{
			IRTTIType* type = effectParams.GetParameterType(i);
			if ( type && CanSupportType( type->GetName() ) )
			{
				componentsNames.PushBack( effectParams.GetParameterName(i).AsString().AsChar() );
			}
		}

		// Sort list and add to combo box
		Sort( componentsNames.Begin(), componentsNames.End() );
		for ( Uint32 i=0; i<componentsNames.Size(); i++ )
		{
			m_ctrlChoice->AppendString( componentsNames[i].AsChar() );
		}

		// Find current value on list and select it
		String str;
		GrabValue( str );
		int index = m_ctrlChoice->FindString( str.AsChar() );
		if ( index >= 0 )
		{
			m_ctrlChoice->SetSelection( index );
		}
		else
		{
			m_ctrlChoice->SetSelection( 0 );
		}

		// Notify of selection changes
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEffectParameterListSelection::OnChoiceChanged ), NULL, this );
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CEffectParameterColorListSelection::CanSupportType( const CName &typeName )
{
	if ( typeName == GetTypeName< Color >() || typeName == GetTypeName< Vector >() )
	{
		return true;
	}
	else
	{
		return false;
	}
}
