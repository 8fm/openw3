/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityAppearanceSelection.h"
#include "editorExternalResources.h"
#include "../../common/game/spawnTreeBaseEntryGenerator.h"

#pragma  optimize( "", off )
void CEntityAppearanceSelection::FillChoices()
{	
	CEntityTemplate* entityTemplate = NULL;

	if ( m_propertyItem->GetParentObject( 0 ).AsObject() != nullptr )
	{
		IEntityTemplatePropertyOwner* getter = NULL;
		CBasePropItem* pitem = m_propertyItem;
		while ( getter == NULL && pitem != NULL )
		{
			getter = dynamic_cast< IEntityTemplatePropertyOwner * >( pitem->GetParentObject( 0 ).AsObject() );
			pitem = pitem->GetParent();
		}

		if ( getter )
		{
			entityTemplate = getter->Editor_GetEntityTemplate();
		}
	}
	else if ( SCutsceneActorDef* actorDef = m_propertyItem->GetParentObject( 0 ).As< SCutsceneActorDef >() )
	{
		entityTemplate = actorDef->GetEntityTemplate();
	}
	else if ( SCreatureEntryEntryGeneratorNodeParam* entry = m_propertyItem->GetParentObject( 0 ).As< SCreatureEntryEntryGeneratorNodeParam >() )
	{
		entityTemplate = entry->GetEntityTemplate();
	}

	m_ctrlChoice->AppendString( CName::NONE.AsString().AsChar() );
	if ( entityTemplate )
	{
		const TDynArray< CName > & app = entityTemplate->GetEnabledAppearancesNames();

		for ( Uint32 i=0; i<app.Size(); ++i )
		{
			m_ctrlChoice->AppendString( app[i].AsString().AsChar() );
		}
	}
}
#pragma  optimize( "", on )
