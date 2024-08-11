/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "lightEntityCollector.h"

#include "sceneExplorer.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/selectionManager.h"
#include "../../common/engine/spotLightComponent.h"
#include "../../common/engine/pointLightComponent.h"
#include "../../games/r4/gameplayLightComponent.h"


BEGIN_EVENT_TABLE( CEdLightEntityCollector, wxFrame )
	EVT_LISTBOX_DCLICK(	XRCID("m_lightEntitiesList"),	CEdLightEntityCollector::OnLightEntitiesDoubleClick )
	EVT_BUTTON(			XRCID("m_refresh"),				CEdLightEntityCollector::OnRefreshClicked )
	EVT_BUTTON(			XRCID("m_apply"),				CEdLightEntityCollector::OnApplyClicked )
	EVT_BUTTON(			XRCID("m_removeFromList"),		CEdLightEntityCollector::OnRemoveFromListClicked )
	EVT_BUTTON(			XRCID("m_close"),				CEdLightEntityCollector::OnCloseClicked )
	EVT_LISTBOX(		XRCID("m_lightEntitiesList"),	CEdLightEntityCollector::OnItemSelected )
END_EVENT_TABLE()


CEdLightEntityCollector::CEdLightEntityCollector( wxWindow* parent )
{
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("LightEntityCollector") );

	m_lightEntitiesList = XRCCTRL( *this, "m_lightEntitiesList", wxListBox );

	OnRefreshClicked( wxCommandEvent() );
}

CEdLightEntityCollector::~CEdLightEntityCollector()
{
}

void CEdLightEntityCollector::OnRefreshClicked( wxCommandEvent& event )
{
	RunLaterOnce( [ this ](){ Refresh(); } );
}


void CEdLightEntityCollector::Refresh()
{
	// Refresh lists
	m_lightEntities.Clear();
	m_lightEntitiesList->Clear();

	// Make sure we have a world to work with
	CWorld* world = GGame->GetActiveWorld();

	if ( world )
	{
		ScanLayerGroup( world->GetWorldLayers(), world );
	}
}

void CEdLightEntityCollector::OnItemSelected( wxCommandEvent& event )
{
	wxArrayInt selections;
	m_lightEntitiesList->GetSelections(selections);

	CSelectionManager* manager = GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetSelectionManager() : NULL;
	if ( manager == NULL )
		return;

	manager->DeselectAll();

	for ( Uint32 i = 0; i < selections.size(); i++ )
	{
		CNode* node = Cast<CNode>( m_lightEntities[selections[i]] );

		if ( node )
		{
			manager->Select( node );
		}
	}

	RefreshProperties( selections[0] );
}

void CEdLightEntityCollector::RefreshProperties( int index )
{
	CEntity* entity = m_lightEntities[index];

	if ( entity )
	{
		CGameplayLightComponent* glComponent = entity->FindComponent<CGameplayLightComponent>();

		if ( glComponent )
		{
			TDynArray< CName > instancePropNames;
			glComponent->GetInstancePropertyNames( instancePropNames );

			if ( !instancePropNames.Empty() )
			{
				Bool isLightOn;	
				Bool isCityLight;
				Bool isInteractive;
				Bool isAffectedByWeather;

				GetPropertyValue( glComponent, TXT("isLightOn"), isLightOn );
				GetPropertyValue( glComponent, TXT("isCityLight"), isCityLight );
				GetPropertyValue( glComponent, TXT("isInteractive"), isInteractive );
				GetPropertyValue( glComponent, TXT("isAffectedByWeather"), isAffectedByWeather );

				XRCCTRL( *this, "m_isLightOn", wxCheckBox )->SetValue( glComponent->IsLightOn() );
				XRCCTRL( *this, "m_isCityLight", wxCheckBox )->SetValue( glComponent->IsCityLight() );
				XRCCTRL( *this, "m_isInteractive", wxCheckBox )->SetValue( glComponent->IsInteractive() );
				XRCCTRL( *this, "m_isAffectedByWeather", wxCheckBox )->SetValue( glComponent->IsAffectedByWeather() );
			}
		}
	}
}

void CEdLightEntityCollector::ScanLayerGroup( CLayerGroup* layerGroup, CWorld* world )
{
	TDynArray< CLayerInfo* > layerInfos;
	layerGroup->GetLayers( layerInfos, true, true, true );

	//special-case bypassing for streaming_tiles layer
 	if ( layerGroup->IsSystemGroup() ) 
  		return;

	//RED_LOG( RED_LOG_CHANNEL(System), TXT("Group: %s "), layerGroup->GetName().AsChar() );

	// Scan all the layers in this group
	for ( auto it=layerInfos.Begin(); it != layerInfos.End(); ++it )
	{
		CLayerInfo* layerInfo = *it;

		if ( layerInfo->IsLoaded() && !layerInfo->GetDepotPath().Empty() )
		{
			ScanLayer( layerInfo->GetLayer(), world );
		}
	}
}

void CEdLightEntityCollector::ScanLayer( CLayer* layer, CWorld* world )
{
	const LayerEntitiesArray& entities = layer->GetEntities();
	const CGameplayLightComponent* glComponent;
	const CSpotLightComponent* slComponent;
	const CPointLightComponent* plComponent;


	//RED_LOG( RED_LOG_CHANNEL(System), TXT("	Layer: %s"), layer->GetFriendlyName().AsChar() );

	// Scan layer for our light entities
	for ( auto it=entities.Begin(); it != entities.End(); ++it )
	{
		CEntity* entity = *it;

		//RED_LOG( RED_LOG_CHANNEL(System), TXT("		Entity: %s"), entity->GetFriendlyName().AsChar() );

		//we only care for entities that have the gameplayLightComponent
		glComponent = entity->FindComponent<CGameplayLightComponent>();
		if( !glComponent )
		{
			continue;
		}

		//any entity that has made it this far is now a candidate for our list
		m_lightEntities.PushBack( entity );

		//find out if we have any instance props
		TDynArray< CProperty* > modifiedProperties;
		slComponent = entity->FindComponent<CSpotLightComponent>();
		plComponent = entity->FindComponent<CPointLightComponent>();

		if ( slComponent )
			slComponent->CollectModifiedInstanceProperties( modifiedProperties );		

		if ( plComponent )
			plComponent->CollectModifiedInstanceProperties( modifiedProperties );		

		glComponent->CollectModifiedInstanceProperties( modifiedProperties );


		if ( modifiedProperties.Size() > 0 )
			m_lightEntitiesList->Append( wxString::Format( wxT("%s  || ***  %s  ***"), layer->GetDepotPath().AsChar(), entity->GetName().AsChar() ) );
		else
			m_lightEntitiesList->Append( wxString::Format( wxT("%s  ||  %s"), layer->GetDepotPath().AsChar(), entity->GetName().AsChar() ) );
	}
}


void CEdLightEntityCollector::OnLightEntitiesDoubleClick( wxCommandEvent& event )
{
	RunLaterOnce( [ this ](){ Focus(); } );
}

void CEdLightEntityCollector::Focus()
{
	wxArrayInt selections;
	m_lightEntitiesList->GetSelections(selections);

	if ( selections.size() > 0 )
	{
		int index = selections[0]; //no matter how many selected, just focus on the top one

		wxTheFrame->GetWorldEditPanel()->LookAtNode( m_lightEntities[index] );
		wxTheFrame->GetWorldEditPanel()->GetSceneExplorer()->UpdateSelected();
	}
}

void CEdLightEntityCollector::OnApplyClicked( wxCommandEvent& event )
{
	wxArrayInt selections;
	m_lightEntitiesList->GetSelections(selections);

	if ( selections.size() > 0 )
	{
		// Collect all used layers
		TDynArray< CLayer* > layers;
		for ( Uint32 i = 0; i < selections.size(); ++i )
		{
			layers.PushBackUnique( m_lightEntities[selections[i]]->GetLayer() );
		}


		// Try to mark them as modified
		for ( auto it = layers.Begin(); it != layers.End(); ++it )
		{
			if ( (*it)->GetLayerInfo() )
			{
				(*it)->GetLayerInfo()->MarkModified();
			}
			(*it)->MarkModified();
		}

		//read checkboxes
		Bool isLightOn				= XRCCTRL( *this, "m_isLightOn", wxCheckBox )->GetValue();
		Bool isCityLight			= XRCCTRL( *this, "m_isCityLight", wxCheckBox )->GetValue();
		Bool isInteractive		= XRCCTRL( *this, "m_isInteractive", wxCheckBox )->GetValue();
		Bool isAffectedByWeather	= XRCCTRL( *this, "m_isAffectedByWeather", wxCheckBox )->GetValue();

		// process checked entities
		for ( Uint32 i = 0; i < selections.size(); ++i )
		{
			CEntity* entity = m_lightEntities[selections[i]];

			if ( entity && entity->GetLayer()->IsModified() )
			{
				CGameplayLightComponent* glComponent = entity->FindComponent<CGameplayLightComponent>();

				if ( glComponent )
				{
					TDynArray< CName > instancePropNames;
					glComponent->GetInstancePropertyNames( instancePropNames );

					if ( !instancePropNames.Empty() )
					{
						SetPropertyValue( glComponent, TXT("isLightOn"), isLightOn );
						SetPropertyValue( glComponent, TXT("isCityLight"), isCityLight );
						SetPropertyValue( glComponent, TXT("isInteractive"), isInteractive );
						SetPropertyValue( glComponent, TXT("isAffectedByWeather"), isAffectedByWeather );

						glComponent->SetLight( isLightOn );
					}
					else
					{
						RED_ASSERT( "%s doesn't seem to have any instance properties! Contact Shadi Dadenji!", entity->GetFriendlyName().AsChar() );
					}
				}
			}
		}
	}
}

void CEdLightEntityCollector::OnRemoveFromListClicked( wxCommandEvent& event )
{
	wxArrayInt selections;
	m_lightEntitiesList->GetSelections( selections );

	for ( Uint32 i = 0; i < selections.size(); ++i )
	{
		int index = selections[i];

		CEntity* entity = m_lightEntities[selections[i]-i];

		if ( entity )
		{
			m_lightEntitiesList->Delete( selections[i]-i );
			m_lightEntities.RemoveFast( entity );
		}
	}
}

void CEdLightEntityCollector::OnCloseClicked( wxCommandEvent& event )
{
	DestroyLater( this );
}
