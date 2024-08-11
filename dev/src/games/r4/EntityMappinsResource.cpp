#include "build.h"
#include "EntityMappinsResource.h"

IMPLEMENT_ENGINE_CLASS( SEntityMapPinInfo );
IMPLEMENT_ENGINE_CLASS( CEntityMapPinsResource );

CEntityMapPinsResource::CEntityMapPinsResource()
{
}

CEntityMapPinsResource::~CEntityMapPinsResource()
{
}

#ifndef NO_EDITOR

Bool CEntityMapPinsResource::AddEntry( const SEntityMapPinInfo& info )
{
	// just a quick fix for duplicated pins
	for ( Uint32 i = 0; i < m_mappinsInfo.Size(); ++i )
	{
		if ( info.m_entityName     == m_mappinsInfo[ i ].m_entityName &&
			 info.m_entityPosition == m_mappinsInfo[ i ].m_entityPosition )
		{
			return false;
		}
	}
	//end of fix

	m_mappinsInfo.PushBack( info );
	return true;
}

#endif //NO_EDITOR