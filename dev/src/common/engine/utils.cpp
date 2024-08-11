/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "utils.h"
#include "entity.h"
#include "component.h"
#include "layer.h"


const CResource* CResourceObtainer::GetResource( const CEntity* entity )
{
	const CResource* resource = Cast< const CResource > ( entity->GetTemplate() );
	if ( nullptr == resource )
	{
		resource = entity->GetLayer();
	}
	return resource;
}

const CResource* CResourceObtainer::GetResource( const CComponent* component )
{
	return GetResource( component->GetEntity() );
}

const CResource* CResourceObtainer::GetResource( const CNode* node )
{
	if ( node->IsA< CComponent >() )
	{
		return GetResource( static_cast< const CComponent* >( node ) );
	}
	else if ( node->IsA< CEntity >() )
	{
		return GetResource( static_cast< const CEntity* >( node ) );
	}
	else
	{
		return GetResource( static_cast< const CObject* >( node ) );
	}
}

const CResource* CResourceObtainer::GetResource( const CObject* object )
{
	for ( const CObject* obj = object; obj != nullptr; obj = obj->GetParent() )
	{
		if( obj->IsA< CResource >() == true )
		{
			return Cast< const CResource >( obj );
		}
	}
	return nullptr;
}

