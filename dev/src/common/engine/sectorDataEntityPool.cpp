/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "sectorDataEntityPool.h"

#include "entity.h"
#include "dynamicLayer.h"

CSectorDataEntityPool::CSectorDataEntityPool( CDynamicLayer* layer )
	: m_layer( layer )
{
}

CSectorDataEntityPool::~CSectorDataEntityPool()
{
}

CEntity* CSectorDataEntityPool::Allocate( const Matrix& referenceMatrix )
{
	// tempshit
	CEntity* entityObject = ::CreateObject< CEntity >( m_layer );

	EngineTransform transform( referenceMatrix );
	entityObject->SetRawPlacement( &transform.GetPosition(), &transform.GetRotation(), &transform.GetScale() );

	return entityObject;
}

void CSectorDataEntityPool::Release( CEntity* entity )
{
	if ( entity )
	{
		RED_FATAL_ASSERT( !entity->IsInitialized(), "Invalid entity state when released to the pool" );
		RED_FATAL_ASSERT( !entity->IsAttached(), "Invalid entity state when released to the pool" );
		entity->Discard();
	}
}


