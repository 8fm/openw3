/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sceneExplorer.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layerGroup.h"

CEdSelectionProperties::CEdSelectionProperties( wxWindow* parent, const PropertiesPageSettings& settings, CEdUndoManager* undoManager, CWorld *world /* = NULL */ )
	: CEdPropertiesBrowserWithStatusbar( parent, settings, undoManager )
    , m_world( world )
{
	Get().SetAllowGrabbing( false );
	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SceneExplorerSelectionChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( SelectionPropertiesChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( LayerSelectionChanged ), this );
	SEvents::GetInstance().RegisterListener( CNAME( ActiveWorldChanging ), this );
}

CEdSelectionProperties::~CEdSelectionProperties()
{
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdSelectionProperties::SetWorld( CWorld *world )
{
    if ( m_world != world )
    {
        Get().SetNoObject();
        m_world = world;
    }
}

void CEdSelectionProperties::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
    typedef CSelectionManager::SSelectionEventData SSelectionEventData;

    if ( m_world )
	{
		if ( name == CNAME( SelectionChanged ) )
		{
			const SSelectionEventData& eventData = GetEventData< SSelectionEventData >( data );
			if ( eventData.m_world == m_world )
			{
				// Collect selected entities from active world
				TDynArray< CNode* > nodes;
				m_world->GetSelectionManager()->GetSelectedRoots( nodes );

				// Show the objects
				m_propertiesBrowser->ClearPropertyStyles();
				Get().SetObjects( ( TDynArray< CObject*> & ) nodes );
			}
		}
		else if ( name == CNAME( SceneExplorerSelectionChanged ) && m_world == GGame->GetActiveWorld() )
		{
			// Collect selected objects
			TDynArray< ISerializable* > selection;
			TDynArray< CObject* > objects;
			TDynArray< CName > instanceProps, modifiedProps;
			wxTheFrame->GetSceneExplorer()->GetSelection( selection );
			for ( auto it=selection.Begin(); it != selection.End(); ++it )
			{
				CObject* obj = Cast< CObject >( *it );
				if ( obj != nullptr )
				{
					objects.PushBack( obj );

					// Check if we're handling a component
					CComponent* component = Cast< CComponent >( obj );
					if ( component != nullptr )
					{
						// Check if the component has instance properties2
						TDynArray< CName > instanceProperties;
						component->GetInstancePropertyNames( instanceProperties );
						if ( !instanceProperties.Empty() )
						{
							// Put the properties in the instance props array with the properties from the other objects
							instanceProps.PushBackUnique( instanceProperties );

							// Get all properties which have been modified
							TDynArray< CProperty* > modifiedProperties;
							component->CollectModifiedInstanceProperties( modifiedProperties );
							
							// Put them in the list
							for ( auto it=modifiedProperties.Begin(); it != modifiedProperties.End(); ++it )
							{
								modifiedProps.PushBackUnique( (*it)->GetName() );
							}
						}
					}
				}
			}

			// Remove the modified properties from the instance properties list
			for ( auto it=modifiedProps.Begin(); it != modifiedProps.End(); ++it )
			{
				instanceProps.Remove( *it );
			}

			// Highlight instance and overridden properties
			m_propertiesBrowser->ClearPropertyStyles();
			for ( auto it=instanceProps.Begin(); it != instanceProps.End(); ++it )
			{
				m_propertiesBrowser->SetPropertyStyle( *it, SEdPropertiesPagePropertyStyle( wxColor( 213, 225, 221 ) ) );
			}
			for ( auto it=modifiedProps.Begin(); it != modifiedProps.End(); ++it )
			{
				m_propertiesBrowser->SetPropertyStyle( *it, SEdPropertiesPagePropertyStyle( wxColor( 238, 204, 181 ) ) );
			}

			// Show the objects
			Get().SetObjects( objects );
		}
		else if ( name == CNAME( LayerSelectionChanged ) )
		{
			const SSelectionEventData& eventData = GetEventData< SSelectionEventData >( data );
			if ( eventData.m_world == m_world )
			{
				// Try to grab the selection from the scene explorer first
				TDynArray< ISerializable* > selection;
				wxTheFrame->GetSceneExplorer()->GetSelection( selection );

				// No selection, fill from selection manager
				if ( selection.Empty() )
				{
					ISerializable *layer = m_world->GetSelectionManager()->GetSelectedLayer();
					if ( layer )
					{
						selection.PushBack( layer );
					}
				}

				// Some selection found, try to use it
				if ( !selection.Empty() )
				{
					TDynArray< CLayerInfo* > layers;
					TDynArray< CLayerGroup* > groups;

					// We cannot modify both layers and groups, so extract them individually
					for ( auto it=selection.Begin(); it != selection.End(); ++it )
					{
						if ( (*it)->IsA< CLayerInfo >() )
						{
							layers.PushBack( static_cast< CLayerInfo* >( *it ) );
						}
						else if ( (*it)->IsA< CLayerGroup >() )
						{
							groups.PushBack( static_cast< CLayerGroup* >( *it ) );
						}
					}

					// We have layers, use them
					m_propertiesBrowser->ClearPropertyStyles();
					if ( !layers.Empty() )
					{
						Get().SetObjects( layers );
					}
					else if ( !groups.Empty() ) // we have groups, use them
					{
						Get().SetObjects( groups );
					}
				}
			}
		}
		else if( name == CNAME( SelectionPropertiesChanged ) )
		{
			CWorld* world = GetEventData< CWorld* >( data );
			if ( m_world == world )
			{
				Get().RefreshValues();
			}
		}
		else if ( name == CNAME( ActiveWorldChanging ) )
		{
			SetWorld( NULL );
		}
	}
}

void CEdSelectionProperties::SetEntityEditorAsOwner()
{
	m_propertiesBrowser->SetEntityEditorAsOwner();
}