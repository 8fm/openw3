/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibNavmeshBorderAreaComponent.h"

#include "pathlibNavmeshComponent.h"

IMPLEMENT_ENGINE_CLASS( CNavmeshBorderAreaComponent )

////////////////////////////////////////////////////////////////////////////
// CNavmeshBorderAreaComponent
////////////////////////////////////////////////////////////////////////////
CNavmeshBorderAreaComponent::~CNavmeshBorderAreaComponent()
{

}

Bool CNavmeshBorderAreaComponent::IsEffectingNavmesh( CNavmeshComponent* component )
{
	if ( m_isDisabled )
	{
		return false;
	}

	CEntity* entity = m_lockedToSpecyficNavmesh.Get();
	if ( entity )
	{
		if ( component->GetEntity() != entity )
		{
			return false;
		}
	}

	return true;
}