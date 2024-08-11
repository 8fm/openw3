/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../../common/engine/skeletonProvider.h"
#include "effectBoneListSelection.h"
#include "entityEditor.h"
#include "../../common/core/depot.h"
#include "../../games/r4/r4DLCEntityTemplateSlotMounter.h"

THandle< CEntity > CEffectBoneListSelection::m_editedEntity;

CEffectBoneListSelection::CEffectBoneListSelection( CPropertyItem* item )
	: CListSelection( item )
	, m_propertyItem( item )
{
}

void CEffectBoneListSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	LoadData();

	if ( m_ctrlChoice->GetCount() > 0 )
	{
		// Notify of selection changes
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEffectBoneListSelection::OnChoiceChanged ), NULL, this );
	}
}

void CEffectBoneListSelection::LoadData()
{
	// Fill choice control with values
	TDynArray< String > bonesNames;

	CEntity* entity = m_editedEntity.Get();
	if ( entity != NULL )
	{
		CAnimatedComponent* animComponent = entity->GetRootAnimatedComponent();
		if( animComponent != NULL )
		{
			TDynArray< ISkeletonDataProvider::BoneInfo > bones;
			animComponent->GetBones( bones );
			for ( TDynArray< ISkeletonDataProvider::BoneInfo >::const_iterator bone = bones.Begin();
				bone != bones.End();
				++bone )
			{
				bonesNames.PushBack( bone->m_name.AsString() );
			}
		}
	}

	static_cast<wxItemContainer*>( m_ctrlChoice )->Clear();

	if ( bonesNames.Size() == 0 )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->AppendString( TXT("( no parameters available )") );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		// Sort list and add to combo box
		Sort( bonesNames.Begin(), bonesNames.End() );
		for ( Uint32 i=0; i < bonesNames.Size(); i++ )
		{
			m_ctrlChoice->AppendString( bonesNames[i].AsChar() );
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
	}
}

void CEffectBoneListSelection::SetEntity( CEntity* entity )
{
	m_editedEntity = entity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

CSlotBoneListSelection::CSlotBoneListSelection( CPropertyItem* item )
	: ICustomPropertyEditor( item )
{
}

void CSlotBoneListSelection::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// We need a skeleton to pick the bone
	CEntity* temporaryEntity = nullptr;
	CComponent* component = GetSlotComponent( &temporaryEntity );
	if ( component && component->QuerySkeletonDataProvider() )
	{
		m_propertyItem->AddButton( m_propertyItem->GetPage()->GetStyle().m_iconPick, wxCommandEventHandler( CSlotBoneListSelection::OnBonePick ), this );
	}

	// We can always clear the selected bone
	m_propertyItem->AddButton( m_propertyItem->GetPage()->GetStyle().m_iconClear, wxCommandEventHandler( CSlotBoneListSelection::OnBoneReset ), this );

	if( temporaryEntity ) 
	{
		temporaryEntity->Discard();
	}
}

Bool CSlotBoneListSelection::DrawValue( wxDC& dc, const wxRect &valueRect, const wxColour& textColour )
{
	return false;
}

Bool CSlotBoneListSelection::SaveValue()
{
	return true;
}

Bool CSlotBoneListSelection::GrabValue( String& displayValue )
{
	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		// As CName
		CName name;
		m_propertyItem->Read( &name );
		displayValue = name.AsString();
		return true;
	}

	return false;
}

CComponent* CSlotBoneListSelection::GetSlotComponent( CEntity** temporaryEntity )
{	
	CEdEntitySlotProperties* props = m_propertyItem->GetPage()->QueryEntitySlotProperties();
	if ( props )
	{
		CEdEntityEditor* ed = props->GetEntityEditor();
		if ( ed )
		{
			CEntity* entity = ed->GetPreviewPanel()->GetEntity();
			if ( entity )
			{
				const EntitySlot* slot = props->GetSlot();
				if ( slot )
				{
					return entity->FindComponent( slot->GetComponentName() );
				}
			}
		}
	}
	IGameplayDLCMounter* gameplayDLCMounter = m_propertyItem->FindPropertyParentOfType< IGameplayDLCMounter >( 0 );
	if( gameplayDLCMounter )
	{
		CR4EntityTemplateSlotDLCMounter* entityTemplateSlotDLCMounter = Cast<CR4EntityTemplateSlotDLCMounter>( gameplayDLCMounter );
		if( entityTemplateSlotDLCMounter != nullptr )
		{
			CDiskFile* entityTemplateFile = GDepot->FindFile( entityTemplateSlotDLCMounter->GetBaseEntityTemplatePath() );
			if( entityTemplateFile )
			{
				CEntityTemplate* entityTemplate = Cast<CEntityTemplate>( entityTemplateFile->Load() );
				entityTemplate->CreateFullDataBuffer( nullptr, EntityTemplateInstancingInfo(), nullptr );

				// Create temporary entity for capturing
				*temporaryEntity = entityTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() );

			}					
			if ( *temporaryEntity != nullptr ) 
			{
				const EntitySlot* slot = m_propertyItem->FindPropertyParentOfType< EntitySlot >( 0 );
				if ( slot )
				{
					return (*temporaryEntity)->FindComponent( slot->GetComponentName() );
				}
			}
		}
	}
	// No contex component
	return NULL;
}

void CSlotBoneListSelection::OnBonePick( wxCommandEvent& event )
{
	// We need a skeleton to pick the bone
	CEntity* temporaryEntity = nullptr;
	CComponent* component = GetSlotComponent( &temporaryEntity );
	if ( component )
	{
		ISkeletonDataProvider* skeleton = (ISkeletonDataProvider*) component->QuerySkeletonDataProvider();
		if ( skeleton )
		{
			CName boneName;
			if ( m_propertyItem->Read( &boneName ) )
			{
				if ( PickBone( m_propertyItem->GetPage(), skeleton, boneName ) )
				{
					m_propertyItem->Write( &boneName );
					m_propertyItem->GrabPropertyValue();
				}
			}
		}
	}

	if( temporaryEntity ) 
	{
		temporaryEntity->Discard();
	}
}

void CSlotBoneListSelection::OnBoneReset( wxCommandEvent& event )
{
	CName boneName;
	if ( m_propertyItem->Read( &boneName ) )
	{
		boneName = CName::NONE;
		m_propertyItem->Write( &boneName );
		m_propertyItem->GrabPropertyValue();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
