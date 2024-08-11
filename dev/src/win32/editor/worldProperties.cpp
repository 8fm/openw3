/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "worldProperties.h"

CEdWorldProperties::CEdWorldProperties( wxWindow* parent, const PropertiesPageSettings& settings, CEdUndoManager* undoManager )
	: CEdPropertiesBrowserWithStatusbar( parent, settings, undoManager )
{
	SEvents::GetInstance().RegisterListener( CNAME( ActiveWorldChanging ), this );
	SEvents::GetInstance().RegisterListener( CNAME( ActiveWorldChanged ), this );	
}

CEdWorldProperties::~CEdWorldProperties()
{
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdWorldProperties::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( ActiveWorldChanging ) )
	{
		Get().SetNoObject();
    }
	else if ( name == CNAME( ActiveWorldChanged ) )
	{
		Get().SetObject( GGame->GetActiveWorld().Get() );
	}
}

