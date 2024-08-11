/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "edEntitySetupListParam.h"

#include "edEntitySetup.h"

IMPLEMENT_ENGINE_CLASS( CEdEntitySetupListParam )

CEdEntitySetupListParam::CEdEntitySetupListParam()
	: m_detachFromTemplate( false )
{}
CEdEntitySetupListParam::~CEdEntitySetupListParam()
{}

void CEdEntitySetupListParam::OnSpawn( CEntity* entity )
{
	for ( auto it = m_effectors.Begin(), end = m_effectors.End(); it != end; ++it )
	{
		IEdEntitySetupEffector* e = it->Get();
		if ( e )
		{
			e->OnSpawn( entity );
		}
	}
	if ( m_detachFromTemplate )
	{
		entity->DetachTemplate();
	}
}