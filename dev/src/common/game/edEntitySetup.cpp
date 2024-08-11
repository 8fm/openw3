/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "edEntitySetup.h"

IMPLEMENT_ENGINE_CLASS( IEdEntitySetupEffector )
IMPLEMENT_ENGINE_CLASS( CEdSpawnEntitySetupEffector )

///////////////////////////////////////////////////////////////////////////////
// CEdSpawnEntitySetupEffector
///////////////////////////////////////////////////////////////////////////////

void CEdSpawnEntitySetupEffector::OnSpawn( CEntity* entity )
{
	CLayer* layer = entity->GetLayer();
	if ( !layer )
	{
		return;
	}

	CEntityTemplate* entityTemplate = m_template.Get();
	if ( !entityTemplate )
	{
		return;
	}

	Vector worldPos = entity->GetWorldPositionRef();
	EulerAngles worldOri = entity->GetWorldRotation();

	worldPos += worldOri.TransformVector( m_localPosition );
	worldOri + m_localOrientation;

	EntitySpawnInfo sinfo;
	sinfo.m_spawnPosition = worldPos;
	sinfo.m_spawnRotation = worldOri;
	sinfo.m_template = entityTemplate;
	sinfo.m_detachTemplate = m_detachTemplate;	// Use template
	sinfo.m_tags = m_extraTags;
	layer->CreateEntitySync( sinfo );
}