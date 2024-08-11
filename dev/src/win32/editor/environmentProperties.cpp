/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "environmentProperties.h"
#include "../../common/engine/environmentComponentArea.h"

CEdEnvironmentProperties::CEdEnvironmentProperties( CEntity* entity, wxWindow* parent, CEdUndoManager* undoManager )
	: CEdPropertiesBrowserWithStatusbar( parent, PropertiesPageSettings(), undoManager )
	, m_pEntity( entity )
{
	TDynArray< CObject* > objects;
	
	// Add area environment object
	if ( objects.Empty() )
	{
		TDynArray< CAreaEnvironmentComponent* > c;
		CollectEntityComponents( entity, c );
		if ( !c.Empty() )
		{
			objects.PushBack( c[0] );
		}
	}
	
	// Show the objects
	ASSERT( !objects.Empty() );	
	Get().SetObjects( objects );

	// Register listener
	SEvents::GetInstance().RegisterListener( CNAME( SimpleCurveChanged ), this );
}

CEdEnvironmentProperties::~CEdEnvironmentProperties()
{
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdEnvironmentProperties::DispatchEditorEvent( const CName& name, IEdEventData* data )
{	
	if ( name == CNAME( SimpleCurveChanged ) )
	{
		TDynArray< CAreaEnvironmentComponent* > envComponents;
		CollectEntityComponents( m_pEntity, envComponents );
		for ( Uint32 env_i=0; env_i<envComponents.Size(); ++env_i )
		{
			CAreaEnvironmentComponent *component = envComponents[env_i];
			component->NotifyPropertiesImplicitChange();
		}
	}
}
