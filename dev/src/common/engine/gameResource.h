/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "localizedContent.h"
#include "../core/resource.h"
#include "../core/engineTransform.h"

struct SWorldDescription
{
	DECLARE_RTTI_STRUCT( SWorldDescription );

	LocalizedString m_worldName;
	TSoftHandle< CWorld > m_world;
};

BEGIN_CLASS_RTTI( SWorldDescription );
	PROPERTY_CUSTOM_EDIT( m_worldName, TXT( "The name of this world" ), TXT( "LocalizedStringEditor" ) );
	PROPERTY_EDIT( m_world, TXT( "The resource" ) );
END_CLASS_RTTI();

class CGameResource : public CResource
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CGameResource, CResource );

protected:
	TDynArray< SWorldDescription >		m_worlds;
	TSoftHandle< CEntityTemplate >		m_defaultPlayerTemplate;
	TSoftHandle< CEntityTemplate >		m_defaultCameraTemplate;
	EngineTransform						m_startingPoint;
	String								m_newGameLoadingVideo; 
	TDynArray< CName >					m_playGoChunksToActivate;

public:
	String GetStartWorldPath() const;
	void GetWorldPaths( TDynArray< String >& worldPaths ) const;
	const TSoftHandle< CEntityTemplate >&	GetPlayerTemplate() const { return m_defaultPlayerTemplate; }
	const TSoftHandle< CEntityTemplate >&	GetCameraTemplate() const { return m_defaultCameraTemplate; }
	const TSoftHandle< CWorld >* FindWorld( String& path ) const;
	const String& GetNewGameLoadingVideo() const { return m_newGameLoadingVideo; }
	const TDynArray< CName >& GetPlayGoChunksToActivate() const { return m_playGoChunksToActivate; }

	void OnStartingPointChanged( const EngineTransform newTransform );
	RED_INLINE Bool GetStartingPoint( Vector& pos, EulerAngles& rot ) 
	{ 
		if ( m_startingPoint.IsIdentity() )
		{
			return false;
		}
		pos = m_startingPoint.GetPosition();
		rot = m_startingPoint.GetRotation();
		return true;
	}
};

BEGIN_ABSTRACT_CLASS_RTTI( CGameResource )
	PARENT_CLASS( CResource )
	PROPERTY_EDIT( m_worlds, TXT( "Worlds used by the game" ) );
	PROPERTY_EDIT( m_defaultPlayerTemplate, TXT( "Default template to spawn player from" ) )
	PROPERTY_EDIT( m_defaultCameraTemplate, TXT( "Default template to spawn camera from" ) )
	PROPERTY_EDIT( m_startingPoint, TXT( "Where the main quest starts (player spawn point)" ) );
	PROPERTY_EDIT( m_newGameLoadingVideo, TXT("Video to play on new game while the world is loading in the background") );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_playGoChunksToActivate, TXT("PlayGo chunks to activate by default. Leave empty to activate all!"), TXT("PlayGoChunkSelector") );
END_CLASS_RTTI()
