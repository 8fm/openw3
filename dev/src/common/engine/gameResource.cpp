/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "gameResource.h"

IMPLEMENT_ENGINE_CLASS( SWorldDescription );
IMPLEMENT_ENGINE_CLASS( CGameResource );

String CGameResource::GetStartWorldPath() const
{
	if ( m_worlds.Empty() == true )
	{
		return String::EMPTY;
	}

	return m_worlds[ 0 ].m_world.GetPath();
}

void CGameResource::GetWorldPaths( TDynArray< String >& worldPaths ) const
{
	for ( Uint32 i = 0; i < m_worlds.Size(); ++i )
	{
		worldPaths.PushBack( m_worlds[ i ].m_world.GetPath() );
	}
}

void CGameResource::OnStartingPointChanged( const EngineTransform newTransform )
{
	// TODO: gameplay will add some validation here in future
	m_startingPoint = newTransform;
#ifdef NO_RESOURCE_IMPORT
	RED_HALT( "Cannot resave resource in game build" );
#else
	Save();
#endif
}

const TSoftHandle< CWorld >* CGameResource::FindWorld( String& path ) const
{
	for ( Uint32 i = 0; i < m_worlds.Size(); ++i )
	{	
		if ( m_worlds[ i ].m_world.GetPath().EqualsNC( path ) )
		{
			return &m_worlds[ i ].m_world;
		}
	}
	return NULL;
}
