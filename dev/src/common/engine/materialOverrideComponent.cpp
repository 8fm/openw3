/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialOverrideComponent.h"
#include "materialInstance.h"

IMPLEMENT_ENGINE_CLASS( CMaterialOverrideComponent );

CMaterialOverrideComponent::CMaterialOverrideComponent ()
	: m_isGlobalOverride( false )
	, m_override( NULL )
{}

void CMaterialOverrideComponent::SetGlobalOverride( bool enable )
{
	/*if ( enable == m_isGlobalOverride )
	{
		return;
	}

	CEntity *entity   = GetEntity();
	bool updateEntity = (NULL!=entity && (enable || this==entity->GetGlobalMaterialOverrideComponent()));
	
	// Set new override value
	m_isGlobalOverride = enable;
	
	// Update entity if needed
	if ( updateEntity )
	{
		entity->UpdateGlobalMaterialOverrideComponent();
	}*/	
}

IMaterial* CMaterialOverrideComponent::OverrideMaterial( IMaterial *material ) const
{
	return m_override.Get();
}

void CMaterialOverrideComponent::OnPropertyPostChange( IProperty* property )
{
	/*if ( property && property->GetName()==TXT("isGlobalOverride") )
	{
		CEntity *entity = GetEntity();
		if ( entity )
		{
			entity->UpdateGlobalMaterialOverrideComponent();
		}
	}*/
}
